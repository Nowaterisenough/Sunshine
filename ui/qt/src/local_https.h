#pragma once

#include <QDateTime>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslError>

inline void allowLocalSunshineCertificate(QNetworkReply *reply) {
    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError> &errors) {
        const auto url = reply->url();
        if (url.scheme() != "https" || (url.host() != "127.0.0.1" && url.host() != "::1" && url.host() != "localhost")) return;
        const auto chain = reply->sslConfiguration().peerCertificateChain();
        if (chain.isEmpty()) return;
        const auto now = QDateTime::currentDateTimeUtc();
        if (chain.first().effectiveDate() > now || chain.first().expiryDate() < now) {
            qWarning("Local Sunshine certificate is outside its validity period.");
            return;
        }
        for (const auto &error : errors) {
            // Secure Transport reports a self-signed leaf as an untrusted root.
            if (error.error() == QSslError::CertificateUntrusted && chain.size() == 1 &&
                chain.first().isSelfSigned() && error.certificate() == chain.first()) continue;
            if (error.error() != QSslError::SelfSignedCertificate && error.error() != QSslError::HostNameMismatch) {
                qWarning("Local Sunshine TLS error (%d): %s", error.error(), qPrintable(error.errorString()));
                return;
            }
        }
        reply->ignoreSslErrors(errors);
    });
}
