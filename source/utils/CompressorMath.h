/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cmath>
#include <algorithm>

namespace CompressorMath
{
    // Convert attack/release time in ms to a one-pole coefficient
    // coeff = exp(-1 / (time_seconds * sampleRate))
    inline float timeToCoeff (float timeMs, double sampleRate)
    {
        if (timeMs <= 0.0f)
            return 0.0f;
        return std::exp (-1.0f / (float) (timeMs * 0.001 * sampleRate));
    }

    // Convert ms to samples
    inline int msToSamples (float ms, double sampleRate)
    {
        return (int) std::round (ms * 0.001 * sampleRate);
    }

    // Soft-knee gain computation (downward compression)
    // Returns gain reduction in dB (negative value) for a given input level (dB)
    inline float computeGainReduction (float inputDB, float threshold, float ratio,
                                       float kneeWidth, float range)
    {
        float halfKnee = kneeWidth * 0.5f;

        float gainReductionDB;

        if (inputDB <= threshold - halfKnee)
        {
            // Below knee: no compression
            gainReductionDB = 0.0f;
        }
        else if (inputDB >= threshold + halfKnee)
        {
            // Above knee: full compression
            float excess = inputDB - threshold;
            gainReductionDB = excess * (1.0f - 1.0f / ratio);
        }
        else
        {
            // In knee: quadratic transition
            float x = inputDB - threshold + halfKnee;
            gainReductionDB = (1.0f - 1.0f / ratio) * x * x / (2.0f * kneeWidth);
        }

        // Apply range limit
        gainReductionDB = std::min (gainReductionDB, range);

        return -gainReductionDB;
    }

    // Upward compression: boosts signals below threshold
    // Returns gain boost in dB (positive value) for a given input level (dB)
    inline float computeUpwardGain (float inputDB, float threshold, float ratio,
                                     float kneeWidth, float range)
    {
        float halfKnee = kneeWidth * 0.5f;

        float boostDB;

        if (inputDB >= threshold + halfKnee)
        {
            // Above knee: no upward compression
            boostDB = 0.0f;
        }
        else if (inputDB <= threshold - halfKnee)
        {
            // Below knee: full upward compression
            float deficit = threshold - inputDB;
            boostDB = deficit * (1.0f - 1.0f / ratio);
        }
        else
        {
            // In knee: quadratic transition
            float x = threshold + halfKnee - inputDB;
            boostDB = (1.0f - 1.0f / ratio) * x * x / (2.0f * kneeWidth);
        }

        // Apply range limit (max boost)
        boostDB = std::min (boostDB, range);

        return boostDB;
    }
}
