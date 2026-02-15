/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "BandCompressorCore.h"
#include <cmath>
#include <algorithm>

void BandCompressorCore::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;

    levelDetectorL.prepare(sampleRate);
    levelDetectorR.prepare(sampleRate);
    envelopeL.prepare(sampleRate);
    envelopeR.prepare(sampleRate);

    reset();
}

void BandCompressorCore::reset()
{
    levelDetectorL.reset();
    levelDetectorR.reset();
    envelopeL.reset();
    envelopeR.reset();
    currentGainReductionDB = 0.0f;
}

void BandCompressorCore::updateParameters(const BandCompressorParameters& params)
{
    enabled = params.enabled;
    stereoLink = params.stereoLink;
    outputGainLinear = MathUtils::dbToLinear(params.outputGain);

    gainComputer.setParameters(params.threshold, params.ratio, params.knee, params.range);
    gainComputer.setExpandMode(params.expandMode);

    envelopeL.setAttackMs(params.attackMs);
    envelopeL.setReleaseMs(params.releaseMs);
    envelopeL.setBandFrequency(params.bandFrequency);

    envelopeR.setAttackMs(params.attackMs);
    envelopeR.setReleaseMs(params.releaseMs);
    envelopeR.setBandFrequency(params.bandFrequency);
}

float BandCompressorCore::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled)
    {
        currentGainReductionDB = 0.0f;
        return 0.0f;
    }

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const bool isStereo = numChannels >= 2;

    float peakGR = 0.0f;

    float* leftData = buffer.getWritePointer(0);
    float* rightData = isStereo ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Detect level (L and R separately) -- returns dB already
        float levelDBL = levelDetectorL.processSample(leftData[i]);
        float levelDBR = isStereo ? levelDetectorR.processSample(rightData[i]) : levelDBL;

        // 2. Apply stereo linking (blend L/R levels based on stereoLink %)
        float linkFactor = stereoLink * 0.01f; // 0.0 to 1.0
        float linkedDBL, linkedDBR;

        if (linkFactor >= 1.0f)
        {
            // Full link: use max of both channels
            float maxDB = std::max(levelDBL, levelDBR);
            linkedDBL = maxDB;
            linkedDBR = maxDB;
        }
        else if (linkFactor <= 0.0f)
        {
            // No link: independent channels
            linkedDBL = levelDBL;
            linkedDBR = levelDBR;
        }
        else
        {
            // Partial link: blend between independent and max
            float maxDB = std::max(levelDBL, levelDBR);
            linkedDBL = levelDBL + linkFactor * (maxDB - levelDBL);
            linkedDBR = levelDBR + linkFactor * (maxDB - levelDBR);
        }

        // 3. Compute gain reduction via GainComputer
        float grL = gainComputer.computeGainReduction(linkedDBL);
        float grR = gainComputer.computeGainReduction(linkedDBR);

        // 4. Smooth via EnvelopeFollower
        float smoothedGRL = envelopeL.processSample(grL);
        float smoothedGRR = isStereo ? envelopeR.processSample(grR) : smoothedGRL;

        // 5. Apply gain reduction to the buffer samples
        float gainL = MathUtils::dbToLinear(smoothedGRL);
        float gainR = MathUtils::dbToLinear(smoothedGRR);

        leftData[i] *= gainL;
        if (isStereo)
            rightData[i] *= gainR;

        // 6. Apply output gain
        leftData[i] *= outputGainLinear;
        if (isStereo)
            rightData[i] *= outputGainLinear;

        // 7. Track gain reduction for metering (use the more aggressive of the two)
        float sampleGR = isStereo ? std::min(smoothedGRL, smoothedGRR) : smoothedGRL;
        if (sampleGR < peakGR)
            peakGR = sampleGR;
    }

    currentGainReductionDB = peakGR;
    return currentGainReductionDB;
}
