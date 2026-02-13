/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../analyzer/TripleBuffer.h"
#include "../utils/Constants.h"
#include <array>

struct MeterFrame
{
    float inputL = -100.0f;
    float inputR = -100.0f;
    float outputL = -100.0f;
    float outputR = -100.0f;
    std::array<float, Constants::kMaxBands> bandGR = {};
    float globalGR = 0.0f;
};

class LevelMeterData
{
public:
    LevelMeterData() = default;
    void prepare(double sampleRate);
    void reset();
    void pushInputLevel(const juce::AudioBuffer<float>& buffer);
    void pushOutputLevel(const juce::AudioBuffer<float>& buffer);
    void pushBandGainReduction(int bandIndex, float grDB);
    TripleBuffer<MeterFrame>& getBuffer() { return meterBuffer; }
private:
    void publishFrame();
    float computePeakDB(const float* data, int numSamples) const;
    TripleBuffer<MeterFrame> meterBuffer;
    MeterFrame currentFrame;
    double currentSampleRate = 44100.0;
};
