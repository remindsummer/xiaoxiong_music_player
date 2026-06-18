#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class SettingsService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString defaultScanDirectory READ defaultScanDirectory WRITE setDefaultScanDirectory NOTIFY defaultScanDirectoryChanged)
    Q_PROPERTY(QString defaultDownloadDirectory READ defaultDownloadDirectory WRITE setDefaultDownloadDirectory NOTIFY defaultDownloadDirectoryChanged)
    Q_PROPERTY(bool lyricOnlineEnabled READ lyricOnlineEnabled WRITE setLyricOnlineEnabled NOTIFY lyricOnlineEnabledChanged)
    Q_PROPERTY(QString metingApiBases READ metingApiBases WRITE setMetingApiBases NOTIFY metingApiBasesChanged)
    Q_PROPERTY(QString storageLocation READ storageLocation CONSTANT)
    Q_PROPERTY(QString appDataLocation READ appDataLocation CONSTANT)

public:
    explicit SettingsService(QObject *parent = nullptr);

    QString theme() const;
    void setTheme(const QString &theme);

    QString language() const;
    void setLanguage(const QString &language);

    QString defaultScanDirectory() const;
    void setDefaultScanDirectory(const QString &directory);

    QString defaultDownloadDirectory() const;
    void setDefaultDownloadDirectory(const QString &directory);

    bool lyricOnlineEnabled() const;
    void setLyricOnlineEnabled(bool enabled);

    QString metingApiBases() const;
    void setMetingApiBases(const QString &basesText);

    QString storageLocation() const;
    QString appDataLocation() const;

    Q_INVOKABLE void save();
    Q_INVOKABLE void load();
    Q_INVOKABLE void reset();

signals:
    void themeChanged();
    void languageChanged();
    void defaultScanDirectoryChanged();
    void defaultDownloadDirectoryChanged();
    void lyricOnlineEnabledChanged();
    void metingApiBasesChanged();

private:
    QString settingsFilePath() const;
    void applyDefaults();
    bool loadFromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject() const;
    bool migrateFromLegacyQSettings();
    void applyMetingApiBases();

    QString m_theme;
    QString m_language;
    QString m_defaultScanDirectory;
    QString m_defaultDownloadDirectory;
    bool m_lyricOnlineEnabled = true;
    QString m_metingApiBases;
};
