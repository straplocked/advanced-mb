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
#include "../../utils/Constants.h"
#include "../../parameters/ParameterIDs.h"
#include <functional>
#include <memory>

/**
 * @brief Floating overlay panel that exposes per-band compressor parameters.
 *
 * Layout (approximately 450 x 200 px, centred horizontally in the display):
 *
 *   [Band N label] [Compress/Expand toggle] [Enable toggle] [Audition] [X close]
 *    THRESHOLD   RANGE    ATTACK   RELEASE   OUTPUT
 *     (knob)    (knob)   (knob)   (knob)    (knob)
 *    RATIO      KNEE   LOOKAHEAD  STEREO LINK
 *     (knob)    (knob)   (knob)     (knob)
 *
 * All knobs use APVTS SliderAttachments that are rebuilt when the selected band
 * changes.  The panel header bar is tinted with the band's colour from
 * ColourPalette.
 */
class BandControlPanel : public juce::Component
{
public:
    explicit BandControlPanel ( juce::AudioProcessorValueTreeState& apvts );
    ~BandControlPanel() override;

    /** Select a band (0..N-1) to display, or -1 to hide the panel. */
    void setSelectedBand ( int bandIndex );
    int getSelectedBand() const { return selectedBand; }

    void paint ( juce::Graphics& g ) override;
    void resized() override;

    /** Called when the user clicks the close button. */
    std::function<void()> onClose;

private:
    void rebuildAttachments();
    void configureKnob ( juce::Slider& knob, juce::Label& label, const juce::String& text );

    juce::AudioProcessorValueTreeState& apvts;
    int selectedBand = -1;

    // =====================================================================
    // Knobs
    // =====================================================================
    juce::Slider thresholdKnob, rangeKnob, attackKnob, releaseKnob, outputKnob;
    juce::Slider ratioKnob, kneeKnob, lookaheadKnob, stereoLinkKnob;

    // =====================================================================
    // Labels
    // =====================================================================
    juce::Label thresholdLabel, rangeLabel, attackLabel, releaseLabel, outputLabel;
    juce::Label ratioLabel, kneeLabel, lookaheadLabel, stereoLinkLabel;

    // =====================================================================
    // Toggles / buttons
    // =====================================================================
    juce::TextButton enableButton   { "ON" };
    juce::TextButton modeButton     { "COMP" };
    juce::TextButton auditionButton { "SOLO" };
    juce::TextButton closeButton    { "X" };
    juce::Label      bandLabel;

    // =====================================================================
    // APVTS Attachments - rebuilt when band changes
    // =====================================================================
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rangeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lookaheadAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoLinkAttach;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> auditionAttach;

    // Band colour look-up table ------------------------------------------------
    static inline const juce::Colour bandColours[Constants::kMaxBands] =
    {
        ColourPalette::band1, ColourPalette::band2, ColourPalette::band3,
        ColourPalette::band4, ColourPalette::band5, ColourPalette::band6
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( BandControlPanel )
};
