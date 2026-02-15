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

#include "MultibandDisplay.h"
#include "../../PluginProcessor.h"
#include "../../utils/FrequencyUtils.h"
#include "../LookAndFeel/Fonts.h"
#include "../../parameters/ParameterIDs.h"

//==============================================================================
MultibandDisplay::MultibandDisplay()
{
    setOpaque ( true );
}

MultibandDisplay::~MultibandDisplay()
{
    stopTimer();
}

//==============================================================================
void MultibandDisplay::setProcessor ( AdvancedMBProcessor* p )
{
    processor = p;

    if ( processor != nullptr )
        startTimerHz ( Constants::kUITimerHz );
    else
        stopTimer();
}

//==============================================================================
// Coordinate helpers
//==============================================================================

float MultibandDisplay::frequencyToX ( float freq ) const
{
    return MathUtils::frequencyToX ( freq, static_cast<float> ( getWidth() ) );
}

float MultibandDisplay::xToFrequency ( float x ) const
{
    return MathUtils::xToFrequency ( x, static_cast<float> ( getWidth() ) );
}

float MultibandDisplay::dbToY ( float dB ) const
{
    return MathUtils::gainToY ( dB, static_cast<float> ( getHeight() ), minDB, maxDB );
}

float MultibandDisplay::yToDb ( float y ) const
{
    return MathUtils::yToGain ( y, static_cast<float> ( getHeight() ), minDB, maxDB );
}

//==============================================================================
// Timer
//==============================================================================

void MultibandDisplay::timerCallback()
{
    updateDisplayRange();
    repaint();
}

//==============================================================================
// Paint
//==============================================================================

void MultibandDisplay::paint ( juce::Graphics& g )
{
    drawBackground ( g );
    drawGrid ( g );
    drawSpectrum ( g );
    drawBandRegions ( g );
    drawRangeRegions ( g );
    drawOutputCurve ( g );
    drawCrossovers ( g );
    drawBandControlDots ( g );
    drawBandButtons ( g );
    drawHoverPreview ( g );
    drawAxisLabels ( g );
}

void MultibandDisplay::resized()
{
    // Nothing extra needed - coordinate helpers use live getWidth/Height.
}

//==============================================================================
// Layer 1 - Background
//==============================================================================

void MultibandDisplay::drawBackground ( juce::Graphics& g )
{
    g.fillAll ( ColourPalette::curveArea );
}

//==============================================================================
// Layer 2 - Grid
//==============================================================================

void MultibandDisplay::drawGrid ( juce::Graphics& g )
{
    const auto w = static_cast<float> ( getWidth() );
    const auto h = static_cast<float> ( getHeight() );

    // Frequency grid lines (log-spaced)
    const float freqLines[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
                                 2000.0f, 5000.0f, 10000.0f, 20000.0f };

    for ( auto freq : freqLines )
    {
        float x = frequencyToX ( freq );
        bool major = ( freq == 100.0f || freq == 1000.0f || freq == 10000.0f );
        g.setColour ( major ? ColourPalette::gridLineStrong : ColourPalette::gridLine );
        g.drawVerticalLine ( juce::roundToInt ( x ), 0.0f, h );
    }

    // dB grid lines (horizontal)
    for ( float dB = minDB; dB <= maxDB; dB += 6.0f )
    {
        float y = dbToY ( dB );
        bool major = ( dB == 0.0f || dB == -12.0f || dB == -24.0f || dB == -48.0f );
        g.setColour ( major ? ColourPalette::gridLineStrong : ColourPalette::gridLine );
        g.drawHorizontalLine ( juce::roundToInt ( y ), 0.0f, w );
    }
}

//==============================================================================
// Layer 3 - Spectrum analyser
//==============================================================================

void MultibandDisplay::drawSpectrum ( juce::Graphics& g )
{
    if ( processor == nullptr )
        return;

    auto& preFFT  = processor->getPreFFT();
    auto& postFFT = processor->getPostFFT();

    const int numBins = preFFT.getNumBins();
    const double sr = preFFT.getSampleRate();

    if ( numBins <= 0 || sr <= 0.0 )
        return;

    // Helper lambda to build a spectrum path from a TripleBuffer
    auto buildPath = [&] ( auto& fftProc, juce::Path& path )
    {
        fftProc.getMagnitudeBuffer().swapRead();
        const float* data = fftProc.getMagnitudeBuffer().getReadPointer();

        path.clear();
        bool started = false;

        for ( int i = 1; i < numBins; ++i )
        {
            float freq = static_cast<float> ( i ) * static_cast<float> ( sr )
                         / static_cast<float> ( numBins * 2 );

            if ( freq < Constants::kMinFrequency || freq > Constants::kMaxFrequency )
                continue;

            float mag = data[i];
            float dB  = ( mag > 1e-10f ) ? ( 20.0f * std::log10 ( mag ) ) : minDB;
            dB = juce::jlimit ( minDB, maxDB, dB );

            float x = frequencyToX ( freq );
            float y = dbToY ( dB );

            if ( ! started )
            {
                path.startNewSubPath ( x, y );
                started = true;
            }
            else
            {
                path.lineTo ( x, y );
            }
        }
    };

    int analyzerMode = processor->getParameterAttachment().getAnalyzerMode();

    // Pre spectrum
    if ( analyzerMode == Constants::AnalyzerPre || analyzerMode == Constants::AnalyzerPrePost )
    {
        buildPath ( preFFT, preSpectrumPath );
        g.setColour ( ColourPalette::spectrumPre );
        g.strokePath ( preSpectrumPath,
                       juce::PathStrokeType ( 1.5f, juce::PathStrokeType::curved ) );
    }

    // Post spectrum
    if ( analyzerMode == Constants::AnalyzerPost || analyzerMode == Constants::AnalyzerPrePost )
    {
        buildPath ( postFFT, postSpectrumPath );
        g.setColour ( ColourPalette::spectrumPost );
        g.strokePath ( postSpectrumPath,
                       juce::PathStrokeType ( 1.5f, juce::PathStrokeType::curved ) );
    }
}

