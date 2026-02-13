/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <juce_core/juce_core.h>

namespace ParameterIDs
{
    // Helper to generate band-specific parameter IDs
    inline juce::String bandParam (int bandIndex, const juce::String& suffix)
    {
        return "band" + juce::String (bandIndex) + "_" + suffix;
    }

    // Helper to generate crossover-specific parameter IDs
    inline juce::String crossoverParam (int crossoverIndex, const juce::String& suffix)
    {
        return "xover" + juce::String (crossoverIndex) + "_" + suffix;
    }

    // Per-band parameter suffixes
    namespace Band
    {
        const juce::String enabled    = "enabled";
        const juce::String mode       = "mode";        // compress/expand
        const juce::String threshold  = "threshold";
        const juce::String range      = "range";
        const juce::String ratio      = "ratio";
        const juce::String attack     = "attack";
        const juce::String release    = "release";
        const juce::String outputGain = "output_gain";
        const juce::String knee       = "knee";
        const juce::String lookahead  = "lookahead";
        const juce::String stereoLink = "stereo_link";
        const juce::String stereoMode = "stereo_mode";
        const juce::String scMode     = "sc_mode";
        const juce::String scFreqLo   = "sc_freq_lo";
        const juce::String scFreqHi   = "sc_freq_hi";
        const juce::String scExternal = "sc_external";
        const juce::String audition   = "audition";
    }

    // Per-crossover parameter suffixes
    namespace Crossover
    {
        const juce::String frequency = "freq";
        const juce::String slope     = "slope";
    }

    // Global parameter IDs
    namespace Global
    {
        const juce::String processMode  = "process_mode";
        const juce::String oversample   = "oversample";
        const juce::String analyzerMode = "analyzer_mode";
        const juce::String globalOutput = "global_output";
        const juce::String mix          = "mix";
        const juce::String midiLearn    = "midi_learn";
        const juce::String numBands     = "num_bands";
    }
}
