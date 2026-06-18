#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "../models/playlist_model.h"

class PlaylistService : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistService(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList playlists() const;
    Q_INVOKABLE QVariantList tracks(const QString &playlistId) const;

    Q_INVOKABLE bool createPlaylist(const QString &name);
    Q_INVOKABLE bool renamePlaylist(const QString &playlistId, const QString &newName);
    Q_INVOKABLE bool deletePlaylist(const QString &playlistId);

    Q_INVOKABLE bool addTrack(const QString &playlistId, const QString &path, const QString &title);
    Q_INVOKABLE bool addTrackMap(const QString &playlistId, const QVariantMap &track);
    Q_INVOKABLE bool removeTrack(const QString &playlistId, const QString &path);

    Q_INVOKABLE bool saveToStorage(const QString &storagePath = QString());
    Q_INVOKABLE bool loadFromStorage(const QString &storagePath = QString());

    Q_INVOKABLE QString favoritesPlaylistId() const;
    Q_INVOKABLE bool isFavorite(const QString &path) const;
    Q_INVOKABLE bool setFavorite(const QString &path, const QString &title, bool favorited);
    Q_INVOKABLE bool toggleFavorite(const QString &path, const QString &title);
    Q_INVOKABLE bool toggleFavoriteTrack(const QVariantMap &track);

signals:
    // 歌单或其曲目发生增删改时发出，供 QML 跨页刷新。
    void changed();

private:
    int indexOfPlaylist(const QString &playlistId) const;
    bool hasPlaylistName(const QString &name, const QString &excludePlaylistId = QString()) const;
    void ensureSystemPlaylists();
    int favoritesPlaylistIndex() const;

    QList<PlaylistModel> m_playlists;
    QString m_lastStoragePath;
};
