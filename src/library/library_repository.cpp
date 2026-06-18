#include "library_repository.h"

#include "../artwork/library_cover_prefetcher.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
QString defaultLibraryStoragePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/library.json");
}
} // namespace

LibraryRepository::LibraryRepository(QObject *parent)
    : QObject(parent)
    , m_scanner(this)
    , m_resolver(new LibraryMetadataResolver(this))
{
    connect(m_resolver, &LibraryMetadataResolver::trackResolved,
            this, &LibraryRepository::onTrackResolved);
    // 一批元数据回填完成后统一落盘一次，避免频繁写文件。
    connect(m_resolver, &LibraryMetadataResolver::finished,
            this, [this]() { saveToStorage(); });

    // 启动时自动从本地恢复曲库，并对缺失元数据的曲目排队补齐。
    loadFromStorage();
    enqueueUnresolved();
}

void LibraryRepository::setCoverPrefetcher(LibraryCoverPrefetcher *prefetcher)
{
    m_coverPrefetcher = prefetcher;
}

void LibraryRepository::enqueueCoverPrefetch(const Track &track)
{
    if (!m_coverPrefetcher) {
        return;
    }
    m_coverPrefetcher->enqueue(track.path, track.title, track.artist);
}

QVariantList LibraryRepository::scanAndStore(const QString &path)
{
    m_tracks = m_scanner.scanDirectoryTracks(path);
    // 扫描结果立即持久化（此时仅有文件名信息），重启后可恢复。
    saveToStorage();
    // 异步补齐时长/歌手/专辑等元数据，完成后通过 tracksUpdated 通知刷新。
    enqueueUnresolved();
    if (m_coverPrefetcher) {
        m_coverPrefetcher->enqueueTracks(m_tracks);
    }
    return toVariantList(m_tracks);
}

void LibraryRepository::onTrackResolved(const QString &id,
                                        const QString &title,
                                        const QString &artist,
                                        const QString &album,
                                        qint64 durationMs)
{
    for (Track &track : m_tracks) {
        if (track.id != id) {
            continue;
        }

        bool changed = false;
        if (durationMs > 0 && track.durationMs != durationMs) {
            track.durationMs = durationMs;
            changed = true;
        }
        if (!title.trimmed().isEmpty() && track.title != title) {
            track.title = title;
            changed = true;
        }
        if (!artist.trimmed().isEmpty() && track.artist != artist) {
            track.artist = artist;
            changed = true;
        }
        if (!album.trimmed().isEmpty() && track.album != album) {
            track.album = album;
            changed = true;
        }

        if (changed) {
            emit tracksUpdated();
        }
        enqueueCoverPrefetch(track);
        return;
    }
}

void LibraryRepository::enqueueUnresolved()
{
    if (!m_resolver) {
        return;
    }
    for (const Track &track : m_tracks) {
        const bool needsDuration = track.durationMs <= 0;
        const bool needsArtist = track.artist.isEmpty()
            || track.artist == QStringLiteral("Unknown Artist");
        if (needsDuration || needsArtist) {
            m_resolver->enqueue(track.id, track.path);
        }
    }
}

bool LibraryRepository::addTrackFromPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QFileInfo info(trimmed);
    if (!info.exists() || !info.isFile()) {
        return false;
    }

    const QString absolutePath = info.absoluteFilePath();
    for (const Track &existing : m_tracks) {
        if (existing.path == absolutePath) {
            return true;
        }
    }

    Track track = m_scanner.trackFromPath(absolutePath);
    if (track.path.isEmpty()) {
        return false;
    }

    m_tracks.append(track);
    saveToStorage();
    emit tracksUpdated();

    if (m_resolver) {
        const bool needsDuration = track.durationMs <= 0;
        const bool needsArtist = track.artist.isEmpty()
            || track.artist == QStringLiteral("Unknown Artist");
        if (needsDuration || needsArtist) {
            m_resolver->enqueue(track.id, track.path);
        }
    }

    enqueueCoverPrefetch(track);
    return true;
}

QVariantList LibraryRepository::allTracks() const
{
    return toVariantList(m_tracks);
}

QVariantList LibraryRepository::filterTracks(const QString &keyword) const
{
    return toVariantList(filterTracksInternal(keyword));
}

bool LibraryRepository::loadFromStorage(const QString &storagePath)
{
    const QString targetPath = storagePath.isEmpty()
        ? (m_lastStoragePath.isEmpty() ? defaultLibraryStoragePath() : m_lastStoragePath)
        : storagePath;
    m_lastStoragePath = targetPath;

    QFile file(targetPath);
    if (!file.exists()) {
        // 首次运行没有存档文件，视为空曲库，正常返回。
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return false;
    }

    QList<Track> loaded;
    const QJsonArray array = doc.array();
    loaded.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        loaded.append(Track::fromVariantMap(value.toObject().toVariantMap()));
    }

    m_tracks = loaded;
    return true;
}

bool LibraryRepository::saveToStorage(const QString &storagePath) const
{
    const QString targetPath = storagePath.isEmpty()
        ? (m_lastStoragePath.isEmpty() ? defaultLibraryStoragePath() : m_lastStoragePath)
        : storagePath;

    const QFileInfo info(targetPath);
    QDir().mkpath(info.absolutePath());

    QJsonArray array;
    for (const Track &track : m_tracks) {
        array.append(QJsonObject::fromVariantMap(track.toVariantMap()));
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

int LibraryRepository::trackCount() const
{
    return m_tracks.size();
}

QList<Track> LibraryRepository::tracks() const
{
    return m_tracks;
}

void LibraryRepository::setTracks(const QList<Track> &tracks)
{
    m_tracks = tracks;
}

QVariantList LibraryRepository::toVariantList(const QList<Track> &tracks) const
{
    QVariantList result;
    result.reserve(tracks.size());
    for (const Track &track : tracks) {
        result.append(track.toVariantMap());
    }
    return result;
}

QList<Track> LibraryRepository::filterTracksInternal(const QString &keyword) const
{
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        return m_tracks;
    }

    QList<Track> result;
    result.reserve(m_tracks.size());
    for (const Track &track : m_tracks) {
        if (track.title.contains(trimmed, Qt::CaseInsensitive)
            || track.artist.contains(trimmed, Qt::CaseInsensitive)
            || track.album.contains(trimmed, Qt::CaseInsensitive)) {
            result.append(track);
        }
    }
    return result;
}
