#include "audio_visualizer_service.h"

#include "audio_spectrum_analyzer.h"

#include <QAudioFormat>
#include <QtMath>
#include <algorithm>

namespace {
constexpr int kStaleMs = 250;
constexpr float kAttack = 0.55f;
constexpr float kRelease = 0.14f;
constexpr float kDecayFactor = 0.88f;

int clampRefreshFps(int fps)
{
    return qBound(15, fps, 60);
}
} // namespace

AudioVisualizerService::AudioVisualizerService(QObject *parent)
    : QObject(parent)
{
    m_targetLevels = QVector<float>(AudioSpectrumAnalyzer::kBinCount, 0.0f);
    m_displayLevels = QVector<float>(AudioSpectrumAnalyzer::kBinCount, 0.0f);
    m_levelsForQml = QVariantList(AudioSpectrumAnalyzer::kBinCount, 0.0);

    m_tickTimer = new QTimer(this);
    applyRefreshInterval();
    connect(m_tickTimer, &QTimer::timeout, this, [this]() {
        smoothAndPublish();
        decayLevels();
    });
    m_tickTimer->start();
}

QVariantList AudioVisualizerService::spectrumLevels() const
{
    return m_levelsForQml;
}

bool AudioVisualizerService::spectrumAvailable() const
{
    return m_spectrumAvailable;
}

int AudioVisualizerService::refreshFps() const
{
    return m_refreshFps;
}

void AudioVisualizerService::setRefreshFps(int fps)
{
    const int clamped = clampRefreshFps(fps);
    if (m_refreshFps == clamped) {
        return;
    }
    m_refreshFps = clamped;
    applyRefreshInterval();
    emit refreshFpsChanged();
}

bool AudioVisualizerService::processingEnabled() const
{
    return m_processingEnabled;
}

void AudioVisualizerService::setProcessingEnabled(bool enabled)
{
    if (m_processingEnabled == enabled) {
        return;
    }
    m_processingEnabled = enabled;
    if (!enabled) {
        reset();
    }
    emit processingEnabledChanged();
}

void AudioVisualizerService::applyRefreshInterval()
{
    m_throttleMs = qMax(16, 1000 / m_refreshFps);
    if (m_tickTimer) {
        m_tickTimer->setInterval(m_throttleMs);
    }
}

void AudioVisualizerService::feedBuffer(const QAudioBuffer &buffer)
{
    if (!m_processingEnabled || !buffer.isValid() || buffer.frameCount() == 0) {
        return;
    }

    if (!m_spectrumAvailable) {
        m_spectrumAvailable = true;
        emit spectrumAvailableChanged();
    }

    m_lastBufferMs = m_throttle.elapsed();

    const QAudioFormat format = buffer.format();
    const int channelCount = std::max(1, format.channelCount());
    const int sampleRate = format.sampleRate() > 0 ? format.sampleRate() : 44100;

    QVector<float> mono;
    mono.reserve(static_cast<int>(buffer.frameCount()));

    if (format.sampleFormat() == QAudioFormat::Float) {
        const float *data = buffer.constData<float>();
        const int frameCount = static_cast<int>(buffer.frameCount());
        for (int frame = 0; frame < frameCount; ++frame) {
            float sum = 0.0f;
            for (int ch = 0; ch < channelCount; ++ch) {
                sum += data[frame * channelCount + ch];
            }
            mono.append(sum / channelCount);
        }
    } else if (format.sampleFormat() == QAudioFormat::Int16) {
        const qint16 *data = buffer.constData<qint16>();
        const int frameCount = static_cast<int>(buffer.frameCount());
        for (int frame = 0; frame < frameCount; ++frame) {
            float sum = 0.0f;
            for (int ch = 0; ch < channelCount; ++ch) {
                sum += data[frame * channelCount + ch] / 32768.0f;
            }
            mono.append(sum / channelCount);
        }
    } else {
        return;
    }

    if (m_throttle.isValid() && m_throttle.elapsed() < m_throttleMs) {
        return;
    }
    m_throttle.restart();

    m_targetLevels = AudioSpectrumAnalyzer::analyze(
        mono.constData(),
        mono.size(),
        sampleRate);
}

void AudioVisualizerService::reset()
{
    m_spectrumAvailable = false;
    m_lastBufferMs = 0;
    m_targetLevels.fill(0.0f);
    m_displayLevels.fill(0.0f);
    publishLevels(m_displayLevels);
    emit spectrumAvailableChanged();
}

void AudioVisualizerService::publishLevels(const QVector<float> &levels)
{
    m_levelsForQml.clear();
    m_levelsForQml.reserve(levels.size());
    for (float value : levels) {
        m_levelsForQml.append(static_cast<double>(value));
    }
    emit spectrumChanged();
}

void AudioVisualizerService::smoothAndPublish()
{
    if (!m_processingEnabled || !m_spectrumAvailable) {
        return;
    }

    bool changed = false;
    for (int i = 0; i < m_displayLevels.size(); ++i) {
        const float target = i < m_targetLevels.size() ? m_targetLevels[i] : 0.0f;
        float &display = m_displayLevels[i];
        const float delta = target - display;
        if (qAbs(delta) < 0.0005f) {
            continue;
        }
        const float coeff = target > display ? kAttack : kRelease;
        display += delta * coeff;
        changed = true;
    }

    if (changed) {
        publishLevels(m_displayLevels);
    }
}

void AudioVisualizerService::decayLevels()
{
    if (!m_processingEnabled || !m_spectrumAvailable) {
        return;
    }

    if (m_throttle.isValid() && m_throttle.elapsed() - m_lastBufferMs > kStaleMs) {
        bool hasEnergy = false;
        for (float &value : m_targetLevels) {
            value *= kDecayFactor;
            if (value > 0.02f) {
                hasEnergy = true;
            }
        }
        if (!hasEnergy) {
            reset();
        }
    }
}
