#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

class QMediaPlayer;
class QTimer;

// 基于 QMediaPlayer 的异步元数据探测器：
// 逐个加载音频文件（不播放），读取时长与标题/歌手/专辑标签后回填。
// 全程串行，单实例处理一个队列，避免同时打开过多解码器。
class LibraryMetadataResolver : public QObject
{
    Q_OBJECT

public:
    explicit LibraryMetadataResolver(QObject *parent = nullptr);

    void enqueue(const QString &id, const QString &path);
    bool isBusy() const;

signals:
    void trackResolved(const QString &id,
                       const QString &title,
                       const QString &artist,
                       const QString &album,
                       qint64 durationMs);
    void finished();

private:
    struct PendingItem {
        QString id;
        QString path;
    };

    void processNext();
    void resolveCurrentFromPlayer();
    void finishCurrent(const QString &title,
                       const QString &artist,
                       const QString &album,
                       qint64 durationMs);

    QMediaPlayer *m_player = nullptr;
    QTimer *m_timeout = nullptr;
    QQueue<PendingItem> m_queue;
    PendingItem m_current;
    bool m_busy = false;
    bool m_awaitingLoad = false;
};
