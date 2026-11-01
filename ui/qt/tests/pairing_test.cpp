#include "pairing_controller.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QQueue>
#include <QSignalSpy>
#include <QtTest>
#include <cstring>

namespace {
QJsonObject state(qint64 revision, qint64 id = 0, const QString &instance = "core-a") {
    return {{"protocol_version", 1}, {"instance_id", instance}, {"owner", "gui"},
        {"revision", revision}, {"status", id ? "pairing" : "idle"}, {"pairing_client_name", "Living room TV"},
        {"capabilities", QJsonArray{"state-v1", "events-v1", "pairing"}},
        {"notification", QJsonObject{{"id", id}, {"active", id != 0}, {"action", id ? "open_pin" : ""}}}};
}
QByteArray sseFrame(const QJsonObject &value) {
    return "event: tray-state\ndata: " + QJsonDocument(value).toJson(QJsonDocument::Compact) + "\n\n";
}
struct Response { QByteArray data; int code = 200; int delay = 0; };
class Reply : public QNetworkReply {
public:
    Reply(const QNetworkRequest &request, QObject *parent, bool streaming, Response response = {}) : QNetworkReply(parent) {
        setRequest(request); setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, response.code);
        setHeader(QNetworkRequest::ContentTypeHeader, streaming ? "text/event-stream; charset=utf-8" : "application/json");
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!streaming) QTimer::singleShot(response.delay, this, [this, response] {
            if (isFinished()) return;
            push(response.data);
            if (!isFinished()) { setFinished(true); emit finished(); }
        });
    }
    void push(const QByteArray &data) { if (!isFinished()) { bytes += data; emit readyRead(); } }
    void abort() override {
        if (isFinished()) return;
        setError(OperationCanceledError, "cancelled"); setFinished(true); emit finished();
    }
    qint64 bytesAvailable() const override { return bytes.size() + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char *data, qint64 length) override {
        const auto count = qMin(length, qint64(bytes.size()));
        if (count <= 0) return -1;
        std::memcpy(data, bytes.constData(), size_t(count)); bytes.remove(0, count); return count;
    }
private:
    QByteArray bytes;
};
class Network : public QNetworkAccessManager {
public:
    struct Call { QByteArray method; QNetworkRequest request; QJsonObject body; };
    QList<Call> calls;
    QJsonObject current = state(1);
    QPointer<Reply> stream;
    QQueue<Response> preflights, pins;
    void publish(const QJsonObject &value) { current = value; stream->push(sseFrame(value)); }
    int count(const QString &path) const {
        return int(std::count_if(calls.begin(), calls.end(), [&](const auto &call) { return call.request.url().path() == path; }));
    }
protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *data) override {
        calls.append({request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray(), request,
            QJsonDocument::fromJson(data ? data->readAll() : QByteArray()).object()});
        if (request.url().path() == "/api/tray/events") {
            stream = new Reply(request, this, true);
            auto *reply = stream.data();
            QTimer::singleShot(0, reply, [this, reply] { reply->push(sseFrame(current)); });
            return reply;
        }
        auto &queue = request.url().path() == "/api/pin" ? pins : preflights;
        Response response = queue.isEmpty() ? Response{request.url().path() == "/api/pin"
            ? QByteArray(R"({"status":"true"})") : QJsonDocument(current).toJson()} : queue.dequeue();
        return new Reply(request, this, false, response);
    }
};
}