//==============================================================================
// Layer 4 - Band regions (coloured fills between crossovers)
//==============================================================================

void MultibandDisplay::drawBandRegions ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return;

    const float h = static_cast<float> ( getHeight() );

    for ( int b = 0; b < numBands; ++b )
    {
        float x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );

        juce::Colour fill = bandFillColours[bands[static_cast<size_t> ( b )].slotIndex % Constants::kMaxBands];

        if ( b == selectedBand )
            fill = fill.brighter ( 0.3f );

        g.setColour ( fill );
        g.fillRect ( x0, 0.0f, x1 - x0, h );
    }
}

//==============================================================================
// Helper: get the output curve dB and range boundary dB at a band's centre
//==============================================================================

void MultibandDisplay::getBandCurveValues ( int bandPos,
                                             float& outputCurveDB,
                                             float& rangeBoundaryDB ) const
{
    outputCurveDB = 0.0f;
    rangeBoundaryDB = 0.0f;

    if ( processor == nullptr || bandManager == nullptr )
        return;

    const auto& bands = bandManager->getActiveBands();

    if ( bandPos < 0 || bandPos >= static_cast<int> ( bands.size() ) )
        return;

    int slot = bands[static_cast<size_t> ( bandPos )].slotIndex;
    auto params = processor->getParameterAttachment().getCurrentParameters();

    float grDB = processor->getBandGainReduction ( slot );
    float outDB = params.bands[slot].outputGain;
    float range = params.bands[slot].range;

    outputCurveDB = outDB + grDB;

    // Range boundary is anchored to outputGain (the dot).
    // Positive range → boundary above dot, negative → below.
    rangeBoundaryDB = outDB + range;
}

//==============================================================================
// Layer 5 - Range regions (curved fill between outDB and rangeBoundary curves)
//==============================================================================

void MultibandDisplay::drawRangeRegions ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return;

    const int numPoints = 64;

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        float outputCurveDB, rangeBoundaryDB;
        getBandCurveValues ( b, outputCurveDB, rangeBoundaryDB );

        float outDB = params.bands[slot].outputGain;

        float x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );

        juce::Colour bandCol = bandColours[slot % Constants::kMaxBands];

        // Build a closed path between the outDB curve and the
        // rangeBoundary curve.  The fill shows the range area — always
        // between outDB and outDB + range.  The dot sits at outDB
        // independently.
        juce::Path fillPath;

        // Forward pass: outDB curve (left → right)
        for ( int i = 0; i <= numPoints; ++i )
        {
            float t = static_cast<float> ( i ) / static_cast<float> ( numPoints );
            float x = x0 + t * ( x1 - x0 );
            float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
            float shape = sinVal * sinVal;
            float dB = outDB * shape;
            float y = dbToY ( dB );

            if ( i == 0 )
                fillPath.startNewSubPath ( x, y );
            else
                fillPath.lineTo ( x, y );
        }

        // Reverse pass: rangeBoundary curve (right → left)
        for ( int i = numPoints; i >= 0; --i )
        {
            float t = static_cast<float> ( i ) / static_cast<float> ( numPoints );
            float x = x0 + t * ( x1 - x0 );
            float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
            float shape = sinVal * sinVal;
            float dB = rangeBoundaryDB * shape;
            float y = dbToY ( dB );
            fillPath.lineTo ( x, y );
        }

        fillPath.closeSubPath();

        // Semi-transparent fill
        g.setColour ( bandCol.withAlpha ( 0.20f ) );
        g.fillPath ( fillPath );

        // Range boundary curve stroke
        juce::Path boundaryPath;

        for ( int i = 0; i <= numPoints; ++i )
        {
            float t = static_cast<float> ( i ) / static_cast<float> ( numPoints );
            float x = x0 + t * ( x1 - x0 );
            float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
            float shape = sinVal * sinVal;
            float dB = rangeBoundaryDB * shape;
            float y = dbToY ( dB );

            if ( i == 0 )
                boundaryPath.startNewSubPath ( x, y );
            else
                boundaryPath.lineTo ( x, y );
        }

        bool isHoveredRange = ( hoveredRange == b );
        bool isDraggingRange = ( draggingRangeBand == b );
        float strokeWidth = ( isHoveredRange || isDraggingRange ) ? 2.0f : 1.0f;
        g.setColour ( bandCol.withAlpha ( isHoveredRange || isDraggingRange ? 0.7f : 0.45f ) );
        g.strokePath ( boundaryPath,
                       juce::PathStrokeType ( strokeWidth, juce::PathStrokeType::curved ) );
    }
}

//==============================================================================
// Layer 6 - Output curve (yellow/orange line — top edge of range fill)
//
// A single continuous line across the full display showing the top edge
// of the range area.  In gaps the line sits at 0 dB.  Within a band
// the line peaks at outDB + max(0, range) using a sin² bell shape.
// When range > 0, the yellow line sits above the dot; when range <= 0,
// it sits on the dot.
//==============================================================================

