#include "library_cover_prefetcher.h"

#include "cover_art_service.h"
#include "../network/meting_api.h"

#include <QBuffer>
#include <QImage>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {
constexpr int kPerItemTimeoutMs = 8000;
} // namespace

LibraryCoverPrefetcher::LibraryCoverPrefetcher(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_timeout(new QTimer(this))
    , m_network(new QNetworkAccessManager(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(kPerItemTimeoutMs);

    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (m_phase == Phase::LoadingEmbedded && m_awaitingLoad) {
            finishEmbeddedLookup();
        }
    });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (m_phase != Phase::LoadingEmbedded || !m_awaitingLoad) {
            return;
        }
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            finishEmbeddedLookup();
        } else if (status == QMediaPlayer::InvalidMedia) {
            m_awaitingLoad = false;
            m_timeout->stop();
            startMetingLookup();
        }
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &) {
        if (m_phase != Phase::LoadingEmbedded || !m_awaitingLoad) {
            return;
        }
        m_awaitingLoad = false;
        m_timeout->stop();
        startMetingLookup();
    });
}

void LibraryCoverPrefetcher::setCoverArtService(CoverArtService *coverArt)
{
    m_coverArt = coverArt;
}

void LibraryCoverPrefetcher::enqueue(const QString &path, const QString &title, const QString &artist)
{
    const QString normalizedPath = path.trimmed();
    if (normalizedPath.isEmpty() || normalizedPath.startsWith(QStringLiteral("online:"))) {
        return;
    }

    if (m_coverArt && !m_coverArt->cachedCoverPath(normalizedPath).isEmpty()) {
        return;
    }
    if (isQueuedOrCurrent(normalizedPath)) {
        return;
    }

    m_queue.enqueue(PendingItem{normalizedPath, title.trimmed(), artist.trimmed()});
    if (!m_busy) {
        processNext();
    }
}

void LibraryCoverPrefetcher::enqueueTracks(const QList<Track> &tracks)
{
    for (const Track &track : tracks) {
        enqueue(track.path, track.title, track.artist);
    }
}

bool LibraryCoverPrefetcher::isQueuedOrCurrent(const QString &path) const
{
    if (m_busy && m_current.path == path) {
        return true;
    }
    for (const PendingItem &item : m_queue) {
        if (item.path == path) {
            return true;
        }
    }
    return false;
}

void LibraryCoverPrefetcher::processNext()
{
    if (m_metingReply) {
        m_metingReply->abort();
        m_metingReply->deleteLater();
        m_metingReply = nullptr;
    }

    if (m_queue.isEmpty()) {
        m_busy = false;
        m_phase = Phase::Idle;
        m_current = PendingItem{};
        m_player->setSource(QUrl());
        return;
    }

    m_busy = true;
    m_current = m_queue.dequeue();

    if (m_coverArt && !m_coverArt->cachedCoverPath(m_current.path).isEmpty()) {
        finishCurrent();
        return;
    }

    startEmbeddedLookup();
}

void LibraryCoverPrefetcher::startEmbeddedLookup()
{
    m_phase = Phase::LoadingEmbedded;
    m_awaitingLoad = true;
    m_timeout->start();
    m_player->setSource(QUrl::fromLocalFile(m_current.path));
}

void LibraryCoverPrefetcher::finishEmbeddedLookup()
{
    m_awaitingLoad = false;
    m_timeout->stop();

    if (m_coverArt) {
        const QMediaMetaData md = m_player->metaData();
        QByteArray imageData;
        const QVariant thumbVariant = md.value(QMediaMetaData::ThumbnailImage);
        const QVariant coverVariant = md.value(QMediaMetaData::CoverArtImage);

        auto variantToJpeg = [](const QVariant &variant) -> QByteArray {
            if (variant.canConvert<QImage>()) {
                const QImage image = variant.value<QImage>();
                if (image.isNull()) {
                    return QByteArray();
                }
                QByteArray bytes;
                QBuffer buffer(&bytes);
                buffer.open(QIODevice::WriteOnly);
                if (!image.save(&buffer, "JPG")) {
                    return QByteArray();
                }
                return bytes;
            }
            if (variant.canConvert<QByteArray>()) {
                return variant.toByteArray();
            }
            return QByteArray();
        };

        imageData = variantToJpeg(thumbVariant);
        if (imageData.isEmpty()) {
            imageData = variantToJpeg(coverVariant);
        }
        if (!imageData.isEmpty()) {
            m_coverArt->saveCoverBytes(m_current.path, imageData);
            finishCurrent();
            return;
        }
    }

    startMetingLookup();
}

void LibraryCoverPrefetcher::startMetingLookup()
{
    m_phase = Phase::WaitingMeting;

    if (!m_coverArt) {
        finishCurrent();
        return;
    }

    const QString keyword = MetingApi::buildSearchKeyword(m_current.title, m_current.artist);
    if (keyword.trimmed().isEmpty()) {
        finishCurrent();
        return;
    }

    QNetworkRequest request(MetingApi::buildUrl(QStringLiteral("netease"),
                                                QStringLiteral("search"),
                                                keyword));
    MetingApi::applyCommonRequestHeaders(request);

    m_metingReply = m_network->get(request);
    connect(m_metingReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_metingReply;
        if (!reply) {
            return;
        }
        m_metingReply = nullptr;

        if (reply->error() == QNetworkReply::NoError && m_coverArt) {
            const QString picUrl = MetingApi::picUrlFromSearchResponse(reply->readAll());
            if (!picUrl.isEmpty()) {
                m_coverArt->requestCover(m_current.path, picUrl);
            }
        }
        reply->deleteLater();
        finishCurrent();
    });
}

void LibraryCoverPrefetcher::finishCurrent()
{
    m_phase = Phase::Idle;
    m_player->setSource(QUrl());
    processNext();
}
