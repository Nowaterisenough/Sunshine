#include "update_controller.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQueue>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <cstring>

namespace {
const QString currentCommit(40, 'a');
const QString nextCommit(40, 'b');
const QString tag = "qt-" + nextCommit.left(12);
const QByteArray payload = "test package; never executable";
const QString installer = "Sunshine.v2026.1101.WindowsInstaller.exe";
const QString portable = "Sunshine.v2026.1101.Portable-x64.zip";
const QString winPanel = "Sunshine-Qt-Panel-6.12.0-win-x64.zip";
const QString macPanel = "Sunshine-Qt-Panel-6.12.0-macos-arm64.zip";
QJsonObject asset(const QString &name) {
    return {{"name", name}, {"size", payload.size()},
        {"digest", "sha256:" + QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex())},
        {"browser_download_url", "https://github.com/Nowaterisenough/Sunshine/releases/download/" + tag + "/" + name}};
}
QJsonObject release() {
    return {{"tag_name", tag}, {"target_commitish", nextCommit}, {"prerelease", true},
        {"published_at", "2026-11-01T12:00:00Z"}, {"body", "Fixture release notes"},
        {"html_url", "https://github.com/Nowaterisenough/Sunshine/releases/tag/" + tag},
        {"assets", QJsonArray{asset(installer), asset(portable), asset(winPanel), asset(macPanel)}}};
}
struct Response {
    QByteArray bytes;
    int status = 200;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int delay = 0;
    QUrl redirect;
};
class Reply : public QNetworkReply {
public:
    Reply(const QNetworkRequest &request, Response spec, QObject *parent) : QNetworkReply(parent), data(spec.bytes) {
        setUrl(request.url()); setRequest(request);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, spec.status);
        if (spec.error != NoError) setError(spec.error, "fixture error");
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QTimer::singleShot(spec.delay, this, [this, spec] {
            if (isFinished()) return;
            if (!spec.redirect.isEmpty()) emit redirected(spec.redirect);
            if (isFinished()) return;
            emit readyRead();
            if (!isFinished()) { setFinished(true); emit finished(); }
        });
    }
    void abort() override {
        if (isFinished()) return;
        setError(OperationCanceledError, "cancelled"); setFinished(true); emit finished();
    }
    qint64 bytesAvailable() const override { return data.size() - offset + QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char *buffer, qint64 size) override {
        const qint64 count = qMin(size, data.size() - offset);
        if (count <= 0) return -1;
        std::memcpy(buffer, data.constData() + offset, size_t(count)); offset += count; return count;
    }
private:
    QByteArray data;
    qint64 offset = 0;
};
class Network : public QNetworkAccessManager {
public:
    QJsonArray releases{release()};
    QString comparison = "ahead";
    Response package{payload};
    QQueue<Response> overrides;
    QList<QNetworkRequest> calls;
protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *) override {
        Q_ASSERT(operation == GetOperation);
        calls.append(request);
        Response spec;
        if (!overrides.isEmpty()) spec = overrides.dequeue();
        else if (request.url().path().endsWith("/releases")) spec.bytes = QJsonDocument(releases).toJson();
        else if (request.url().path().contains("/compare/")) spec.bytes = QJsonDocument(QJsonObject{{"status", comparison}}).toJson();
        else spec = package;
        return new Reply(request, spec, this);
    }
};
}

