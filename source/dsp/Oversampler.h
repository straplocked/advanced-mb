/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <memory>

// Oversampler supporting 1x, 2x, and 4x oversampling factors.
// Uses JUCE's polyphase IIR half-band oversampling for high quality anti-aliasing.

class Oversampler
{
public:
    Oversampler() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setFactor (int factorIndex);  // 0=1x, 1=2x, 2=4x

    juce::dsp::AudioBlock<float> upsample (juce::AudioBuffer<float>& buffer);
    void downsample (juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const;
    double getUpsampledRate() const { return upsampledRate; }
    int getFactor() const { return factor; }

private:
    void createOversampler (int maxBlockSize, int numChannels);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int factor = 1;
    int factorIndex = 0;
    double baseSampleRate = 44100.0;
    double upsampledRate = 44100.0;
    int maxBlock = 512;
    int channels = 2;
};
