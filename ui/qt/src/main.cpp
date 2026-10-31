#include "sunshine_client.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QIcon>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QSslSocket>
#include <QSystemTrayIcon>
#include <QTemporaryDir>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Sunshine");
    QCoreApplication::setApplicationName("SunshinePanel");
    QCoreApplication::setApplicationVersion("1.0.0");
    QQuickStyle::setStyle("Basic");
#ifdef Q_OS_WIN
    app.setFont(QFont("Segoe UI Variable", 10));
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("Sunshine native control panel (Qt 6.12)");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"hidden", "Start in the system tray."});
    parser.addOption({"remove-autostart", "Remove the legacy per-user startup entry (installer compatibility)."});
    parser.addOption({"url", "Open a local Sunshine management page.", "url"});
    parser.addOption({"port", "Local Sunshine HTTPS management port.", "port"});
    parser.addOption({"page", "Initial page (0-7).", "index", "0"});
    parser.addOption({"section", "Initial streaming settings section (0-3).", "index", "0"});
    parser.addOption({"screenshot", "Save a window image for visual verification.", "path"});
    parser.addOption({"quit-after", "Exit after N milliseconds (UI smoke tests).", "milliseconds"});
    parser.addOption({"smoke-test", "Use isolated temporary settings and IPC for UI tests."});
    parser.addOption({"size", "Initial window size, for example 820x580.", "size"});
    parser.addOption({"dark", "Start with the dark theme (visual verification)."});
    parser.addOption({"light", "Start with the light theme (visual verification)."});
    parser.process(app);
    if (parser.isSet("remove-autostart")) {
#ifdef Q_OS_WIN
        QSettings startup("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        startup.remove("Sunshine GUI Desktop");
        startup.sync();
        return startup.status() == QSettings::NoError ? 0 : 1;
#else
        return 0;
#endif
    }

    QTemporaryDir testProfile;
    if (parser.isSet("smoke-test")) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, testProfile.path());
    }
    const auto runtime = parser.isSet("smoke-test") ? testProfile.path() : QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(runtime);
    const auto socketName = "sunshine-panel-" + QString::fromLatin1(QCryptographicHash::hash(runtime.toUtf8(), QCryptographicHash::Sha256).toHex().left(20));
    QLockFile lock(runtime + "/panel.lock");
    lock.setStaleLockTime(0);
    if (!lock.tryLock()) {
        QLocalSocket socket;
        socket.connectToServer(socketName);
        if (!socket.waitForConnected(1500)) return 1;
        socket.write(QJsonDocument(QJsonObject{{"hidden", parser.isSet("hidden")},
            {"url", parser.value("url")}}).toJson(QJsonDocument::Compact) + '\n');
        socket.waitForBytesWritten(1000);
        return 0;
    }

#ifdef Q_OS_MACOS
    // Official Qt kits also ship an OpenSSL plugin without its external libraries.
    if (!QSslSocket::setActiveBackend("securetransport")) {
        qCritical("The deployed macOS Secure Transport TLS plugin is unavailable.");
        return 5;
    }
#endif
    SunshineClient client;
    if (parser.isSet("port")) client.setPort(parser.value("port").toInt());
    QQmlApplicationEngine engine;
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    engine.setInitialProperties({{"host", QVariant::fromValue(&client)},
        {"trayAvailable", trayAvailable}, {"forceDark", parser.isSet("dark")}, {"forceLight", parser.isSet("light")},
        {"settingsCategory", qBound(0, parser.value("section").toInt(), 3)},
        {"initialPage", qBound(0, parser.value("page").toInt(), 7)}});
    engine.loadFromModule("Sunshine.Panel", "Main");
    if (engine.rootObjects().isEmpty()) return 2;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window) return 3;
    if (parser.isSet("size")) {
        const auto dimensions = parser.value("size").split('x');
        if (dimensions.size() == 2) window->resize(qMax(720, dimensions[0].toInt()), qMax(480, dimensions[1].toInt()));
    }
    app.setWindowIcon(QIcon(":/qt/qml/Sunshine/Panel/assets/sunshine.svg"));
    app.setQuitOnLastWindowClosed(!trayAvailable);

    auto showWindow = [window] { window->show(); window->raise(); window->requestActivate(); };
    auto quitPanel = [&] { window->setProperty("quitting", true); app.quit(); };
    auto navigate = [&](const QString &urlText) {
        if (!urlText.isEmpty()) {
            const QUrl url(urlText);
            if ((url.host() == "127.0.0.1" || url.host() == "localhost" || url.host() == "::1") &&
                (url.scheme() == "https" || url.scheme() == "http")) {
                if (url.port() > 0) client.setPort(url.port());
                const QStringList paths = {"/", "/apps", "/pin", "/config", "/troubleshooting", "/about", "/welcome"};
                const auto index = paths.indexOf(url.path());
                if (index >= 0) window->setProperty("pageIndex", index);
            }
        }
        showWindow();
    };
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(socketName);
    if (!server.listen(socketName)) return 4;
    QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
        while (auto *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            QObject::connect(socket, &QLocalSocket::readyRead, &app, [&, socket] {
                if (socket->bytesAvailable() > 8192) { socket->disconnectFromServer(); return; }
                if (!socket->canReadLine()) return;
                const auto document = QJsonDocument::fromJson(socket->readLine());
                if (document.isObject() && !document.object().value("hidden").toBool())
                    navigate(document.object().value("url").toString());
                socket->disconnectFromServer();
            });
        }
    });

    QMenu menu;
    menu.addAction(QObject::tr("打开 Sunshine"), &app, showWindow);
    menu.addAction(QObject::tr("刷新主机状态"), &client, &SunshineClient::refresh);
    menu.addSeparator();
    menu.addAction(QObject::tr("退出控制面板"), &app, quitPanel);
    QSystemTrayIcon tray(app.windowIcon());
    tray.setToolTip("Sunshine"); tray.setContextMenu(&menu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) showWindow();
    });
    if (trayAvailable) tray.show();
    if (parser.isSet("hidden") && trayAvailable) window->hide();
    else navigate(parser.value("url"));
    client.startPolling();

    if (parser.isSet("screenshot")) {
        const QString path = parser.value("screenshot");
        QTimer::singleShot(1800, &app, [window, path] { window->grabWindow().save(path); });
    }
    if (parser.isSet("quit-after"))
        QTimer::singleShot(qMax(2000, parser.value("quit-after").toInt()), &app, quitPanel);
    return app.exec();
}
