#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSaveFile>
#include <QTimer>
#include <functional>
#include <memory>

class UpdateController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY changed)
    Q_PROPERTY(QString packageName READ packageName NOTIFY changed)
    Q_PROPERTY(QString packageDescription READ packageDescription CONSTANT)
    Q_PROPERTY(QString lastChecked READ lastChecked NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool canInstall READ canInstall CONSTANT)
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY preferencesChanged)
    Q_PROPERTY(bool autoDownload READ autoDownload WRITE setAutoDownload NOTIFY preferencesChanged)
    Q_PROPERTY(bool showButton READ showButton WRITE setShowButton NOTIFY preferencesChanged)
    Q_PROPERTY(bool includePrerelease READ includePrerelease WRITE setIncludePrerelease NOTIFY preferencesChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY preferencesChanged)
public:
    enum class Package { WindowsInstaller, WindowsPortable, WindowsPanel, MacArmPanel, Unsupported };
    struct Environment {
        QString commit;
        QDateTime commitTime;
        Package package = Package::Unsupported;
        QString cachePath;
        std::function<bool(const QString &, bool)> openPackage;
    };
    explicit UpdateController(QObject *parent = nullptr);
    UpdateController(Environment environment, QNetworkAccessManager *network, QObject *parent = nullptr);
    static Environment environment();
    static bool trustedUrl(const QUrl &url);
    static QJsonObject selectAsset(const QJsonArray &assets, Package package);

    QString state() const { return m_state; }
    QString message() const { return m_message; }
    QString currentVersion() const;
    QString latestVersion() const { return m_release.value("tag_name").toString(); }
    QString releaseNotes() const { return m_release.value("body").toString(); }
    QString packageName() const { return m_asset.value("name").toString(); }
    QString packageDescription() const;
    QString lastChecked() const { return m_lastChecked; }
    double progress() const { return m_progress; }
    bool available() const { return m_available; }
    bool busy() const { return m_state == "checking" || m_state == "downloading"; }
    bool canInstall() const { return m_environment.package == Package::WindowsInstaller; }
    bool autoCheck() const { return m_autoCheck; }
    bool autoDownload() const { return m_autoDownload; }
    bool showButton() const { return m_showButton; }
    bool includePrerelease() const { return m_includePrerelease; }
    int intervalMinutes() const { return m_intervalMinutes; }
    void setAutoCheck(bool value);
    void setAutoDownload(bool value);
    void setShowButton(bool value);
    void setIncludePrerelease(bool value);
    void setIntervalMinutes(int value);
    void start();
    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openDownloaded();
    Q_INVOKABLE void openRelease();
signals:
    void changed();
    void preferencesChanged();
    void installerStarted();
private:
    void status(const QString &state, const QString &message);
    void savePreferences();
    void fetch(const QUrl &url, std::function<void(const QByteArray &)> callback);
    QNetworkReply *get(const QUrl &url, int timeout);
    void inspectRelease(const QJsonObject &release);
    bool verifiedFile() const;
    Environment m_environment;
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_timer;
    QJsonObject m_release, m_asset;
    QString m_state = "idle", m_message, m_lastChecked, m_downloadPath;
    QByteArray m_digest;
    std::unique_ptr<QSaveFile> m_file;
    qint64 m_bytes = 0;
    double m_progress = 0;
    bool m_available = false, m_autoCheck = true, m_autoDownload = false;
    bool m_showButton = true, m_includePrerelease = true;
    int m_intervalMinutes = 60;
    int m_generation = 0;
};
