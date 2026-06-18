#include "playlist_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

namespace {
QString defaultPlaylistStoragePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/playlists.json");
}

const char *kFavoritesPlaylistId = "__favorites__";
const char *kFavoritesPlaylistName = "我喜欢";

QVariantMap trackMapFromPathTitle(const QString &path, const QString &title)
{
    return QVariantMap{
        {QStringLiteral("path"), path},
        {QStringLiteral("title"), title},
    };
}
} // namespace

PlaylistService::PlaylistService(QObject *parent)
    : QObject(parent)
{
    loadFromStorage();
    ensureSystemPlaylists();
}

QVariantList PlaylistService::playlists() const
{
    QVariantList result;
    result.reserve(m_playlists.size());
    for (const PlaylistModel &playlist : m_playlists) {
        result.append(playlist.toVariantMap());
    }
    return result;
}

QVariantList PlaylistService::tracks(const QString &playlistId) const
{
    const int playlistIndex = indexOfPlaylist(playlistId);
    if (playlistIndex < 0) {
        return {};
    }

    QVariantList result;
    const QList<PlaylistTrackModel> &trackModels = m_playlists.at(playlistIndex).tracks;
    result.reserve(trackModels.size());
    for (const PlaylistTrackModel &track : trackModels) {
        result.append(track.toVariantMap());
    }
    return result;
}

bool PlaylistService::createPlaylist(const QString &name)
{
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty() || hasPlaylistName(normalizedName)) {
        return false;
    }

    PlaylistModel playlist;
    playlist.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    playlist.name = normalizedName;
    m_playlists.append(playlist);
    emit changed();
    return true;
}

bool PlaylistService::renamePlaylist(const QString &playlistId, const QString &newName)
{
    const int playlistIndex = indexOfPlaylist(playlistId);
    const QString normalizedName = newName.trimmed();
    if (playlistIndex < 0 || normalizedName.isEmpty() || hasPlaylistName(normalizedName, playlistId)) {
        return false;
    }
    if (m_playlists.at(playlistIndex).isSystem) {
        return false;
    }

    m_playlists[playlistIndex].name = normalizedName;
    emit changed();
    return true;
}

bool PlaylistService::deletePlaylist(const QString &playlistId)
{
    const int playlistIndex = indexOfPlaylist(playlistId);
    if (playlistIndex < 0) {
        return false;
    }
    if (m_playlists.at(playlistIndex).isSystem) {
        return false;
    }

    m_playlists.removeAt(playlistIndex);
    emit changed();
    return true;
}

bool PlaylistService::addTrack(const QString &playlistId, const QString &path, const QString &title)
{
    QVariantMap trackMap{
        {QStringLiteral("path"), path},
        {QStringLiteral("title"), title},
    };
    return addTrackMap(playlistId, trackMap);
}

bool PlaylistService::addTrackMap(const QString &playlistId, const QVariantMap &trackMap)
{
    const int playlistIndex = indexOfPlaylist(playlistId);
    const PlaylistTrackModel incoming = PlaylistTrackModel::fromVariantMap(trackMap);
    const QString normalizedPath = incoming.path.trimmed();
    if (playlistIndex < 0 || normalizedPath.isEmpty()) {
        return false;
    }

    PlaylistModel &playlist = m_playlists[playlistIndex];
    for (PlaylistTrackModel &existing : playlist.tracks) {
        if (existing.path.compare(normalizedPath, Qt::CaseInsensitive) == 0) {
            if (incoming.isOnline && existing.streamUrl.isEmpty() && !incoming.streamUrl.isEmpty()) {
                existing = incoming;
                if (existing.title.isEmpty()) {
                    existing.title = QFileInfo(normalizedPath).baseName();
                }
                emit changed();
                return true;
            }
            return false;
        }
    }

    PlaylistTrackModel track = incoming;
    if (track.title.isEmpty()) {
        track.title = QFileInfo(normalizedPath).baseName();
    }

    playlist.tracks.append(track);
    emit changed();
    return true;
}

bool PlaylistService::removeTrack(const QString &playlistId, const QString &path)
{
    const int playlistIndex = indexOfPlaylist(playlistId);
    const QString normalizedPath = path.trimmed();
    if (playlistIndex < 0 || normalizedPath.isEmpty()) {
        return false;
    }

    PlaylistModel &playlist = m_playlists[playlistIndex];
    for (int i = 0; i < playlist.tracks.size(); ++i) {
        if (playlist.tracks.at(i).path.compare(normalizedPath, Qt::CaseInsensitive) == 0) {
            playlist.tracks.removeAt(i);
            emit changed();
            return true;
        }
    }

    return false;
}

