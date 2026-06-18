#include "meting_stream_resolver.h"

#include "meting_api.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

MetingStreamResolver::MetingStreamResolver(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void MetingStreamResolver::resolve(const QString &streamApiUrl)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    const QUrl apiUrl = MetingApi::rewriteServiceUrl(QUrl(streamApiUrl.trimmed()));
    if (!apiUrl.isValid() || apiUrl.isEmpty()) {
        emit failed(QStringLiteral("无效的 stream API 地址"));
        return;
    }

    QNetworkRequest request(apiUrl);
    MetingApi::applyCommonRequestHeaders(request);
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_reply;
        if (!reply) {
            return;
        }
        m_reply = nullptr;
        handleReply(reply);
        reply->deleteLater();
    });
}

void MetingStreamResolver::handleReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit failed(QStringLiteral("解析播放链接失败：%1").arg(reply->errorString()));
        return;
    }

    QUrl streamUrl = reply->url();
    const QVariant redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectTarget.isValid()) {
        streamUrl = redirectTarget.toUrl();
        if (streamUrl.isRelative()) {
            streamUrl = reply->url().resolved(streamUrl);
        }
    }

    const QByteArray body = reply->readAll().trimmed();
    if (!body.isEmpty() && (body.startsWith("http://") || body.startsWith("https://"))) {
        streamUrl = QUrl(QString::fromUtf8(body));
    }

    if (!streamUrl.isValid() || streamUrl.isEmpty()) {
        emit failed(QStringLiteral("无法解析播放链接"));
        return;
    }

    emit resolved(streamUrl);
}
