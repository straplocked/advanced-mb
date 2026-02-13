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
#include "../../utils/MathUtils.h"
#include <functional>

class AdvancedMBProcessor;

/**
 * @brief Central multiband visualization and interaction component.
 *
 * Draws (bottom to top):
 *   1. Dark background with frequency grid (50 Hz - 20 kHz log) and dB grid (-60 to +6 dB)
 *   2. Spectrum analyzer curves (pre/post FFT from processor)
 *   3. Band regions - coloured vertical regions between crossover frequencies (15% alpha fill)
 *   4. Gain reduction visualization per band
 *   5. Crossover lines - dashed vertical lines at crossover frequencies
 *   6. Crossover handles - draggable circles at top of crossover lines
 *   7. Threshold handles - per-band horizontal lines showing threshold level
 *   8. Frequency / dB axis labels
 *
 * Interactions:
 *   - Drag crossover handle left/right to move crossover frequency
 *   - Click band region to select band (triggers BandControlPanel)
 *   - Right-click band region for context menu (enable/disable, audition)
 */
class MultibandDisplay : public juce::Component,
                         public juce::Timer
{
public:
    MultibandDisplay();
    ~MultibandDisplay() override;

    void setProcessor ( AdvancedMBProcessor* p );
    void setSelectedBand ( int bandIndex ) { selectedBand = bandIndex; repaint(); }

    void paint ( juce::Graphics& g ) override;
    void resized() override;
    void timerCallback() override; // 30 Hz refresh

    void mouseDown ( const juce::MouseEvent& e ) override;
    void mouseDrag ( const juce::MouseEvent& e ) override;
    void mouseUp ( const juce::MouseEvent& e ) override;

    /** Callback invoked when a band region is clicked (bandIndex 0..N-1). */
    std::function<void ( int )> onBandSelected;

private:
    // Coordinate helpers --------------------------------------------------
    float frequencyToX ( float freq ) const;
    float xToFrequency ( float x ) const;
    float dbToY ( float dB ) const;
    float yToDb ( float y ) const;

    // Drawing methods (called in layer order) -----------------------------
    void drawBackground ( juce::Graphics& g );
    void drawGrid ( juce::Graphics& g );
    void drawSpectrum ( juce::Graphics& g );
    void drawBandRegions ( juce::Graphics& g );
    void drawGainReduction ( juce::Graphics& g );
    void drawCrossovers ( juce::Graphics& g );
    void drawThresholds ( juce::Graphics& g );
    void drawAxisLabels ( juce::Graphics& g );

    // Hit-testing helpers -------------------------------------------------
    int crossoverHandleAtPoint ( juce::Point<float> point ) const;
    int bandAtX ( float x ) const;
    void showBandContextMenu ( int bandIndex );

    // Data ----------------------------------------------------------------
    AdvancedMBProcessor* processor = nullptr;
    int selectedBand = -1;
    int draggingCrossover = -1; // -1 = not dragging

    float minDB = -60.0f;
    float maxDB = 6.0f;

    static constexpr float kHandleRadius = 7.0f;
    static constexpr float kHandleHitRadius = 12.0f;

    // Band colour look-up tables (indexed by band) -----------------------
    static inline const juce::Colour bandColours[Constants::kMaxBands] =
    {
        ColourPalette::band1, ColourPalette::band2, ColourPalette::band3,
        ColourPalette::band4, ColourPalette::band5, ColourPalette::band6
    };

    static inline const juce::Colour bandFillColours[Constants::kMaxBands] =
    {
        ColourPalette::band1Fill, ColourPalette::band2Fill, ColourPalette::band3Fill,
        ColourPalette::band4Fill, ColourPalette::band5Fill, ColourPalette::band6Fill
    };

    // Cached spectrum path data (updated per timer tick) ------------------
    juce::Path preSpectrumPath;
    juce::Path postSpectrumPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( MultibandDisplay )
};
