#include "library_metadata_resolver.h"

#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>

namespace {
// 单个文件的最长等待时间，避免损坏/异常文件卡住整个队列。
constexpr int kPerItemTimeoutMs = 8000;
} // namespace

LibraryMetadataResolver::LibraryMetadataResolver(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(kPerItemTimeoutMs);

    connect(m_timeout, &QTimer::timeout, this, [this]() {
        if (!m_awaitingLoad) {
            return;
        }
        // 超时：用当前能拿到的信息收尾（通常为空），跳到下一个。
        resolveCurrentFromPlayer();
    });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (!m_awaitingLoad) {
            return;
        }
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            resolveCurrentFromPlayer();
        } else if (status == QMediaPlayer::InvalidMedia) {
            // 无法解析的文件，跳过。
            m_awaitingLoad = false;
            m_timeout->stop();
            finishCurrent(QString(), QString(), QString(), 0);
        }
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &) {
        if (!m_awaitingLoad) {
            return;
        }
        m_awaitingLoad = false;
        m_timeout->stop();
        finishCurrent(QString(), QString(), QString(), 0);
    });
}

void LibraryMetadataResolver::enqueue(const QString &id, const QString &path)
{
    if (id.isEmpty() || path.isEmpty()) {
        return;
    }
    m_queue.enqueue(PendingItem{id, path});
    if (!m_busy) {
        processNext();
    }
}

bool LibraryMetadataResolver::isBusy() const
{
    return m_busy;
}

void LibraryMetadataResolver::processNext()
{
    if (m_queue.isEmpty()) {
        m_busy = false;
        m_player->setSource(QUrl());
        emit finished();
        return;
    }

    m_busy = true;
    m_current = m_queue.dequeue();
    m_awaitingLoad = true;
    m_timeout->start();
    m_player->setSource(QUrl::fromLocalFile(m_current.path));
}

void LibraryMetadataResolver::resolveCurrentFromPlayer()
{
    m_awaitingLoad = false;
    m_timeout->stop();

    const QMediaMetaData md = m_player->metaData();

    const QString title = md.stringValue(QMediaMetaData::Title);
    QString artist = md.stringValue(QMediaMetaData::AlbumArtist);
    if (artist.isEmpty()) {
        artist = md.stringValue(QMediaMetaData::ContributingArtist);
    }
    if (artist.isEmpty()) {
        artist = md.stringValue(QMediaMetaData::Author);
    }
    const QString album = md.stringValue(QMediaMetaData::AlbumTitle);
    const qint64 durationMs = m_player->duration();

    finishCurrent(title, artist, album, durationMs);
}

void LibraryMetadataResolver::finishCurrent(const QString &title,
                                            const QString &artist,
                                            const QString &album,
                                            qint64 durationMs)
{
    emit trackResolved(m_current.id, title, artist, album, durationMs);
    processNext();
}
