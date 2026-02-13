/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "LinkwitzRileyFilter.h"
#include <cmath>

void LinkwitzRileyFilter::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    rebuildFilters();
}

void LinkwitzRileyFilter::reset()
{
    for (auto& f : lpFilters)
        f.reset();
    for (auto& f : hpFilters)
        f.reset();
}

void LinkwitzRileyFilter::setFrequency (float freq)
{
    if (frequency != freq)
    {
        frequency = freq;
        rebuildFilters();
    }
}

void LinkwitzRileyFilter::setSlope (int newSlopeIndex)
{
    if (slopeIndex != newSlopeIndex)
    {
        slopeIndex = newSlopeIndex;
        rebuildFilters();
    }
}

void LinkwitzRileyFilter::rebuildFilters()
{
    lpFilters.clear();
    hpFilters.clear();

    // Linkwitz-Riley crossover filter configurations per slope index:
    //   0 = 6 dB/oct:  1 first-order LP + 1 first-order HP
    //   1 = 12 dB/oct: 1 second-order LP (Q=0.5) + 1 second-order HP (Q=0.5)
    //   2 = 18 dB/oct: 1 first-order + 1 second-order for each
    //   3 = 24 dB/oct: 2 cascaded second-order Butterworth (Q=0.7071) for each
    //   4 = 36 dB/oct: 3 cascaded second-order for each
    //   5 = 48 dB/oct: 4 cascaded second-order for each

    const double freq = static_cast<double> (frequency);

    switch (slopeIndex)
    {
        case 0: // 6 dB/oct -- single first-order stage
        {
            lpFilters.resize (1);
            hpFilters.resize (1);

            lpFilters[0].setFirstOrder (true);
            lpFilters[0].setParameters (FilterType::LowPass, freq, 0.5, 0.0);
            lpFilters[0].prepare (currentSampleRate);

            hpFilters[0].setFirstOrder (true);
            hpFilters[0].setParameters (FilterType::HighPass, freq, 0.5, 0.0);
            hpFilters[0].prepare (currentSampleRate);
            break;
        }

        case 1: // 12 dB/oct -- single second-order, Q=0.5 (LR2)
        {
            lpFilters.resize (1);
            hpFilters.resize (1);

            lpFilters[0].setParameters (FilterType::LowPass, freq, 0.5, 0.0);
            lpFilters[0].prepare (currentSampleRate);

            hpFilters[0].setParameters (FilterType::HighPass, freq, 0.5, 0.0);
            hpFilters[0].prepare (currentSampleRate);
            break;
        }

        case 2: // 18 dB/oct -- 1 first-order + 1 second-order for each (LR3)
        {
            lpFilters.resize (2);
            hpFilters.resize (2);

            // First-order stage
            lpFilters[0].setFirstOrder (true);
            lpFilters[0].setParameters (FilterType::LowPass, freq, 0.5, 0.0);
            lpFilters[0].prepare (currentSampleRate);

            // Second-order Butterworth stage
            lpFilters[1].setParameters (FilterType::LowPass, freq, 0.7071, 0.0);
            lpFilters[1].prepare (currentSampleRate);

            hpFilters[0].setFirstOrder (true);
            hpFilters[0].setParameters (FilterType::HighPass, freq, 0.5, 0.0);
            hpFilters[0].prepare (currentSampleRate);

            hpFilters[1].setParameters (FilterType::HighPass, freq, 0.7071, 0.0);
            hpFilters[1].prepare (currentSampleRate);
            break;
        }

        case 3: // 24 dB/oct -- 2 cascaded second-order Butterworth (LR4)
        {
            lpFilters.resize (2);
            hpFilters.resize (2);

            for (int i = 0; i < 2; ++i)
            {
                lpFilters[i].setParameters (FilterType::LowPass, freq, 0.7071, 0.0);
                lpFilters[i].prepare (currentSampleRate);

                hpFilters[i].setParameters (FilterType::HighPass, freq, 0.7071, 0.0);
                hpFilters[i].prepare (currentSampleRate);
            }
            break;
        }

        case 4: // 36 dB/oct -- 3 cascaded second-order stages
        {
            lpFilters.resize (3);
            hpFilters.resize (3);

            for (int i = 0; i < 3; ++i)
            {
                lpFilters[i].setParameters (FilterType::LowPass, freq, 0.7071, 0.0);
                lpFilters[i].prepare (currentSampleRate);

                hpFilters[i].setParameters (FilterType::HighPass, freq, 0.7071, 0.0);
                hpFilters[i].prepare (currentSampleRate);
            }
            break;
        }

        case 5: // 48 dB/oct -- 4 cascaded second-order stages
        {
            lpFilters.resize (4);
            hpFilters.resize (4);

            for (int i = 0; i < 4; ++i)
            {
                lpFilters[i].setParameters (FilterType::LowPass, freq, 0.7071, 0.0);
                lpFilters[i].prepare (currentSampleRate);

                hpFilters[i].setParameters (FilterType::HighPass, freq, 0.7071, 0.0);
                hpFilters[i].prepare (currentSampleRate);
            }
            break;
        }

        default:
            // Fallback to 24 dB/oct
            slopeIndex = 3;
            rebuildFilters();
            break;
    }
}

void LinkwitzRileyFilter::processSample (float input, float& lpOut, float& hpOut)
{
    // Cascade through LP filter stages
    float lpSignal = input;
    for (auto& f : lpFilters)
        lpSignal = f.processSample (lpSignal);

    // Cascade through HP filter stages
    float hpSignal = input;
    for (auto& f : hpFilters)
        hpSignal = f.processSample (hpSignal);

    lpOut = lpSignal;
    hpOut = hpSignal;
}

void LinkwitzRileyFilter::process (const float* input, float* lpOutput, float* hpOutput, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        processSample (input[i], lpOutput[i], hpOutput[i]);
}
