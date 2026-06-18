#include "library_scanner.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

LibraryScanner::LibraryScanner(QObject *parent)
    : QObject(parent)
{
}

QVariantList LibraryScanner::scanDirectory(const QString &path) const
{
    const QList<Track> tracks = scanDirectoryTracks(path);
    QVariantList result;
    result.reserve(tracks.size());
    for (const Track &track : tracks) {
        result.append(track.toVariantMap());
    }
    return result;
}

QList<Track> LibraryScanner::scanDirectoryTracks(const QString &path) const
{
    QList<Track> result;
    if (path.trimmed().isEmpty()) {
        return result;
    }

    const QDir rootDir(path);
    if (!rootDir.exists()) {
        return result;
    }

    QDirIterator iterator(
        rootDir.absolutePath(),
        QDir::Files | QDir::Readable | QDir::NoSymLinks,
        QDirIterator::Subdirectories);

    while (iterator.hasNext()) {
        const QString filePath = iterator.next();
        if (!isSupportedAudioFile(filePath)) {
            continue;
        }
        result.append(buildTrack(filePath));
    }

    return result;
}

Track LibraryScanner::trackFromPath(const QString &filePath) const
{
    if (!isSupportedAudioFile(filePath)) {
        return Track{};
    }
    return buildTrack(filePath);
}

bool LibraryScanner::isSupportedAudioFile(const QString &filePath) const
{
    static const QStringList kSupportedExtensions = {
        QStringLiteral("mp3"),
        QStringLiteral("flac"),
        QStringLiteral("wav"),
        QStringLiteral("m4a"),
        QStringLiteral("aac"),
        QStringLiteral("ogg"),
        QStringLiteral("wma"),
        QStringLiteral("ape"),
    };

    const QString ext = QFileInfo(filePath).suffix().toLower();
    return kSupportedExtensions.contains(ext);
}

Track LibraryScanner::buildTrack(const QString &filePath) const
{
    const QFileInfo info(filePath);
    Track track;
    track.path = info.absoluteFilePath();
    track.title = info.completeBaseName();
    track.artist = QStringLiteral("Unknown Artist");
    track.album = QStringLiteral("Unknown Album");
    track.durationMs = 0;

    // wave-a 阶段先保证扫描可用，时长在后续接元数据解析器后补齐。
    const QByteArray hash = QCryptographicHash::hash(
        track.path.toUtf8(), QCryptographicHash::Sha1);
    track.id = QString::fromLatin1(hash.toHex());

    return track;
}
