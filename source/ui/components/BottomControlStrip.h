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
#include <juce_audio_processors/juce_audio_processors.h>
#include "../LookAndFeel/ColourPalette.h"
#include "../LookAndFeel/Fonts.h"
#include "../../parameters/ParameterIDs.h"

/**
 * @brief Compact global controls strip at the bottom of the editor.
 *
 * Single-row, 28px tall inline text strip inspired by FabFilter Pro-MB.
 * [Dynamic Phase] [Oversampling: Off] [Analyzer: Pre+Post] | [Mix 100%] [Output 0.0 dB]
 */
class BottomControlStrip : public juce::Component
{
public:
    explicit BottomControlStrip ( juce::AudioProcessorValueTreeState& apvts );
    ~BottomControlStrip() override;

    void paint ( juce::Graphics& g ) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    // Inline combo boxes
    juce::ComboBox processingModeCombo;
    juce::ComboBox oversamplingCombo;
    juce::ComboBox analyzerModeCombo;

    // Prefix labels
    juce::Label oversamplingLabel;
    juce::Label analyzerLabel;
    juce::Label outputLabel;
    juce::Label mixLabel;

    // Interactive sliders (LinearBar, transparent = text only)
    juce::Slider globalOutputSlider;
    juce::Slider mixSlider;

    // APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processingModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> analyzerModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   globalOutputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( BottomControlStrip )
};
