#pragma once

#include <QByteArray>
#include <QString>

struct AudioTagMetadata {
    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;
};

// 基于 TagLib 的本地音频标签读取（不创建 QMediaPlayer，不与播放争用 WMF）。
class AudioTagReader
{
public:
    static AudioTagMetadata readMetadata(const QString &path);
    static QByteArray readEmbeddedCover(const QString &path);
};
