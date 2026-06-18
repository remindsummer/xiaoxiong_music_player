#pragma once

#include <QString>
#include <QVariantMap>
#include <QtGlobal>

struct Track
{
    QString id;
    QString path;
    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;

    QVariantMap toVariantMap() const
    {
        return QVariantMap{
            {QStringLiteral("id"), id},
            {QStringLiteral("path"), path},
            {QStringLiteral("title"), title},
            {QStringLiteral("artist"), artist},
            {QStringLiteral("album"), album},
            {QStringLiteral("durationMs"), durationMs},
        };
    }

    static Track fromVariantMap(const QVariantMap &map)
    {
        Track track;
        track.id = map.value(QStringLiteral("id")).toString();
        track.path = map.value(QStringLiteral("path")).toString();
        track.title = map.value(QStringLiteral("title")).toString();
        track.artist = map.value(QStringLiteral("artist")).toString();
        track.album = map.value(QStringLiteral("album")).toString();
        track.durationMs = map.value(QStringLiteral("durationMs")).toLongLong();
        return track;
    }
};

using TrackModel = Track;
