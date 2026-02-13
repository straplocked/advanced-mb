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
 * @brief Global controls panel displayed at the bottom of the editor.
 *
 * Provides Processing Mode, Oversampling, Analyzer Mode combo boxes plus
 * Global Output and Mix rotary knobs. All controls are wired to the APVTS
 * via standard JUCE attachments.
 *
 * Height: Constants::kBottomStripHeight (120px)
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

    // ========================= Combo boxes =========================
    juce::ComboBox processingModeCombo;
    juce::ComboBox oversamplingCombo;
    juce::ComboBox analyzerModeCombo;

    juce::Label processingModeLabel;
    juce::Label oversamplingLabel;
    juce::Label analyzerModeLabel;

    // ========================= Knobs =========================
    juce::Slider globalOutputSlider;
    juce::Slider mixSlider;

    juce::Label globalOutputTitle;
    juce::Label mixTitle;
    juce::Label globalOutputValue;
    juce::Label mixValue;

    // ========================= APVTS Attachments =========================
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processingModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> analyzerModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   globalOutputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mixAttach;

    void updateValueLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( BottomControlStrip )
};
