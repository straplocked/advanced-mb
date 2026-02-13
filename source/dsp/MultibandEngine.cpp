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
    numBands = std::clamp(params.numBands, 1, Constants::kMaxBands);

    globalOutputGain = MathUtils::dbToLinear(params.globalOutput);
    mixAmount = std::clamp(params.mix, 0.0f, 200.0f) * 0.01f; // 0.0 to 2.0

    // Update crossover bank
    crossoverBank.setNumBands(numBands);

    for (int i = 0; i < Constants::kMaxCrossovers; ++i)
    {
        crossoverBank.setCrossoverFrequency(i, params.crossovers[i].frequency);
        crossoverBank.setCrossoverSlope(i, params.crossovers[i].slope);
    }

    // Update band compressors
    for (int b = 0; b < Constants::kMaxBands; ++b)
        bandCompressors[b].updateParameters(params.bands[b]);

    // Update oversampler factor
    oversampler.setFactor(params.oversampleIndex);
}

void MultibandEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), currentChannels);

    // 1. Store dry copy for dry/wet mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 2. Split via crossover bank into band buffers
    crossoverBank.process(buffer, bandBuffers);

    // 3. Process each active band through its BandCompressorCore
    for (int b = 0; b < numBands; ++b)
        bandCompressors[b].process(bandBuffers[b]);

    // 4. Sum all band outputs back into the main buffer
    buffer.clear();
    for (int b = 0; b < numBands; ++b)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0,
                           bandBuffers[b].getReadPointer(ch),
                           numSamples);
        }
    }

    // 5. Apply dry/wet mix
    //    output = dry * (1 - mix) + wet * mix
    //    When mix = 1.0 (100%), fully wet
    //    When mix = 0.0 (0%), fully dry
    //    mix > 1.0 allows over-processing
    if (mixAmount < 1.999f) // Not fully wet (with margin)
    {
        float dryAmount = 1.0f - mixAmount;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Scale wet signal
            buffer.applyGain(ch, 0, numSamples, mixAmount);
            // Add dry signal
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
