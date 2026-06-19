#pragma once

#include <QObject>

class PlaybackService;
class QWindow;

// 系统原生媒体控制（Windows SMTC / Linux MPRIS 等）的统一入口。
// 各平台提供独立实现，通过 create() 在编译期选择。
class MediaSessionIntegration : public QObject
{
    Q_OBJECT

public:
    explicit MediaSessionIntegration(QObject *parent = nullptr);
    ~MediaSessionIntegration() override;

    static MediaSessionIntegration *create(QObject *parent = nullptr);

    Q_INVOKABLE virtual void setWindow(QObject *windowObject);
    virtual void attachToPlayback(PlaybackService *playback);

protected:
    PlaybackService *m_playback = nullptr;
};
