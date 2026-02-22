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

#include "LevelMeterComponent.h"
#include "../LookAndFeel/Fonts.h"

LevelMeterComponent::LevelMeterComponent()
{
    startTimerHz ( Constants::kUITimerHz ); // 30 Hz
}

LevelMeterComponent::~LevelMeterComponent()
{
    stopTimer();
}

void LevelMeterComponent::setLevels ( float peakL, float peakR, float rmsL, float rmsR )
{
    targetPeakL = peakL;
    targetPeakR = peakR;
    targetRmsL  = rmsL;
    targetRmsR  = rmsR;

    // Update peak hold (only rises, decay handled in timerCallback)
    if ( peakL > peakHoldL )
        peakHoldL = peakL;
    if ( peakR > peakHoldR )
        peakHoldR = peakR;
}

void LevelMeterComponent::timerCallback()
{
    // Smooth levels toward targets
    auto smoothLevel = [] ( float current, float target, float decay )
    {
        if ( target > current )
            return target;               // instant rise
        return current * decay + target * ( 1.0f - decay );  // slow decay
    };

    peakLevelL = smoothLevel ( peakLevelL, targetPeakL, kLevelDecay );
    peakLevelR = smoothLevel ( peakLevelR, targetPeakR, kLevelDecay );
    rmsLevelL  = smoothLevel ( rmsLevelL,  targetRmsL,  kLevelDecay );
    rmsLevelR  = smoothLevel ( rmsLevelR,  targetRmsR,  kLevelDecay );

    // Decay peak hold
    peakHoldL -= kPeakDecayRate;
    peakHoldR -= kPeakDecayRate;

    if ( peakHoldL < kMinDB )
        peakHoldL = kMinDB;
    if ( peakHoldR < kMinDB )
        peakHoldR = kMinDB;

    repaint();
}

float LevelMeterComponent::levelToY ( float dB, float meterHeight ) const
{
    float normalized = ( dB - kMinDB ) / ( kMaxDB - kMinDB );
    return meterHeight * ( 1.0f - juce::jlimit ( 0.0f, 1.0f, normalized ) );
}

void LevelMeterComponent::paint ( juce::Graphics& g )
{
    auto bounds = getLocalBounds().toFloat();
    float labelHeight = meterLabel.isNotEmpty() ? 12.0f : 0.0f;

    // Draw label at top if present
    if ( meterLabel.isNotEmpty() )
    {
        g.setColour ( ColourPalette::axisText );
        g.setFont ( Fonts::getBody ( 10.0f ) );
        g.drawText ( meterLabel, bounds.removeFromTop ( labelHeight ),
                     juce::Justification::centred, false );
    }

    // Meter area (two bars with 2px gap between them)
    float barGap = 2.0f;
    float meterWidth = ( bounds.getWidth() - barGap ) * 0.5f;
    float h = bounds.getHeight();
    float meterTop = bounds.getY();

    // Background
    g.setColour ( ColourPalette::curveArea );
    g.fillRoundedRectangle ( bounds, 2.0f );

    float leftX  = bounds.getX();
    float rightX = bounds.getX() + meterWidth + barGap;

    if ( meterMode == MeterMode::GainReduction )
    {
        // ---------- GR bar rendering (grows downward from top) ----------
        auto drawGRBar = [&] ( float x, float w, float dB )
        {
            // dB is negative (e.g. -12). Normalize: 0 dB → 0 fill, kGRMinDB → full fill.
            float normalized = juce::jlimit ( 0.0f, 1.0f, dB / kGRMinDB );
            float barH = h * normalized;
            g.setColour ( ColourPalette::gainReduction );
            g.fillRect ( x, meterTop, w, barH );
        };

        drawGRBar ( leftX,  meterWidth, peakLevelL );
        drawGRBar ( rightX, meterWidth, peakLevelR );
    }
    else
    {
        // ---------- Peak bar rendering ----------
        auto drawPeakBar = [&] ( float x, float w, float dB )
        {
            float meterY = levelToY ( dB, h );

            // Gradient: green (bottom) -> yellow (mid) -> red (top)
            juce::ColourGradient gradient ( ColourPalette::meterGreen, x, meterTop + h,
                                            ColourPalette::meterRed, x, meterTop, false );
            gradient.addColour ( 0.6, ColourPalette::meterYellow );
            g.setGradientFill ( gradient );
            g.fillRect ( x, meterTop + meterY, w, h - meterY );
        };

        drawPeakBar ( leftX,  meterWidth, peakLevelL );
        drawPeakBar ( rightX, meterWidth, peakLevelR );

        // ---------- RMS overlay (narrower, semi-transparent) ----------
        auto drawRmsBar = [&] ( float x, float w, float dB )
        {
            float rmsInset = w * 0.2f;
            float rmsW = w - rmsInset * 2.0f;
            float rmsX = x + rmsInset;
            float meterY = levelToY ( dB, h );

            g.setColour ( juce::Colours::white.withAlpha ( 0.15f ) );
            g.fillRect ( rmsX, meterTop + meterY, rmsW, h - meterY );
        };

        drawRmsBar ( leftX,  meterWidth, rmsLevelL );
        drawRmsBar ( rightX, meterWidth, rmsLevelR );

        // ---------- Peak hold indicators ----------
        auto drawPeakHold = [&] ( float x, float w, float peak )
        {
            if ( peak > kMinDB + 1.0f )
            {
                float peakY = levelToY ( peak, h );
                g.setColour ( ColourPalette::textBright );
                g.fillRect ( x, meterTop + peakY, w, 1.0f );
            }
        };

        drawPeakHold ( leftX,  meterWidth, peakHoldL );
        drawPeakHold ( rightX, meterWidth, peakHoldR );
    }

    // Border
    g.setColour ( ColourPalette::gridLine );
    g.drawRoundedRectangle ( bounds, 2.0f, 0.5f );
}
