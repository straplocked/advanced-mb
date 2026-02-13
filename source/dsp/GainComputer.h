/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "../utils/CompressorMath.h"
#include <algorithm>
#include <cmath>

class GainComputer
{
public:
    GainComputer() = default;

    void setParameters(float newThreshold, float newRatio, float newKnee, float newRange)
    {
        threshold = newThreshold;
        ratio = newRatio;
        knee = newKnee;
        range = newRange;
    }

    void setExpandMode(bool enabled) { expandMode = enabled; }

    // Returns gain change in dB
    // Compress mode: negative (gain reduction)
    // Expand mode: negative below threshold (downward expansion/gating)
    float computeGainReduction(float inputDB) const
    {
        if (expandMode)
            return computeExpansion(inputDB);
        return CompressorMath::computeGainReduction(inputDB, threshold, ratio, knee, range);
    }

    float getThreshold() const { return threshold; }

private:
    float computeExpansion(float inputDB) const
    {
        // Downward expansion: reduce gain when signal is below threshold
        float halfKnee = knee * 0.5f;
        float expansionDB;

        if (inputDB >= threshold + halfKnee)
        {
            expansionDB = 0.0f; // Above threshold: no expansion
        }
        else if (inputDB <= threshold - halfKnee)
        {
            float deficit = threshold - inputDB;
            expansionDB = deficit * (ratio - 1.0f); // Expand by ratio
        }
        else
        {
            // Knee region
            float x = threshold + halfKnee - inputDB;
            expansionDB = (ratio - 1.0f) * x * x / (2.0f * knee);
        }

        expansionDB = std::min(expansionDB, std::abs(range));
        return -expansionDB; // Negative = gain reduction
    }

    float threshold = -20.0f;
    float ratio = 4.0f;
    float knee = 6.0f;
    float range = 30.0f;
    bool expandMode = false;
};