bool PlaylistService::saveToStorage(const QString &storagePath)
{
    const QString targetPath = storagePath.isEmpty()
        ? (m_lastStoragePath.isEmpty() ? defaultPlaylistStoragePath() : m_lastStoragePath)
        : storagePath;
    m_lastStoragePath = targetPath;

    const QFileInfo info(targetPath);
    QDir().mkpath(info.absolutePath());

    QJsonArray array;
    for (const PlaylistModel &playlist : m_playlists) {
        array.append(QJsonObject::fromVariantMap(playlist.toVariantMap()));
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool PlaylistService::loadFromStorage(const QString &storagePath)
{
    const QString targetPath = storagePath.isEmpty()
        ? (m_lastStoragePath.isEmpty() ? defaultPlaylistStoragePath() : m_lastStoragePath)
        : storagePath;
    m_lastStoragePath = targetPath;

    QFile file(targetPath);
    if (!file.exists()) {
        // 首次运行没有存档文件，视为没有歌单，正常返回。
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

    QList<PlaylistModel> loaded;
    const QJsonArray array = doc.array();
    loaded.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        loaded.append(PlaylistModel::fromVariantMap(value.toObject().toVariantMap()));
    }

    m_playlists = loaded;
    ensureSystemPlaylists();
    return true;
}

void PlaylistService::ensureSystemPlaylists()
{
    for (PlaylistModel &playlist : m_playlists) {
        if (playlist.id == QString::fromLatin1(kFavoritesPlaylistId)) {
            playlist.isSystem = true;
            if (playlist.name.trimmed().isEmpty()) {
                playlist.name = QString::fromUtf8(kFavoritesPlaylistName);
            }
            return;
        }
    }

    PlaylistModel favorites;
    favorites.id = QString::fromLatin1(kFavoritesPlaylistId);
    favorites.name = QString::fromUtf8(kFavoritesPlaylistName);
    favorites.isSystem = true;
    m_playlists.prepend(favorites);
    emit changed();
}

int PlaylistService::favoritesPlaylistIndex() const
{
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists.at(i).id == QString::fromLatin1(kFavoritesPlaylistId)) {
            return i;
        }
    }
    return -1;
}

QString PlaylistService::favoritesPlaylistId() const
{
    const int index = favoritesPlaylistIndex();
    if (index < 0) {
        return QString();
    }
    return m_playlists.at(index).id;
}

bool PlaylistService::isFavorite(const QString &path) const
{
    const int index = favoritesPlaylistIndex();
    const QString normalizedPath = path.trimmed();
    if (index < 0 || normalizedPath.isEmpty()) {
        return false;
    }

    const QList<PlaylistTrackModel> &tracks = m_playlists.at(index).tracks;
    for (const PlaylistTrackModel &track : tracks) {
        if (track.path.compare(normalizedPath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool PlaylistService::setFavorite(const QString &path, const QString &title, bool favorited)
{
    const QString favoritesId = favoritesPlaylistId();
    if (favoritesId.isEmpty()) {
        return false;
    }

    if (favorited) {
        return addTrackMap(favoritesId, trackMapFromPathTitle(path, title));
    }
    return removeTrack(favoritesId, path);
}

bool PlaylistService::toggleFavoriteTrack(const QVariantMap &track)
{
    const QString path = track.value(QStringLiteral("path")).toString().trimmed();
    if (path.isEmpty()) {
        return false;
    }
    if (isFavorite(path)) {
        return removeTrack(favoritesPlaylistId(), path);
    }
    return addTrackMap(favoritesPlaylistId(), track);
}

bool PlaylistService::toggleFavorite(const QString &path, const QString &title)
{
    const QVariantMap trackMap{
        {QStringLiteral("path"), path},
        {QStringLiteral("title"), title},
    };
    return toggleFavoriteTrack(trackMap);
}

int PlaylistService::indexOfPlaylist(const QString &playlistId) const
{
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists.at(i).id == playlistId) {
            return i;
        }
    }
    return -1;
}

bool PlaylistService::hasPlaylistName(const QString &name, const QString &excludePlaylistId) const
{
    for (const PlaylistModel &playlist : m_playlists) {
        if (!excludePlaylistId.isEmpty() && playlist.id == excludePlaylistId) {
            continue;
        }
        if (playlist.name.compare(name, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}