void MultibandDisplay::drawOutputCurve ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const float w = static_cast<float> ( getWidth() );

    // Pre-compute per-band data for fast lookup
    struct BandCurveData
    {
        float x0, x1;          // pixel bounds
        float peakDB;           // outputGain + gainReduction at centre
        int   slot;
        juce::Colour colour;
    };

    std::vector<BandCurveData> bandData;
    bandData.reserve ( static_cast<size_t> ( numBands ) );

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        float outDB = params.bands[slot].outputGain;
        float range = params.bands[slot].range;

        BandCurveData d;
        d.x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        d.x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );
        d.peakDB = outDB + std::max ( 0.0f, range );
        d.slot = slot;
        d.colour = bandColours[slot % Constants::kMaxBands];
        bandData.push_back ( d );
    }

    if ( bandData.empty() )
        return;

    // Build a single continuous path across the full width.
    // At each X we find which band (if any) we're in and compute
    // the interpolated dB value.
    juce::Path curvePath;
    const int steps = juce::jmax ( static_cast<int> ( w ), 200 );
    int currentBandIdx = -1;   // index into bandData

    for ( int i = 0; i <= steps; ++i )
    {
        float x = static_cast<float> ( i ) * w / static_cast<float> ( steps );
        float dB = 0.0f;
        int hitBand = -1;

        // Find which band this X falls in
        for ( int bi = 0; bi < static_cast<int> ( bandData.size() ); ++bi )
        {
            if ( x >= bandData[static_cast<size_t> ( bi )].x0 &&
                 x <= bandData[static_cast<size_t> ( bi )].x1 )
            {
                hitBand = bi;
                break;
            }
        }

        if ( hitBand >= 0 )
        {
            const auto& bd = bandData[static_cast<size_t> ( hitBand )];
            float range = bd.x1 - bd.x0;

            if ( range > 0.0f )
            {
                float t = ( x - bd.x0 ) / range;  // 0..1 across band
                float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
                float shape = sinVal * sinVal;     // sin² bell: 0 at edges, 1 at center
                dB = bd.peakDB * shape;
            }
        }

        float y = dbToY ( dB );

        if ( i == 0 )
            curvePath.startNewSubPath ( x, y );
        else
            curvePath.lineTo ( x, y );

        currentBandIdx = hitBand;
    }

    // Stroke the curve with a gradient that picks up band colours.
    // For simplicity we use the first band's colour with full-width stroke,
    // then overlay per-band coloured segments on top.
    g.setColour ( ColourPalette::thresholdLine.withAlpha ( 0.5f ) );
    g.strokePath ( curvePath,
                   juce::PathStrokeType ( 2.0f, juce::PathStrokeType::curved ) );

    // Per-band coloured segments (clipped to band bounds)
    for ( const auto& bd : bandData )
    {
        juce::Path segPath;
        bool started = false;

        for ( int i = 0; i <= steps; ++i )
        {
            float x = static_cast<float> ( i ) * w / static_cast<float> ( steps );

            if ( x < bd.x0 || x > bd.x1 )
            {
                if ( started )
                    break;
                continue;
            }

            float range = bd.x1 - bd.x0;
            float dB = 0.0f;

            if ( range > 0.0f )
            {
                float t = ( x - bd.x0 ) / range;
                float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
                dB = bd.peakDB * sinVal * sinVal;
            }

            float y = dbToY ( dB );

            if ( ! started )
            {
                segPath.startNewSubPath ( x, y );
                started = true;
            }
            else
            {
                segPath.lineTo ( x, y );
            }
        }

        g.setColour ( bd.colour.withAlpha ( 0.85f ) );
        g.strokePath ( segPath,
                       juce::PathStrokeType ( 2.5f, juce::PathStrokeType::curved ) );
    }
}

//==============================================================================
// Layer 6 - Crossover lines and handles (per-band left/right edges)
//==============================================================================

