#include "pairing_controller.h"
#include "local_https.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <memory>

PairingController::PairingController(QObject *parent, QNetworkAccessManager *network)
    : QObject(parent), m_network(network ? network : new QNetworkAccessManager(this)) {
    m_network->setProxy(QNetworkProxy::NoProxy);
    m_retry.setSingleShot(true);
    m_watchdog.setSingleShot(true);
    connect(&m_retry, &QTimer::timeout, this, &PairingController::connectEvents);
    connect(&m_watchdog, &QTimer::timeout, this, [this] { if (m_stream) m_stream->abort(); });
}

PairingController::~PairingController() { stop(); }

void PairingController::stop() {
    m_started = false;
    ++m_generation;
    m_retry.stop(); m_watchdog.stop();
    for (auto reply : {m_stream, m_request}) {
        if (reply) { disconnect(reply, nullptr, this, nullptr); reply->abort(); reply->deleteLater(); }
    }
    m_stream.clear(); m_request.clear(); m_buffer.clear();
    m_pending = m_online = m_busy = false;
}

void PairingController::start(int port) {
    if (port < 1 || port > 65535) return;
    if (m_started && m_port == port) return;
    stop();
    m_port = port;
    m_instance.clear(); m_requestId.clear(); m_seenRequest.clear(); m_submittedRequest.clear();
    m_clientName.clear(); m_error.clear(); m_revision = -1; m_retryDelay = 3000;
    m_started = true;
    emit changed();
    connectEvents();
}

void PairingController::setPort(int port) {
    if (port == m_port || port < 1 || port > 65535) return;
    if (m_started) start(port); else m_port = port;
}

void PairingController::reconnectIfIdle() {
    if (!m_started || m_stream) return;
    m_retry.stop();
    connectEvents();
}

