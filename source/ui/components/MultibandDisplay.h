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
#include "../../dsp/BandManager.h"
#include <functional>

class AdvancedMBProcessor;

/**
 * @brief Central multiband visualization and interaction component.
 *
 * Supports gap-aware rendering: bands can have gaps between them where
 * audio passes through unprocessed. Hovering over a gap shows a preview
 * of where a new band would be created.
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
    void mouseMove ( const juce::MouseEvent& e ) override;
    void mouseExit ( const juce::MouseEvent& e ) override;

    /** Callback invoked when a band region is clicked (bandIndex 0..N-1). */
    std::function<void ( int )> onBandSelected;

    /** Callback invoked when the already-selected band is clicked again (deselect). */
    std::function<void()> onBandDeselected;

    /** Callback invoked when a new band is created by clicking empty space. */
    std::function<void ( int )> onBandCreated;

    void setBandManager ( BandManager* bm ) { bandManager = bm; }

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
    void drawRangeRegions ( juce::Graphics& g );
    void drawOutputCurve ( juce::Graphics& g );
    void drawCrossovers ( juce::Graphics& g );
    void drawBandControlDots ( juce::Graphics& g );
    void drawBandButtons ( juce::Graphics& g );
    void drawHoverPreview ( juce::Graphics& g );
    void drawAxisLabels ( juce::Graphics& g );

    // Hit-testing helpers -------------------------------------------------
    int crossoverHandleAtPoint ( juce::Point<float> point ) const;
    int controlDotAtPoint ( juce::Point<float> point ) const;
    int rangeBoundaryAtPoint ( juce::Point<float> point ) const;
    int smButtonAtPoint ( juce::Point<float> point, bool& isSolo ) const;

    /** Get the output curve dB and range boundary dB at a band's centre. */
    void getBandCurveValues ( int bandPos, float& outputCurveDB, float& rangeBoundaryDB ) const;
    int bandAtX ( float x ) const;
    void showBandContextMenu ( int bandIndex );
    void updateCursor();
    void updateDisplayRange();

    /** Get the S/M button rectangles for a band. */
    void getBandButtonRects ( int bandPos, juce::Rectangle<float>& soloRect,
                              juce::Rectangle<float>& muteRect ) const;

    /** Get the control dot center for a band. */
    juce::Point<float> getControlDotCenter ( int bandPos ) const;

    /** Find the gap boundaries at the given X position.
        Returns false if the position is inside a user band. */
    bool getGapAtX ( float x, float& gapLeftHz, float& gapRightHz ) const;

    /** Compute proposed band boundaries centered on freq, clamped to gap. */
    void computeHoverBand ( float freq, float gapLeftHz, float gapRightHz,
                            float& bandLeftHz, float& bandRightHz ) const;

    // Data ----------------------------------------------------------------
    AdvancedMBProcessor* processor = nullptr;
    BandManager* bandManager = nullptr;
    int selectedBand = -1;
    int draggingCrossover = -1;     // -1 = not dragging
    int draggingControlDot = -1;    // -1 = not dragging a control dot
    int draggingRangeBand = -1;     // -1 = not dragging a range boundary
    bool draggingLeftEdge = false;  // true = dragging left edge, false = right

    // Hover state
    int hoveredBand = -1;
    int hoveredCrossover = -1;
    int hoveredControlDot = -1;
    int hoveredRange = -1;

    // Hover preview for band creation
    bool showingHoverPreview = false;
    float hoverFreq = 0.0f;
    float hoverLeftFreq = 0.0f;
    float hoverRightFreq = 0.0f;

    // Drag tooltip (shared between crossover drag and range drag)
    bool showingFreqTooltip = false;
    float tooltipFreq = 0.0f;
    bool showingRangeTooltip = false;
    juce::String rangeTooltipText;
    float tooltipX = 0.0f;
    float tooltipY = 0.0f;

    float minDB = -36.0f;
    float maxDB = 36.0f;
    float targetMinDB = -36.0f;
    float targetMaxDB = 36.0f;

    static constexpr float kHandleHitRadius = 12.0f;
    static constexpr float kControlDotRadius = 7.0f;
    static constexpr float kControlDotHitRadius = 12.0f;
    static constexpr float kButtonWidth = 16.0f;
    static constexpr float kButtonHeight = 14.0f;
    static constexpr float kButtonSpacing = 2.0f;
    static constexpr float kButtonTopMargin = 4.0f;

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
