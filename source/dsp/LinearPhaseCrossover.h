/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "../utils/Constants.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

class LinearPhaseCrossover
{
public:
    LinearPhaseCrossover() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setNumBands(int n);
    void setCrossoverFrequency(int index, float hz);

    void process(const juce::AudioBuffer<float>& input,
                 std::array<juce::AudioBuffer<float>, Constants::kMaxRegions>& bandOutputs);

    int getLatencySamples() const { return kernelSize / 2; }

private:
    void rebuildKernels();

    int numBands = 1;
    int kernelSize = 2048;
    double sampleRate = 44100.0;
    int channels = 2;

    std::array<float, Constants::kMaxRegionCrossovers> crossoverFreqs;

    // FIR convolution engines per band per channel
    std::array<std::array<juce::dsp::FIR::Filter<float>, 2>, Constants::kMaxRegions> firFilters;
    std::array<std::vector<float>, Constants::kMaxRegions> kernels;
};
