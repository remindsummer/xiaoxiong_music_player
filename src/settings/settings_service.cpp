#include "settings_service.h"

#include "../network/meting_api.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace {

const char *kSettingsFileName = "settings.json";

// 旧版 QSettings 键名，仅用于一次性迁移。
const char *kLegacyThemeKey = "settings/theme";
const char *kLegacyLanguageKey = "settings/language";
const char *kLegacyDefaultScanDirectoryKey = "settings/defaultScanDirectory";
const char *kLegacyDefaultDownloadDirectoryKey = "settings/defaultDownloadDirectory";
const char *kLegacyLyricOnlineEnabledKey = "settings/lyricOnlineEnabled";
const char *kLegacyMetingApiBasesKey = "settings/metingApiBases";

QString defaultTheme()
{
    return QStringLiteral("system");
}

QString defaultLanguage()
{
    return QStringLiteral("zh_CN");
}

QString defaultScanDirectoryValue()
{
    const QString musicLocation = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (!musicLocation.isEmpty()) {
        return musicLocation;
    }
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString defaultDownloadDirectoryValue()
{
    const QString musicLocation = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (!musicLocation.isEmpty()) {
        return musicLocation + QStringLiteral("/Downloads");
    }
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + QStringLiteral("/Music/Downloads");
}

QString defaultMetingApiBasesValue()
{
    return MetingApi::defaultApiBases().join(QStringLiteral("\n"));
}

QStringList parseMetingApiBasesText(const QString &basesText)
{
    const QStringList lines = basesText.split(QRegularExpression(QStringLiteral("[\\r\\n]+")),
                                              Qt::SkipEmptyParts);
    QStringList result;
    result.reserve(lines.size());
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

QString metingApiBasesTextFromJsonValue(const QJsonValue &value)
{
    if (value.isArray()) {
        QStringList bases;
        const QJsonArray array = value.toArray();
        bases.reserve(array.size());
        for (const QJsonValue &item : array) {
            const QString trimmed = item.toString().trimmed();
            if (!trimmed.isEmpty()) {
                bases.append(trimmed);
            }
        }
        if (!bases.isEmpty()) {
            return bases.join(QStringLiteral("\n"));
        }
    }

    const QString text = value.toString().trimmed();
    if (!text.isEmpty()) {
        return text;
    }

    return defaultMetingApiBasesValue();
}

} // namespace

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
{
    load();
}

QString SettingsService::theme() const
{
    return m_theme;
}

void SettingsService::setTheme(const QString &theme)
{
    if (m_theme == theme) {
        return;
    }
    m_theme = theme;
    emit themeChanged();
}

QString SettingsService::language() const
{
    return m_language;
}

void SettingsService::setLanguage(const QString &language)
{
    if (m_language == language) {
        return;
    }
    m_language = language;
    emit languageChanged();
}

QString SettingsService::defaultScanDirectory() const
{
    return m_defaultScanDirectory;
}

void SettingsService::setDefaultScanDirectory(const QString &directory)
{
    if (m_defaultScanDirectory == directory) {
        return;
    }
    m_defaultScanDirectory = directory;
    emit defaultScanDirectoryChanged();
}

QString SettingsService::defaultDownloadDirectory() const
{
    return m_defaultDownloadDirectory;
}

void SettingsService::setDefaultDownloadDirectory(const QString &directory)
{
    if (m_defaultDownloadDirectory == directory) {
        return;
    }
    m_defaultDownloadDirectory = directory;
    emit defaultDownloadDirectoryChanged();
}

bool SettingsService::lyricOnlineEnabled() const
{
    return m_lyricOnlineEnabled;
}

void SettingsService::setLyricOnlineEnabled(bool enabled)
{
    if (m_lyricOnlineEnabled == enabled) {
        return;
    }
    m_lyricOnlineEnabled = enabled;
    emit lyricOnlineEnabledChanged();
}

QString SettingsService::metingApiBases() const
{
    return m_metingApiBases;
}

void SettingsService::setMetingApiBases(const QString &basesText)
{
    if (m_metingApiBases == basesText) {
        return;
    }
    m_metingApiBases = basesText;
    applyMetingApiBases();
    emit metingApiBasesChanged();
}

QString SettingsService::storageLocation() const
{
    return settingsFilePath();
}

QString SettingsService::appDataLocation() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString SettingsService::settingsFilePath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QLatin1Char('/') + QString::fromLatin1(kSettingsFileName);
}

