/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "MultibandEngine.h"
#include "../utils/MathUtils.h"
#include <algorithm>
#include <cmath>

void MultibandEngine::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxBlockSize;
    currentChannels = std::min(numChannels, 2);

    // Prepare crossover filter bank
    crossoverBank.prepare(sampleRate, maxBlockSize, currentChannels);

    // Prepare all band compressors
    for (auto& comp : bandCompressors)
        comp.prepare(sampleRate, maxBlockSize);

    // Prepare oversampler
    oversampler.prepare(sampleRate, maxBlockSize, currentChannels);

    // Allocate region buffers
    for (auto& buf : regionBuffers)
        buf.setSize(currentChannels, maxBlockSize);

    // Allocate dry buffer
    dryBuffer.setSize(currentChannels, maxBlockSize);

    reset();
}

void MultibandEngine::reset()
{
    crossoverBank.reset();

    for (auto& comp : bandCompressors)
        comp.reset();

    oversampler.reset();

    for (auto& buf : regionBuffers)
        buf.clear();

    dryBuffer.clear();
}

void MultibandEngine::updateParameters(const MultibandParameters& params)
{
    numRegions = std::clamp(params.numRegions, 0, Constants::kMaxRegions);

    globalOutputGain = MathUtils::dbToLinear(params.globalOutput);
    mixAmount = std::clamp(params.mix, 0.0f, 200.0f) * 0.01f;

    // Store region slot mapping
    for (int i = 0; i < Constants::kMaxRegions; ++i)
        currentRegionSlotMap[i] = params.regionSlotMap[i];

    // Update crossover bank with region crossover frequencies
    crossoverBank.setNumBands(std::max(numRegions, 1));

    int numRC = numRegions - 1;
    for (int i = 0; i < numRC; ++i)
    {
        crossoverBank.setCrossoverFrequency(i, params.regionCrossoverFreqs[i]);
        crossoverBank.setCrossoverSlope(i, params.regionCrossoverSlope);
    }

    // Update band compressors — only those with active user bands
    for (int r = 0; r < numRegions; ++r)
    {
        int slot = currentRegionSlotMap[r];
        if (slot >= 0 && slot < Constants::kMaxBands)
            bandCompressors[slot].updateParameters(params.bands[slot]);
    }

    oversampler.setFactor(params.oversampleIndex);
}

void MultibandEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), currentChannels);

    // 0 regions = pass through (no processing)
    if (numRegions <= 0)
        return;

    // 1 region that is a gap = pass through
    if (numRegions == 1 && currentRegionSlotMap[0] < 0)
        return;

    // 1. Store dry copy for dry/wet mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 2. Split via crossover bank into region buffers
    crossoverBank.process(buffer, regionBuffers);

    // 3. Process each region: compress user bands, leave gaps untouched
    for (int r = 0; r < numRegions; ++r)
    {
        int slot = currentRegionSlotMap[r];
        if (slot >= 0 && slot < Constants::kMaxBands)
            bandCompressors[slot].process(regionBuffers[r]);
        // gaps (slot < 0) pass through unprocessed
    }

    // 4. Sum all region outputs back into the main buffer
    buffer.clear();
    for (int r = 0; r < numRegions; ++r)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0,
                           regionBuffers[r].getReadPointer(ch),
                           numSamples);
        }
    }

    // 5. Apply dry/wet mix
    if (mixAmount < 1.999f)
    {
        float dryAmount = 1.0f - mixAmount;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.applyGain(ch, 0, numSamples, mixAmount);
            buffer.addFrom(ch, 0, dryBuffer.getReadPointer(ch), numSamples, dryAmount);
        }
    }

    // 6. Apply global output gain
    if (std::abs(globalOutputGain - 1.0f) > 1.0e-6f)
        buffer.applyGain(0, numSamples, globalOutputGain);
}

int MultibandEngine::getLatencySamples() const
{
    return oversampler.getLatencySamples();
}

float MultibandEngine::getBandGainReduction(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= Constants::kMaxBands)
        return 0.0f;

    return bandCompressors[bandIndex].getCurrentGainReduction();
}