QNetworkReply *PairingController::send(const QByteArray &method, const QString &path, const QByteArray &body) {
    QNetworkRequest request(QUrl(QString("https://127.0.0.1:%1%2").arg(m_port).arg(path)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(path.endsWith("/events") ? 45000 : 10000);
    request.setRawHeader("Accept", path.endsWith("/events") ? "text/event-stream" : "application/json");
    if (method == "POST") request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto *reply = m_network->sendCustomRequest(request, method, body);
    reply->setReadBufferSize(64 * 1024);
    allowLocalSunshineCertificate(reply);
    return reply;
}

void PairingController::connectEvents() {
    if (!m_started || m_stream) return;
    m_buffer.clear();
    auto *reply = send("GET", "/api/tray/events");
    m_stream = reply;
    const int generation = m_generation;
    m_watchdog.start(10000);
    connect(reply, &QIODevice::readyRead, this, [this, reply, generation] {
        if (generation != m_generation) return;
        const auto mime = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 ||
            mime.section(';', 0, 0).trimmed() != "text/event-stream") { reply->abort(); return; }
        m_watchdog.start(45000); // The host sends a heartbeat every 30 seconds.
        while (reply->bytesAvailable() > 0) {
            m_buffer += reply->read(64 * 1024);
            if (!consumeEvents()) { reply->abort(); return; }
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation] {
        reply->deleteLater();
        if (generation != m_generation) return;
        m_stream.clear(); m_watchdog.stop(); m_buffer.clear();
        m_online = m_pending = false;
        m_error = tr("主机连接已中断，请等待重新连接。");
        emit changed();
        if (m_started) {
            m_retry.start(m_retryDelay);
            m_retryDelay = qMin(m_retryDelay * 2, 30000);
        }
    });
}

bool PairingController::consumeEvents() {
    m_buffer.replace("\r\n", "\n");
    if (m_buffer.size() > 256 * 1024) return false;
    qsizetype end;
    while ((end = m_buffer.indexOf("\n\n")) >= 0) {
        const auto frame = m_buffer.left(end);
        m_buffer.remove(0, end + 2);
        QByteArray event, data;
        for (const auto &line : frame.split('\n')) {
            if (line.startsWith("event:")) event = line.mid(6).trimmed();
            if (line.startsWith("data:")) {
                auto part = line.mid(5);
                if (part.startsWith(' ')) part.remove(0, 1);
                data += part + '\n';
            }
        }
        if (event != "tray-state") continue;
        const auto document = QJsonDocument::fromJson(data);
        if (!document.isObject() || !applyState(document.object())) return false;
    }
    return true;
}

bool PairingController::validState(const QJsonObject &state) {
    return state.value("protocol_version").toInt() == 1 &&
        !state.value("instance_id").toString().isEmpty() && state.value("instance_id").toString().size() <= 128 &&
        state.value("revision").toInteger(-1) >= 0 &&
        state.value("capabilities").toArray().contains("state-v1") &&
        QStringList{"gui", "core", "disabled"}.contains(state.value("owner").toString()) &&
        QStringList{"idle", "streaming", "paused", "pairing", "notification"}.contains(state.value("status").toString());
}

QString PairingController::pairingKey(const QJsonObject &state) {
    const auto notification = state.value("notification").toObject();
    if (!validState(state) || state.value("status").toString() != "pairing" ||
        !notification.value("active").toBool() || notification.value("action").toString() != "open_pin" ||
        notification.value("id").toInteger() <= 0) return {};
    return state.value("instance_id").toString() + "/" + QString::number(notification.value("id").toInteger());
}

bool PairingController::applyState(const QJsonObject &state) {
    if (!validState(state)) return false;
    const auto instance = state.value("instance_id").toString();
    const auto revision = state.value("revision").toInteger();
    if (instance == m_instance && (revision < m_revision || (revision == m_revision && m_online))) return true;
    const bool reconnected = !m_online;
    m_instance = instance; m_revision = revision; m_online = true; m_retryDelay = 3000;
    m_requestId = pairingKey(state);
    m_pending = !m_requestId.isEmpty() && m_requestId != m_submittedRequest;
    m_clientName = state.value("pairing_client_name").toString().simplified().left(64);
    if (m_clientName.isEmpty()) m_clientName = "Moonlight";
    const bool fresh = m_pending && m_requestId != m_seenRequest;
    if (fresh) m_seenRequest = m_requestId;
    if (fresh || reconnected) m_error.clear();
    emit changed();
    if (fresh) emit requestReceived();
    return true;
}

void PairingController::setError(const QString &message) { m_error = message; emit changed(); }

void PairingController::jsonRequest(const QByteArray &method, const QString &path, const QJsonObject &body,
                                   std::function<void(bool, const QJsonObject &)> callback) {
    auto *reply = send(method, path, method == "GET" ? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_request = reply;
    const int generation = m_generation;
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QIODevice::readyRead, this, [reply, bytes] {
        bytes->append(reply->readAll());
        if (bytes->size() > 256 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, generation, callback] {
        reply->deleteLater();
        if (generation != m_generation) return;
        m_request.clear();
        const auto document = QJsonDocument::fromJson(*bytes);
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        callback(reply->error() == QNetworkReply::NoError && status == 200 && document.isObject(), document.object());
    });
}

void PairingController::submit(const QString &pin, const QString &name, const QString &expectedRequest) {
    if (m_busy) return;
    if (!m_pending || !m_online || expectedRequest != m_requestId) {
        setError(tr("配对请求已失效，请在 Moonlight 中重新发起连接。")); return;
    }
    if (!QRegularExpression("^[0-9]{4}$").match(pin).hasMatch() || name.trimmed().isEmpty() || name.toUtf8().size() > 256) {
        setError(tr("请输入 Moonlight 显示的 4 位数字连接码和设备名称。")); return;
    }
    m_busy = true; m_error.clear(); emit changed();
    // The PIN endpoint has no request-id parameter. Recheck immediately before
    // submitting so a stale popup cannot knowingly target a different request.
    jsonRequest("GET", "/api/tray/state", {}, [this, pin, name, expectedRequest](bool ok, const QJsonObject &state) {
        if (m_requestId != expectedRequest) { m_busy = false; emit changed(); return; }
        if (!ok || !m_online || !m_pending) {
            m_busy = false;
            setError(tr("请求已变化或无法确认，请在 Moonlight 中重新发起配对。")); return;
        }
        if (pairingKey(state) != expectedRequest) {
            m_busy = false;
            if (!applyState(state) || m_requestId == expectedRequest)
                setError(tr("请求已变化或无法确认，请在 Moonlight 中重新发起配对。"));
            return;
        }
        jsonRequest("POST", "/api/pin", {{"pin", pin}, {"name", name.trimmed()}},
            [this, expectedRequest](bool posted, const QJsonObject &result) {
                m_busy = false;
                const auto status = result.value("status");
                if (posted && (status.toBool() || status.toString() == "true")) {
                    m_submittedRequest = expectedRequest;
                    if (m_requestId == expectedRequest) m_pending = false;
                    emit changed(); emit pinSubmitted();
                } else if (m_requestId == expectedRequest) {
                    setError(posted ? tr("主机未接受连接码，请确认请求仍有效后重试。") :
                        tr("未能确认提交结果，请先查看 Moonlight 的状态后再重试。"));
                } else {
                    emit changed();
                }
            });
    });
}
