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

#include <juce_graphics/juce_graphics.h>
#include "BinaryData.h"

/**
 * @brief Centralized font management for embedded typefaces.
 *
 * Loads Space Grotesk (body) and Audiowide (title) from binary data
 * and provides convenience accessors used throughout the UI.
 */
class Fonts
{
public:
    static juce::Typeface::Ptr getSpaceGroteskTypeface()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::SpaceGroteskRegular_ttf,
            BinaryData::SpaceGroteskRegular_ttfSize );
        return typeface;
    }

    static juce::Typeface::Ptr getAudiowideTypeface()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AudiowideRegular_ttf,
            BinaryData::AudiowideRegular_ttfSize );
        return typeface;
    }

    /** Body font (Space Grotesk) at the given size. */
    static juce::Font getBody ( float height )
    {
        return juce::Font ( getSpaceGroteskTypeface() ).withHeight ( height );
    }

    /** Bold body font (Space Grotesk, synthetic bold). */
    static juce::Font getBold ( float height )
    {
        return juce::Font ( getSpaceGroteskTypeface() ).withHeight ( height ).boldened();
    }

    /** Title/branding font (Audiowide) at the given size. */
    static juce::Font getTitle ( float height )
    {
        return juce::Font ( getAudiowideTypeface() ).withHeight ( height );
    }
};
