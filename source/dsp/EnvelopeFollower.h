/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "../utils/CompressorMath.h"
#include <cmath>
#include <algorithm>

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    void prepare(double sampleRate);
    void reset();

    // Set attack/release as percentage (0-100%)
    void setAttackPercent(float pct);
    void setReleasePercent(float pct);

    // Set the band's center frequency for frequency-dependent timing
    void setBandFrequency(float freq);

    float processSample(float targetGR);

private:
    void updateCoefficients();

    // Convert percentage to ms based on band frequency
    float percentToAttackMs(float pct) const;
    float percentToReleaseMs(float pct) const;

    double currentSampleRate = 44100.0;
    float attackPct = 50.0f;
    float releasePct = 50.0f;
    float bandFrequency = 1000.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentGR = 0.0f;
};
