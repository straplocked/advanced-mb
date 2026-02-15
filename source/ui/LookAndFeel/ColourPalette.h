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

namespace ColourPalette
{
    // =========================================================================
    // Background / Grid
    // =========================================================================
    inline const juce::Colour background       { 0xff040408 };
    inline const juce::Colour curveArea         { 0xff0e0e18 };
    inline const juce::Colour gridLine          { 0xff141420 };
    inline const juce::Colour gridLineStrong    { 0xff1e1e2c };
    inline const juce::Colour axisText          { 0xff6a7a6a };

    // =========================================================================
    // Band colours
    // =========================================================================
    inline const juce::Colour band1             { 0xff4a90d9 };   // Blue
    inline const juce::Colour band2             { 0xff5ec462 };   // Green
    inline const juce::Colour band3             { 0xffe8c33a };   // Yellow
    inline const juce::Colour band4             { 0xffdf7c3a };   // Orange
    inline const juce::Colour band5             { 0xffe04848 };   // Red
    inline const juce::Colour band6             { 0xffa85ec4 };   // Purple

    // Per-band fill colours (15% alpha versions)
    inline const juce::Colour band1Fill         { 0x264a90d9 };
    inline const juce::Colour band2Fill         { 0x265ec462 };
    inline const juce::Colour band3Fill         { 0x26e8c33a };
    inline const juce::Colour band4Fill         { 0x26df7c3a };
    inline const juce::Colour band5Fill         { 0x26e04848 };
    inline const juce::Colour band6Fill         { 0x26a85ec4 };

    // =========================================================================
    // Crossover
    // =========================================================================
    inline const juce::Colour crossoverHandle   { 0xffe8e8e8 };
    inline const juce::Colour crossoverLine     { 0x80e8e8e8 };

    // =========================================================================
    // Compressor-specific colours
    // =========================================================================
    inline const juce::Colour gainReduction     { 0xffff4444 };
    inline const juce::Colour gainReductionFill { 0x40ff4444 };
    inline const juce::Colour thresholdLine     { 0xff7aff3a };
    inline const juce::Colour thresholdAccent   { 0xff66cc33 };
    inline const juce::Colour meterGreen        { 0xff4caf50 };
    inline const juce::Colour meterYellow       { 0xffffeb3b };
    inline const juce::Colour meterRed          { 0xffff5252 };
    inline const juce::Colour kneeRegion        { 0x207aff3a };
    inline const juce::Colour operatingPoint    { 0xffffffff };
    inline const juce::Colour transferCurve     { 0xff7aff3a };

    // =========================================================================
    // UI Elements
    // =========================================================================
    inline const juce::Colour controlBar        { 0xff040408 };
    inline const juce::Colour presetBar         { 0xff080810 };
    inline const juce::Colour knobBackground    { 0xff141422 };
    inline const juce::Colour knobForeground    { 0xffbbbbcc };
    inline const juce::Colour textBright        { 0xffe8e8e8 };
    inline const juce::Colour textDim           { 0xff5a6a5a };

    // =========================================================================
    // Panel gradients
    // =========================================================================
    inline const juce::Colour panelTop          { 0xff0a0a14 };
    inline const juce::Colour panelBottom       { 0xff030306 };

    // =========================================================================
    // Bevels
    // =========================================================================
    inline const juce::Colour bevelLight        { 0x14ffffff };
    inline const juce::Colour bevelDark         { 0x38000000 };
    inline const juce::Colour screenShadow      { 0x58000000 };
    inline const juce::Colour screenHighlight   { 0x06ffffff };

    // =========================================================================
    // Knob metallic
    // =========================================================================
    inline const juce::Colour knobEdge          { 0xff1e1e34 };
    inline const juce::Colour knobShadow        { 0x40020204 };
    inline const juce::Colour knobIndicator     { 0xff7aff3a };

    // =========================================================================
    // Hover preview (band creation)
    // =========================================================================
    inline const juce::Colour hoverPreviewLine  { 0x80e8e8e8 };   // dashed boundary lines
    inline const juce::Colour hoverPreviewDot   { 0xffe8e8e8 };   // dot on threshold line
    inline const juce::Colour hoverPreviewFill  { 0x14ffffff };   // subtle fill

    // =========================================================================
    // Spectrum
    // =========================================================================
    inline const juce::Colour spectrumPre       { 0x30667766 };
    inline const juce::Colour spectrumPost      { 0x507aff3a };

    // =========================================================================
    // Sidechain ring meter gradient (green -> yellow -> red)
    // =========================================================================
    inline const juce::Colour ringMeterLow      { 0xff4caf50 };
    inline const juce::Colour ringMeterMid      { 0xffffeb3b };
    inline const juce::Colour ringMeterHigh     { 0xffff5252 };

    // =========================================================================
    // Band control dots & buttons
    // =========================================================================
    inline const juce::Colour controlDotInner    { 0xffffffff };   // white inner circle
    inline const juce::Colour buttonActive       { 0xddffffff };   // S/M when toggled on
    inline const juce::Colour buttonInactive     { 0x60ffffff };   // S/M when off
    inline const juce::Colour buttonBackground   { 0x30000000 };   // button bg
    inline const juce::Colour rangeBoundary      { 0x80ffffff };   // thin line at range edge

    // =========================================================================
    // Floating overlay panel
    // =========================================================================
    inline const juce::Colour overlayBackground  { 0xe00a0a16 };   // ~88% opaque dark
    inline const juce::Colour overlayBorder      { 0x30ffffff };
    inline const juce::Colour overlayShadow      { 0x60000000 };
    inline const juce::Colour expertToggleNormal { 0x80ffffff };
    inline const juce::Colour expertToggleActive { 0xddffffff };
}
