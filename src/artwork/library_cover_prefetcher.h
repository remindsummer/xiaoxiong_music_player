#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

#include "../models/track_model.h"

class CoverArtService;
class QMediaPlayer;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// 扫描后异步预拉本地曲目封面：内嵌图优先，Meting 搜图兜底。
class LibraryCoverPrefetcher : public QObject
{
    Q_OBJECT

public:
    explicit LibraryCoverPrefetcher(QObject *parent = nullptr);

    void setCoverArtService(CoverArtService *coverArt);

    void enqueue(const QString &path, const QString &title, const QString &artist);
    void enqueueTracks(const QList<Track> &tracks);

    // 立即为单曲拉取封面（内嵌图 → 文件名 ID → Meting 搜索），用于用户手动触发。
    void fetchNow(const QString &path, const QString &title, const QString &artist);

signals:
    void fetchFinished(const QString &path, bool success, const QString &message);

private:
    struct PendingItem {
        QString path;
        QString title;
        QString artist;
    };

    enum class Phase {
        Idle,
        LoadingEmbedded,
        WaitingMeting,
    };

    bool isQueuedOrCurrent(const QString &path) const;
    void processNext();
    void startEmbeddedLookup();
    void finishEmbeddedLookup();
    void startMetingLookup();
    void finishCurrent();
    void finishManualFetch(bool success, const QString &message);
    void cancelInFlight();
    void requestCoverThenContinue(const QString &picUrl,
                                  const QString &server,
                                  const QString &onlineId);

    CoverArtService *m_coverArt = nullptr;
    QMediaPlayer *m_player = nullptr;
    QTimer *m_timeout = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_metingReply = nullptr;
    QQueue<PendingItem> m_queue;
    PendingItem m_current;
    Phase m_phase = Phase::Idle;
    bool m_busy = false;
    bool m_awaitingLoad = false;
    bool m_manualFetch = false;
};
