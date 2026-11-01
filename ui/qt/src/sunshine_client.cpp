#include "sunshine_client.h"
#include "local_https.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>

namespace {
bool failedStatus(const QJsonValue &value) {
    return (value.isBool() && !value.toBool()) || value.toString() == "false";
}
QString scalar(const QJsonValue &value) {
    if (value.isString()) return value.toString();
    if (value.isBool()) return value.toBool() ? "true" : "false";
    return value.toVariant().toString();
}
}

SunshineClient::SunshineClient(QObject *parent, QNetworkAccessManager *network) :
    QObject(parent), m_network(network ? network : new QNetworkAccessManager(this)) {
    m_network->setProxy(QNetworkProxy::NoProxy);
    QSettings settings;
    m_port = settings.value("host/port", 47990).toInt();
    if (m_port < 1 || m_port > 65535) m_port = 47990;
    m_executable = settings.value("host/executable").toString();
    m_poll.setInterval(5000);
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (m_busy || m_refreshing) return;
        if (m_connected) refreshSessions();
        else if (!m_setupRequired) refresh();
    });
}

void SunshineClient::startPolling() { refresh(); m_poll.start(); }

QVariantMap SunshineClient::config() const {
    QJsonObject result = m_config;
    for (auto it = m_edits.begin(); it != m_edits.end(); ++it) result[it.key()] = it.value();
    return result.toVariantMap();
}

void SunshineClient::setPort(int port) {
    if (port == m_port || m_busy) return;
    if (port < 1 || port > 65535) { notify(tr("端口必须在 1–65535 之间。"), true); return; }
    if (dirty()) { notify(tr("请先保存或放弃设置修改，再切换连接端口。"), true); return; }
    ++m_generation;
    m_port = port;
    m_connected = m_setupRequired = m_refreshing = m_polling = m_loadingLogs = false;
    m_config = {}; m_apps = {}; m_clients = {}; m_sessions = {}; m_logs.clear();
    QSettings().setValue("host/port", port);
    emit stateChanged(); emit configChanged(); emit appsChanged(); emit clientsChanged();
    emit sessionsChanged(); emit logsChanged();
    refresh();
}

void SunshineClient::setExecutable(const QString &path) {
    m_executable = path.startsWith("file:") ? QUrl(path).toLocalFile() : path;
    QSettings().setValue("host/executable", m_executable);
    emit stateChanged();
}

void SunshineClient::notify(const QString &text, bool error) {
    m_notice = text; m_noticeIsError = error; emit noticeChanged();
}
void SunshineClient::clearNotice() { notify({}); }

void SunshineClient::request(const QByteArray &method, const QString &path, const QJsonObject &body,
                            Callback callback, bool quiet, bool text) {
    // Only the local host is supported. Never forward local credentials or relaxed TLS to a remote URL.
    const QUrl url(QString("https://127.0.0.1:%1%2").arg(m_port).arg(path));
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(10000);
    req.setRawHeader("Accept", text ? "text/plain" : "application/json");
    if (text) req.setRawHeader("X-Log-Offset", "0");
    if (method != "GET") req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QByteArray payload = method == "GET" ? QByteArray() : QJsonDocument(body).toJson(QJsonDocument::Compact);
    auto *reply = m_network->sendCustomRequest(req, method, payload);
    const int generation = m_generation;
    allowLocalSunshineCertificate(reply);
    connect(reply, &QIODevice::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > 5 * 1024 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [=, this] {
        reply->deleteLater();
        if (generation != m_generation) return;
        const auto data = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error;
        QJsonObject object;
        if (status >= 300 && status < 400) {
            const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            m_setupRequired = redirect.path() == "/welcome";
            error = m_setupRequired ? tr("首次使用，请在“连接设置”中创建管理账户。") : tr("主机返回了不支持的重定向。");
        } else if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            error = status == 0 ? tr("尚未连接本机 Sunshine。请启动主机或检查连接端口。") :
                    status == 401 ? tr("主机拒绝了本地访问，请检查 Sunshine 的访问设置。") :
                    tr("无法完成请求：%1（HTTP %2）").arg(reply->errorString()).arg(status);
        } else if (!text) {
            QJsonParseError parse;
            const auto document = QJsonDocument::fromJson(data, &parse);
            if (parse.error != QJsonParseError::NoError || !document.isObject()) {
                error = tr("主机返回了无效的 JSON 数据。");
            } else {
                object = document.object();
                if (failedStatus(object.value("status")) || failedStatus(object.value("success"))) {
                    error = object.value("error").toString(object.value("status_message").toString(tr("操作未成功，请检查主机日志。")));
                }
            }
        }
        if (!error.isEmpty()) {
            if (status == 0 || status == 401 || (status >= 300 && status < 400)) {
                m_connected = false;
                m_sessions = {};
                emit sessionsChanged();
            }
            emit stateChanged();
            if (!quiet || m_setupRequired) notify(error, true);
        }
        callback(error.isEmpty(), object, data);
    });
}

