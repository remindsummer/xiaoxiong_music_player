#include "media_session_integration.h"

#include "playback_service.h"

#ifdef Q_OS_WIN
#include "windows_media_session_integration.h"
#endif

MediaSessionIntegration::MediaSessionIntegration(QObject *parent)
    : QObject(parent)
{
}

MediaSessionIntegration::~MediaSessionIntegration() = default;

MediaSessionIntegration *MediaSessionIntegration::create(QObject *parent)
{
#ifdef Q_OS_WIN
    return new WindowsMediaSessionIntegration(parent);
#else
    return new MediaSessionIntegration(parent);
#endif
}

void MediaSessionIntegration::setWindow(QObject *windowObject)
{
    Q_UNUSED(windowObject);
}

void MediaSessionIntegration::attachToPlayback(PlaybackService *playback)
{
    m_playback = playback;
}