void MultibandDisplay::drawCrossovers ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    const float h = static_cast<float> ( getHeight() );

    // Draw left and right edge lines for each band
    for ( int b = 0; b < numBands; ++b )
    {
        const auto& band = bands[static_cast<size_t> ( b )];
        juce::Colour bandCol = bandColours[band.slotIndex % Constants::kMaxBands];

        for ( int edge = 0; edge < 2; ++edge )
        {
            float freq = ( edge == 0 ) ? band.leftFreqHz : band.rightFreqHz;

            // Skip edges at spectrum boundaries
            if ( freq <= Constants::kMinFrequency + 0.5f ||
                 freq >= Constants::kMaxFrequency - 0.5f )
                continue;

            // Skip left edges that are shared with the previous band's right edge
            // (the right edge of the previous band already drew this line)
            bool isShared = false;
            if ( edge == 0 && b > 0 )
            {
                float prevRight = bands[static_cast<size_t> ( b - 1 )].rightFreqHz;
                isShared = std::abs ( freq - prevRight ) < 0.5f;
            }
            if ( isShared )
                continue;

            // Check if this right edge is shared with the next band's left edge
            bool isSharedRight = false;
            if ( edge == 1 && b + 1 < numBands )
            {
                float nextLeft = bands[static_cast<size_t> ( b + 1 )].leftFreqHz;
                isSharedRight = std::abs ( freq - nextLeft ) < 0.5f;
            }

            float x = frequencyToX ( freq );

            // Encode crossover index: band * 2 + edge (0=left, 1=right)
            int crossoverIdx = b * 2 + edge;

            // For shared boundaries, also check if the paired index is hovered/dragged
            int pairedIdx = -1;
            if ( isSharedRight )
                pairedIdx = ( b + 1 ) * 2 + 0; // next band's left edge

            bool isDragging = ( draggingCrossover == crossoverIdx ) ||
                              ( pairedIdx >= 0 && draggingCrossover == pairedIdx );
            bool isHovered  = ( hoveredCrossover == crossoverIdx ) ||
                              ( pairedIdx >= 0 && hoveredCrossover == pairedIdx );

            juce::Colour lineCol = ColourPalette::crossoverLine;
            if ( isDragging )
                lineCol = bandCol.brighter ( 0.6f );
            else if ( isHovered )
                lineCol = bandCol.brighter ( 0.3f );

            g.setColour ( lineCol );

            if ( isSharedRight )
            {
                // Shared boundary: draw solid line (not dashed)
                g.drawLine ( x, 0.0f, x, h, isHovered || isDragging ? 2.0f : 1.5f );
            }
            else
            {
                // Solo edge: draw dashed line
                const float dashLengths[] = { 6.0f, 4.0f };
                g.drawDashedLine ( juce::Line<float> ( x, 0.0f, x, h ),
                                   dashLengths, 2, isHovered || isDragging ? 2.0f : 1.5f );
            }
        }
    }

    // Frequency tooltip during crossover drag
    if ( showingFreqTooltip && draggingCrossover >= 0 )
    {
        juce::String freqText = FrequencyUtils::formatFrequency ( tooltipFreq );
        g.setFont ( Fonts::getBody ( 11.0f ) );

        int textW = g.getCurrentFont().getStringWidth ( freqText ) + 10;
        int textH = 18;
        int tx = juce::roundToInt ( tooltipX ) - textW / 2;
        int ty = juce::roundToInt ( tooltipY ) - textH - 8;

        // Clamp to display bounds
        tx = juce::jlimit ( 2, getWidth() - textW - 2, tx );
        ty = juce::jmax ( 2, ty );

        g.setColour ( ColourPalette::panelTop.withAlpha ( 0.9f ) );
        g.fillRoundedRectangle ( static_cast<float> ( tx ), static_cast<float> ( ty ),
                                  static_cast<float> ( textW ), static_cast<float> ( textH ), 3.0f );
        g.setColour ( ColourPalette::textBright );
        g.drawText ( freqText, tx, ty, textW, textH, juce::Justification::centred, false );
    }

    // Range tooltip during range boundary drag
    if ( showingRangeTooltip && draggingRangeBand >= 0 )
    {
        g.setFont ( Fonts::getBody ( 11.0f ) );

        int textW = g.getCurrentFont().getStringWidth ( rangeTooltipText ) + 10;
        int textH = 18;
        int tx = juce::roundToInt ( tooltipX ) - textW / 2;
        int ty = juce::roundToInt ( tooltipY ) - textH - 8;

        tx = juce::jlimit ( 2, getWidth() - textW - 2, tx );
        ty = juce::jmax ( 2, ty );

        g.setColour ( ColourPalette::panelTop.withAlpha ( 0.9f ) );
        g.fillRoundedRectangle ( static_cast<float> ( tx ), static_cast<float> ( ty ),
                                  static_cast<float> ( textW ), static_cast<float> ( textH ), 3.0f );
        g.setColour ( ColourPalette::textBright );
        g.drawText ( rangeTooltipText, tx, ty, textW, textH, juce::Justification::centred, false );
    }
}

//==============================================================================
// Layer 9 - Band control dots (output gain handle)
//==============================================================================

juce::Point<float> MultibandDisplay::getControlDotCenter ( int bandPos ) const
{
    if ( bandManager == nullptr || processor == nullptr )
        return {};

    const auto& bands = bandManager->getActiveBands();

    if ( bandPos < 0 || bandPos >= static_cast<int> ( bands.size() ) )
        return {};

    const auto& band = bands[static_cast<size_t> ( bandPos )];
    auto params = processor->getParameterAttachment().getCurrentParameters();
    float centerFreq = std::sqrt ( band.leftFreqHz * band.rightFreqHz );
    float outputGain = params.bands[band.slotIndex].outputGain;

    return { frequencyToX ( centerFreq ), dbToY ( outputGain ) };
}

void MultibandDisplay::drawBandControlDots ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return;

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        auto center = getControlDotCenter ( b );
        juce::Colour bandCol = bandColours[slot % Constants::kMaxBands];
        float radius = kControlDotRadius;

        bool isHovered = ( hoveredControlDot == b );
        bool isDragging = ( draggingControlDot == b );

        if ( isHovered || isDragging )
            radius += 2.0f;

        // --- Filled dot ---
        // Outer circle in band colour
        g.setColour ( bandCol );
        g.fillEllipse ( center.x - radius, center.y - radius,
                        radius * 2.0f, radius * 2.0f );

        // Inner white circle
        float innerR = radius * 0.55f;
        g.setColour ( ColourPalette::controlDotInner );
        g.fillEllipse ( center.x - innerR, center.y - innerR,
                        innerR * 2.0f, innerR * 2.0f );
    }
}

//==============================================================================
// Layer 10 - S/M buttons on each band
//==============================================================================

