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

void EnvelopeFollower::setAttackPercent(float pct)
{
    attackPct = std::clamp(pct, 0.0f, 100.0f);
    updateCoefficients();
}

void EnvelopeFollower::setReleasePercent(float pct)
{
    releasePct = std::clamp(pct, 0.0f, 100.0f);
    updateCoefficients();
}

void EnvelopeFollower::setBandFrequency(float freq)
{
    bandFrequency = std::max(freq, 20.0f);
    updateCoefficients();
}

float EnvelopeFollower::percentToAttackMs(float pct) const
{
    // Exponential mapping: 0% -> 0.1ms, 100% -> 200ms
    // Using formula: ms = minMs * (maxMs/minMs)^(pct/100)
    constexpr float minMs = 0.1f;
    constexpr float maxMs = 200.0f;

    float ms = minMs * std::pow(maxMs / minMs, pct / 100.0f);

    // Frequency scaling: lower frequencies get longer attack times
    // Multiply by sqrt(1000 / bandFreq) so low bands are slower
    float freqScale = std::sqrt(1000.0f / bandFrequency);
    return ms * freqScale;
}

float EnvelopeFollower::percentToReleaseMs(float pct) const
{
    // Exponential mapping: 0% -> 5ms, 100% -> 2000ms
    constexpr float minMs = 5.0f;
    constexpr float maxMs = 2000.0f;

    float ms = minMs * std::pow(maxMs / minMs, pct / 100.0f);

    // Same frequency scaling as attack
    float freqScale = std::sqrt(1000.0f / bandFrequency);
    return ms * freqScale;
}

void EnvelopeFollower::updateCoefficients()
{
    if (currentSampleRate <= 0.0)
        return;

    float attackMs = percentToAttackMs(attackPct);
    float releaseMs = percentToReleaseMs(releasePct);

    // One-pole smoothing coefficient: coeff = exp(-1 / (timeInSeconds * sampleRate))
    // When timeMs is very small, clamp to avoid instability
    float attackTimeSec = std::max(attackMs * 0.001f, 1.0e-6f);
    float releaseTimeSec = std::max(releaseMs * 0.001f, 1.0e-6f);

    attackCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * attackTimeSec));
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
