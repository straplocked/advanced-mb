/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "FFTProcessor.h"
#include <cmath>
#include <algorithm>

FFTProcessor::FFTProcessor() = default;

void FFTProcessor::prepare(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    ringBuffer.assign((size_t)fftSize, 0.0f);
    ringWritePos = 0;
    fftData.assign((size_t)fftSize * 2, 0.0f);
    magnitudeBuffer.resize((size_t)getNumBins());
}

void FFTProcessor::reset()
{
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
    ringWritePos = 0;
}

void FFTProcessor::pushSamples(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    int numChannels = buffer.getNumChannels();
    if (numChannels == 0) return;
    float invChannels = 1.0f / (float)numChannels;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getReadPointer(ch)[i];
        sample *= invChannels;
        ringBuffer[(size_t)ringWritePos] = sample;
        ++ringWritePos;
        if (ringWritePos >= fftSize)
        {
            performFFT();
            std::copy(ringBuffer.begin() + hopSize, ringBuffer.begin() + fftSize, ringBuffer.begin());
            ringWritePos = hopSize;
        }
    }
}

void FFTProcessor::performFFT()
{
    std::copy(ringBuffer.begin(), ringBuffer.begin() + fftSize, fftData.begin());
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable(fftData.data(), (size_t)fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data(), true);
    int numBins = getNumBins();
    float* dest = magnitudeBuffer.getWritePointer();
    float fftSizeF = (float)fftSize;
    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = fftData[(size_t)bin] / fftSizeF;
        if (bin > 0 && bin < numBins - 1) mag *= 2.0f;
        float dB = 20.0f * std::log10(std::max(mag, 1e-10f));
        dest[bin] = std::max(dB, -100.0f);
    }
    magnitudeBuffer.finishWrite();
}