class UpdateTest : public QObject {
    Q_OBJECT
    QTemporaryDir root;
    int opened = 0;
    bool installed = false;
    bool launchSuccess = true;
    UpdateController::Environment environment(UpdateController::Package package = UpdateController::Package::WindowsInstaller) {
        return {currentCommit, QDateTime::fromString("2026-10-31T12:00:00Z", Qt::ISODate), package,
            root.path() + "/" + QUuid::createUuid().toString(QUuid::Id128),
            [this](const QString &, bool install) { ++opened; installed = install; return launchSuccess; }};
    }
private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("SunshineTests");
        QCoreApplication::setApplicationName("UpdateContract");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, root.path());
    }
    void init() { QSettings().clear(); opened = 0; installed = false; launchSuccess = true; }
    void selectsOnlyTheCorrectPackage_data() {
        QTest::addColumn<int>("kind"); QTest::addColumn<QString>("name");
        QTest::newRow("installed") << int(UpdateController::Package::WindowsInstaller) << installer;
        QTest::newRow("portable") << int(UpdateController::Package::WindowsPortable) << portable;
        QTest::newRow("win-panel") << int(UpdateController::Package::WindowsPanel) << winPanel;
        QTest::newRow("mac-arm") << int(UpdateController::Package::MacArmPanel) << macPanel;
    }
    void selectsOnlyTheCorrectPackage() {
        QFETCH(int, kind); QFETCH(QString, name);
        Network net; UpdateController controller(environment(UpdateController::Package(kind)), &net);
        controller.check(); QTRY_COMPARE(controller.state(), "available");
        QCOMPARE(controller.packageName(), name);
        for (const auto &call : net.calls) {
            QVERIFY(UpdateController::trustedUrl(call.url()));
            QCOMPARE(call.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(), int(QNetworkRequest::UserVerifiedRedirectPolicy));
            QVERIFY(!call.hasRawHeader("Authorization"));
        }
    }
    void ignoresLegacyAndDraftAndHonorsStableChannel() {
        Network net;
        auto legacy = release(); legacy["tag_name"] = "v9999.0";
        auto draft = release(); draft["draft"] = true;
        net.releases.prepend(legacy); net.releases.prepend(draft);
        UpdateController controller(environment(), &net);
        controller.setIncludePrerelease(false);
        controller.check(); QTRY_COMPARE(controller.state(), "current"); QVERIFY(!controller.available());
        controller.setIncludePrerelease(true);
        controller.check(); QTRY_COMPARE(controller.state(), "available");
    }
    void sameCommitDoesNotOfferUpdate() {
        Network net; auto env = environment(); env.commit = nextCommit;
        UpdateController controller(env, &net);
        controller.check(); QTRY_COMPARE(controller.state(), "current");
        QVERIFY(!controller.available()); QCOMPARE(net.calls.size(), 1);
    }
    void refusesDowngradeAndUnrelatedBranch_data() {
        QTest::addColumn<QString>("relation"); QTest::addColumn<QString>("state");
        QTest::newRow("older") << "behind" << "current";
        QTest::newRow("same") << "identical" << "current";
        QTest::newRow("diverged") << "diverged" << "error";
        QTest::newRow("malformed") << "" << "error";
    }
    void refusesDowngradeAndUnrelatedBranch() {
        QFETCH(QString, relation); QFETCH(QString, state);
        Network net; net.comparison = relation;
        UpdateController controller(environment(), &net);
        controller.check(); QTRY_COMPARE(controller.state(), state); QVERIFY(!controller.available());
    }
    void refusesUnsafeOrUnverifiablePackage_data() {
        QTest::addColumn<QString>("field"); QTest::addColumn<QJsonValue>("value");
        QTest::newRow("digest") << "digest" << QJsonValue("sha256:bad");
        QTest::newRow("http") << "browser_download_url" << QJsonValue("http://github.com/Nowaterisenough/Sunshine/releases/download/" + tag + "/" + installer);
        QTest::newRow("other-host") << "browser_download_url" << QJsonValue("https://example.com/" + installer);
        QTest::newRow("other-tag") << "browser_download_url" << QJsonValue("https://github.com/Nowaterisenough/Sunshine/releases/download/qt-old/" + installer);
        QTest::newRow("empty") << "size" << QJsonValue(0);
        QTest::newRow("oversize") << "size" << QJsonValue(2.0 * 1024 * 1024 * 1024);
    }
    void refusesUnsafeOrUnverifiablePackage() {
        QFETCH(QString, field); QFETCH(QJsonValue, value);
        Network net; auto badAsset = asset(installer); badAsset[field] = value;
        auto badRelease = release(); badRelease["assets"] = QJsonArray{badAsset}; net.releases = {badRelease};
        UpdateController controller(environment(), &net);
        controller.check(); QTRY_COMPARE(controller.state(), "error");
        controller.download(); controller.openDownloaded(); QCOMPARE(opened, 0);
    }
    void rejectsAmbiguousAndMissingAssets() {
        const auto kind = UpdateController::Package::WindowsInstaller;
        QVERIFY(UpdateController::selectAsset({asset(installer), asset(installer)}, kind).isEmpty());
        QVERIFY(UpdateController::selectAsset({asset(winPanel), asset(portable)}, kind).isEmpty());
        QVERIFY(!UpdateController::trustedUrl(QUrl("https://github.com.evil.test/x")));
        QVERIFY(!UpdateController::trustedUrl(QUrl("https://github.com@evil.test/x")));
        QVERIFY(!UpdateController::trustedUrl(QUrl("https://github.com:444/Nowaterisenough/Sunshine/releases/x")));
        QVERIFY(UpdateController::trustedUrl(QUrl("https://release-assets.githubusercontent.com/file?sig=fixture")));
    }
    void verifiesBeforeOpeningAndLeavesInstallationExplicit() {
        Network net; UpdateController controller(environment(), &net);
        QSignalSpy launched(&controller, &UpdateController::installerStarted);
        controller.setAutoDownload(true);
        controller.check(); QTRY_COMPARE(controller.state(), "ready");
        QCOMPARE(controller.progress(), 1.0); QCOMPARE(opened, 0); QCOMPARE(launched.size(), 0);
        launchSuccess = false;
        controller.openDownloaded(); QCOMPARE(controller.state(), "ready"); QCOMPARE(launched.size(), 0);
        launchSuccess = true;
        controller.openDownloaded(); QCOMPARE(launched.size(), 1); QVERIFY(installed);
    }
    void archivesNeverLaunchAnInstaller() {
        Network net; UpdateController controller(environment(UpdateController::Package::MacArmPanel), &net);
        controller.check(); QTRY_VERIFY(controller.available()); controller.download();
        QTRY_COMPARE(controller.state(), "ready");
        QSignalSpy launched(&controller, &UpdateController::installerStarted);
        controller.openDownloaded(); QCOMPARE(opened, 1); QVERIFY(!installed); QCOMPARE(launched.size(), 0);
    }
    void corruptOrIncompleteDownloadsCannotBeOpened_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("truncated") << payload.left(4);
        QTest::newRow("changed") << QByteArray(payload.size(), 'x');
        QTest::newRow("too-large") << payload + "extra";
    }
    void corruptOrIncompleteDownloadsCannotBeOpened() {
        QFETCH(QByteArray, bytes);
        Network net; net.package.bytes = bytes;
        UpdateController controller(environment(), &net);
        controller.check(); QTRY_VERIFY(controller.available()); controller.download();
        QTRY_COMPARE(controller.state(), "error"); controller.openDownloaded(); QCOMPARE(opened, 0);
        net.package.bytes = payload; controller.download(); QTRY_COMPARE(controller.state(), "ready");
    }
    void rechecksFileBeforeInstallation() {
        Network net; const auto env = environment(); UpdateController controller(env, &net);
        controller.check(); QTRY_VERIFY(controller.available()); controller.download(); QTRY_COMPARE(controller.state(), "ready");
        QFile file(env.cachePath + "/" + tag + "/" + installer);
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("modified"); file.close();
        controller.openDownloaded(); QCOMPARE(controller.state(), "error"); QCOMPARE(opened, 0);
    }
    void cancellationIgnoresLateRepliesAndAllowsRetry() {
        Network net; UpdateController controller(environment(), &net);
        net.overrides.enqueue({QJsonDocument(net.releases).toJson(), 200, QNetworkReply::NoError, 100});
        controller.check(); controller.cancel(); QTest::qWait(150);
        QCOMPARE(controller.state(), "idle"); QVERIFY(!controller.available());
        controller.check(); QTRY_VERIFY(controller.available());
        net.package.delay = 100; controller.download(); controller.cancel(); QTest::qWait(150);
        QCOMPARE(controller.state(), "available"); QCOMPARE(opened, 0);
        net.package.delay = 0; controller.download(); QTRY_COMPARE(controller.state(), "ready");
    }
    void tlsAndHttpFailuresAreVisibleAndRecoverable_data() {
        QTest::addColumn<int>("error"); QTest::addColumn<int>("status");
        QTest::newRow("tls") << int(QNetworkReply::SslHandshakeFailedError) << 0;
        QTest::newRow("rate-limit") << int(QNetworkReply::ContentAccessDenied) << 403;
        QTest::newRow("timeout") << int(QNetworkReply::TimeoutError) << 0;
    }
    void tlsAndHttpFailuresAreVisibleAndRecoverable() {
        QFETCH(int, error); QFETCH(int, status);
        Network net; net.overrides.enqueue({{}, status, QNetworkReply::NetworkError(error)});
        UpdateController controller(environment(), &net);
        controller.check(); QTRY_COMPARE(controller.state(), "error");
        QVERIFY(!controller.message().isEmpty()); controller.check(); QTRY_COMPARE(controller.state(), "available");
    }
    void maliciousRedirectIsRejected() {
        Network net; net.overrides.enqueue({"[]", 200, QNetworkReply::NoError, 0, QUrl("http://example.com/file")});
        UpdateController controller(environment(), &net);
        controller.check(); QTRY_COMPARE(controller.state(), "error"); QVERIFY(!controller.available());
    }
    void preferencesPersistAndDefaultsDoNotAutoInstall() {
        Network net; UpdateController first(environment(), &net);
        QVERIFY(first.autoCheck()); QVERIFY(first.includePrerelease()); QVERIFY(!first.autoDownload());
        first.setAutoCheck(false); first.setShowButton(false); first.setIntervalMinutes(360);
        first.setIncludePrerelease(false); first.setAutoDownload(true);
        UpdateController second(environment(), &net);
        QVERIFY(!second.autoCheck()); QVERIFY(!second.showButton()); QVERIFY(!second.includePrerelease());
        QVERIFY(second.autoDownload()); QCOMPARE(second.intervalMinutes(), 360);
        second.setIntervalMinutes(0); QCOMPARE(second.intervalMinutes(), 360);
        second.start(); QCOMPARE(net.calls.size(), 0); QCOMPARE(opened, 0);
    }
};
QTEST_GUILESS_MAIN(UpdateTest)
#include "update_test.moc"
