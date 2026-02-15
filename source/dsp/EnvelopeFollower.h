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

    // Set attack/release in milliseconds
    void setAttackMs(float ms);
    void setReleaseMs(float ms);

    // Set the band's center frequency for frequency-dependent timing
    void setBandFrequency(float freq);

    float processSample(float targetGR);

private:
    void updateCoefficients();

    double currentSampleRate = 44100.0;
    float attackMs = 5.0f;
    float releaseMs = 100.0f;
    float bandFrequency = 1000.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentGR = 0.0f;
};