void MultibandDisplay::getBandButtonRects ( int bandPos,
                                             juce::Rectangle<float>& soloRect,
                                             juce::Rectangle<float>& muteRect ) const
{
    if ( bandManager == nullptr )
        return;

    const auto& bands = bandManager->getActiveBands();

    if ( bandPos < 0 || bandPos >= static_cast<int> ( bands.size() ) )
        return;

    float x0 = frequencyToX ( bands[static_cast<size_t> ( bandPos )].leftFreqHz );
    float x1 = frequencyToX ( bands[static_cast<size_t> ( bandPos )].rightFreqHz );
    float bandCenterX = ( x0 + x1 ) * 0.5f;

    float totalW = kButtonWidth * 2.0f + kButtonSpacing;
    float startX = bandCenterX - totalW * 0.5f;

    soloRect = { startX, kButtonTopMargin, kButtonWidth, kButtonHeight };
    muteRect = { startX + kButtonWidth + kButtonSpacing, kButtonTopMargin,
                 kButtonWidth, kButtonHeight };
}

void MultibandDisplay::drawBandButtons ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return;

    g.setFont ( Fonts::getBold ( 9.0f ) );

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        juce::Rectangle<float> soloRect, muteRect;
        getBandButtonRects ( b, soloRect, muteRect );

        // Check if band is too narrow for buttons
        float x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );
        if ( ( x1 - x0 ) < ( kButtonWidth * 2.0f + kButtonSpacing + 4.0f ) )
            continue;

        // Read parameter state
        juce::String soloID = ParameterIDs::bandParam ( slot, ParameterIDs::Band::audition );
        juce::String enabledID = ParameterIDs::bandParam ( slot, ParameterIDs::Band::enabled );

        bool isSolo = false;
        bool isMuted = false;

        if ( auto* p = processor->getAPVTS().getParameter ( soloID ) )
            isSolo = p->getValue() > 0.5f;

        if ( auto* p = processor->getAPVTS().getParameter ( enabledID ) )
            isMuted = p->getValue() < 0.5f;

        juce::Colour bandCol = bandColours[slot % Constants::kMaxBands];

        // Solo button
        g.setColour ( ColourPalette::buttonBackground );
        g.fillRoundedRectangle ( soloRect, 3.0f );
        g.setColour ( isSolo ? bandCol : ColourPalette::buttonInactive );
        g.drawText ( "S", soloRect, juce::Justification::centred, false );
        if ( isSolo )
        {
            g.setColour ( bandCol.withAlpha ( 0.3f ) );
            g.fillRoundedRectangle ( soloRect, 3.0f );
        }

        // Mute button
        g.setColour ( ColourPalette::buttonBackground );
        g.fillRoundedRectangle ( muteRect, 3.0f );
        g.setColour ( isMuted ? ColourPalette::gainReduction : ColourPalette::buttonInactive );
        g.drawText ( "M", muteRect, juce::Justification::centred, false );
        if ( isMuted )
        {
            g.setColour ( ColourPalette::gainReduction.withAlpha ( 0.3f ) );
            g.fillRoundedRectangle ( muteRect, 3.0f );
        }
    }
}

//==============================================================================
// Layer 11 - Hover preview for band creation
//==============================================================================

void MultibandDisplay::drawHoverPreview ( juce::Graphics& g )
{
    if ( ! showingHoverPreview )
        return;

    const float h = static_cast<float> ( getHeight() );
    float x0 = frequencyToX ( hoverLeftFreq );
    float x1 = frequencyToX ( hoverRightFreq );
    float cx = frequencyToX ( hoverFreq );

    // Subtle fill
    g.setColour ( ColourPalette::hoverPreviewFill );
    g.fillRect ( x0, 0.0f, x1 - x0, h );

    // Dashed boundary lines
    g.setColour ( ColourPalette::hoverPreviewLine );
    const float dashLengths[] = { 4.0f, 4.0f };
    g.drawDashedLine ( juce::Line<float> ( x0, 0.0f, x0, h ), dashLengths, 2, 1.0f );
    g.drawDashedLine ( juce::Line<float> ( x1, 0.0f, x1, h ), dashLengths, 2, 1.0f );

    // Dot at center on the 0 dB line
    float zeroY = dbToY ( 0.0f );
    g.setColour ( ColourPalette::hoverPreviewDot );
    g.fillEllipse ( cx - 4.0f, zeroY - 4.0f, 8.0f, 8.0f );
}

//==============================================================================
// Layer 9 - Axis labels
//==============================================================================

void MultibandDisplay::drawAxisLabels ( juce::Graphics& g )
{
    g.setFont ( Fonts::getBody ( 10.0f ) );
    g.setColour ( ColourPalette::axisText );

    const float h = static_cast<float> ( getHeight() );

    // Frequency labels along the bottom
    const float freqLabels[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
                                  2000.0f, 5000.0f, 10000.0f, 20000.0f };

    for ( auto freq : freqLabels )
    {
        float x = frequencyToX ( freq );
        juce::String text = FrequencyUtils::formatFrequency ( freq );
        g.drawText ( text,
                     juce::roundToInt ( x ) - 30, juce::roundToInt ( h ) - 14,
                     60, 14,
                     juce::Justification::centred, false );
    }

    // dB labels along the left edge
    for ( float dB = minDB; dB <= maxDB; dB += 12.0f )
    {
        float y = dbToY ( dB );
        juce::String text = juce::String ( juce::roundToInt ( dB ) ) + " dB";
        g.drawText ( text,
                     2, juce::roundToInt ( y ) - 7,
                     44, 14,
                     juce::Justification::centredLeft, false );
    }
}

//==============================================================================
// Gap and hover helpers
//==============================================================================

