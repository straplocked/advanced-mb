/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "LevelMeterData.h"
#include <cmath>
#include <algorithm>

void LevelMeterData::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    meterBuffer.resize(1);
    reset();
}

void LevelMeterData::reset()
{
    currentFrame = {};
}

void LevelMeterData::pushInputLevel(const juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    currentFrame.inputL = computePeakDB(buffer.getReadPointer(0), numSamples);
    if (buffer.getNumChannels() >= 2)
        currentFrame.inputR = computePeakDB(buffer.getReadPointer(1), numSamples);
    else
        currentFrame.inputR = currentFrame.inputL;
}

void LevelMeterData::pushOutputLevel(const juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    currentFrame.outputL = computePeakDB(buffer.getReadPointer(0), numSamples);
    if (buffer.getNumChannels() >= 2)
        currentFrame.outputR = computePeakDB(buffer.getReadPointer(1), numSamples);
    else
        currentFrame.outputR = currentFrame.outputL;
    publishFrame();
}

void LevelMeterData::pushBandGainReduction(int bandIndex, float grDB)
{
    if (bandIndex >= 0 && bandIndex < Constants::kMaxBands)
    {
        currentFrame.bandGR[(size_t)bandIndex] = grDB;
        // Update global GR as the maximum across bands
        float maxGR = 0.0f;
        for (auto gr : currentFrame.bandGR)
            maxGR = std::min(maxGR, gr);
        currentFrame.globalGR = maxGR;
    }
}

void LevelMeterData::publishFrame()
{
    auto* dest = meterBuffer.getWritePointer();
    dest[0] = currentFrame;
    meterBuffer.finishWrite();
}

float LevelMeterData::computePeakDB(const float* data, int numSamples) const
{
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = std::max(peak, std::abs(data[i]));
    if (peak < 1e-10f)
        return -100.0f;
    return 20.0f * std::log10(peak);
}
