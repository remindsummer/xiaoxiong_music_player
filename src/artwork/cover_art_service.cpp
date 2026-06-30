#include "cover_art_service.h"

#include "../network/meting_api.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

CoverArtService::CoverArtService(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void CoverArtService::requestCover(const QString &trackKey,
                                   const QString &picUrl,
                                   const QString &server,
                                   const QString &onlineId)
{
    const QString normalizedKey = normalizeTrackKey(trackKey);
    if (normalizedKey.isEmpty()) {
        emit coverReady(normalizedKey, QString());
        return;
    }

    const QString cached = cachedCoverPath(normalizedKey);
    if (!cached.isEmpty()) {
        emit coverReady(normalizedKey, cached);
        return;
    }

    if (!picUrl.trimmed().isEmpty()) {
        startDownload(normalizedKey, QUrl(picUrl.trimmed()));
        return;
    }

    if (!onlineId.trimmed().isEmpty()) {
        const QString safeServer = server.trimmed().isEmpty() ? QStringLiteral("netease") : server.trimmed();
        startDownload(normalizedKey, MetingApi::buildUrl(safeServer, QStringLiteral("pic"), onlineId.trimmed()));
        return;
    }

    emit coverReady(normalizedKey, QString());
}

void CoverArtService::cacheCoverFromUrl(const QString &trackKey, const QString &picUrl)
{
    requestCover(trackKey, picUrl);
}

QString CoverArtService::saveCoverBytes(const QString &trackKey, const QByteArray &imageData)
{
    const QString normalizedKey = normalizeTrackKey(trackKey);
    if (normalizedKey.isEmpty() || imageData.isEmpty()) {
        return QString();
    }

    if (!ensureCacheDir()) {
        return QString();
    }

    const QString cachePath = cacheFilePathForKey(normalizedKey);
    QFile file(cachePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    file.write(imageData);
    file.close();

    const QString coverPath = QUrl::fromLocalFile(cachePath).toString();
    emit coverReady(normalizedKey, coverPath);
    return coverPath;
}

QString CoverArtService::cachedCoverPath(const QString &trackKey) const
{
    const QString path = cacheFilePathForKey(trackKey);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return QString();
    }
    return QUrl::fromLocalFile(path).toString();
}

QString CoverArtService::normalizeTrackKey(const QString &trackKey) const
{
    const QString trimmed = trackKey.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    if (trimmed.startsWith(QStringLiteral("online:"))) {
        return trimmed;
    }
    return QFileInfo(trimmed).absoluteFilePath();
}

QString CoverArtService::cacheFilePathForKey(const QString &trackKey) const
{
    const QString normalized = normalizeTrackKey(trackKey);
    if (normalized.isEmpty()) {
        return QString();
    }
    const QByteArray hash = QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/cover_cache");
    return dir + QLatin1Char('/') + QString::fromLatin1(hash) + QStringLiteral(".jpg");
}

bool CoverArtService::ensureCacheDir() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/cover_cache");
    return QDir().mkpath(dir);
}

void CoverArtService::startDownload(const QString &trackKey, const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        emit coverReady(trackKey, QString());
        return;
    }

    if (QNetworkReply *staleReply = m_reply) {
        m_reply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    m_pendingTrackKey = trackKey;
    QNetworkRequest request(url);
    MetingApi::applyCommonRequestHeaders(request);
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_reply;
        if (!reply) {
            return;
        }
        m_reply = nullptr;

        const QString trackKey = m_pendingTrackKey;
        m_pendingTrackKey.clear();

        if (reply->error() != QNetworkReply::NoError) {
            emit coverReady(trackKey, QString());
            reply->deleteLater();
            return;
        }

        const QByteArray data = reply->readAll();
        reply->deleteLater();

        if (data.isEmpty()) {
            emit coverReady(trackKey, QString());
            return;
        }

        if (!ensureCacheDir()) {
            emit coverReady(trackKey, QString());
            return;
        }

        const QString cachePath = cacheFilePathForKey(trackKey);
        QFile file(cachePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit coverReady(trackKey, QString());
            return;
        }
        file.write(data);
        file.close();

        emit coverReady(trackKey, QUrl::fromLocalFile(cachePath).toString());
    });
}
