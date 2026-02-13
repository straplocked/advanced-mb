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

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/**
 * @brief Manages factory and user presets for the multiband compressor.
 *
 * Presets are stored as XML files under a platform-specific "AdvancedMB"
 * application data directory.  Factory presets are compiled-in; user
 * presets live on disk and are rescanned on construction and after
 * save / delete operations.
 */
class PresetManager
{
public:
    explicit PresetManager ( juce::AudioProcessorValueTreeState& apvts );

    //==============================================================================
    // Navigation

    void savePreset ( const juce::String& name );
    void loadPreset ( const juce::String& name );
    void deletePreset ( const juce::String& name );

    juce::StringArray getPresetList() const;
    juce::String getCurrentPresetName() const;

    void loadNextPreset();
    void loadPreviousPreset();

    //==============================================================================
    // Factory preset helpers

    juce::StringArray getFactoryCategories() const;
    juce::StringArray getFactoryPresetsInCategory ( const juce::String& category ) const;
    void loadFactoryPreset ( const juce::String& name );

    //==============================================================================
    // Listener interface

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void presetChanged() = 0;
    };

    void addListener ( Listener* l )    { listeners.add ( l ); }
    void removeListener ( Listener* l ) { listeners.remove ( l ); }

    //==============================================================================
    // Directory access

    juce::File getUserPresetsDirectory() const;

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::StringArray presetList;
    juce::String currentPresetName { "Init" };

    juce::ListenerList<Listener> listeners;

    void rebuildPresetList();
    void saveStateToFile ( const juce::File& file );
    bool loadStateFromFile ( const juce::File& file );
    void applyFactoryPresetValues ( const juce::String& name );
    void notifyListeners();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( PresetManager )
};