void SunshineClient::refresh() {
    if (m_refreshing || m_busy) return;
    m_refreshing = true;
    request("GET", "/api/config", {}, [this](bool ok, const QJsonObject &obj, const QByteArray &) {
        m_refreshing = false;
        m_connected = ok;
        if (ok) {
            m_setupRequired = false;
            if (!dirty()) { m_config = obj; emit configChanged(); }
            loadApps(); loadClients(); refreshSessions();
        }
        emit stateChanged();
    }, m_poll.isActive());
}

void SunshineClient::loadApps() {
    request("GET", "/api/apps", {}, [this](bool ok, const QJsonObject &obj, const QByteArray &) {
        if (ok) { m_apps = obj.value("apps").toArray(); emit appsChanged(); }
    });
}
void SunshineClient::loadClients() {
    request("GET", "/api/clients/list", {}, [this](bool ok, const QJsonObject &obj, const QByteArray &) {
        if (ok) { m_clients = obj.value("named_certs").toArray(); emit clientsChanged(); }
    });
}
void SunshineClient::refreshSessions() {
    if (!m_connected || m_polling) return;
    m_polling = true;
    request("GET", "/api/runtime/sessions", {}, [this](bool ok, const QJsonObject &obj, const QByteArray &) {
        m_polling = false;
        if (ok) m_sessions = obj.value("sessions").toArray();
        else m_sessions = {};
        emit sessionsChanged();
    }, true);
}
void SunshineClient::refreshLogs() {
    if (!m_connected || m_loadingLogs) return;
    m_loadingLogs = true;
    request("GET", "/api/logs", {}, [this](bool ok, const QJsonObject &, const QByteArray &data) {
        m_loadingLogs = false;
        if (ok) { m_logs = QString::fromUtf8(data).right(1024 * 1024); emit logsChanged(); }
    }, false, true);
}

QString SunshineClient::value(const QString &key, const QString &fallback) const {
    if (m_edits.contains(key)) return scalar(m_edits.value(key));
    return m_config.contains(key) ? scalar(m_config.value(key)) : fallback;
}
void SunshineClient::setValue(const QString &key, const QString &value) {
    if (m_busy || !m_connected) return;
    if (scalar(m_config.value(key)) == value) m_edits.remove(key);
    else m_edits.insert(key, value);
    emit configChanged();
}
void SunshineClient::discardChanges() { if (!m_busy) { m_edits = {}; emit configChanged(); refresh(); } }

QJsonObject SunshineClient::writableConfig(const QJsonObject &source) {
    static const QStringList readonly = {"status", "platform", "version", "display_devices", "adapters",
        "active_encoder", "active_nvenc_frame_budget", "nvidia_vsync", "pair_name",
        "widget_token_configured", "usb_forwarding_config_version"};
    QJsonObject result;
    for (auto it = source.begin(); it != source.end(); ++it) {
        if (!readonly.contains(it.key()) && !it.value().isObject() && !it.value().isArray())
            result.insert(it.key(), scalar(it.value()));
    }
    return result;
}

bool SunshineClient::beginMutation() {
    if (m_busy) return false;
    if (!m_connected && !m_setupRequired) { notify(tr("请先连接本机 Sunshine 服务。"), true); return false; }
    m_busy = true; clearNotice(); emit stateChanged(); return true;
}
void SunshineClient::endMutation() { m_busy = false; emit stateChanged(); }

void SunshineClient::saveConfig() {
    if (!dirty() || !beginMutation()) return;
    const auto edits = m_edits;
    request("GET", "/api/config", {}, [this, edits](bool ok, const QJsonObject &current, const QByteArray &) {
        if (!ok) { endMutation(); return; }
        for (auto it = edits.begin(); it != edits.end(); ++it) {
            if (current.value(it.key()) != m_config.value(it.key())) {
                notify(tr("“%1”已被其他界面修改，请放弃本地修改并重新读取。其余配置未写入。").arg(it.key()), true);
                endMutation(); return;
            }
        }
        auto merged = writableConfig(current);
        for (auto it = edits.begin(); it != edits.end(); ++it) merged[it.key()] = it.value();
        request("POST", "/api/config", merged, [this](bool saved, const QJsonObject &, const QByteArray &) {
            if (saved) {
                m_edits = {}; emit configChanged();
                notify(tr("设置已保存。重启主机后完整生效；重启会中断正在进行的串流。"));
                emit operationSucceeded("config");
            }
            endMutation();
            if (saved) refresh();
        });
    });
}

