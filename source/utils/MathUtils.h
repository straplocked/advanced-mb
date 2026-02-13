/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cmath>
#include "Constants.h"

namespace MathUtils
{
    inline float dbToGain (float dB) { return std::pow (10.0f, dB / 20.0f); }
    inline float gainToDb (float gain) { return 20.0f * std::log10 (std::max (gain, 1e-10f)); }

    // Aliases used throughout the multiband engine
    inline float dbToLinear (float dB) { return dbToGain (dB); }
    inline float linearToDb (float linear) { return gainToDb (linear); }

    inline float frequencyToNormalized (float freq)
    {
        return std::log (freq / Constants::kMinFrequency)
             / std::log (Constants::kMaxFrequency / Constants::kMinFrequency);
    }

    inline float normalizedToFrequency (float normalized)
    {
        return Constants::kMinFrequency
             * std::pow (Constants::kMaxFrequency / Constants::kMinFrequency, normalized);
    }

    inline float frequencyToX (float freq, float width)
    {
        return frequencyToNormalized (freq) * width;
    }

    inline float xToFrequency (float x, float width)
    {
        return normalizedToFrequency (x / width);
    }

    inline float gainToY (float gainDB, float height, float minDB, float maxDB)
    {
        return height * (1.0f - (gainDB - minDB) / (maxDB - minDB));
    }

    inline float yToGain (float y, float height, float minDB, float maxDB)
    {
        return minDB + (1.0f - y / height) * (maxDB - minDB);
    }
}
