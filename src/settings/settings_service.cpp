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

int defaultSpectrumFps()
{
    return 30;
}

int clampSpectrumFps(int fps)
{
    return qBound(15, fps, 60);
}

QString defaultSpectrumStyle()
{
    return QStringLiteral("smoothMirror");
}

QStringList validSpectrumStyles()
{
    return {
        QStringLiteral("smoothMirror"),
        QStringLiteral("mirrorBars"),
        QStringLiteral("lineTrace"),
        QStringLiteral("upperGlow"),
        QStringLiteral("particleDots"),
    };
}

QString normalizeSpectrumStyle(const QString &style)
{
    const QString trimmed = style.trimmed();
    if (validSpectrumStyles().contains(trimmed)) {
        return trimmed;
    }
    return defaultSpectrumStyle();
}

qreal defaultSpectrumOpacity()
{
    return 0.58;
}

qreal clampSpectrumOpacity(qreal opacity)
{
    return qBound(0.15, opacity, 1.0);
}

QString normalizeCloseBehavior(const QString &behavior)
{
    const QString trimmed = behavior.trimmed();
    if (trimmed == QStringLiteral("tray") || trimmed == QStringLiteral("quit")) {
        return trimmed;
    }
    return QStringLiteral("ask");
}

QString defaultCloseBehavior()
{
    return QStringLiteral("ask");
}

} // namespace

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
    , m_spectrumStyle(defaultSpectrumStyle())
    , m_spectrumOpacity(defaultSpectrumOpacity())
    , m_closeBehavior(defaultCloseBehavior())
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
    QString normalized = language.trimmed();
    if (normalized != QStringLiteral("en_US") && normalized != QStringLiteral("zh_CN")) {
        normalized = defaultLanguage();
    }
    if (m_language == normalized) {
        return;
    }
    m_language = normalized;
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

bool SettingsService::spectrumEnabled() const
{
    return m_spectrumEnabled;
}

void SettingsService::setSpectrumEnabled(bool enabled)
{
    if (m_spectrumEnabled == enabled) {
        return;
    }
    m_spectrumEnabled = enabled;
    emit spectrumEnabledChanged();
}

int SettingsService::spectrumFps() const
{
    return m_spectrumFps;
}

void SettingsService::setSpectrumFps(int fps)
{
    const int clamped = clampSpectrumFps(fps);
    if (m_spectrumFps == clamped) {
        return;
    }
    m_spectrumFps = clamped;
    emit spectrumFpsChanged();
}

QString SettingsService::spectrumStyle() const
{
    return m_spectrumStyle;
}

void SettingsService::setSpectrumStyle(const QString &style)
{
    const QString normalized = normalizeSpectrumStyle(style);
    if (m_spectrumStyle == normalized) {
        return;
    }
    m_spectrumStyle = normalized;
    emit spectrumStyleChanged();
}

qreal SettingsService::spectrumOpacity() const
{
    return m_spectrumOpacity;
}

void SettingsService::setSpectrumOpacity(qreal opacity)
{
    const qreal clamped = clampSpectrumOpacity(opacity);
    if (qFuzzyCompare(m_spectrumOpacity, clamped)) {
        return;
    }
    m_spectrumOpacity = clamped;
    emit spectrumOpacityChanged();
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

QString SettingsService::closeBehavior() const
{
    return m_closeBehavior;
}

void SettingsService::setCloseBehavior(const QString &behavior)
{
    const QString normalized = normalizeCloseBehavior(behavior);
    if (m_closeBehavior == normalized) {
        return;
    }

    m_closeBehavior = normalized;
    emit closeBehaviorChanged();
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
    setSpectrumEnabled(true);
    setSpectrumFps(defaultSpectrumFps());
    setSpectrumStyle(defaultSpectrumStyle());
    setSpectrumOpacity(defaultSpectrumOpacity());
    setMetingApiBases(defaultMetingApiBasesValue());
    setCloseBehavior(defaultCloseBehavior());
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
    setSpectrumEnabled(object.value(QStringLiteral("spectrumEnabled")).toBool(true));
    setSpectrumFps(clampSpectrumFps(object.value(QStringLiteral("spectrumFps")).toInt(defaultSpectrumFps())));
    setSpectrumStyle(normalizeSpectrumStyle(
        object.value(QStringLiteral("spectrumStyle")).toString(defaultSpectrumStyle())));
    setSpectrumOpacity(clampSpectrumOpacity(
        object.value(QStringLiteral("spectrumOpacity")).toDouble(defaultSpectrumOpacity())));
    setMetingApiBases(metingApiBasesTextFromJsonValue(object.value(QStringLiteral("metingApiBases"))));
    setCloseBehavior(object.value(QStringLiteral("closeBehavior")).toString(defaultCloseBehavior()));
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
    object.insert(QStringLiteral("spectrumEnabled"), m_spectrumEnabled);
    object.insert(QStringLiteral("spectrumFps"), m_spectrumFps);
    object.insert(QStringLiteral("spectrumStyle"), m_spectrumStyle);
    object.insert(QStringLiteral("spectrumOpacity"), m_spectrumOpacity);
    object.insert(QStringLiteral("metingApiBases"), metingArray);
    object.insert(QStringLiteral("closeBehavior"), m_closeBehavior);
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
