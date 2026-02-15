/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "EnvelopeFollower.h"
#include <cmath>
#include <algorithm>

void EnvelopeFollower::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updateCoefficients();
}

void EnvelopeFollower::reset()
{
    currentGR = 0.0f;
}

void EnvelopeFollower::setAttackMs(float ms)
{
    attackMs = std::max(ms, 0.001f);
    updateCoefficients();
}

void EnvelopeFollower::setReleaseMs(float ms)
{
    releaseMs = std::max(ms, 0.01f);
    updateCoefficients();
}

void EnvelopeFollower::setBandFrequency(float freq)
{
    bandFrequency = std::max(freq, 20.0f);
    updateCoefficients();
}

void EnvelopeFollower::updateCoefficients()
{
    if (currentSampleRate <= 0.0)
        return;

    // Apply frequency scaling: lower frequencies get longer times
    float freqScale = std::sqrt(1000.0f / bandFrequency);

    float scaledAttackMs  = attackMs  * freqScale;
    float scaledReleaseMs = releaseMs * freqScale;

    // One-pole smoothing coefficient: coeff = exp(-1 / (timeInSeconds * sampleRate))
    float attackTimeSec  = std::max(scaledAttackMs  * 0.001f, 1.0e-6f);
    float releaseTimeSec = std::max(scaledReleaseMs * 0.001f, 1.0e-6f);

    attackCoeff  = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * attackTimeSec));
    releaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * releaseTimeSec));
}

float EnvelopeFollower::processSample(float targetGR)
{
    // Simple one-pole attack/release envelope follower
    // targetGR is negative (gain reduction in dB)
    // "Attack" = moving towards more gain reduction (more negative)
    // "Release" = moving towards less gain reduction (less negative / towards 0)

    float coeff;

    if (targetGR < currentGR)
        coeff = attackCoeff;   // Signal needs more gain reduction -> attack
    else
        coeff = releaseCoeff;  // Signal needs less gain reduction -> release

    currentGR = coeff * currentGR + (1.0f - coeff) * targetGR;
    return currentGR;
}
