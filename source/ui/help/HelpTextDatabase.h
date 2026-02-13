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
#include <unordered_map>

/**
 * @brief Static database of help text strings for every control in the
 *        multiband compressor UI.
 *
 * Look-up is by component ID (the string set via setComponentID()).
 * The text is returned as a juce::String; an empty string means
 * no help entry exists for that ID.
 */
namespace HelpText
{

inline juce::String getHelpText ( const juce::String& componentID )
{
    static const std::unordered_map<std::string, const char*> database = {

        // =================================================================
        // Header / Toolbar
        // =================================================================
        { "header.prevPreset",   "Previous Preset \xe2\x80\x94 cycles backward through the preset list." },
        { "header.nextPreset",   "Next Preset \xe2\x80\x94 cycles forward through the preset list." },
        { "header.presetName",   "Current Preset \xe2\x80\x94 click to browse and select factory or user presets." },
        { "header.undo",         "Undo \xe2\x80\x94 reverses the last parameter change (Cmd/Ctrl+Z)." },
        { "header.redo",         "Redo \xe2\x80\x94 re-applies the last undone change (Cmd/Ctrl+Shift+Z)." },
        { "header.abToggle",     "A/B Compare \xe2\x80\x94 toggles between two independent compressor states for quick comparison." },
        { "header.midiLearn",    "MIDI Learn \xe2\x80\x94 assigns MIDI CC messages to any parameter. Click, then move a MIDI controller to map it." },
        { "header.help",         "Help Overlay \xe2\x80\x94 toggles this contextual help layer. Hover over any control to see a description." },

        // =================================================================
        // Band controls (per-band, generic descriptions)
        // =================================================================
        { "band.threshold",      "Threshold \xe2\x80\x94 the level above which compression begins for this band (-90 to 0 dB). Lower values compress more of the signal." },
        { "band.ratio",          "Ratio \xe2\x80\x94 how aggressively the signal is compressed above the threshold. 2:1 halves the excess, \xe2\x88\x9e:1 is hard limiting (1:1 to 100:1)." },
        { "band.attack",         "Attack \xe2\x80\x94 how quickly compression engages after the signal crosses the threshold. Lower values catch transients; higher values let them through (0\xe2\x80\x93" "100%)." },
        { "band.release",        "Release \xe2\x80\x94 how quickly compression recovers once the signal drops below the threshold. Shorter values are more responsive; longer values are smoother (0\xe2\x80\x93" "100%)." },
        { "band.knee",           "Knee \xe2\x80\x94 how gradually compression transitions around the threshold. 0 dB = hard knee (abrupt), higher = softer transition (0 to 36 dB)." },
        { "band.range",          "Range \xe2\x80\x94 limits the maximum gain change for this band. Lower values cap the amount of compression or expansion applied (-30 to +30 dB)." },
        { "band.outputGain",     "Output Gain \xe2\x80\x94 adjusts the level of this band after compression, for per-band makeup gain (-30 to +30 dB)." },
        { "band.lookahead",      "Lookahead \xe2\x80\x94 delays the audio so the compressor reacts before transients arrive. Adds latency but improves transient control (0 to 20 ms)." },
        { "band.stereoLink",     "Stereo Link \xe2\x80\x94 how much the left and right channels share gain reduction in this band. 100% = fully linked, 0% = independent (dual mono)." },
        { "band.stereoMode",     "Stereo Mode \xe2\x80\x94 selects the channel configuration for this band: Stereo (L/R) or Mid/Side processing." },
        { "band.enabled",        "Band Enabled \xe2\x80\x94 toggles compression processing for this frequency band on or off." },
        { "band.mode",           "Band Mode \xe2\x80\x94 switches between Compress (downward compression) and Expand (upward expansion) for this band." },
        { "band.solo",           "Solo \xe2\x80\x94 auditions only this frequency band in isolation, muting all others." },
        { "band.mute",           "Mute \xe2\x80\x94 silences this frequency band while keeping all others active." },
        { "band.audition",       "Audition Sidechain \xe2\x80\x94 monitors the sidechain signal for this band so you can hear what the detector sees." },
        { "band.scMode",         "Sidechain Mode \xe2\x80\x94 Band uses the crossover-filtered signal, Free uses custom frequency filters for the detector." },
        { "band.scFreqLo",       "SC Low Frequency \xe2\x80\x94 high-pass cutoff for the sidechain filter in Free mode. Removes low-frequency energy from the detection signal." },
        { "band.scFreqHi",       "SC High Frequency \xe2\x80\x94 low-pass cutoff for the sidechain filter in Free mode. Removes high-frequency energy from the detection signal." },
        { "band.scExternal",     "External Sidechain \xe2\x80\x94 uses the external sidechain input instead of the main audio for detection in this band." },

        // =================================================================
        // Global controls
        // =================================================================
        { "global.processMode",  "Processing Mode \xe2\x80\x94 selects the crossover filter topology. Dynamic Phase preserves transients, Linear Phase avoids phase shift, Minimum Phase has lowest latency." },
        { "global.oversample",   "Oversampling \xe2\x80\x94 processes at a higher sample rate for cleaner compression. Higher factors improve quality at the cost of CPU (1x, 2x, 4x)." },
        { "global.analyzerMode", "Analyzer Mode \xe2\x80\x94 toggles the real-time FFT spectrum display. Off, Pre (input only), Post (output only), or Pre+Post (both overlaid)." },
        { "global.globalOutput",  "Global Output \xe2\x80\x94 master output gain applied after all bands are summed (-30 to +30 dB)." },
        { "global.mix",          "Mix \xe2\x80\x94 blends the dry (unprocessed) and wet (compressed) signals for parallel compression. 100% = fully compressed, 0% = bypass (0 to 200%)." },
        { "global.numBands",     "Number of Bands \xe2\x80\x94 sets how many frequency bands are active (1 to 6). More bands give finer control but increase complexity." },

        // =================================================================
        // Crossover controls
        // =================================================================
        { "crossover.frequency", "Crossover Frequency \xe2\x80\x94 the split point between adjacent bands (20 Hz to 20 kHz). Drag to reshape the frequency allocation." },
        { "crossover.slope",     "Crossover Slope \xe2\x80\x94 steepness of the crossover filter (6, 12, 18, 24, 36, or 48 dB/oct). Steeper slopes give sharper band separation." },

        // =================================================================
        // Metering / Display
        // =================================================================
        { "meter.input",         "Input Meter \xe2\x80\x94 shows the signal level entering the multiband compressor." },
        { "meter.output",        "Output Meter \xe2\x80\x94 shows the signal level after all processing and the global output gain." },
        { "meter.gr",            "Gain Reduction Meter \xe2\x80\x94 shows the amount of gain reduction applied by each band in real time." },
        { "display.spectrum",    "Spectrum Display \xe2\x80\x94 real-time FFT analysis of the audio signal. Crossover frequencies appear as vertical dividers." },
        { "display.bandGR",      "Band Gain Reduction \xe2\x80\x94 per-band gain reduction bars overlaid on the spectrum display. Taller bars mean more compression." },
    };

    auto it = database.find ( componentID.toStdString() );
    if ( it != database.end() )
        return juce::String::fromUTF8 ( it->second );

    return {};
}

} // namespace HelpText
