/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <juce_dsp/juce_dsp.h>
#include "TripleBuffer.h"
#include "../utils/Constants.h"
#include <vector>

class FFTProcessor
{
public:
    FFTProcessor();
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void pushSamples(const juce::AudioBuffer<float>& buffer, int numSamples);
    TripleBuffer<float>& getMagnitudeBuffer() { return magnitudeBuffer; }
    const TripleBuffer<float>& getMagnitudeBuffer() const { return magnitudeBuffer; }
    int getNumBins() const { return fftSize / 2 + 1; }
    double getSampleRate() const { return currentSampleRate; }
private:
    void performFFT();
    static constexpr int fftOrder = Constants::kFFTOrder;
    static constexpr int fftSize = Constants::kFFTSize;
    static constexpr int hopSize = fftSize / 2;
    juce::dsp::FFT fft{fftOrder};
    juce::dsp::WindowingFunction<float> window{(size_t)fftSize, juce::dsp::WindowingFunction<float>::hann};
    std::vector<float> ringBuffer;
    int ringWritePos = 0;
    std::vector<float> fftData;
    TripleBuffer<float> magnitudeBuffer;
    double currentSampleRate = 44100.0;
};