class PairingTest : public QObject {
    Q_OBJECT
private slots:
    void noPairingPopupForNormalConnections() {
        Network net; PairingController controller(nullptr, &net);
        QSignalSpy opened(&controller, &PairingController::requestReceived);
        controller.start(47990); QTest::qWait(10);
        auto streaming = state(2); streaming["status"] = "streaming"; net.publish(streaming);
        for (int i = 0; i < 5; ++i) net.stream->push(": tray-keepalive\n\n");
        QCOMPARE(opened.size(), 0); QVERIFY(!controller.pending()); QCOMPARE(net.calls.size(), 1);
        QCOMPARE(net.calls.first().request.rawHeader("Accept"), QByteArray("text/event-stream"));
    }
    void fragmentedAndRepeatedEventsOpenOnce() {
        Network net; PairingController controller(nullptr, &net);
        QSignalSpy opened(&controller, &PairingController::requestReceived);
        controller.start(47990); QTest::qWait(10);
        auto request = state(2, 7); request["pairing_client_name"] = QString::fromUtf8("客厅电视");
        auto bytes = sseFrame(request); bytes.replace("\n", "\r\n");
        for (int offset = 0; offset < bytes.size(); offset += 3) net.stream->push(bytes.mid(offset, 3));
        QCOMPARE(opened.size(), 1); QVERIFY(controller.pending()); QCOMPARE(controller.requestId(), "core-a/7");
        QCOMPARE(controller.clientName(), QString::fromUtf8("客厅电视"));
        net.publish(request); request["revision"] = 3; net.publish(request);
        net.publish(state(1));
        QCOMPARE(opened.size(), 1); QVERIFY(controller.pending());
        net.publish(state(4, 8)); QCOMPARE(opened.size(), 2);
        net.publish(state(1, 1, "core-b")); QCOMPARE(opened.size(), 3);
    }
    void cancellationAndDisconnectInvalidateInput() {
        Network net; net.current = state(1, 1);
        PairingController controller(nullptr, &net); controller.start(47990);
        QTRY_VERIFY(controller.pending());
        net.publish(state(2)); QVERIFY(!controller.pending());
        controller.submit("1234", "TV", "core-a/1"); QCOMPARE(net.count("/api/pin"), 0);
        net.publish(state(3, 2)); QVERIFY(controller.pending()); net.stream->abort();
        QVERIFY(!controller.pending()); controller.submit("1234", "TV", "core-a/2");
        QCOMPARE(net.count("/api/tray/state"), 0);
    }
    void clientNamesCannotExpandThePopupWithControlWhitespace() {
        Network net; net.current = state(1, 1);
        net.current["pairing_client_name"] = QString("  TV\n\t") + QString(100, 'x');
        PairingController controller(nullptr, &net); controller.start(47990);
        QTRY_VERIFY(controller.pending());
        QVERIFY(!controller.clientName().contains('\n')); QVERIFY(!controller.clientName().contains('\t'));
        QCOMPARE(controller.clientName().size(), 64);
    }
    void reconnectRestoresSameRequestWithoutAnotherAlert() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        QSignalSpy opened(&controller, &PairingController::requestReceived);
        controller.start(47990); QTRY_COMPARE(opened.size(), 1);
        net.stream->abort(); QVERIFY(!controller.pending());
        QTRY_VERIFY_WITH_TIMEOUT(controller.pending(), 5000);
        QVERIFY(controller.error().isEmpty());
        QCOMPARE(opened.size(), 1); QCOMPARE(net.count("/api/tray/events"), 2);
        net.stream->abort(); controller.reconnectIfIdle();
        QTRY_VERIFY_WITH_TIMEOUT(controller.pending(), 1000);
        QCOMPARE(opened.size(), 1); QCOMPARE(net.count("/api/tray/events"), 3);
    }
    void rejectsInvalidPinsAndRechecksBeforeSubmission() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        QSignalSpy submitted(&controller, &PairingController::pinSubmitted);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        for (const auto &pin : {"123", "12345", "12ab"}) controller.submit(pin, "TV", controller.requestId());
        controller.submit("1234", "", controller.requestId());
        controller.submit("1234", "TV", "stale"); QCOMPARE(net.count("/api/tray/state"), 0);
        controller.submit("0123", " TV ", controller.requestId());
        controller.submit("1234", "TV", controller.requestId());
        QTRY_COMPARE(submitted.size(), 1);
        QCOMPARE(net.count("/api/tray/state"), 1); QCOMPARE(net.count("/api/pin"), 1);
        QCOMPARE(net.calls.last().body.value("pin").toString(), "0123");
        QCOMPARE(net.calls.last().body.value("name").toString(), "TV");
        QVERIFY(!controller.pending()); QVERIFY(!controller.busy());
        net.publish(state(2, 1)); QVERIFY(!controller.pending());
        for (const auto &call : net.calls) {
            QCOMPARE(call.request.url().host(), "127.0.0.1"); QCOMPARE(call.request.url().scheme(), "https");
            QCOMPARE(call.request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(), int(QNetworkRequest::ManualRedirectPolicy));
        }
    }
    void staleSnapshotNeverPosts() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.current = state(3, 2);
        controller.submit("1234", "TV", controller.requestId());
        QTRY_VERIFY(!controller.busy()); QCOMPARE(net.count("/api/pin"), 0);
        QCOMPARE(controller.requestId(), "core-a/2"); QVERIFY(controller.pending()); QVERIFY(controller.error().isEmpty());
    }
    void expirationWhileCheckingNeverPosts() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.preflights.enqueue({QJsonDocument(net.current).toJson(), 200, 100});
        controller.submit("1234", "TV", controller.requestId()); net.publish(state(2));
        QTRY_VERIFY(!controller.busy()); QCOMPARE(net.count("/api/pin"), 0);
    }
    void changedPortDiscardsOldReplies() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.preflights.enqueue({QJsonDocument(net.current).toJson(), 200, 100});
        controller.submit("1234", "TV", controller.requestId());
        net.current = state(1, 1, "another-host"); controller.setPort(49990);
        QTRY_COMPARE(controller.requestId(), "another-host/1"); QTest::qWait(150);
        QVERIFY(!controller.busy()); QCOMPARE(net.count("/api/pin"), 0);
        QCOMPARE(net.calls.last().request.url().port(), 49990);
    }
    void postFailureKeepsRequestRetryable() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        QSignalSpy submitted(&controller, &PairingController::pinSubmitted);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.pins.enqueue({R"({"status":"false"})"});
        controller.submit("1234", "TV", controller.requestId()); QTRY_VERIFY(!controller.busy());
        QVERIFY(controller.pending()); QVERIFY(!controller.error().isEmpty()); QCOMPARE(submitted.size(), 0);
        controller.submit("5678", "TV", controller.requestId()); QTRY_COMPARE(submitted.size(), 1);
    }
    void malformedAndOversizedStreamsFailClosed() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.stream->push("event: tray-state\ndata: invalid\n\n"); QVERIFY(!controller.pending());
        controller.start(49990); QTRY_VERIFY(controller.pending());
        net.stream->push(QByteArray(256 * 1024 + 1, 'x')); QVERIFY(!controller.pending());
    }
    void rejectsUnknownProtocolAndRedirects() {
        Network net; net.current = state(1, 1); net.current["protocol_version"] = 2;
        PairingController controller(nullptr, &net); controller.start(47990);
        QTest::qWait(10); QVERIFY(!controller.pending());
        net.current = state(1, 1); controller.start(49990); QTRY_VERIFY(controller.pending());
        net.preflights.enqueue({"{}", 307});
        controller.submit("1234", "TV", controller.requestId()); QTRY_VERIFY(!controller.busy());
        QCOMPARE(net.count("/api/pin"), 0);
    }
    void oldSubmissionDoesNotDismissNewRequest() {
        Network net; net.current = state(1, 1); PairingController controller(nullptr, &net);
        controller.start(47990); QTRY_VERIFY(controller.pending());
        net.pins.enqueue({R"({"status":true})", 200, 100});
        controller.submit("1234", "TV", controller.requestId()); QTRY_COMPARE(net.count("/api/pin"), 1);
        net.publish(state(3, 2)); QTRY_VERIFY(!controller.busy());
        QVERIFY(controller.pending()); QCOMPARE(controller.requestId(), "core-a/2");
    }
};
QTEST_GUILESS_MAIN(PairingTest)
#include "pairing_test.moc"
