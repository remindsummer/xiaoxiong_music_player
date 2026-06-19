#include <QtTest>
#include <QtMath>
#include <vector>

#include "../../src/player/audio_spectrum_analyzer.h"

class TestAudioSpectrumAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    void sineWavePeaksNearExpectedBin();
};

void TestAudioSpectrumAnalyzer::sineWavePeaksNearExpectedBin()
{
    constexpr int sampleRate = 44100;
    constexpr float frequency = 440.0f;
    std::vector<float> samples(AudioSpectrumAnalyzer::kFftSize, 0.0f);
    for (int i = 0; i < AudioSpectrumAnalyzer::kFftSize; ++i) {
        samples[i] = qSin(2.0f * float(M_PI) * frequency * i / sampleRate);
    }

    const QVector<float> levels = AudioSpectrumAnalyzer::analyze(
        samples.data(),
        static_cast<int>(samples.size()),
        sampleRate);

    QCOMPARE(levels.size(), AudioSpectrumAnalyzer::kBinCount);

    int peakIndex = 0;
    float peakValue = 0.0f;
    for (int i = 0; i < levels.size(); ++i) {
        if (levels[i] > peakValue) {
            peakValue = levels[i];
            peakIndex = i;
        }
    }

    QVERIFY(peakValue > 0.5f);
    QVERIFY(peakIndex >= 8);
    QVERIFY(peakIndex <= 18);
}

QTEST_APPLESS_MAIN(TestAudioSpectrumAnalyzer)
#include "test_audio_spectrum_analyzer.moc"
