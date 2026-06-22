#include "audio_tag_reader.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>

#if __has_include(<taglib/tag_c.h>)
#include <taglib/tag_c.h>
#elif __has_include(<tag_c.h>)
#include <tag_c.h>
#else
#error "TagLib C headers not found"
#endif

namespace {

QString utf8ToQString(const char *value)
{
    if (!value || value[0] == '\0') {
        return QString();
    }
    return QString::fromUtf8(value);
}

QByteArray filePathForTagLib(const QString &path)
{
    const QString nativePath = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
#if defined(Q_OS_WIN)
    return nativePath.toUtf8();
#else
    return nativePath.toLocal8Bit();
#endif
}

QString firstPropertyValue(TagLib_File *file, const char *key)
{
    char **values = taglib_property_get(file, key);
    if (!values || !values[0]) {
        if (values) {
            taglib_property_free(values);
        }
        return QString();
    }
    const QString result = utf8ToQString(values[0]);
    taglib_property_free(values);
    return result;
}

} // namespace

AudioTagMetadata AudioTagReader::readMetadata(const QString &path)
{
    AudioTagMetadata metadata;
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return metadata;
    }

    const QByteArray filePath = filePathForTagLib(fileInfo.absoluteFilePath());
    TagLib_File *file = taglib_file_new(filePath.constData());
    if (!file || !taglib_file_is_valid(file)) {
        if (file) {
            taglib_file_free(file);
        }
        return metadata;
    }

    if (TagLib_Tag *tag = taglib_file_tag(file)) {
        metadata.title = utf8ToQString(taglib_tag_title(tag));
        metadata.artist = utf8ToQString(taglib_tag_artist(tag));
        metadata.album = utf8ToQString(taglib_tag_album(tag));
    }

    if (const TagLib_AudioProperties *properties = taglib_file_audioproperties(file)) {
        metadata.durationMs = static_cast<qint64>(taglib_audioproperties_length(properties)) * 1000LL;
    }

    const QString albumArtist = firstPropertyValue(file, "ALBUMARTIST");
    const QString trackArtist = firstPropertyValue(file, "ARTIST");
    if (!albumArtist.isEmpty()) {
        metadata.artist = albumArtist;
    } else if (metadata.artist.isEmpty() && !trackArtist.isEmpty()) {
        metadata.artist = trackArtist;
    }

    const QString title = firstPropertyValue(file, "TITLE");
    if (!title.isEmpty()) {
        metadata.title = title;
    }
    const QString album = firstPropertyValue(file, "ALBUM");
    if (!album.isEmpty()) {
        metadata.album = album;
    }

    taglib_file_free(file);
    return metadata;
}

QByteArray AudioTagReader::readEmbeddedCover(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return QByteArray();
    }

    const QByteArray filePath = filePathForTagLib(fileInfo.absoluteFilePath());
    TagLib_File *file = taglib_file_new(filePath.constData());
    if (!file || !taglib_file_is_valid(file)) {
        if (file) {
            taglib_file_free(file);
        }
        return QByteArray();
    }

    TagLib_Complex_Property_Attribute ***properties = taglib_complex_property_get(file, "PICTURE");
    if (!properties) {
        taglib_file_free(file);
        return QByteArray();
    }

    TagLib_Complex_Property_Picture_Data picture{};
    taglib_picture_from_complex_property(properties, &picture);

    QByteArray imageData;
    if (picture.data && picture.size > 0) {
        imageData = QByteArray(picture.data, static_cast<int>(picture.size));
    }

    taglib_complex_property_free(properties);
    taglib_file_free(file);
    return imageData;
}
