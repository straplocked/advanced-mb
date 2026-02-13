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

    // Allocate band buffers
    for (auto& buf : bandBuffers)
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

    for (auto& buf : bandBuffers)
        buf.clear();

    dryBuffer.clear();
}

void MultibandEngine::updateParameters(const MultibandParameters& params)
{
    numBands = std::clamp(params.numBands, 0, Constants::kMaxBands);

    globalOutputGain = MathUtils::dbToLinear(params.globalOutput);
    mixAmount = std::clamp(params.mix, 0.0f, 200.0f) * 0.01f;

    // Store slot mapping
    for (int i = 0; i < Constants::kMaxBands; ++i)
        currentSlotOrder[i] = params.slotOrder[i];

    // Update crossover bank (min 1 for the bank's internal logic)
    crossoverBank.setNumBands(std::max(numBands, 1));

    for (int i = 0; i < Constants::kMaxCrossovers; ++i)
    {
        crossoverBank.setCrossoverFrequency(i, params.crossovers[i].frequency);
        crossoverBank.setCrossoverSlope(i, params.crossovers[i].slope);
    }

    // Update band compressors using slot mapping
    for (int pos = 0; pos < numBands; ++pos)
    {
        int slot = currentSlotOrder[pos];
        bandCompressors[slot].updateParameters(params.bands[slot]);
    }

    oversampler.setFactor(params.oversampleIndex);
}

void MultibandEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), currentChannels);

    // 0 bands = pass through (no processing)
    if (numBands <= 0)
        return;

    // 1. Store dry copy for dry/wet mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 2. Split via crossover bank into band buffers
    crossoverBank.process(buffer, bandBuffers);

    // 3. Process each active band using slot mapping
    for (int pos = 0; pos < numBands; ++pos)
    {
        int slot = currentSlotOrder[pos];
        bandCompressors[slot].process(bandBuffers[pos]);
    }

    // 4. Sum all band outputs back into the main buffer
    buffer.clear();
    for (int pos = 0; pos < numBands; ++pos)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0,
                           bandBuffers[pos].getReadPointer(ch),
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
