#pragma once

#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

struct PlaylistTrackModel
{
    QString path;
    QString title;
    QString artist;
    QString streamUrl;
    QString picUrl;
    QString server;
    QString onlineId;
    bool isOnline = false;

    QVariantMap toVariantMap() const
    {
        return QVariantMap{
            {QStringLiteral("path"), path},
            {QStringLiteral("title"), title},
            {QStringLiteral("artist"), artist},
            {QStringLiteral("streamUrl"), streamUrl},
            {QStringLiteral("picUrl"), picUrl},
            {QStringLiteral("server"), server},
            {QStringLiteral("onlineId"), onlineId},
            {QStringLiteral("isOnline"), isOnline},
        };
    }

    static PlaylistTrackModel fromVariantMap(const QVariantMap &map)
    {
        PlaylistTrackModel track;
        track.path = map.value(QStringLiteral("path")).toString();
        track.title = map.value(QStringLiteral("title")).toString();
        track.artist = map.value(QStringLiteral("artist")).toString();
        track.streamUrl = map.value(QStringLiteral("streamUrl")).toString();
        track.picUrl = map.value(QStringLiteral("picUrl")).toString();
        track.server = map.value(QStringLiteral("server")).toString();
        track.onlineId = map.value(QStringLiteral("onlineId")).toString();
        track.isOnline = map.value(QStringLiteral("isOnline")).toBool()
                         || track.path.startsWith(QStringLiteral("online:"));
        return track;
    }
};

struct PlaylistModel
{
    QString id;
    QString name;
    bool isSystem = false;
    QList<PlaylistTrackModel> tracks;

    QVariantMap toVariantMap() const
    {
        QVariantList trackList;
        trackList.reserve(tracks.size());
        for (const PlaylistTrackModel &track : tracks) {
            trackList.append(track.toVariantMap());
        }

        return QVariantMap{
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("isSystem"), isSystem},
            {QStringLiteral("tracks"), trackList},
        };
    }

    static PlaylistModel fromVariantMap(const QVariantMap &map)
    {
        PlaylistModel playlist;
        playlist.id = map.value(QStringLiteral("id")).toString();
        playlist.name = map.value(QStringLiteral("name")).toString();
        playlist.isSystem = map.value(QStringLiteral("isSystem")).toBool();
        const QVariantList trackList = map.value(QStringLiteral("tracks")).toList();
        playlist.tracks.reserve(trackList.size());
        for (const QVariant &trackValue : trackList) {
            playlist.tracks.append(PlaylistTrackModel::fromVariantMap(trackValue.toMap()));
        }
        return playlist;
    }
};