bool MultibandDisplay::getGapAtX ( float x, float& gapLeftHz, float& gapRightHz ) const
{
    if ( bandManager == nullptr )
    {
        gapLeftHz = Constants::kMinFrequency;
        gapRightHz = Constants::kMaxFrequency;
        return true;
    }

    float freq = xToFrequency ( x );
    const auto& bands = bandManager->getActiveBands();

    // Check if inside a user band
    for ( const auto& band : bands )
    {
        if ( freq >= band.leftFreqHz && freq <= band.rightFreqHz )
            return false;
    }

    // Find bounding gap
    gapLeftHz = Constants::kMinFrequency;
    gapRightHz = Constants::kMaxFrequency;

    for ( const auto& band : bands )
    {
        if ( band.rightFreqHz <= freq )
            gapLeftHz = std::max ( gapLeftHz, band.rightFreqHz );
        if ( band.leftFreqHz >= freq )
            gapRightHz = std::min ( gapRightHz, band.leftFreqHz );
    }

    return true;
}

void MultibandDisplay::computeHoverBand ( float freq, float gapLeftHz, float gapRightHz,
                                           float& bandLeftHz, float& bandRightHz ) const
{
    float halfWidthOctaves = Constants::kDefaultBandWidthOctaves * 0.5f;
    bandLeftHz  = freq / std::pow ( 2.0f, halfWidthOctaves );
    bandRightHz = freq * std::pow ( 2.0f, halfWidthOctaves );

    // Clamp to [20Hz, 20kHz] and gap
    bandLeftHz  = std::max ( bandLeftHz,  std::max ( gapLeftHz, Constants::kMinFrequency ) );
    bandRightHz = std::min ( bandRightHz, std::min ( gapRightHz, Constants::kMaxFrequency ) );
}

//==============================================================================
// Hit testing helpers
//==============================================================================

int MultibandDisplay::crossoverHandleAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    // Check each band's left and right edge handles
    for ( int b = 0; b < numBands; ++b )
    {
        const auto& band = bands[static_cast<size_t> ( b )];

        for ( int edge = 0; edge < 2; ++edge )
        {
            float freq = ( edge == 0 ) ? band.leftFreqHz : band.rightFreqHz;

            // Skip edges at spectrum boundaries
            if ( freq <= Constants::kMinFrequency + 0.5f ||
                 freq >= Constants::kMaxFrequency - 0.5f )
                continue;

            // For shared left edges, skip — the previous band's right edge handles it
            if ( edge == 0 && b > 0 )
            {
                float prevRight = bands[static_cast<size_t> ( b - 1 )].rightFreqHz;
                if ( std::abs ( freq - prevRight ) < 0.5f )
                    continue;
            }

            float cx = frequencyToX ( freq );

            // Hit test: horizontal distance to the line across full height
            if ( std::abs ( point.x - cx ) < kHandleHitRadius )
                return b * 2 + edge; // encode as band*2 + edge
        }
    }

    return -1;
}

int MultibandDisplay::rangeBoundaryAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return -1;

    constexpr float hitTolerance = 8.0f;

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        float x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );

        if ( point.x < x0 || point.x > x1 )
            continue;

        // Compute the range boundary curve Y at the mouse's X position
        float outputCurveDB, rangeBoundaryDB;
        getBandCurveValues ( b, outputCurveDB, rangeBoundaryDB );

        float range = x1 - x0;
        if ( range <= 0.0f )
            continue;

        float t = ( point.x - x0 ) / range;
        float sinVal = std::sin ( juce::MathConstants<float>::pi * t );
        float shape = sinVal * sinVal;
        float boundaryY = dbToY ( rangeBoundaryDB * shape );

        if ( std::abs ( point.y - boundaryY ) < hitTolerance )
            return b;
    }

    return -1;
}

int MultibandDisplay::bandAtX ( float x ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    if ( numBands < 1 )
        return -1;

    for ( int b = 0; b < numBands; ++b )
    {
        float bx0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float bx1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );

        if ( x >= bx0 && x < bx1 )
            return b;
    }

    return -1;
}

int MultibandDisplay::controlDotAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        auto center = getControlDotCenter ( b );
        float dist = point.getDistanceFrom ( center );

        if ( dist < kControlDotHitRadius )
            return b;
    }

    return -1;
}

int MultibandDisplay::smButtonAtPoint ( juce::Point<float> point, bool& isSolo ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    for ( int b = 0; b < numBands; ++b )
    {
        // Skip if band is too narrow for buttons
        float x0 = frequencyToX ( bands[static_cast<size_t> ( b )].leftFreqHz );
        float x1 = frequencyToX ( bands[static_cast<size_t> ( b )].rightFreqHz );
        if ( ( x1 - x0 ) < ( kButtonWidth * 2.0f + kButtonSpacing + 4.0f ) )
            continue;

        juce::Rectangle<float> soloRect, muteRect;
        getBandButtonRects ( b, soloRect, muteRect );

        if ( soloRect.contains ( point ) )
        {
            isSolo = true;
            return b;
        }

        if ( muteRect.contains ( point ) )
        {
            isSolo = false;
            return b;
        }
    }

    return -1;
}

//==============================================================================
// Auto-scaling dB range
//==============================================================================

