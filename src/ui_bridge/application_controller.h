#pragma once

#include <QObject>

class PlaybackService;
class LibraryScanner;
class LibraryRepository;
class PlaylistService;
class LyricService;
class SearchService;
class SettingsService;
class CoverArtService;
class LibraryCoverPrefetcher;
class OnlineDownloadService;
class AudioVisualizerService;
class UiTranslationService;
class TrayService;
class MediaSessionIntegration;

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString appName READ appName NOTIFY uiLanguageRevisionChanged)
    Q_PROPERTY(int uiLanguageRevision READ uiLanguageRevision NOTIFY uiLanguageRevisionChanged)
    Q_PROPERTY(QObject* playback READ playback CONSTANT)
    Q_PROPERTY(QObject* libraryRepository READ libraryRepository CONSTANT)
    Q_PROPERTY(QObject* search READ search CONSTANT)
    Q_PROPERTY(QObject* playlist READ playlist CONSTANT)
    Q_PROPERTY(QObject* lyrics READ lyrics CONSTANT)
    Q_PROPERTY(QObject* settings READ settings CONSTANT)
    Q_PROPERTY(QObject* downloadService READ downloadService CONSTANT)
    Q_PROPERTY(QObject* playbackService READ playbackService CONSTANT)
    Q_PROPERTY(QObject* libraryService READ libraryService CONSTANT)
    Q_PROPERTY(QObject* searchService READ searchService CONSTANT)
    Q_PROPERTY(QObject* playlistService READ playlistService CONSTANT)
    Q_PROPERTY(QObject* lyricsService READ lyricsService CONSTANT)
    Q_PROPERTY(QObject* settingsService READ settingsService CONSTANT)
    Q_PROPERTY(QObject* audioVisualizer READ audioVisualizer CONSTANT)
    Q_PROPERTY(QObject* trayService READ trayService CONSTANT)
    Q_PROPERTY(QObject* mediaSessionIntegration READ mediaSessionIntegration CONSTANT)

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    QString appName() const;
    Q_INVOKABLE QString healthCheck() const;
    QObject* playback() const;
    QObject* libraryRepository() const;
    QObject* search() const;
    QObject* playlist() const;
    QObject* lyrics() const;
    QObject* settings() const;
    QObject* downloadService() const;
    QObject* playbackService() const;
    QObject* libraryService() const;
    QObject* searchService() const;
    QObject* playlistService() const;
    QObject* lyricsService() const;
    QObject* settingsService() const;
    QObject* audioVisualizer() const;
    QObject* trayService() const;
    QObject* mediaSessionIntegration() const;

    int uiLanguageRevision() const;
    void setUiTranslationService(UiTranslationService *translationService);

    Q_INVOKABLE bool applyUiLanguage();

signals:
    void uiLanguageRevisionChanged();

private:
    void bumpUiLanguageRevision();
    PlaybackService *m_playbackService = nullptr;
    LibraryScanner *m_libraryScanner = nullptr;
    LibraryRepository *m_libraryRepository = nullptr;
    PlaylistService *m_playlistService = nullptr;
    LyricService *m_lyricService = nullptr;
    SearchService *m_searchService = nullptr;
    SettingsService *m_settingsService = nullptr;
    CoverArtService *m_coverArtService = nullptr;
    LibraryCoverPrefetcher *m_coverPrefetcher = nullptr;
    OnlineDownloadService *m_downloadService = nullptr;
    AudioVisualizerService *m_audioVisualizerService = nullptr;
    UiTranslationService *m_translationService = nullptr;
    TrayService *m_trayService = nullptr;
    MediaSessionIntegration *m_mediaSessionIntegration = nullptr;
    int m_uiLanguageRevision = 0;
};
