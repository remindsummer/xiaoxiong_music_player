#pragma once

#include <QObject>
#include <QVariantMap>

class CoverArtService;
class LibraryRepository;
class MetingStreamResolver;
class SettingsService;
class QNetworkAccessManager;
class QNetworkReply;

class OnlineDownloadService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString downloadState READ downloadState NOTIFY downloadStateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit OnlineDownloadService(QObject *parent = nullptr);

    void setSettingsService(SettingsService *settings);
    void setLibraryRepository(LibraryRepository *library);
    void setCoverArtService(CoverArtService *coverArt);

    QString downloadState() const;
    QString lastError() const;

    Q_INVOKABLE void downloadTrack(const QVariantMap &track);

signals:
    void downloadStateChanged();
    void lastErrorChanged();
    void downloadFinished(const QString &localPath);
    void downloadFailed(const QString &message);

private:
    enum class State {
        Idle,
        Downloading,
        Success,
        Failed
    };

    void setState(State state);
    void setLastError(const QString &message);
    static QString sanitizeFileNamePart(const QString &value);
    QString buildTargetPath(const QVariantMap &track) const;
    void startFileDownload(const QUrl &streamUrl, const QString &targetPath, const QVariantMap &track);

    SettingsService *m_settings = nullptr;
    LibraryRepository *m_library = nullptr;
    CoverArtService *m_coverArt = nullptr;
    MetingStreamResolver *m_streamResolver = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    State m_state = State::Idle;
    QString m_lastError;
    QVariantMap m_pendingTrack;
    QString m_pendingTargetPath;
};
