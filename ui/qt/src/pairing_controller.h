#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <functional>

class PairingController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool pending READ pending NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString requestId READ requestId NOTIFY changed)
    Q_PROPERTY(QString clientName READ clientName NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit PairingController(QObject *parent = nullptr, QNetworkAccessManager *network = nullptr);
    ~PairingController() override;
    bool pending() const { return m_pending; }
    bool busy() const { return m_busy; }
    QString requestId() const { return m_requestId; }
    QString clientName() const { return m_clientName; }
    QString error() const { return m_error; }
    void start(int port);
    void setPort(int port);
    void reconnectIfIdle();
    Q_INVOKABLE void submit(const QString &pin, const QString &name, const QString &expectedRequest);
signals:
    void changed();
    void requestReceived();
    void pinSubmitted();
private:
    void stop();
    void connectEvents();
    bool consumeEvents();
    bool applyState(const QJsonObject &state);
    static bool validState(const QJsonObject &state);
    static QString pairingKey(const QJsonObject &state);
    QNetworkReply *send(const QByteArray &method, const QString &path, const QByteArray &body = {});
    void jsonRequest(const QByteArray &method, const QString &path, const QJsonObject &body,
                     std::function<void(bool, const QJsonObject &)> callback);
    void setError(const QString &message);
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_stream;
    QPointer<QNetworkReply> m_request;
    QTimer m_retry, m_watchdog;
    QByteArray m_buffer;
    QString m_instance, m_requestId, m_seenRequest, m_submittedRequest, m_clientName, m_error;
    qint64 m_revision = -1;
    int m_port = 47990, m_generation = 0, m_retryDelay = 3000;
    bool m_started = false, m_online = false, m_pending = false, m_busy = false;
};
