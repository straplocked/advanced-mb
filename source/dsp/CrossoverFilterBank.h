/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "LinkwitzRileyFilter.h"
#include "../utils/Constants.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

class CrossoverFilterBank
{
public:
    CrossoverFilterBank() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setNumBands(int n);
    void setCrossoverFrequency(int index, float hz);
    void setCrossoverSlope(int index, int slopeIndex);

    // Split input into band outputs
    void process(const juce::AudioBuffer<float>& input,
                 std::array<juce::AudioBuffer<float>, Constants::kMaxBands>& bandOutputs);

    int getNumBands() const { return numBands; }
    float getCrossoverFrequency(int index) const;

private:
    int numBands = 1;
    int maxBlock = 512;
    int channels = 2;
    double sampleRate = 44100.0;

    // Per-channel crossover filters
    std::array<std::array<LinkwitzRileyFilter, Constants::kMaxCrossovers>, 2> crossovers;

    // Temp buffers for cascaded splitting
    std::array<juce::AudioBuffer<float>, Constants::kMaxBands> tempBuffers;
};
