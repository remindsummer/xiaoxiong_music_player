#pragma once

#include <QObject>
#include <QList>
#include <QVariantList>

#include "library_scanner.h"
#include "library_metadata_resolver.h"
#include "../models/track_model.h"

class LibraryCoverPrefetcher;

class LibraryRepository : public QObject
{
    Q_OBJECT

public:
    explicit LibraryRepository(QObject *parent = nullptr);

    void setCoverPrefetcher(LibraryCoverPrefetcher *prefetcher);

    Q_INVOKABLE QVariantList scanAndStore(const QString &path);
    Q_INVOKABLE bool addTrackFromPath(const QString &path);
    Q_INVOKABLE QVariantList allTracks() const;
    Q_INVOKABLE QVariantList filterTracks(const QString &keyword) const;
    Q_INVOKABLE bool loadFromStorage(const QString &storagePath = QString());
    Q_INVOKABLE bool saveToStorage(const QString &storagePath = QString()) const;
    Q_INVOKABLE int trackCount() const;

    QList<Track> tracks() const;
    void setTracks(const QList<Track> &tracks);

signals:
    // 元数据异步回填后通知 QML 重新读取曲库。
    void tracksUpdated();

private:
    QVariantList toVariantList(const QList<Track> &tracks) const;
    QList<Track> filterTracksInternal(const QString &keyword) const;
    void onTrackResolved(const QString &id,
                         const QString &title,
                         const QString &artist,
                         const QString &album,
                         qint64 durationMs);
    void enqueueUnresolved();
    void enqueueCoverPrefetch(const Track &track);

    LibraryScanner m_scanner;
    LibraryMetadataResolver *m_resolver = nullptr;
    LibraryCoverPrefetcher *m_coverPrefetcher = nullptr;
    QList<Track> m_tracks;
    QString m_lastStoragePath;
};
