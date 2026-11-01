#include "sunshine_client.h"
#include <QBuffer>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QQueue>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <cstring>

struct Response {
    QByteArray body = R"({"status":"true"})";
    int code = 200;
    int delay = 0;
    QString redirect;
};

class Reply : public QNetworkReply {
public:
    Reply(const QNetworkRequest &request, Response spec, QObject *parent) : QNetworkReply(parent), bytes(spec.body) {
        setRequest(request); setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, spec.code);
        if (!spec.redirect.isEmpty()) setAttribute(QNetworkRequest::RedirectionTargetAttribute, QUrl(spec.redirect));
        if (spec.code >= 400) setError(QNetworkReply::ContentAccessDenied, "HTTP error");
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QTimer::singleShot(spec.delay, this, [this] { setFinished(true); emit readyRead(); emit finished(); });
    }
    void abort() override { setError(OperationCanceledError, "aborted"); }
    qint64 bytesAvailable() const override { return bytes.size() - offset + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char *data, qint64 length) override {
        const auto count = qMin(length, qint64(bytes.size()) - offset);
        if (count <= 0) return -1;
        std::memcpy(data, bytes.constData() + offset, size_t(count)); offset += count; return count;
    }
private:
    QByteArray bytes;
    qint64 offset = 0;
};

class Network : public QNetworkAccessManager {
public:
    struct Call { QByteArray method; QNetworkRequest request; QJsonObject body; };
    QList<Call> calls;
    QHash<QString, QQueue<Response>> replies;
    QJsonObject config{{"status", "true"}, {"platform", "windows"}, {"version", "test"}, {"sunshine_name", "Old"},
        {"max_bitrate", "40000"}, {"custom_option", "preserve"}, {"display_devices", QJsonArray()}, {"widget_token_configured", "true"}};
    QJsonArray apps{QJsonObject{{"name", "Desktop"}, {"cmd", ""}, {"prep-cmd", QJsonArray{QJsonObject{{"do", "prepare"}}}}}};
    void enqueue(const QString &key, Response response) { replies[key].enqueue(response); }
    int count(const QByteArray &method, const QString &path) const {
        return int(std::count_if(calls.begin(), calls.end(), [&](const Call &call) { return call.method == method && call.request.url().path() == path; }));
    }
protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *data) override {
        const auto method = request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
        const auto body = QJsonDocument::fromJson(data ? data->readAll() : QByteArray()).object();
        calls.append({method, request, body});
        const auto path = request.url().path();
        const auto key = QString::fromLatin1(method) + " " + path;
        if (!replies[key].isEmpty()) return new Reply(request, replies[key].dequeue(), this);
        Response response;
        if (method == "GET" && path == "/api/config") response.body = QJsonDocument(config).toJson();
        else if (method == "GET" && path == "/api/apps") response.body = QJsonDocument(QJsonObject{{"apps", apps}}).toJson();
        else if (method == "GET" && path == "/api/clients/list") response.body = R"({"status":"true","named_certs":[]})";
        else if (method == "GET" && path == "/api/runtime/sessions") response.body = R"({"success":true,"sessions":[]})";
        else if (method == "POST" && path == "/api/config") { config = body; config["status"] = "true"; }
        return new Reply(request, response, this);
    }
};

class ClientTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QCoreApplication::setOrganizationName("SunshineTests"); QCoreApplication::setApplicationName("PanelContract"); }
    void init() { QSettings().clear(); }
    void cleanup() { QSettings().clear(); }

    void readsExistingApiAndNeverCallsAi() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected()); QTRY_COMPARE(client.apps().size(), 1);
        for (const auto &call : net.calls) {
            QCOMPARE(call.request.url().scheme(), "https"); QCOMPARE(call.request.url().host(), "127.0.0.1");
            QVERIFY(!call.request.url().path().contains("ai"));
            QCOMPARE(call.request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(), int(QNetworkRequest::ManualRedirectPolicy));
        }
    }
    void configSavePreservesUneditedAndConcurrentFields() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        client.setValue("sunshine_name", "New");
        net.config["max_bitrate"] = "60000";
        QSignalSpy success(&client, &SunshineClient::operationSucceeded);
        client.saveConfig(); QTRY_COMPARE(success.size(), 1);
        QCOMPARE(net.config.value("sunshine_name").toString(), "New");
        QCOMPARE(net.config.value("max_bitrate").toString(), "60000");
        QCOMPARE(net.config.value("custom_option").toString(), "preserve");
        QVERIFY(!net.config.contains("display_devices")); QVERIFY(!net.config.contains("widget_token_configured"));
        QVERIFY(!net.config.contains("platform")); QVERIFY(!client.dirty());
    }
    void conflictingEditNeverPosts() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        client.setValue("sunshine_name", "New"); net.config["sunshine_name"] = "Another editor";
        client.saveConfig(); QTRY_VERIFY(!client.busy());
        QCOMPARE(net.count("POST", "/api/config"), 0); QVERIFY(client.dirty()); QVERIFY(client.noticeIsError());
    }
    void stringFalseIsFailureAndKeepsDraft() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        client.setValue("sunshine_name", "New");
        net.enqueue("POST /api/config", {R"({"status":"false","error":"disk full"})"});
        client.saveConfig(); QTRY_VERIFY(!client.busy());
        QVERIFY(client.dirty()); QCOMPARE(client.notice(), "disk full");
    }
    void startupRedirectRequiresSetupWithoutFollowing() {
        Network net; SunshineClient client(nullptr, &net);
        net.enqueue("GET /api/config", {{}, 307, 0, "/welcome"});
        client.refresh(); QTRY_VERIFY(client.setupRequired()); QVERIFY(!client.connected());
        QCOMPARE(net.calls.size(), 1);
    }
    void malformedJsonDoesNotConnect() {
        Network net; SunshineClient client(nullptr, &net);
        net.enqueue("GET /api/config", {"<html>no</html>"});
        client.refresh(); QTRY_VERIFY(client.noticeIsError()); QVERIFY(!client.connected());
    }
    void appEditPreservesAdvancedFields() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_COMPARE(client.apps().size(), 1);
        client.saveApp(0, {{"name", "Renamed"}, {"cmd", "new-command"}});
        QTRY_COMPARE(net.count("POST", "/api/apps"), 1);
        const auto call = *std::find_if(net.calls.begin(), net.calls.end(), [](const Network::Call &c) { return c.method == "POST"; });
        QCOMPARE(call.body.value("apps").toArray(), net.apps);
        const auto edited = call.body.value("editApp").toObject();
        QCOMPARE(edited.value("index").toInt(), 0); QVERIFY(edited.contains("prep-cmd"));
    }
    void changedAppOrderBlocksIndexMutation() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_COMPARE(client.apps().size(), 1);
        net.apps.prepend(QJsonObject{{"name", "New game"}});
        client.deleteApp(0); QTRY_VERIFY(!client.busy());
        QCOMPARE(net.count("DELETE", "/api/apps/0"), 0); QCOMPARE(client.apps().size(), 2);
    }
    void pinValidationAndRequestContract() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        client.pair("12ab", "TV"); QCOMPARE(net.count("POST", "/api/pin"), 0);
        client.pair("123456", "TV"); QCOMPARE(net.count("POST", "/api/pin"), 0);
        client.pair("1234", "TV"); QTRY_COMPARE(net.count("POST", "/api/pin"), 1);
        const auto call = *std::find_if(net.calls.begin(), net.calls.end(), [](const Network::Call &c) { return c.method == "POST"; });
        QCOMPARE(call.body.value("pin").toString(), "1234"); QCOMPARE(call.body.value("name").toString(), "TV");
    }
    void lateReplyFromOldPortIsIgnored() {
        Network net; SunshineClient client(nullptr, &net);
        net.enqueue("GET /api/config", {R"({"status":"true","sunshine_name":"Old host"})", 200, 100});
        client.refresh(); client.setPort(49990); QTRY_VERIFY(client.connected());
        QTest::qWait(150); QCOMPARE(client.config().value("sunshine_name").toString(), "Old");
    }
    void unsavedDraftPreventsSwitchingHost() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected()); client.setValue("sunshine_name", "Draft");
        client.setPort(49990); QCOMPARE(client.port(), 47990); QVERIFY(client.dirty());
    }
    void logsArePlainTextAndExportable() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        net.enqueue("GET /api/logs", {"[Info] hello\n[Error] example\n"});
        client.refreshLogs(); QTRY_VERIFY(!client.logs().isEmpty());
        QCOMPARE(net.calls.last().request.rawHeader("X-Log-Offset"), QByteArray("0"));
        QTemporaryDir dir; QVERIFY(client.exportLogs(QUrl::fromLocalFile(dir.filePath("sunshine.log"))));
        QFile file(dir.filePath("sunshine.log")); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), client.logs().toUtf8());
    }
    void restartUsesAcknowledgedTrayEndpoint() {
        Network net; SunshineClient client(nullptr, &net);
        client.refresh(); QTRY_VERIFY(client.connected());
        QSignalSpy success(&client, &SunshineClient::operationSucceeded);
        client.restart(); QTRY_COMPARE(success.size(), 1);
        QCOMPARE(net.count("POST", "/api/restart"), 0);
        QCOMPARE(net.count("POST", "/api/tray/action"), 1);
        QCOMPARE(net.calls.last().body.value("action").toString(), "restart");
        QVERIFY(!client.connected());
    }
};
QTEST_GUILESS_MAIN(ClientTest)
#include "client_test.moc"
