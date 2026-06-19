#include "windows_media_session_integration.h"

#include "../app/windows_app_identity.h"
#include "playback_service.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QPointer>
#include <QWindow>
#include <QDebug>

#include <windows.h>
#include <systemmediatransportcontrolsinterop.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage::Streams;

struct WindowsMediaSessionIntegration::Impl
{
    SystemMediaTransportControls smtc{nullptr};
    winrt::event_token buttonToken{};
    bool initialized = false;
    bool apartmentInited = false;
    QPointer<PlaybackService> playback;
};

namespace {

winrt::hstring toWinRtString(const QString &text)
{
    return winrt::hstring(reinterpret_cast<const wchar_t *>(text.utf16()), static_cast<uint32_t>(text.size()));
}

} // namespace

WindowsMediaSessionIntegration::WindowsMediaSessionIntegration(QObject *parent)
    : MediaSessionIntegration(parent)
    , m_impl(new Impl)
{
}

WindowsMediaSessionIntegration::~WindowsMediaSessionIntegration()
{
    if (!m_impl) {
        return;
    }

    if (m_impl->initialized && m_impl->smtc) {
        try {
            m_impl->smtc.ButtonPressed(m_impl->buttonToken);
            m_impl->smtc.IsEnabled(false);
            m_impl->smtc.DisplayUpdater().ClearAll();
            m_impl->smtc = nullptr;
        } catch (const winrt::hresult_error &error) {
            qWarning() << "MediaSession cleanup failed:" << error.code().value;
        }
    }

    if (m_impl->apartmentInited) {
        try {
            winrt::uninit_apartment();
        } catch (...) {
        }
        m_impl->apartmentInited = false;
    }

    delete m_impl;
    m_impl = nullptr;
}

void WindowsMediaSessionIntegration::setWindow(QObject *windowObject)
{
    m_windowObject = windowObject;

    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window || !m_impl || m_impl->initialized) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        qWarning() << "MediaSession: window handle unavailable";
        return;
    }

    WindowsAppIdentity::assignWindowIdentity(window);

    try {
        if (!m_impl->apartmentInited) {
            winrt::init_apartment(winrt::apartment_type::single_threaded);
            m_impl->apartmentInited = true;
        }

        auto interop = winrt::get_activation_factory<SystemMediaTransportControls,
                                                      ISystemMediaTransportControlsInterop>();
        const HRESULT hr = interop->GetForWindow(
            hwnd,
            winrt::guid_of<ISystemMediaTransportControls>(),
            winrt::put_abi(m_impl->smtc));
        if (FAILED(hr)) {
            winrt::throw_hresult(hr);
        }

        m_impl->smtc.IsPlayEnabled(true);
        m_impl->smtc.IsPauseEnabled(true);
        m_impl->smtc.IsPreviousEnabled(true);
        m_impl->smtc.IsNextEnabled(true);
        m_impl->smtc.IsStopEnabled(true);
        m_impl->smtc.IsEnabled(false);

        const QPointer<PlaybackService> playbackPtr = m_playback;
        m_impl->buttonToken = m_impl->smtc.ButtonPressed(
            [playbackPtr](const SystemMediaTransportControls &,
                          const SystemMediaTransportControlsButtonPressedEventArgs &args) {
                if (!playbackPtr) {
                    return;
                }

                const char *method = nullptr;
                switch (args.Button()) {
                case SystemMediaTransportControlsButton::Play:
                case SystemMediaTransportControlsButton::Pause:
                    method = "togglePlayPause";
                    break;
                case SystemMediaTransportControlsButton::Next:
                    method = "next";
                    break;
                case SystemMediaTransportControlsButton::Previous:
                    method = "previous";
                    break;
                case SystemMediaTransportControlsButton::Stop:
                    method = "stop";
                    break;
                default:
                    break;
                }

                if (method) {
                    QMetaObject::invokeMethod(playbackPtr, method, Qt::QueuedConnection);
                }
            });

        m_impl->initialized = true;
        qInfo() << "MediaSession: Windows SMTC initialized";
        syncEnabledState();
    } catch (const winrt::hresult_error &error) {
        qWarning() << "MediaSession init failed:" << error.code().value << error.message().c_str();
        m_impl->smtc = nullptr;
        m_impl->initialized = false;
    }
}

