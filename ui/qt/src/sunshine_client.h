#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

class SunshineClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool setupRequired READ setupRequired NOTIFY stateChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY configChanged)
    Q_PROPERTY(QString notice READ notice NOTIFY noticeChanged)
    Q_PROPERTY(bool noticeIsError READ noticeIsError NOTIFY noticeChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap config READ config NOTIFY configChanged)
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)
    Q_PROPERTY(QVariantList clients READ clients NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(QString logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QString executable READ executable WRITE setExecutable NOTIFY stateChanged)

public:
    explicit SunshineClient(QObject *parent = nullptr, QNetworkAccessManager *network = nullptr);
    bool connected() const { return m_connected; }
    bool busy() const { return m_busy; }
    bool setupRequired() const { return m_setupRequired; }
    bool dirty() const { return !m_edits.isEmpty(); }
    QString notice() const { return m_notice; }
    bool noticeIsError() const { return m_noticeIsError; }
    int port() const { return m_port; }
    QVariantMap config() const;
    QVariantList apps() const { return m_apps.toVariantList(); }
    QVariantList clients() const { return m_clients.toVariantList(); }
    QVariantList sessions() const { return m_sessions.toVariantList(); }
    QString logs() const { return m_logs; }
    QString executable() const { return m_executable; }
    void setPort(int port);
    void setExecutable(const QString &path);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshSessions();
    Q_INVOKABLE void refreshLogs();
    Q_INVOKABLE void setValue(const QString &key, const QString &value);
    Q_INVOKABLE QString value(const QString &key, const QString &fallback = {}) const;
    Q_INVOKABLE void discardChanges();
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void saveApp(int index, const QVariantMap &fields);
    Q_INVOKABLE void deleteApp(int index);
    Q_INVOKABLE void pair(const QString &pin, const QString &name);
    Q_INVOKABLE void unpair(const QString &uuid);
    Q_INVOKABLE void renameClient(const QString &uuid, const QString &name);
    Q_INVOKABLE void setClientEnabled(const QString &uuid, bool enabled);
    Q_INVOKABLE void setup(const QString &username, const QString &password, const QString &confirmation);
    Q_INVOKABLE void stopSessions();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void startHost();
    Q_INVOKABLE void clearNotice();
    Q_INVOKABLE bool exportLogs(const QUrl &url);
    void startPolling();

signals:
    void stateChanged();
    void configChanged();
    void appsChanged();
    void appListInvalidated();
    void clientsChanged();
    void sessionsChanged();
    void logsChanged();
    void noticeChanged();
    void operationSucceeded(const QString &operation);

private:
    using Callback = std::function<void(bool, const QJsonObject &, const QByteArray &)>;
    void request(const QByteArray &method, const QString &path, const QJsonObject &body,
                 Callback callback, bool quiet = false, bool text = false);
    bool beginMutation();
    void endMutation();
    void notify(const QString &text, bool error = false);
    void mutate(const QString &path, const QJsonObject &body, const QString &operation);
    void loadApps();
    void loadClients();
    void changeApp(int index, const QVariantMap &fields, bool remove);
    static QJsonObject writableConfig(const QJsonObject &source);

    QNetworkAccessManager *m_network;
    QTimer m_poll;
    QJsonObject m_config;
    QJsonObject m_edits;
    QJsonArray m_apps, m_clients, m_sessions;
    QString m_notice, m_logs, m_executable;
    int m_port = 47990;
    int m_generation = 0;
    bool m_connected = false, m_busy = false, m_setupRequired = false;
    bool m_noticeIsError = false, m_refreshing = false, m_polling = false, m_loadingLogs = false;
};
