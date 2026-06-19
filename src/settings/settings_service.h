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
    Q_PROPERTY(bool spectrumEnabled READ spectrumEnabled WRITE setSpectrumEnabled NOTIFY spectrumEnabledChanged)
    Q_PROPERTY(int spectrumFps READ spectrumFps WRITE setSpectrumFps NOTIFY spectrumFpsChanged)
    Q_PROPERTY(QString spectrumStyle READ spectrumStyle WRITE setSpectrumStyle NOTIFY spectrumStyleChanged)
    Q_PROPERTY(qreal spectrumOpacity READ spectrumOpacity WRITE setSpectrumOpacity NOTIFY spectrumOpacityChanged)
    Q_PROPERTY(QString metingApiBases READ metingApiBases WRITE setMetingApiBases NOTIFY metingApiBasesChanged)
    Q_PROPERTY(QString closeBehavior READ closeBehavior WRITE setCloseBehavior NOTIFY closeBehaviorChanged)
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

    bool spectrumEnabled() const;
    void setSpectrumEnabled(bool enabled);

    int spectrumFps() const;
    void setSpectrumFps(int fps);

    QString spectrumStyle() const;
    void setSpectrumStyle(const QString &style);

    qreal spectrumOpacity() const;
    void setSpectrumOpacity(qreal opacity);

    QString metingApiBases() const;
    void setMetingApiBases(const QString &basesText);

    QString closeBehavior() const;
    void setCloseBehavior(const QString &behavior);

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
    void spectrumEnabledChanged();
    void spectrumFpsChanged();
    void spectrumStyleChanged();
    void spectrumOpacityChanged();
    void metingApiBasesChanged();
    void closeBehaviorChanged();

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
    bool m_spectrumEnabled = true;
    int m_spectrumFps = 30;
    QString m_spectrumStyle;
    qreal m_spectrumOpacity = 0.58;
    QString m_metingApiBases;
    QString m_closeBehavior;
};
