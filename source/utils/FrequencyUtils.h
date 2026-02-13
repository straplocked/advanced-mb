/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace FrequencyUtils
{
    inline float midiNoteToFrequency (int note)
    {
        return 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
    }

    inline int frequencyToMidiNote (float freq)
    {
        return juce::roundToInt (69.0f + 12.0f * std::log2 (freq / 440.0f));
    }

    inline juce::String midiNoteToName (int note)
    {
        static const char* noteNames[] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        int octave = (note / 12) - 1;
        int nameIndex = note % 12;
        return juce::String (noteNames[nameIndex]) + juce::String (octave);
    }

    inline juce::String frequencyToNoteName (float freq)
    {
        int note = frequencyToMidiNote (freq);
        return midiNoteToName (note);
    }

    inline juce::String formatFrequency (float freq)
    {
        if (freq < 100.0f)
            return juce::String (freq, 1) + " Hz";
        if (freq < 1000.0f)
            return juce::String (freq, 0) + " Hz";
        if (freq < 10000.0f)
            return juce::String (freq / 1000.0f, 2) + " kHz";
        return juce::String (freq / 1000.0f, 1) + " kHz";
    }
}
