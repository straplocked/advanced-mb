/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "CrossoverFilterBank.h"
#include "BandCompressorCore.h"
#include "Oversampler.h"
#include "MidSideProcessor.h"
#include "../utils/Constants.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

struct MultibandParameters
{
    int numBands = 0;
    int processMode = 0; // 0=dynamic, 1=linear, 2=minimum
    int oversampleIndex = 0;
    int analyzerMode = 0;
    float globalOutput = 0.0f;
    float mix = 100.0f;
    std::array<int, Constants::kMaxBands> slotOrder = {0, 1, 2, 3, 4, 5};

    struct CrossoverParams {
        float frequency = 1000.0f;
        int slope = 3; // 24 dB/oct
    };
    std::array<CrossoverParams, Constants::kMaxCrossovers> crossovers;

    std::array<BandCompressorParameters, Constants::kMaxBands> bands;
};

class MultibandEngine
{
public:
    MultibandEngine() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void updateParameters(const MultibandParameters& params);
    void process(juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const;

    // Per-band gain reduction for metering
    float getBandGainReduction(int bandIndex) const;

private:
    CrossoverFilterBank crossoverBank;
    std::array<BandCompressorCore, Constants::kMaxBands> bandCompressors;
    Oversampler oversampler;

    std::array<juce::AudioBuffer<float>, Constants::kMaxBands> bandBuffers;
    juce::AudioBuffer<float> dryBuffer;

    int numBands = 0;
    std::array<int, Constants::kMaxBands> currentSlotOrder = {0, 1, 2, 3, 4, 5};
    float globalOutputGain = 1.0f;
    float mixAmount = 1.0f;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    int currentChannels = 2;
};
