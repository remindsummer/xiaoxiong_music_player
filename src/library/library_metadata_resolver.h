#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

// 基于 TagLib 的异步元数据探测器：
// 逐个读取音频文件标签（不播放），回填时长与标题/歌手/专辑。
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
    void finishCurrent(const QString &title,
                       const QString &artist,
                       const QString &album,
                       qint64 durationMs);

    QQueue<PendingItem> m_queue;
    PendingItem m_current;
    bool m_busy = false;
};
