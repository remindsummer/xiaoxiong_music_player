#pragma once

#include <QVector>
#include <QtGlobal>
#include <complex>
#include <vector>

class AudioSpectrumAnalyzer
{
public:
    static constexpr int kFftSize = 256;
    static constexpr int kBinCount = 32;

    static QVector<float> analyze(const float *samples, int sampleCount, int sampleRate);

private:
    static void applyHannWindow(std::vector<float> &samples);
    static void fft(std::vector<std::complex<float>> &data);
    static QVector<float> mapToBins(const std::vector<float> &magnitudes, int sampleRate);
};
