#include "library_cover_prefetcher.h"

#include "../library/audio_tag_reader.h"
#include "cover_art_service.h"
#include "../network/meting_api.h"

#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace {

bool pathsReferToSameTrack(const QString &left, const QString &right)
{
    if (left == right) {
        return true;
    }
    if (left.startsWith(QStringLiteral("online:")) || right.startsWith(QStringLiteral("online:"))) {
        return left == right;
    }
    return QFileInfo(left).absoluteFilePath() == QFileInfo(right).absoluteFilePath();
}

} // namespace

LibraryCoverPrefetcher::LibraryCoverPrefetcher(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void LibraryCoverPrefetcher::setCoverArtService(CoverArtService *coverArt)
{
    m_coverArt = coverArt;
}

void LibraryCoverPrefetcher::fetchNow(const QString &path, const QString &title, const QString &artist)
{
    const QString normalizedPath = path.trimmed();
    if (normalizedPath.isEmpty()) {
        emit fetchFinished(normalizedPath, false, QStringLiteral("无效的歌曲路径"));
        return;
    }

    if (m_coverArt && !m_coverArt->cachedCoverPath(normalizedPath).isEmpty()) {
        emit fetchFinished(normalizedPath, true, QString());
        return;
    }

    cancelInFlight();
    m_manualFetch = true;
    m_busy = true;
    m_current = PendingItem{normalizedPath, title.trimmed(), artist.trimmed()};
    startEmbeddedLookup();
}

void LibraryCoverPrefetcher::cancelInFlight()
{
    if (QNetworkReply *staleReply = m_metingReply) {
        m_metingReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }
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
    if (QNetworkReply *staleReply = m_metingReply) {
        m_metingReply = nullptr;
        staleReply->abort();
        staleReply->deleteLater();
    }

    if (m_queue.isEmpty()) {
        m_busy = false;
        m_phase = Phase::Idle;
        m_current = PendingItem{};
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
    QTimer::singleShot(0, this, [this]() {
        if (m_coverArt) {
            const QByteArray imageData = AudioTagReader::readEmbeddedCover(m_current.path);
            if (!imageData.isEmpty()) {
                m_coverArt->saveCoverBytes(m_current.path, imageData);
                finishCurrent();
                return;
            }
        }
        startMetingLookup();
    });
}

void LibraryCoverPrefetcher::requestCoverThenContinue(const QString &picUrl,
                                                      const QString &server,
                                                      const QString &onlineId)
{
    if (!m_coverArt) {
        finishCurrent();
        return;
    }

    const QString trackPath = m_current.path;
    if (!m_manualFetch) {
        m_coverArt->requestCover(trackPath, picUrl, server, onlineId);
        finishCurrent();
        return;
    }

    auto *connection = new QMetaObject::Connection();
    *connection = connect(
        m_coverArt,
        &CoverArtService::coverReady,
        this,
        [this, connection, trackPath](const QString &key, const QString &coverPath) {
            if (!pathsReferToSameTrack(key, trackPath)) {
                return;
            }
            disconnect(*connection);
            delete connection;
            finishManualFetch(!coverPath.isEmpty(),
                              coverPath.isEmpty() ? QStringLiteral("下载封面失败") : QString());
        });
    m_coverArt->requestCover(trackPath, picUrl, server, onlineId);
}

void LibraryCoverPrefetcher::startMetingLookup()
{
    m_phase = Phase::WaitingMeting;

    if (!m_coverArt) {
        finishCurrent();
        return;
    }

    const QString embeddedId = MetingApi::extractEmbeddedNeteaseId(m_current.path);
    if (!embeddedId.isEmpty()) {
        requestCoverThenContinue(QString(), QStringLiteral("netease"), embeddedId);
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
                requestCoverThenContinue(picUrl, QString(), QString());
                reply->deleteLater();
                return;
            }
        }
        reply->deleteLater();
        finishCurrent();
    });
}

void LibraryCoverPrefetcher::finishManualFetch(bool success, const QString &message)
{
    if (m_coverArt && !m_coverArt->cachedCoverPath(m_current.path).isEmpty()) {
        success = true;
    }

    const QString path = m_current.path;
    emit fetchFinished(path, success, success ? QString() : message);

    m_manualFetch = false;
    m_busy = false;
    m_phase = Phase::Idle;
    m_current = PendingItem{};

    if (!m_queue.isEmpty()) {
        processNext();
    }
}

void LibraryCoverPrefetcher::finishCurrent()
{
    m_phase = Phase::Idle;

    if (m_manualFetch) {
        const bool success = m_coverArt && !m_coverArt->cachedCoverPath(m_current.path).isEmpty();
        finishManualFetch(success, success ? QString() : QStringLiteral("未找到匹配的封面"));
        return;
    }

    processNext();
}
