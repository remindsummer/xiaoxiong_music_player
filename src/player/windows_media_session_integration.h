#pragma once

#include "media_session_integration.h"

#include <QPointer>

class WindowsMediaSessionIntegration : public MediaSessionIntegration
{
    Q_OBJECT

public:
    explicit WindowsMediaSessionIntegration(QObject *parent = nullptr);
    ~WindowsMediaSessionIntegration() override;

    void setWindow(QObject *windowObject) override;
    void attachToPlayback(PlaybackService *playback) override;

private:
    void syncEnabledState();
    void updatePlaybackStatus();
    void updateMetadata();

    struct Impl;
    Impl *m_impl = nullptr;
    QPointer<QObject> m_windowObject;
};
