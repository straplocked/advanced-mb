/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <juce_core/juce_core.h>
#include "../parameters/ParameterIDs.h"
#include "../utils/Constants.h"
#include <vector>
#include <map>

/**
 * @brief Static database of factory presets for the multiband compressor.
 *
 * Each preset is a name, category, and a map of parameter-ID to value.
 * Only values that differ from the plugin defaults need to be specified;
 * PresetManager applies defaults first, then overwrites with these values.
 */
namespace FactoryPresets
{

struct FactoryPreset
{
    juce::String name;
    juce::String category;
    std::map<juce::String, float> values;
};

//==============================================================================
// Helper aliases to keep preset definitions concise.
//==============================================================================

namespace PID = ParameterIDs;

inline juce::String bp ( int band, const juce::String& suffix )
{
    return PID::bandParam ( band, suffix );
}

inline juce::String xp ( int xover, const juce::String& suffix )
{
    return PID::crossoverParam ( xover, suffix );
}

//==============================================================================
inline const std::vector<FactoryPreset>& getAll()
{
    // clang-format off
    static const std::vector<FactoryPreset> presets =
    {
        //==================================================================
        // Mastering
        //==================================================================
        {
            "Gentle Master", "Mastering",
            {
                { PID::Global::numBands,            3.0f },
                // Crossovers
                { xp ( 0, PID::Crossover::frequency ), 200.0f },
                { xp ( 1, PID::Crossover::frequency ), 3000.0f },
                // Band 0 - Low
                { bp ( 0, PID::Band::threshold ),   -18.0f },
                { bp ( 0, PID::Band::ratio ),       2.0f },
                { bp ( 0, PID::Band::attack ),      60.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                { bp ( 0, PID::Band::knee ),        12.0f },
                // Band 1 - Mid
                { bp ( 1, PID::Band::threshold ),   -16.0f },
                { bp ( 1, PID::Band::ratio ),       1.8f },
                { bp ( 1, PID::Band::attack ),      45.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                { bp ( 1, PID::Band::knee ),        10.0f },
                // Band 2 - High
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       1.5f },
                { bp ( 2, PID::Band::attack ),      30.0f },
                { bp ( 2, PID::Band::release ),     40.0f },
                { bp ( 2, PID::Band::knee ),        8.0f },
            }
        },

        {
            "Loud Master", "Mastering",
            {
                { PID::Global::numBands,            4.0f },
                { xp ( 0, PID::Crossover::frequency ), 120.0f },
                { xp ( 1, PID::Crossover::frequency ), 1200.0f },
                { xp ( 2, PID::Crossover::frequency ), 6000.0f },
                // Band 0
                { bp ( 0, PID::Band::threshold ),   -22.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      70.0f },
                { bp ( 0, PID::Band::release ),     50.0f },
                { bp ( 0, PID::Band::outputGain ),  2.0f },
                // Band 1
                { bp ( 1, PID::Band::threshold ),   -20.0f },
                { bp ( 1, PID::Band::ratio ),       3.5f },
                { bp ( 1, PID::Band::attack ),      55.0f },
                { bp ( 1, PID::Band::release ),     45.0f },
                { bp ( 1, PID::Band::outputGain ),  1.5f },
                // Band 2
                { bp ( 2, PID::Band::threshold ),   -18.0f },
                { bp ( 2, PID::Band::ratio ),       3.0f },
                { bp ( 2, PID::Band::attack ),      40.0f },
                { bp ( 2, PID::Band::release ),     40.0f },
                { bp ( 2, PID::Band::outputGain ),  1.0f },
                // Band 3
                { bp ( 3, PID::Band::threshold ),   -16.0f },
                { bp ( 3, PID::Band::ratio ),       2.5f },
                { bp ( 3, PID::Band::attack ),      25.0f },
                { bp ( 3, PID::Band::release ),     35.0f },
                { bp ( 3, PID::Band::outputGain ),  0.5f },
            }
        },

        {
            "Broadcast", "Mastering",
            {
                { PID::Global::numBands,            5.0f },
                { xp ( 0, PID::Crossover::frequency ), 80.0f },
                { xp ( 1, PID::Crossover::frequency ), 500.0f },
                { xp ( 2, PID::Crossover::frequency ), 2500.0f },
                { xp ( 3, PID::Crossover::frequency ), 8000.0f },
                // Band 0 - Sub
                { bp ( 0, PID::Band::threshold ),   -24.0f },
                { bp ( 0, PID::Band::ratio ),       6.0f },
                { bp ( 0, PID::Band::attack ),      80.0f },
                { bp ( 0, PID::Band::release ),     60.0f },
                // Band 1 - Low-mid
                { bp ( 1, PID::Band::threshold ),   -22.0f },
                { bp ( 1, PID::Band::ratio ),       4.0f },
                { bp ( 1, PID::Band::attack ),      60.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                // Band 2 - Mid
                { bp ( 2, PID::Band::threshold ),   -20.0f },
                { bp ( 2, PID::Band::ratio ),       3.5f },
                { bp ( 2, PID::Band::attack ),      40.0f },
                { bp ( 2, PID::Band::release ),     45.0f },
                // Band 3 - Upper-mid
                { bp ( 3, PID::Band::threshold ),   -18.0f },
                { bp ( 3, PID::Band::ratio ),       3.0f },
                { bp ( 3, PID::Band::attack ),      25.0f },
                { bp ( 3, PID::Band::release ),     40.0f },
                // Band 4 - Air
                { bp ( 4, PID::Band::threshold ),   -16.0f },
                { bp ( 4, PID::Band::ratio ),       2.5f },
                { bp ( 4, PID::Band::attack ),      15.0f },
                { bp ( 4, PID::Band::release ),     35.0f },
            }
        },

        //==================================================================
        // Mixing
        //==================================================================
        {
            "Vocal Polish", "Mixing",
            {
                { PID::Global::numBands,            2.0f },
                { xp ( 0, PID::Crossover::frequency ), 3000.0f },
                // Band 0 - Body
                { bp ( 0, PID::Band::threshold ),   -20.0f },
                { bp ( 0, PID::Band::ratio ),       3.0f },
                { bp ( 0, PID::Band::attack ),      45.0f },
                { bp ( 0, PID::Band::release ),     50.0f },
                { bp ( 0, PID::Band::knee ),        8.0f },
                // Band 1 - Presence
                { bp ( 1, PID::Band::threshold ),   -18.0f },
                { bp ( 1, PID::Band::ratio ),       2.0f },
                { bp ( 1, PID::Band::attack ),      30.0f },
                { bp ( 1, PID::Band::release ),     40.0f },
                { bp ( 1, PID::Band::knee ),        6.0f },
                { bp ( 1, PID::Band::outputGain ),  1.5f },
            }
        },

        {
            "Bass Tightener", "Mixing",
            {
                { PID::Global::numBands,            2.0f },
                { xp ( 0, PID::Crossover::frequency ), 250.0f },
                // Band 0 - Sub/low
                { bp ( 0, PID::Band::threshold ),   -16.0f },
                { bp ( 0, PID::Band::ratio ),       5.0f },
                { bp ( 0, PID::Band::attack ),      75.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                { bp ( 0, PID::Band::knee ),        4.0f },
                // Band 1 - Upper bass / mids (light touch)
                { bp ( 1, PID::Band::threshold ),   -12.0f },
                { bp ( 1, PID::Band::ratio ),       1.5f },
                { bp ( 1, PID::Band::attack ),      40.0f },
                { bp ( 1, PID::Band::release ),     45.0f },
            }
        },

        {
            "Drum Bus", "Mixing",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 150.0f },
                { xp ( 1, PID::Crossover::frequency ), 4000.0f },
                // Band 0 - Kick
                { bp ( 0, PID::Band::threshold ),   -20.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      80.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                { bp ( 0, PID::Band::outputGain ),  1.0f },
                // Band 1 - Snare / toms
                { bp ( 1, PID::Band::threshold ),   -18.0f },
                { bp ( 1, PID::Band::ratio ),       3.0f },
                { bp ( 1, PID::Band::attack ),      55.0f },
                { bp ( 1, PID::Band::release ),     45.0f },
                // Band 2 - Cymbals
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       2.0f },
                { bp ( 2, PID::Band::attack ),      20.0f },
                { bp ( 2, PID::Band::release ),     35.0f },
            }
        },

        {
            "De-ess", "Mixing",
            {
                { PID::Global::numBands,            2.0f },
                { xp ( 0, PID::Crossover::frequency ), 5500.0f },
                // Band 0 - Below sibilance (pass through)
                { bp ( 0, PID::Band::threshold ),   0.0f },
                { bp ( 0, PID::Band::ratio ),       1.0f },
                // Band 1 - Sibilance
                { bp ( 1, PID::Band::threshold ),   -24.0f },
                { bp ( 1, PID::Band::ratio ),       8.0f },
                { bp ( 1, PID::Band::attack ),      10.0f },
                { bp ( 1, PID::Band::release ),     25.0f },
                { bp ( 1, PID::Band::range ),       12.0f },
                { bp ( 1, PID::Band::knee ),        3.0f },
            }
        },

        //==================================================================
        // Creative
        //==================================================================
        {
            "Pumping Low End", "Creative",
            {
                { PID::Global::numBands,            2.0f },
                { xp ( 0, PID::Crossover::frequency ), 200.0f },
                // Band 0 - Heavy low compression
                { bp ( 0, PID::Band::threshold ),   -30.0f },
                { bp ( 0, PID::Band::ratio ),       10.0f },
                { bp ( 0, PID::Band::attack ),      90.0f },
                { bp ( 0, PID::Band::release ),     70.0f },
                { bp ( 0, PID::Band::outputGain ),  4.0f },
                // Band 1 - Untouched highs
                { bp ( 1, PID::Band::threshold ),   0.0f },
                { bp ( 1, PID::Band::ratio ),       1.0f },
            }
        },

        {
            "Breathing", "Creative",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 300.0f },
                { xp ( 1, PID::Crossover::frequency ), 3000.0f },
                // Band 0
                { bp ( 0, PID::Band::threshold ),   -26.0f },
                { bp ( 0, PID::Band::ratio ),       8.0f },
                { bp ( 0, PID::Band::attack ),      95.0f },
                { bp ( 0, PID::Band::release ),     80.0f },
                // Band 1
                { bp ( 1, PID::Band::threshold ),   -28.0f },
                { bp ( 1, PID::Band::ratio ),       6.0f },
                { bp ( 1, PID::Band::attack ),      85.0f },
                { bp ( 1, PID::Band::release ),     75.0f },
                // Band 2
                { bp ( 2, PID::Band::threshold ),   -24.0f },
                { bp ( 2, PID::Band::ratio ),       5.0f },
                { bp ( 2, PID::Band::attack ),      70.0f },
                { bp ( 2, PID::Band::release ),     65.0f },
                { PID::Global::mix,                 75.0f },
            }
        },

        {
            "Wide Stereo", "Creative",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 250.0f },
                { xp ( 1, PID::Crossover::frequency ), 4000.0f },
                // Band 0 - Mono bass, tight
                { bp ( 0, PID::Band::threshold ),   -18.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      65.0f },
                { bp ( 0, PID::Band::release ),     50.0f },
                { bp ( 0, PID::Band::stereoLink ),  100.0f },
                // Band 1 - Mid, moderate link
                { bp ( 1, PID::Band::threshold ),   -16.0f },
                { bp ( 1, PID::Band::ratio ),       2.5f },
                { bp ( 1, PID::Band::attack ),      45.0f },
                { bp ( 1, PID::Band::release ),     45.0f },
                { bp ( 1, PID::Band::stereoLink ),  50.0f },
                // Band 2 - High, independent L/R
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       2.0f },
                { bp ( 2, PID::Band::attack ),      25.0f },
                { bp ( 2, PID::Band::release ),     35.0f },
                { bp ( 2, PID::Band::stereoLink ),  0.0f },
            }
        },

        //==================================================================
        // Starting Points
        //==================================================================
        {
            "3-Band Basic", "Starting Points",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 200.0f },
                { xp ( 1, PID::Crossover::frequency ), 3000.0f },
                // Band 0
                { bp ( 0, PID::Band::threshold ),   -20.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      50.0f },
                { bp ( 0, PID::Band::release ),     50.0f },
                // Band 1
                { bp ( 1, PID::Band::threshold ),   -20.0f },
                { bp ( 1, PID::Band::ratio ),       4.0f },
                { bp ( 1, PID::Band::attack ),      50.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                // Band 2
                { bp ( 2, PID::Band::threshold ),   -20.0f },
                { bp ( 2, PID::Band::ratio ),       4.0f },
                { bp ( 2, PID::Band::attack ),      50.0f },
                { bp ( 2, PID::Band::release ),     50.0f },
            }
        },

        {
            "4-Band Rock", "Starting Points",
            {
                { PID::Global::numBands,            4.0f },
                { xp ( 0, PID::Crossover::frequency ), 100.0f },
                { xp ( 1, PID::Crossover::frequency ), 1000.0f },
                { xp ( 2, PID::Crossover::frequency ), 6000.0f },
                // Band 0 - Sub
                { bp ( 0, PID::Band::threshold ),   -18.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      70.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                // Band 1 - Low mid
                { bp ( 1, PID::Band::threshold ),   -16.0f },
                { bp ( 1, PID::Band::ratio ),       3.0f },
                { bp ( 1, PID::Band::attack ),      50.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                // Band 2 - Presence
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       2.5f },
                { bp ( 2, PID::Band::attack ),      35.0f },
                { bp ( 2, PID::Band::release ),     40.0f },
                // Band 3 - Air
                { bp ( 3, PID::Band::threshold ),   -12.0f },
                { bp ( 3, PID::Band::ratio ),       2.0f },
                { bp ( 3, PID::Band::attack ),      20.0f },
                { bp ( 3, PID::Band::release ),     35.0f },
            }
        },

        {
            "5-Band Full Range", "Starting Points",
            {
                { PID::Global::numBands,            5.0f },
                { xp ( 0, PID::Crossover::frequency ), 80.0f },
                { xp ( 1, PID::Crossover::frequency ), 400.0f },
                { xp ( 2, PID::Crossover::frequency ), 2500.0f },
                { xp ( 3, PID::Crossover::frequency ), 8000.0f },
                // Band 0 - Sub
                { bp ( 0, PID::Band::threshold ),   -20.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      75.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                // Band 1 - Bass
                { bp ( 1, PID::Band::threshold ),   -20.0f },
                { bp ( 1, PID::Band::ratio ),       3.5f },
                { bp ( 1, PID::Band::attack ),      60.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                // Band 2 - Mids
                { bp ( 2, PID::Band::threshold ),   -20.0f },
                { bp ( 2, PID::Band::ratio ),       3.0f },
                { bp ( 2, PID::Band::attack ),      45.0f },
                { bp ( 2, PID::Band::release ),     45.0f },
                // Band 3 - Presence
                { bp ( 3, PID::Band::threshold ),   -20.0f },
                { bp ( 3, PID::Band::ratio ),       2.5f },
                { bp ( 3, PID::Band::attack ),      30.0f },
                { bp ( 3, PID::Band::release ),     40.0f },
                // Band 4 - Air
                { bp ( 4, PID::Band::threshold ),   -20.0f },
                { bp ( 4, PID::Band::ratio ),       2.0f },
                { bp ( 4, PID::Band::attack ),      15.0f },
                { bp ( 4, PID::Band::release ),     35.0f },
            }
        },

        //==================================================================
        // Additional Mixing presets
        //==================================================================
        {
            "Guitar Leveler", "Mixing",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 250.0f },
                { xp ( 1, PID::Crossover::frequency ), 3500.0f },
                // Band 0 - Low rumble control
                { bp ( 0, PID::Band::threshold ),   -22.0f },
                { bp ( 0, PID::Band::ratio ),       4.0f },
                { bp ( 0, PID::Band::attack ),      65.0f },
                { bp ( 0, PID::Band::release ),     50.0f },
                // Band 1 - Body
                { bp ( 1, PID::Band::threshold ),   -16.0f },
                { bp ( 1, PID::Band::ratio ),       2.5f },
                { bp ( 1, PID::Band::attack ),      50.0f },
                { bp ( 1, PID::Band::release ),     45.0f },
                { bp ( 1, PID::Band::knee ),        10.0f },
                // Band 2 - Pick / bite
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       2.0f },
                { bp ( 2, PID::Band::attack ),      25.0f },
                { bp ( 2, PID::Band::release ),     35.0f },
            }
        },

        {
            "Piano Glue", "Mixing",
            {
                { PID::Global::numBands,            3.0f },
                { xp ( 0, PID::Crossover::frequency ), 300.0f },
                { xp ( 1, PID::Crossover::frequency ), 4000.0f },
                // Band 0 - Bass notes
                { bp ( 0, PID::Band::threshold ),   -18.0f },
                { bp ( 0, PID::Band::ratio ),       3.0f },
                { bp ( 0, PID::Band::attack ),      60.0f },
                { bp ( 0, PID::Band::release ),     55.0f },
                { bp ( 0, PID::Band::knee ),        12.0f },
                // Band 1 - Mid register
                { bp ( 1, PID::Band::threshold ),   -16.0f },
                { bp ( 1, PID::Band::ratio ),       2.0f },
                { bp ( 1, PID::Band::attack ),      45.0f },
                { bp ( 1, PID::Band::release ),     50.0f },
                { bp ( 1, PID::Band::knee ),        10.0f },
                // Band 2 - Brightness
                { bp ( 2, PID::Band::threshold ),   -14.0f },
                { bp ( 2, PID::Band::ratio ),       1.5f },
                { bp ( 2, PID::Band::attack ),      30.0f },
                { bp ( 2, PID::Band::release ),     40.0f },
                { bp ( 2, PID::Band::knee ),        8.0f },
            }
        },
    };
    // clang-format on

    return presets;
}

} // namespace FactoryPresets
