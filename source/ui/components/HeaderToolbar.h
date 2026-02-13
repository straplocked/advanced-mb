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
#include "../LookAndFeel/Fonts.h"
#include "../../presets/PresetManager.h"

class AdvancedMBProcessor;

class HeaderToolbar : public juce::Component,
                      private PresetManager::Listener
{
public:
    explicit HeaderToolbar ( AdvancedMBProcessor& processor );
    ~HeaderToolbar() override;

    void paint ( juce::Graphics& g ) override;
    void resized() override;

    /** Called when the preset name button is clicked (e.g. to show a preset menu). */
    std::function<void()> onPresetNameClicked;

    /** Called when the help button is clicked. */
    std::function<void()> onHelpClicked;

private:
    void presetChanged() override;
    void updatePresetName();

    AdvancedMBProcessor& processor;
    PresetManager& presetManager;

    // Undo / Redo (Unicode icon buttons)
    juce::TextButton undoButton { juce::String::charToString ( 0x21A9 ) };
    juce::TextButton redoButton { juce::String::charToString ( 0x21AA ) };

    // Preset navigation
    juce::TextButton prevButton { juce::String::charToString ( 0x25C0 ) };
    juce::TextButton nextButton { juce::String::charToString ( 0x25B6 ) };
    juce::TextButton presetNameButton { "Init" };
    juce::TextButton saveButton { juce::String::charToString ( 0x2913 ) }; // downward arrow

    // A/B comparison
    juce::TextButton abButton { "A" };

    // MIDI Learn toggle
    juce::TextButton midiLearnButton { "MIDI" };

    // Help
    juce::TextButton helpButton { "?" };

    void setupFlatButton ( juce::TextButton& button, const juce::String& componentID,
                           juce::Colour textColour );
    void showSaveDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( HeaderToolbar )
};
