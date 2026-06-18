#pragma once

#include <QObject>
#include <QList>
#include <QVariantList>

#include "../models/track_model.h"

class LibraryScanner : public QObject
{
    Q_OBJECT

public:
    explicit LibraryScanner(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList scanDirectory(const QString &path) const;
    QList<Track> scanDirectoryTracks(const QString &path) const;
    Track trackFromPath(const QString &filePath) const;

private:
    bool isSupportedAudioFile(const QString &filePath) const;
    Track buildTrack(const QString &filePath) const;
};
