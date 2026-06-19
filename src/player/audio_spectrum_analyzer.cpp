#include "audio_spectrum_analyzer.h"

#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kMinMagnitude = 0.0001f;
}

QVector<float> AudioSpectrumAnalyzer::analyze(const float *samples, int sampleCount, int sampleRate)
{
    if (!samples || sampleCount <= 0 || sampleRate <= 0) {
        return QVector<float>(kBinCount, 0.0f);
    }

    std::vector<float> windowed(kFftSize, 0.0f);
    const int copyCount = std::min(sampleCount, kFftSize);
    std::copy(samples, samples + copyCount, windowed.begin());
    applyHannWindow(windowed);

    std::vector<std::complex<float>> spectrum(kFftSize);
    for (int i = 0; i < kFftSize; ++i) {
        spectrum[i] = std::complex<float>(windowed[i], 0.0f);
    }
    fft(spectrum);

    const int half = kFftSize / 2;
    std::vector<float> magnitudes(half, 0.0f);
    for (int i = 0; i < half; ++i) {
        magnitudes[i] = std::abs(spectrum[i]);
    }

    QVector<float> bins = mapToBins(magnitudes, sampleRate);

    float peak = 0.0f;
    for (float value : bins) {
        peak = std::max(peak, value);
    }
    if (peak <= kMinMagnitude) {
        return QVector<float>(kBinCount, 0.0f);
    }

    for (float &value : bins) {
        value = std::clamp(value / peak, 0.0f, 1.0f);
    }
    return bins;
}

void AudioSpectrumAnalyzer::applyHannWindow(std::vector<float> &samples)
{
    const int n = static_cast<int>(samples.size());
    if (n <= 1) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        const float factor = 0.5f * (1.0f - std::cos((2.0f * static_cast<float>(M_PI) * i) / (n - 1)));
        samples[i] *= factor;
    }
}

void AudioSpectrumAnalyzer::fft(std::vector<std::complex<float>> &data)
{
    const int n = static_cast<int>(data.size());
    if (n <= 1) {
        return;
    }

    int j = 0;
    for (int i = 1; i < n; ++i) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        const float angle = -2.0f * static_cast<float>(M_PI) / len;
        const std::complex<float> wlen(std::cos(angle), std::sin(angle));
        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int k = 0; k < len / 2; ++k) {
                const std::complex<float> u = data[i + k];
                const std::complex<float> v = data[i + k + len / 2] * w;
                data[i + k] = u + v;
                data[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

QVector<float> AudioSpectrumAnalyzer::mapToBins(const std::vector<float> &magnitudes, int sampleRate)
{
    QVector<float> bins(kBinCount, 0.0f);
    if (magnitudes.empty() || sampleRate <= 0) {
        return bins;
    }

    const int half = static_cast<int>(magnitudes.size());
    const float nyquist = static_cast<float>(sampleRate) * 0.5f;
    const float minFreq = 60.0f;
    const float maxFreq = std::min(nyquist, 16000.0f);

    for (int bin = 0; bin < kBinCount; ++bin) {
        const float t0 = static_cast<float>(bin) / kBinCount;
        const float t1 = static_cast<float>(bin + 1) / kBinCount;
        const float freq0 = minFreq * std::pow(maxFreq / minFreq, t0);
        const float freq1 = minFreq * std::pow(maxFreq / minFreq, t1);

        const int index0 = std::clamp(static_cast<int>(freq0 / nyquist * half), 0, half - 1);
        const int index1 = std::clamp(static_cast<int>(std::ceil(freq1 / nyquist * half)), index0 + 1, half);

        float sum = 0.0f;
        int count = 0;
        for (int i = index0; i < index1; ++i) {
            sum += magnitudes[i];
            ++count;
        }
        bins[bin] = count > 0 ? sum / count : 0.0f;
    }
    return bins;
}