void SettingsService::applyDefaults()
{
    setTheme(defaultTheme());
    setLanguage(defaultLanguage());
    setDefaultScanDirectory(defaultScanDirectoryValue());
    setDefaultDownloadDirectory(defaultDownloadDirectoryValue());
    setLyricOnlineEnabled(true);
    setMetingApiBases(defaultMetingApiBasesValue());
}

bool SettingsService::loadFromJsonObject(const QJsonObject &object)
{
    if (object.isEmpty()) {
        return false;
    }

    setTheme(object.value(QStringLiteral("theme")).toString(defaultTheme()));
    setLanguage(object.value(QStringLiteral("language")).toString(defaultLanguage()));
    setDefaultScanDirectory(
        object.value(QStringLiteral("defaultScanDirectory")).toString(defaultScanDirectoryValue()));
    setDefaultDownloadDirectory(
        object.value(QStringLiteral("defaultDownloadDirectory")).toString(defaultDownloadDirectoryValue()));
    setLyricOnlineEnabled(object.value(QStringLiteral("lyricOnlineEnabled")).toBool(true));
    setMetingApiBases(metingApiBasesTextFromJsonValue(object.value(QStringLiteral("metingApiBases"))));
    return true;
}

QJsonObject SettingsService::toJsonObject() const
{
    QJsonArray metingArray;
    for (const QString &base : parseMetingApiBasesText(m_metingApiBases)) {
        metingArray.append(base);
    }

    QJsonObject object;
    object.insert(QStringLiteral("theme"), m_theme);
    object.insert(QStringLiteral("language"), m_language);
    object.insert(QStringLiteral("defaultScanDirectory"), m_defaultScanDirectory);
    object.insert(QStringLiteral("defaultDownloadDirectory"), m_defaultDownloadDirectory);
    object.insert(QStringLiteral("lyricOnlineEnabled"), m_lyricOnlineEnabled);
    object.insert(QStringLiteral("metingApiBases"), metingArray);
    return object;
}

bool SettingsService::migrateFromLegacyQSettings()
{
    QSettings legacy;
    const bool hasLegacy = legacy.contains(QString::fromLatin1(kLegacyThemeKey))
                           || legacy.contains(QString::fromLatin1(kLegacyLanguageKey))
                           || legacy.contains(QString::fromLatin1(kLegacyDefaultScanDirectoryKey))
                           || legacy.contains(QString::fromLatin1(kLegacyDefaultDownloadDirectoryKey))
                           || legacy.contains(QString::fromLatin1(kLegacyLyricOnlineEnabledKey))
                           || legacy.contains(QString::fromLatin1(kLegacyMetingApiBasesKey));
    if (!hasLegacy) {
        return false;
    }

    setTheme(legacy.value(QString::fromLatin1(kLegacyThemeKey), defaultTheme()).toString());
    setLanguage(legacy.value(QString::fromLatin1(kLegacyLanguageKey), defaultLanguage()).toString());
    setDefaultScanDirectory(
        legacy.value(QString::fromLatin1(kLegacyDefaultScanDirectoryKey), defaultScanDirectoryValue()).toString());
    setDefaultDownloadDirectory(
        legacy.value(QString::fromLatin1(kLegacyDefaultDownloadDirectoryKey),
                     defaultDownloadDirectoryValue())
            .toString());
    setLyricOnlineEnabled(legacy.value(QString::fromLatin1(kLegacyLyricOnlineEnabledKey), true).toBool());
    setMetingApiBases(
        legacy.value(QString::fromLatin1(kLegacyMetingApiBasesKey), defaultMetingApiBasesValue()).toString());
    return true;
}

void SettingsService::save()
{
    const QString targetPath = settingsFilePath();
    const QFileInfo info(targetPath);
    QDir().mkpath(info.absolutePath());

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(QJsonDocument(toJsonObject()).toJson(QJsonDocument::Indented));
    file.close();
    applyMetingApiBases();
}

void SettingsService::load()
{
    const QString targetPath = settingsFilePath();
    QFile file(targetPath);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray data = file.readAll();
            file.close();

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                loadFromJsonObject(doc.object());
                return;
            }
        }
    }

    if (migrateFromLegacyQSettings()) {
        save();
        return;
    }

    applyDefaults();
}

void SettingsService::reset()
{
    applyDefaults();
    save();
}

void SettingsService::applyMetingApiBases()
{
    MetingApi::setApiBases(parseMetingApiBasesText(m_metingApiBases));
}