void MultibandDisplay::updateDisplayRange()
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const int numBands = bandManager->getNumActiveBands();
    const auto& bands = bandManager->getActiveBands();

    // Start with symmetric defaults centered on 0 dB
    float neededMin = -36.0f;
    float neededMax = 36.0f;

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[static_cast<size_t> ( b )].slotIndex;

        if ( ! params.bands[slot].enabled )
            continue;

        float range = params.bands[slot].range;
        float outputGain = params.bands[slot].outputGain;

        float rangeBoundary = outputGain + range;

        float lowestDB = std::min ( outputGain, rangeBoundary ) - 6.0f;
        float highestDB = std::max ( outputGain, rangeBoundary ) + 6.0f;

        neededMin = std::min ( neededMin, lowestDB );
        neededMax = std::max ( neededMax, highestDB );
    }

    // Keep 0 dB centered: take the larger absolute extent
    float extent = std::max ( std::abs ( neededMin ), std::abs ( neededMax ) );
    extent = juce::jlimit ( 24.0f, 90.0f, extent );
    targetMinDB = -extent;
    targetMaxDB = extent;

    // Smooth transition
    float alpha = 0.1f;
    minDB += ( targetMinDB - minDB ) * alpha;
    maxDB += ( targetMaxDB - maxDB ) * alpha;
}

//==============================================================================
// Cursor management
//==============================================================================

void MultibandDisplay::updateCursor()
{
    if ( hoveredCrossover >= 0 )
        setMouseCursor ( juce::MouseCursor::LeftRightResizeCursor );
    else if ( hoveredControlDot >= 0 || hoveredRange >= 0 )
        setMouseCursor ( juce::MouseCursor::UpDownResizeCursor );
    else if ( hoveredBand >= 0 )
        setMouseCursor ( juce::MouseCursor::PointingHandCursor );
    else if ( showingHoverPreview )
        setMouseCursor ( juce::MouseCursor::CrosshairCursor );
    else
        setMouseCursor ( juce::MouseCursor::NormalCursor );
}

//==============================================================================
// Mouse interaction
//==============================================================================

void MultibandDisplay::mouseMove ( const juce::MouseEvent& e )
{
    auto pos = e.position;

    hoveredCrossover = crossoverHandleAtPoint ( pos );
    hoveredControlDot = ( hoveredCrossover < 0 ) ? controlDotAtPoint ( pos ) : -1;
    hoveredRange = ( hoveredCrossover < 0 && hoveredControlDot < 0 ) ? rangeBoundaryAtPoint ( pos ) : -1;
    hoveredBand = ( hoveredCrossover < 0 && hoveredControlDot < 0 && hoveredRange < 0 ) ? bandAtX ( pos.x ) : -1;

    // Hover preview: show when over empty space (gap)
    showingHoverPreview = false;
    if ( hoveredCrossover < 0 && hoveredControlDot < 0 && hoveredRange < 0 && hoveredBand < 0 )
    {
        float gapLeft, gapRight;
        if ( getGapAtX ( pos.x, gapLeft, gapRight ) )
        {
            float freq = xToFrequency ( pos.x );
            float bandLeft, bandRight;
            computeHoverBand ( freq, gapLeft, gapRight, bandLeft, bandRight );

            // Only show if band would be wide enough
            float widthOctaves = std::log2 ( bandRight / bandLeft );
            if ( widthOctaves >= Constants::kMinBandWidthOctaves )
            {
                showingHoverPreview = true;
                hoverFreq = freq;
                hoverLeftFreq = bandLeft;
                hoverRightFreq = bandRight;
            }
        }
    }

    updateCursor();
    repaint();
}

void MultibandDisplay::mouseExit ( const juce::MouseEvent& )
{
    hoveredCrossover = -1;
    hoveredControlDot = -1;
    hoveredRange = -1;
    hoveredBand = -1;
    showingHoverPreview = false;
    setMouseCursor ( juce::MouseCursor::NormalCursor );
    repaint();
}

void MultibandDisplay::mouseDown ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    auto pos = e.position;

    // 1. Check crossover handles first (encoded as band*2 + edge)
    int handle = crossoverHandleAtPoint ( pos );

    if ( handle >= 0 )
    {
        draggingCrossover = handle;
        draggingLeftEdge = ( handle % 2 == 0 );
        showingFreqTooltip = true;
        showingHoverPreview = false;
        return;
    }

    // 2. Check S/M buttons
    bool isSoloBtn = false;
    int btnBand = smButtonAtPoint ( pos, isSoloBtn );

    if ( btnBand >= 0 )
    {
        const auto& bands = bandManager->getActiveBands();
        int slot = bands[static_cast<size_t> ( btnBand )].slotIndex;

        juce::String paramID = isSoloBtn
            ? ParameterIDs::bandParam ( slot, ParameterIDs::Band::audition )
            : ParameterIDs::bandParam ( slot, ParameterIDs::Band::enabled );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
        {
            float current = param->getValue();
            param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
        }

        selectedBand = btnBand;

        if ( onBandSelected )
            onBandSelected ( selectedBand );

        repaint();
        return;
    }

    // 3. Check control dot (drag to change output gain)
    int dot = controlDotAtPoint ( pos );

    if ( dot >= 0 )
    {
        draggingControlDot = dot;
        selectedBand = dot;
        showingHoverPreview = false;

        if ( onBandSelected )
            onBandSelected ( selectedBand );

        return;
    }

    // 4. Check range boundary curve
    int rangeHit = rangeBoundaryAtPoint ( pos );

    if ( rangeHit >= 0 )
    {
        draggingRangeBand = rangeHit;
        selectedBand = rangeHit;
        showingHoverPreview = false;
        showingRangeTooltip = true;

        if ( onBandSelected )
            onBandSelected ( selectedBand );

        return;
    }

    // Right-click context menu on band region
    if ( e.mods.isPopupMenu() )
    {
        int band = bandAtX ( pos.x );
        if ( band >= 0 )
            showBandContextMenu ( band );
        return;
    }

    // Left-click: select band (no toggle — panel only closes via close button or delete)
    int band = bandAtX ( pos.x );

    if ( band >= 0 )
    {
        selectedBand = band;

        if ( onBandSelected )
            onBandSelected ( selectedBand );

        return;
    }

    // Click on empty space: create a new band
    if ( bandManager != nullptr )
    {
        float clickFreq = xToFrequency ( pos.x );
        int newSlot = bandManager->addBand ( clickFreq );

        if ( newSlot >= 0 )
        {
            const auto& bands = bandManager->getActiveBands();

            for ( int i = 0; i < static_cast<int> ( bands.size() ); ++i )
            {
                if ( bands[static_cast<size_t> ( i )].slotIndex == newSlot )
                {
                    selectedBand = i;

                    if ( onBandCreated )
                        onBandCreated ( i );

                    break;
                }
            }

            showingHoverPreview = false;
            repaint();
        }
    }
}