void WindowsMediaSessionIntegration::attachToPlayback(PlaybackService *playback)
{
    MediaSessionIntegration::attachToPlayback(playback);
    if (!m_impl) {
        return;
    }

    m_impl->playback = playback;
    if (!playback) {
        return;
    }

    connect(playback, &PlaybackService::playbackStateChanged, this, [this]() {
        updatePlaybackStatus();
    });
    connect(playback, &PlaybackService::currentTrackChanged, this, [this]() {
        syncEnabledState();
    });
    connect(playback, &PlaybackService::currentCoverPathChanged, this, [this]() {
        updateMetadata();
    });
}

void WindowsMediaSessionIntegration::syncEnabledState()
{
    if (!m_impl || !m_impl->initialized || !m_playback || !m_impl->smtc) {
        return;
    }

    try {
        const bool hasTrack = m_playback->currentIndex() >= 0;
        m_impl->smtc.IsEnabled(hasTrack);
        if (!hasTrack) {
            auto updater = m_impl->smtc.DisplayUpdater();
            updater.ClearAll();
            updater.Update();
            m_impl->smtc.PlaybackStatus(MediaPlaybackStatus::Stopped);
            return;
        }

        updateMetadata();
        updatePlaybackStatus();
    } catch (const winrt::hresult_error &error) {
        qWarning() << "MediaSession sync failed:" << error.code().value;
    }
}

void WindowsMediaSessionIntegration::updatePlaybackStatus()
{
    if (!m_impl || !m_impl->initialized || !m_playback || !m_impl->smtc) {
        return;
    }

    try {
        MediaPlaybackStatus status = MediaPlaybackStatus::Stopped;
        switch (m_playback->state()) {
        case PlaybackService::PlaybackState::Playing:
            status = MediaPlaybackStatus::Playing;
            break;
        case PlaybackService::PlaybackState::Paused:
            status = MediaPlaybackStatus::Paused;
            break;
        case PlaybackService::PlaybackState::Stopped:
        default:
            status = MediaPlaybackStatus::Stopped;
            break;
        }

        m_impl->smtc.PlaybackStatus(status);
    } catch (const winrt::hresult_error &error) {
        qWarning() << "MediaSession status update failed:" << error.code().value;
    }
}

void WindowsMediaSessionIntegration::updateMetadata()
{
    if (!m_impl || !m_impl->initialized || !m_playback || !m_impl->smtc) {
        return;
    }

    if (m_playback->currentIndex() < 0) {
        return;
    }

    try {
        auto updater = m_impl->smtc.DisplayUpdater();
        updater.ClearAll();
        updater.Type(MediaPlaybackType::Music);
        updater.AppMediaId(WindowsAppIdentity::appUserModelId());

        auto music = updater.MusicProperties();
        music.Title(toWinRtString(m_playback->currentTrackTitle()));
        music.Artist(toWinRtString(m_playback->currentTrackArtist()));

        const QString coverPath = m_playback->currentCoverPath();
        if (!coverPath.isEmpty()) {
            const QString nativePath = QFileInfo(coverPath).absoluteFilePath();
            if (QFileInfo::exists(nativePath)) {
                QImage image(nativePath);
                if (!image.isNull()) {
                    QByteArray bytes;
                    QBuffer buffer(&bytes);
                    buffer.open(QIODevice::WriteOnly);
                    if (image.save(&buffer, "PNG")) {
                        InMemoryRandomAccessStream stream;
                        DataWriter writer(stream);
                        const auto *begin = reinterpret_cast<const uint8_t *>(bytes.constData());
                        writer.WriteBytes(winrt::array_view<const uint8_t>(begin, begin + bytes.size()));
                        writer.StoreAsync().get();
                        writer.FlushAsync().get();
                        writer.DetachStream();
                        stream.Seek(0);
                        updater.Thumbnail(RandomAccessStreamReference::CreateFromStream(stream));
                    }
                }
            }
        }

        updater.Update();
    } catch (const winrt::hresult_error &error) {
        qWarning() << "MediaSession metadata update failed:" << error.code().value;
    }
}
