/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "LevelDetector.h"
#include "../utils/CompressorMath.h"

void LevelDetector::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
    setRMSWindowMs (rmsWindowMs);
}

void LevelDetector::reset()
{
    peakLevel = 0.0f;
    rmsSquared = 0.0f;
}

void LevelDetector::setMode (bool usePeak, bool useRMS)
{
    peak = usePeak;
    rms = useRMS;
}

void LevelDetector::setRMSWindowMs (float ms)
{
    rmsWindowMs = ms;
    rmsCoeff = CompressorMath::timeToCoeff (ms, currentSampleRate);
}

float LevelDetector::processSample (float input)
{
    float absSample = std::abs (input);

    float level = 0.0f;

    if (peak && !rms)
    {
        // Pure peak detection
        peakLevel = std::max (absSample, peakLevel * 0.9999f);
        level = peakLevel;
    }
    else if (rms && !peak)
    {
        // Pure RMS detection
        float sq = input * input;
        rmsSquared = rmsCoeff * rmsSquared + (1.0f - rmsCoeff) * sq;
        level = std::sqrt (rmsSquared);
    }
    else
    {
        // Combined peak+RMS (take max)
        peakLevel = std::max (absSample, peakLevel * 0.9999f);
        float sq = input * input;
        rmsSquared = rmsCoeff * rmsSquared + (1.0f - rmsCoeff) * sq;
        float rmsLevel = std::sqrt (rmsSquared);
        level = std::max (peakLevel, rmsLevel);
    }

    return MathUtils::gainToDb (level);
}
