#include "library_metadata_resolver.h"

#include "audio_tag_reader.h"

#include <QTimer>

LibraryMetadataResolver::LibraryMetadataResolver(QObject *parent)
    : QObject(parent)
{
}

void LibraryMetadataResolver::enqueue(const QString &id, const QString &path)
{
    if (id.isEmpty() || path.isEmpty()) {
        return;
    }
    m_queue.enqueue(PendingItem{id, path});
    if (!m_busy) {
        processNext();
    }
}

bool LibraryMetadataResolver::isBusy() const
{
    return m_busy;
}

void LibraryMetadataResolver::processNext()
{
    if (m_queue.isEmpty()) {
        m_busy = false;
        emit finished();
        return;
    }

    m_busy = true;
    m_current = m_queue.dequeue();

    // 让出事件循环，避免大批量扫库时长时间阻塞 UI。
    QTimer::singleShot(0, this, [this]() {
        const AudioTagMetadata metadata = AudioTagReader::readMetadata(m_current.path);
        finishCurrent(metadata.title, metadata.artist, metadata.album, metadata.durationMs);
    });
}

void LibraryMetadataResolver::finishCurrent(const QString &title,
                                            const QString &artist,
                                            const QString &album,
                                            qint64 durationMs)
{
    emit trackResolved(m_current.id, title, artist, album, durationMs);
    processNext();
}
