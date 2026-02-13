/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "CrossoverFilterBank.h"
#include <algorithm>

void CrossoverFilterBank::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;
    maxBlock = maxBlockSize;
    channels = std::min(numChannels, 2);

    // Prepare all crossover filters for both channels
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < Constants::kMaxCrossovers; ++i)
            crossovers[ch][i].prepare(sampleRate);

    // Allocate temp buffers
    for (auto& buf : tempBuffers)
        buf.setSize(channels, maxBlock);

    reset();
}

void CrossoverFilterBank::reset()
{
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < Constants::kMaxCrossovers; ++i)
            crossovers[ch][i].reset();

    for (auto& buf : tempBuffers)
        buf.clear();
}

void CrossoverFilterBank::setNumBands(int n)
{
    numBands = std::clamp(n, 1, Constants::kMaxBands);
}

void CrossoverFilterBank::setCrossoverFrequency(int index, float hz)
{
    if (index < 0 || index >= Constants::kMaxCrossovers)
        return;

    for (int ch = 0; ch < 2; ++ch)
        crossovers[ch][index].setFrequency(hz);
}

void CrossoverFilterBank::setCrossoverSlope(int index, int slopeIndex)
{
    if (index < 0 || index >= Constants::kMaxCrossovers)
        return;

    for (int ch = 0; ch < 2; ++ch)
        crossovers[ch][index].setSlope(slopeIndex);
}

float CrossoverFilterBank::getCrossoverFrequency(int index) const
{
    if (index < 0 || index >= Constants::kMaxCrossovers)
        return 1000.0f;

    return crossovers[0][index].getFrequency();
}

void CrossoverFilterBank::process(const juce::AudioBuffer<float>& input,
                                  std::array<juce::AudioBuffer<float>, Constants::kMaxBands>& bandOutputs)
{
    const int numSamples = input.getNumSamples();
    const int numChannels = std::min(input.getNumChannels(), channels);

    // Ensure band output buffers are sized correctly
    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        if (bandOutputs[b].getNumChannels() < numChannels ||
            bandOutputs[b].getNumSamples() < numSamples)
        {
            bandOutputs[b].setSize(numChannels, numSamples, false, false, true);
        }
        bandOutputs[b].clear();
    }

    // If only 1 band, pass through directly
    if (numBands <= 1)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            bandOutputs[0].copyFrom(ch, 0, input, ch, 0, numSamples);
        return;
    }

    // Cascaded crossover splitting:
    // For N bands we have N-1 crossover points.
    // Signal flow:
    //   input --> crossover[0] --> LP = band[0], HP = remainder
    //   remainder --> crossover[1] --> LP = band[1], HP = remainder
    //   ...
    //   remainder --> crossover[N-2] --> LP = band[N-2], HP = band[N-1]

    const int numCrossovers = numBands - 1;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        // Start with the input signal as the "remainder" to split
        // We use tempBuffers[0] as the current remainder
        tempBuffers[0].copyFrom(0, 0, input, ch, 0, numSamples);

        for (int xover = 0; xover < numCrossovers; ++xover)
        {
            const float* remainder = tempBuffers[0].getReadPointer(0);
            float* lpOut = bandOutputs[xover].getWritePointer(ch);

            // For intermediate crossovers, write HP into tempBuffers[1]
            // For the last crossover, write HP directly into the last band
            float* hpOut;
            bool isLast = (xover == numCrossovers - 1);

            if (isLast)
                hpOut = bandOutputs[xover + 1].getWritePointer(ch);
            else
                hpOut = tempBuffers[1].getWritePointer(0);

            // Process each sample through this crossover for this channel
            for (int i = 0; i < numSamples; ++i)
                crossovers[ch][xover].processSample(remainder[i], lpOut[i], hpOut[i]);

            // The HP output becomes the new remainder for the next crossover
            if (!isLast)
            {
                // Copy hpOut (tempBuffers[1]) to tempBuffers[0] for next iteration
                std::copy(tempBuffers[1].getReadPointer(0),
                          tempBuffers[1].getReadPointer(0) + numSamples,
                          tempBuffers[0].getWritePointer(0));
            }
        }
    }
}
