#pragma once

#include <QAudioBuffer>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class AudioVisualizerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList spectrumLevels READ spectrumLevels NOTIFY spectrumChanged)
    Q_PROPERTY(bool spectrumAvailable READ spectrumAvailable NOTIFY spectrumAvailableChanged)
    Q_PROPERTY(int refreshFps READ refreshFps WRITE setRefreshFps NOTIFY refreshFpsChanged)
    Q_PROPERTY(bool processingEnabled READ processingEnabled WRITE setProcessingEnabled NOTIFY processingEnabledChanged)

public:
    explicit AudioVisualizerService(QObject *parent = nullptr);

    QVariantList spectrumLevels() const;
    bool spectrumAvailable() const;

    int refreshFps() const;
    void setRefreshFps(int fps);

    bool processingEnabled() const;
    void setProcessingEnabled(bool enabled);

public slots:
    void feedBuffer(const QAudioBuffer &buffer);
    void reset();

signals:
    void spectrumChanged();
    void spectrumAvailableChanged();
    void refreshFpsChanged();
    void processingEnabledChanged();

private:
    void publishLevels(const QVector<float> &levels);
    void smoothAndPublish();
    void decayLevels();
    void applyRefreshInterval();

    QVector<float> m_targetLevels;
    QVector<float> m_displayLevels;
    QVariantList m_levelsForQml;
    bool m_spectrumAvailable = false;
    bool m_processingEnabled = true;
    int m_refreshFps = 30;
    int m_throttleMs = 33;
    QTimer *m_tickTimer = nullptr;
    QElapsedTimer m_throttle;
    qint64 m_lastBufferMs = 0;
};