void SunshineClient::saveApp(int index, const QVariantMap &fields) { changeApp(index, fields, false); }
void SunshineClient::deleteApp(int index) { changeApp(index, {}, true); }
void SunshineClient::changeApp(int index, const QVariantMap &fields, bool remove) {
    if (index < -1 || index >= m_apps.size() || (remove && index < 0)) return;
    const auto name = fields.value("name").toString().trimmed();
    if (!remove && (name.isEmpty() || name.toUtf8().size() > 256)) {
        notify(tr("应用名称不能为空，且不能超过 256 字节。"), true); return;
    }
    if (!beginMutation()) return;
    const auto snapshot = m_apps;
    request("GET", "/api/apps", {}, [=, this](bool ok, const QJsonObject &obj, const QByteArray &) {
        if (!ok) { endMutation(); return; }
        const auto current = obj.value("apps").toArray();
        if (index >= 0 && current != snapshot) {
            m_apps = current; emit appsChanged();
            emit appListInvalidated();
            notify(tr("应用列表已发生变化，已重新加载。请重新选择要修改的应用。"), true);
            endMutation(); return;
        }
        QJsonObject body;
        if (!remove) {
            auto app = index < 0 ? QJsonObject() : current.at(index).toObject();
            for (auto it = fields.begin(); it != fields.end(); ++it) app[it.key()] = QJsonValue::fromVariant(it.value());
            app["name"] = name;
            app["index"] = index;
            body = {{"apps", current}, {"editApp", app}};
        }
        request(remove ? "DELETE" : "POST", remove ? QString("/api/apps/%1").arg(index) : "/api/apps", body,
            [this](bool saved, const QJsonObject &, const QByteArray &) {
                endMutation();
                if (saved) { notify(tr("应用列表已更新。")); loadApps(); emit operationSucceeded("apps"); }
            });
    });
}

void SunshineClient::mutate(const QString &path, const QJsonObject &body, const QString &operation) {
    if (!beginMutation()) return;
    request("POST", path, body, [this, operation](bool ok, const QJsonObject &, const QByteArray &) {
        endMutation();
        if (ok) { notify(tr("操作已完成。")); emit operationSucceeded(operation); refresh(); }
    });
}
void SunshineClient::pair(const QString &pin, const QString &name) {
    if (!QRegularExpression("^[0-9]{4}$").match(pin).hasMatch() || name.trimmed().isEmpty() || name.toUtf8().size() > 256) {
        notify(tr("请输入 4 位数字 PIN 和设备名称。"), true); return;
    }
    mutate("/api/pin", {{"pin", pin}, {"name", name.trimmed()}}, "pair");
}
void SunshineClient::unpair(const QString &uuid) { mutate("/api/clients/unpair", {{"uuid", uuid}}, "clients"); }
void SunshineClient::renameClient(const QString &uuid, const QString &name) {
    if (name.trimmed().isEmpty() || name.toUtf8().size() > 256) { notify(tr("请输入有效的设备名称。"), true); return; }
    mutate("/api/clients/rename", {{"uuid", uuid}, {"name", name.trimmed()}}, "clients");
}
void SunshineClient::setClientEnabled(const QString &uuid, bool enabled) {
    mutate("/api/clients/update", {{"uuid", uuid}, {"enabled", enabled}}, "clients");
}
void SunshineClient::setup(const QString &username, const QString &password, const QString &confirmation) {
    if (!m_setupRequired) return;
    if (username.trimmed().isEmpty() || username.toUtf8().size() > 128 || password.isEmpty() ||
        password.toUtf8().size() > 256 || password != confirmation) {
        notify(tr("请输入有效用户名，确保两次密码一致且非空。"), true); return;
    }
    mutate("/api/password", {{"newUsername", username.trimmed()}, {"newPassword", password},
           {"confirmNewPassword", confirmation}}, "setup");
}
void SunshineClient::stopSessions() { mutate("/api/runtime/sessions/stop", {}, "stop"); }
void SunshineClient::restart() {
    if (dirty() || !beginMutation()) return;
    const int nextPort = value("port", QString::number(m_port - 1)).toInt() + 1;
    // The legacy /api/restart endpoint may exit without replying. The tray API acknowledges first.
    request("POST", "/api/tray/action", {{"action", "restart"}},
        [this, nextPort](bool ok, const QJsonObject &, const QByteArray &) {
            endMutation();
            if (!ok) return;
            m_connected = false; m_sessions = {}; emit stateChanged(); emit sessionsChanged();
            notify(tr("主机已接受重启请求，正在等待重新连接。"));
            emit operationSucceeded("restart");
            QTimer::singleShot(1800, this, [this, nextPort] { setPort(nextPort); refresh(); });
        });
}

void SunshineClient::startHost() {
    if (m_connected) return;
    QString path = m_executable;
    if (path.isEmpty()) {
        QDir directory(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
        path = directory.absoluteFilePath("../../sunshine.exe");
#else
        path = directory.absoluteFilePath("sunshine");
#endif
    }
    const QFileInfo file(path);
    if (!file.isFile() || !file.isExecutable() || file.canonicalFilePath() == QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath()) {
        notify(tr("请在“连接设置”中选择已有的 Sunshine 内核可执行文件。"), true); return;
    }
    if (!QProcess::startDetached(file.absoluteFilePath(), {}, file.absolutePath())) {
        notify(tr("无法启动 Sunshine 内核，请检查文件权限。"), true); return;
    }
    notify(tr("已启动 Sunshine，正在等待本地管理接口就绪。"));
    QTimer::singleShot(2000, this, &SunshineClient::refresh);
}

bool SunshineClient::exportLogs(const QUrl &url) {
    if (!url.isLocalFile()) return false;
    QSaveFile file(url.toLocalFile());
    const auto bytes = m_logs.toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        notify(tr("日志导出失败：%1").arg(file.errorString()), true); return false;
    }
    notify(tr("日志已导出。")); return true;
}
