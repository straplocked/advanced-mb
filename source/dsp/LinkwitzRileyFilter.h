/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include "SVFFilter.h"
#include <vector>

class LinkwitzRileyFilter
{
public:
    LinkwitzRileyFilter() = default;

    void prepare(double sampleRate);
    void reset();
    void setFrequency(float freq);
    void setSlope(int slopeIndex); // 0=6, 1=12, 2=18, 3=24, 4=36, 5=48 dB/oct

    // Process and return both LP and HP outputs
    void processSample(float input, float& lpOut, float& hpOut);
    void process(const float* input, float* lpOutput, float* hpOutput, int numSamples);

    float getFrequency() const { return frequency; }
    int getSlopeIndex() const { return slopeIndex; }

private:
    void rebuildFilters();

    std::vector<SVFFilter> lpFilters;
    std::vector<SVFFilter> hpFilters;

    float frequency = 1000.0f;
    int slopeIndex = 3; // 24 dB/oct default
    double currentSampleRate = 44100.0;
};
