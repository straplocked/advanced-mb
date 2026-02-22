/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "LevelDetector.h"
#include "GainComputer.h"
#include "EnvelopeFollower.h"
#include "../utils/MathUtils.h"
#include <juce_audio_basics/juce_audio_basics.h>

struct BandCompressorParameters
{
    float threshold = -20.0f;
    float ratio = 4.0f;
    float knee = 6.0f;
    float range = 30.0f;
    float attackMs = 5.0f;
    float releaseMs = 100.0f;
    float outputGain = 0.0f;
    float stereoLink = 100.0f;
    float lookaheadMs = 0.0f;
    float bandFrequency = 1000.0f; // center freq for envelope scaling
    bool expandMode = false;
    bool enabled = true;
    bool autoGain = false;
};

class BandCompressorCore
{
public:
    BandCompressorCore() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void updateParameters(const BandCompressorParameters& params);

    // Process a band buffer in-place, returns current GR in dB
    float process(juce::AudioBuffer<float>& buffer);

    float getCurrentGainReduction() const { return currentGainReductionDB; }

private:
    LevelDetector levelDetectorL, levelDetectorR;
    GainComputer gainComputer;
    EnvelopeFollower envelopeL, envelopeR;

    float stereoLink = 100.0f;
    float outputGainLinear = 1.0f;
    float currentGainReductionDB = 0.0f;
    bool enabled = true;

    double currentSampleRate = 44100.0;
};
