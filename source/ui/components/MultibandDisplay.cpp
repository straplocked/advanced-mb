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
    drawGainReduction ( g );
    drawCrossovers ( g );
    drawThresholds ( g );
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

    const float w = static_cast<float> ( getWidth() );
    const float h = static_cast<float> ( getHeight() );
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
    if ( processor == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numBands = params.numBands;

    if ( numBands < 1 )
        return;

    const float w = static_cast<float> ( getWidth() );
    const float h = static_cast<float> ( getHeight() );

    // Collect crossover x positions
    std::vector<float> xPositions;
    xPositions.push_back ( 0.0f );

    for ( int i = 0; i < numBands - 1; ++i )
        xPositions.push_back ( frequencyToX ( params.crossovers[i].frequency ) );

    xPositions.push_back ( w );

    for ( int b = 0; b < numBands; ++b )
    {
        float x0 = xPositions[static_cast<size_t> ( b )];
        float x1 = xPositions[static_cast<size_t> ( b + 1 )];

        juce::Colour fill = bandFillColours[b % Constants::kMaxBands];

        if ( b == selectedBand )
            fill = fill.brighter ( 0.3f );

        g.setColour ( fill );
        g.fillRect ( x0, 0.0f, x1 - x0, h );
    }
}

//==============================================================================
// Layer 5 - Gain reduction bars
//==============================================================================

