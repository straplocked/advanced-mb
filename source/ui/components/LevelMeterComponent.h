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

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel/ColourPalette.h"
#include "../../utils/Constants.h"

/**
 * @brief Stereo peak + RMS vertical level meter with peak hold.
 *
 * Width: Constants::kMeterWidth (20px)
 *
 * The meter displays two vertical bars (left and right channels) with a
 * colour gradient from green (-60 dB) through yellow (-12 dB) to red (0 dB).
 * Peak hold indicators are drawn as thin white lines that decay over time.
 * RMS levels are drawn as a slightly narrower, semi-transparent overlay.
 *
 * Call setLevels() from the UI timer at 30 Hz to feed new data.
 */
class LevelMeterComponent : public juce::Component,
                            private juce::Timer
{
public:
    enum class MeterMode { Level, GainReduction };

    LevelMeterComponent();
    ~LevelMeterComponent() override;

    void paint ( juce::Graphics& g ) override;

    /** Push new peak and RMS levels (in dB) from the metering data. */
    void setLevels ( float peakL, float peakR, float rmsL, float rmsR );

    /** Optional label drawn above the meter (e.g. "In", "Out"). */
    void setLabel ( const juce::String& label ) { meterLabel = label; }

    /** Set meter mode: Level (normal) or GainReduction (inverted, red). */
    void setMeterMode ( MeterMode mode ) { meterMode = mode; }

private:
    void timerCallback() override;

    /** Map a dB value to a Y pixel coordinate within the meter area. */
    float levelToY ( float dB, float meterHeight ) const;

    // Minimum / Maximum dB for the meter range
    static constexpr float kMinDB = -60.0f;
    static constexpr float kMaxDB = 6.0f;

    // GR meter range (0 dB at top, -30 dB at bottom)
    static constexpr float kGRMinDB = -30.0f;

    // Peak hold decay (dB per timer tick)
    static constexpr float kPeakDecayRate = 0.5f;

    // Level smoothing factor (lower = slower decay)
    static constexpr float kLevelDecay = 0.85f;

    // Current display levels (smoothed)
    float peakLevelL = kMinDB;
    float peakLevelR = kMinDB;
    float rmsLevelL  = kMinDB;
    float rmsLevelR  = kMinDB;

    // Peak hold values
    float peakHoldL  = kMinDB;
    float peakHoldR  = kMinDB;

    // Target levels (set by setLevels, smoothed toward by timerCallback)
    float targetPeakL = kMinDB;
    float targetPeakR = kMinDB;
    float targetRmsL  = kMinDB;
    float targetRmsR  = kMinDB;

    juce::String meterLabel;
    MeterMode meterMode = MeterMode::Level;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( LevelMeterComponent )
};
