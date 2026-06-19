#include "application_controller.h"

#include "../artwork/cover_art_service.h"
#include "../artwork/library_cover_prefetcher.h"
#include "../download/online_download_service.h"
#include "../library/library_repository.h"
#include "../library/library_scanner.h"
#include "../lyrics/lyric_service.h"
#include "../player/audio_visualizer_service.h"
#include "../player/playback_service.h"
#include "../playlist/playlist_service.h"
#include "../search/search_service.h"
#include "../i18n/ui_translation_service.h"
#include "../app/tray_service.h"
#include "../player/media_session_integration.h"
#include "../settings/settings_service.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
    , m_playbackService(new PlaybackService(this))
    , m_libraryScanner(new LibraryScanner(this))
    , m_libraryRepository(new LibraryRepository(this))
    , m_playlistService(new PlaylistService(this))
    , m_lyricService(new LyricService(this))
    , m_searchService(new SearchService(this))
    , m_settingsService(new SettingsService(this))
    , m_coverArtService(new CoverArtService(this))
    , m_coverPrefetcher(new LibraryCoverPrefetcher(this))
    , m_downloadService(new OnlineDownloadService(this))
    , m_audioVisualizerService(new AudioVisualizerService(this))
    , m_trayService(new TrayService(this))
    , m_mediaSessionIntegration(MediaSessionIntegration::create(this))
{
    m_playbackService->setCoverArtService(m_coverArtService);
    m_playbackService->setCoverPrefetcher(m_coverPrefetcher);
    m_playbackService->setAudioVisualizer(m_audioVisualizerService);
    m_coverPrefetcher->setCoverArtService(m_coverArtService);
    m_libraryRepository->setCoverPrefetcher(m_coverPrefetcher);
    m_downloadService->setSettingsService(m_settingsService);
    m_downloadService->setLibraryRepository(m_libraryRepository);
    m_downloadService->setCoverArtService(m_coverArtService);

    m_audioVisualizerService->setRefreshFps(m_settingsService->spectrumFps());
    m_audioVisualizerService->setProcessingEnabled(m_settingsService->spectrumEnabled());
    connect(m_settingsService, &SettingsService::spectrumFpsChanged, this, [this]() {
        m_audioVisualizerService->setRefreshFps(m_settingsService->spectrumFps());
    });
    connect(m_settingsService, &SettingsService::spectrumEnabledChanged, this, [this]() {
        m_audioVisualizerService->setProcessingEnabled(m_settingsService->spectrumEnabled());
    });

    m_playbackService->restoreSession();
    m_trayService->initialize();
    m_mediaSessionIntegration->attachToPlayback(m_playbackService);
}

ApplicationController::~ApplicationController()
{
    if (m_playbackService) {
        m_playbackService->saveSession();
    }
}

QString ApplicationController::appName() const
{
    return tr("小熊音乐播放器");
}

int ApplicationController::uiLanguageRevision() const
{
    return m_uiLanguageRevision;
}

void ApplicationController::setUiTranslationService(UiTranslationService *translationService)
{
    m_translationService = translationService;
}

bool ApplicationController::applyUiLanguage()
{
    if (!m_translationService || !m_settingsService) {
        return false;
    }

    m_translationService->applyLanguage(m_settingsService->language());
    if (m_playbackService) {
        m_playbackService->retranslateUi();
    }
    bumpUiLanguageRevision();
    return true;
}

void ApplicationController::bumpUiLanguageRevision()
{
    ++m_uiLanguageRevision;
    emit uiLanguageRevisionChanged();
}

QString ApplicationController::healthCheck() const
{
    return QStringLiteral("ApplicationController ready");
}

QObject *ApplicationController::playback() const
{
    return m_playbackService;
}

QObject *ApplicationController::libraryRepository() const
{
    return m_libraryRepository;
}

QObject *ApplicationController::search() const
{
    return m_searchService;
}

QObject *ApplicationController::playlist() const
{
    return m_playlistService;
}

QObject *ApplicationController::lyrics() const
{
    return m_lyricService;
}

QObject *ApplicationController::settings() const
{
    return m_settingsService;
}

QObject *ApplicationController::downloadService() const
{
    return m_downloadService;
}

QObject *ApplicationController::playbackService() const
{
    return m_playbackService;
}

QObject *ApplicationController::libraryService() const
{
    return m_libraryRepository;
}

QObject *ApplicationController::searchService() const
{
    return m_searchService;
}

QObject *ApplicationController::playlistService() const
{
    return m_playlistService;
}

QObject *ApplicationController::lyricsService() const
{
    return m_lyricService;
}

QObject *ApplicationController::settingsService() const
{
    return m_settingsService;
}

QObject *ApplicationController::audioVisualizer() const
{
    return m_audioVisualizerService;
}

QObject *ApplicationController::trayService() const
{
    return m_trayService;
}

QObject *ApplicationController::mediaSessionIntegration() const
{
    return m_mediaSessionIntegration;
}