void MultibandDisplay::drawGainReduction ( juce::Graphics& g )
{
    if ( processor == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numBands = params.numBands;

    if ( numBands < 1 )
        return;

    const float w = static_cast<float> ( getWidth() );
    const float h = static_cast<float> ( getHeight() );
    const float zeroY = dbToY ( 0.0f );

    std::vector<float> xPositions;
    xPositions.push_back ( 0.0f );

    for ( int i = 0; i < numBands - 1; ++i )
        xPositions.push_back ( frequencyToX ( params.crossovers[i].frequency ) );

    xPositions.push_back ( w );

    for ( int b = 0; b < numBands; ++b )
    {
        float grDB = processor->getBandGainReduction ( b ); // negative value in dB
        if ( std::abs ( grDB ) < 0.1f )
            continue;

        float grY = dbToY ( grDB );

        float x0 = xPositions[static_cast<size_t> ( b )];
        float x1 = xPositions[static_cast<size_t> ( b + 1 )];
        float centerX = ( x0 + x1 ) * 0.5f;
        float barWidth = juce::jmin ( ( x1 - x0 ) * 0.5f, 40.0f );

        // Fill
        g.setColour ( ColourPalette::gainReductionFill );
        g.fillRect ( centerX - barWidth * 0.5f, zeroY,
                     barWidth, grY - zeroY );

        // Outline
        g.setColour ( ColourPalette::gainReduction );
        g.drawRect ( centerX - barWidth * 0.5f, zeroY,
                     barWidth, grY - zeroY, 1.0f );
    }
}

//==============================================================================
// Layer 6 - Crossover lines and handles
//==============================================================================

void MultibandDisplay::drawCrossovers ( juce::Graphics& g )
{
    if ( processor == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numCrossovers = params.numBands - 1;

    const float h = static_cast<float> ( getHeight() );

    for ( int i = 0; i < numCrossovers; ++i )
    {
        float x = frequencyToX ( params.crossovers[i].frequency );

        // Dashed vertical line
        g.setColour ( ColourPalette::crossoverLine );
        const float dashLengths[] = { 6.0f, 4.0f };
        g.drawDashedLine ( juce::Line<float> ( x, 0.0f, x, h ),
                           dashLengths, 2, 1.5f );

        // Handle circle at top - brighten when hovered or dragged
        bool isDragging = ( draggingCrossover == i );
        bool isHovered  = ( hoveredCrossover == i );
        juce::Colour handleCol = ColourPalette::crossoverHandle;

        if ( isDragging )
            handleCol = handleCol.brighter ( 0.4f );
        else if ( isHovered )
            handleCol = handleCol.brighter ( 0.3f );

        g.setColour ( handleCol );
        g.fillEllipse ( x - kHandleRadius, kHandleRadius * 0.5f,
                        kHandleRadius * 2.0f, kHandleRadius * 2.0f );
        g.setColour ( ColourPalette::curveArea );
        g.fillEllipse ( x - kHandleRadius + 2.0f, kHandleRadius * 0.5f + 2.0f,
                        kHandleRadius * 2.0f - 4.0f, kHandleRadius * 2.0f - 4.0f );
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
}

//==============================================================================
// Layer 7 - Threshold lines per band
//==============================================================================

void MultibandDisplay::drawThresholds ( juce::Graphics& g )
{
    if ( processor == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numBands = params.numBands;

    if ( numBands < 1 )
        return;

    const float w = static_cast<float> ( getWidth() );

    std::vector<float> xPositions;
    xPositions.push_back ( 0.0f );

    for ( int i = 0; i < numBands - 1; ++i )
        xPositions.push_back ( frequencyToX ( params.crossovers[i].frequency ) );

    xPositions.push_back ( w );

    for ( int b = 0; b < numBands; ++b )
    {
        if ( ! params.bands[b].enabled )
            continue;

        float threshY = dbToY ( params.bands[b].threshold );
        float x0 = xPositions[static_cast<size_t> ( b )];
        float x1 = xPositions[static_cast<size_t> ( b + 1 )];

        g.setColour ( ( b == selectedBand ) ? ColourPalette::thresholdAccent
                                            : ColourPalette::thresholdLine );
        g.drawLine ( x0, threshY, x1, threshY, 1.5f );
    }
}

//==============================================================================
// Layer 8 - Axis labels
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
// Hit testing helpers
//==============================================================================

int MultibandDisplay::crossoverHandleAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numCrossovers = params.numBands - 1;

    for ( int i = 0; i < numCrossovers; ++i )
    {
        float cx = frequencyToX ( params.crossovers[i].frequency );
        float cy = kHandleRadius * 0.5f + kHandleRadius; // centre of handle circle

        float dx = point.x - cx;
        float dy = point.y - cy;

        if ( ( dx * dx + dy * dy ) < ( kHandleHitRadius * kHandleHitRadius ) )
            return i;
    }

    return -1;
}

int MultibandDisplay::thresholdAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numBands = params.numBands;

    if ( numBands < 1 )
        return -1;

    const float w = static_cast<float> ( getWidth() );
    constexpr float hitTolerance = 8.0f;

    std::vector<float> xPositions;
    xPositions.push_back ( 0.0f );

    for ( int i = 0; i < numBands - 1; ++i )
        xPositions.push_back ( frequencyToX ( params.crossovers[i].frequency ) );

    xPositions.push_back ( w );

    for ( int b = 0; b < numBands; ++b )
    {
        if ( ! params.bands[b].enabled )
            continue;

        float x0 = xPositions[static_cast<size_t> ( b )];
        float x1 = xPositions[static_cast<size_t> ( b + 1 )];

        if ( point.x < x0 || point.x > x1 )
            continue;

        float threshY = dbToY ( params.bands[b].threshold );

        if ( std::abs ( point.y - threshY ) < hitTolerance )
            return b;
    }

    return -1;
}

int MultibandDisplay::bandAtX ( float x ) const
{
    if ( processor == nullptr )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    int numBands = params.numBands;

    if ( numBands < 1 )
        return -1;

    const float w = static_cast<float> ( getWidth() );

    std::vector<float> xPositions;
    xPositions.push_back ( 0.0f );

    for ( int i = 0; i < numBands - 1; ++i )
        xPositions.push_back ( frequencyToX ( params.crossovers[i].frequency ) );

    xPositions.push_back ( w );

    for ( int b = 0; b < numBands; ++b )
    {
        if ( x >= xPositions[static_cast<size_t> ( b )]
             && x < xPositions[static_cast<size_t> ( b + 1 )] )
            return b;
    }

    return -1;
}

//==============================================================================
// Cursor management
//==============================================================================

void MultibandDisplay::updateCursor()
{
    if ( hoveredCrossover >= 0 )
        setMouseCursor ( juce::MouseCursor::LeftRightResizeCursor );
    else if ( hoveredThreshold >= 0 )
        setMouseCursor ( juce::MouseCursor::UpDownResizeCursor );
    else if ( hoveredBand >= 0 )
        setMouseCursor ( juce::MouseCursor::PointingHandCursor );
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
    hoveredThreshold = ( hoveredCrossover < 0 ) ? thresholdAtPoint ( pos ) : -1;
    hoveredBand = ( hoveredCrossover < 0 && hoveredThreshold < 0 ) ? bandAtX ( pos.x ) : -1;

    updateCursor();
    repaint();
}

void MultibandDisplay::mouseExit ( const juce::MouseEvent& )
{
    hoveredCrossover = -1;
    hoveredThreshold = -1;
    hoveredBand = -1;
    setMouseCursor ( juce::MouseCursor::NormalCursor );
    repaint();
}

void MultibandDisplay::mouseDown ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    auto pos = e.position;

    // Check crossover handles first
    int handle = crossoverHandleAtPoint ( pos );

    if ( handle >= 0 )
    {
        draggingCrossover = handle;
        showingFreqTooltip = true;
        return;
    }

    // Check threshold lines
    int thresh = thresholdAtPoint ( pos );

    if ( thresh >= 0 )
    {
        draggingThresholdBand = thresh;
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

    // Left-click: toggle selection (click same band to deselect)
    int band = bandAtX ( pos.x );

    if ( band >= 0 )
    {
        if ( band == selectedBand )
        {
            selectedBand = -1;

            if ( onBandDeselected )
                onBandDeselected();

            return;
        }

        selectedBand = band;

        if ( onBandSelected )
            onBandSelected ( selectedBand );
    }
}

void MultibandDisplay::mouseDrag ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    // Crossover dragging
    if ( draggingCrossover >= 0 )
    {
        float newFreq = xToFrequency ( e.position.x );
        newFreq = juce::jlimit ( Constants::kMinFrequency, Constants::kMaxFrequency, newFreq );

        juce::String paramID = ParameterIDs::crossoverParam (
            draggingCrossover, ParameterIDs::Crossover::frequency );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
            param->setValueNotifyingHost ( param->convertTo0to1 ( newFreq ) );

        tooltipFreq = newFreq;
        tooltipX = e.position.x;
        tooltipY = e.position.y;
        showingFreqTooltip = true;

        repaint();
        return;
    }

    // Threshold dragging
    if ( draggingThresholdBand >= 0 )
    {
        float newDB = yToDb ( e.position.y );
        newDB = juce::jlimit ( Constants::kMinThreshold, Constants::kMaxThreshold, newDB );

        juce::String paramID = ParameterIDs::bandParam (
            draggingThresholdBand, ParameterIDs::Band::threshold );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
            param->setValueNotifyingHost ( param->convertTo0to1 ( newDB ) );

        repaint();
    }
}

void MultibandDisplay::mouseUp ( const juce::MouseEvent& )
{
    draggingCrossover = -1;
    draggingThresholdBand = -1;
    showingFreqTooltip = false;
    repaint();
}

//==============================================================================
// Context menu
//==============================================================================

void MultibandDisplay::showBandContextMenu ( int bandIndex )
{
    if ( processor == nullptr )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    bool enabled   = params.bands[bandIndex].enabled;

    juce::PopupMenu menu;
    menu.addItem ( 1, enabled ? "Disable Band" : "Enable Band" );
    menu.addItem ( 2, "Audition Band" );

    menu.showMenuAsync ( juce::PopupMenu::Options(),
        [this, bandIndex] ( int result )
        {
            if ( result == 1 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    bandIndex, ParameterIDs::Band::enabled );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
            else if ( result == 2 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    bandIndex, ParameterIDs::Band::audition );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
        } );
}