void MultibandDisplay::mouseDrag ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    // Crossover dragging (per-band edge)
    if ( draggingCrossover >= 0 )
    {
        int bandPos = draggingCrossover / 2;
        bool isLeft = ( draggingCrossover % 2 == 0 );

        float newFreq = xToFrequency ( e.position.x );
        newFreq = juce::jlimit ( Constants::kMinFrequency, Constants::kMaxFrequency, newFreq );

        // Update band edge via BandManager (handles clamping)
        if ( bandManager != nullptr )
            bandManager->setBandEdge ( bandPos, isLeft, newFreq );

        tooltipFreq = newFreq;
        tooltipX = e.position.x;
        tooltipY = e.position.y;
        showingFreqTooltip = true;

        repaint();
        return;
    }

    // Control dot dragging (Y-axis → output gain)
    if ( draggingControlDot >= 0 )
    {
        float newDB = yToDb ( e.position.y );
        newDB = juce::jlimit ( Constants::kMinOutputGain, Constants::kMaxOutputGain, newDB );

        int slot = draggingControlDot;

        if ( bandManager != nullptr )
        {
            const auto& bands = bandManager->getActiveBands();

            if ( draggingControlDot < static_cast<int> ( bands.size() ) )
                slot = bands[static_cast<size_t> ( draggingControlDot )].slotIndex;
        }

        juce::String paramID = ParameterIDs::bandParam (
            slot, ParameterIDs::Band::outputGain );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
            param->setValueNotifyingHost ( param->convertTo0to1 ( newDB ) );

        repaint();
        return;
    }

    // Range boundary dragging
    if ( draggingRangeBand >= 0 )
    {
        int slot = draggingRangeBand;

        if ( bandManager != nullptr )
        {
            const auto& bands = bandManager->getActiveBands();

            if ( draggingRangeBand < static_cast<int> ( bands.size() ) )
                slot = bands[static_cast<size_t> ( draggingRangeBand )].slotIndex;
        }

        auto params = processor->getParameterAttachment().getCurrentParameters();

        float outDB = params.bands[slot].outputGain;

        // Mouse Y → dB at band centre (the boundary peak follows sin² shape,
        // but the user drags near the centre so we treat it as the peak value)
        float mouseDB = yToDb ( e.position.y );

        // Signed range: drag above dot → positive, below → negative
        float newRange = mouseDB - outDB;
        newRange = juce::jlimit ( Constants::kMinRange, Constants::kMaxRange, newRange );

        juce::String paramID = ParameterIDs::bandParam (
            slot, ParameterIDs::Band::range );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
            param->setValueNotifyingHost ( param->convertTo0to1 ( newRange ) );

        // Update tooltip
        juce::String prefix = ( newRange >= 0.0f ) ? "+" : "";
        rangeTooltipText = "Range: " + prefix + juce::String ( newRange, 1 ) + " dB";
        tooltipX = e.position.x;
        tooltipY = e.position.y;
        showingRangeTooltip = true;

        repaint();
    }
}

void MultibandDisplay::mouseUp ( const juce::MouseEvent& )
{
    draggingCrossover = -1;
    draggingControlDot = -1;
    draggingRangeBand = -1;
    showingFreqTooltip = false;
    showingRangeTooltip = false;
    repaint();
}

//==============================================================================
// Context menu
//==============================================================================

void MultibandDisplay::showBandContextMenu ( int bandPosition )
{
    if ( processor == nullptr )
        return;

    int slot = bandPosition;

    if ( bandManager != nullptr )
    {
        const auto& bands = bandManager->getActiveBands();

        if ( bandPosition < static_cast<int> ( bands.size() ) )
            slot = bands[static_cast<size_t> ( bandPosition )].slotIndex;
    }

    auto params = processor->getParameterAttachment().getCurrentParameters();
    bool enabled = params.bands[slot].enabled;

    juce::PopupMenu menu;
    menu.addItem ( 1, enabled ? "Disable Band" : "Enable Band" );
    menu.addItem ( 2, "Audition Band" );

    menu.showMenuAsync ( juce::PopupMenu::Options(),
        [this, slot] ( int result )
        {
            if ( result == 1 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    slot, ParameterIDs::Band::enabled );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
            else if ( result == 2 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    slot, ParameterIDs::Band::audition );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
        } );
}
