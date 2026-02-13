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

#include "BottomControlStrip.h"
#include "../../utils/Constants.h"

BottomControlStrip::BottomControlStrip ( juce::AudioProcessorValueTreeState& vts )
    : apvts ( vts )
{
    auto font = Fonts::getBody ( 11.0f );

    // ========================= Combo box styling =========================
    auto styleCombo = [&] ( juce::ComboBox& combo, const juce::StringArray& items,
                            const juce::String& componentID )
    {
        combo.addItemList ( items, 1 );
        combo.setJustificationType ( juce::Justification::centredLeft );
        combo.setColour ( juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack );
        combo.setColour ( juce::ComboBox::outlineColourId, juce::Colours::transparentBlack );
        combo.setColour ( juce::ComboBox::textColourId, ColourPalette::textDim );
        combo.setColour ( juce::ComboBox::arrowColourId, ColourPalette::textDim.withAlpha ( 0.4f ) );
        combo.setComponentID ( componentID );
        addAndMakeVisible ( combo );
    };

    auto stylePrefix = [&] ( juce::Label& label, const juce::String& text )
    {
        label.setText ( text, juce::dontSendNotification );
        label.setFont ( font );
        label.setColour ( juce::Label::textColourId, ColourPalette::textDim );
        label.setJustificationType ( juce::Justification::centredRight );
        addAndMakeVisible ( label );
    };

    // ========================= Combos =========================
    styleCombo ( processingModeCombo,
                 { "Dynamic Phase", "Linear Phase", "Minimum Phase" },
                 "bottom.processMode" );

    stylePrefix ( oversamplingLabel, "Oversampling:" );
    styleCombo ( oversamplingCombo, { "Off", "2x", "4x" }, "bottom.oversample" );

    stylePrefix ( analyzerLabel, "Analyzer:" );
    styleCombo ( analyzerModeCombo, { "Off", "Pre", "Post", "Pre+Post" }, "bottom.analyzerMode" );

    // ========================= Mix & Output sliders =========================
    auto styleSlider = [&] ( juce::Slider& slider, const juce::String& suffix,
                             int decimals, const juce::String& componentID )
    {
        slider.setSliderStyle ( juce::Slider::LinearBar );
        slider.setTextValueSuffix ( suffix );
        slider.setNumDecimalPlacesToDisplay ( decimals );
        slider.setColour ( juce::Slider::trackColourId, juce::Colours::transparentBlack );
        slider.setColour ( juce::Slider::backgroundColourId, juce::Colours::transparentBlack );
        slider.setColour ( juce::Slider::textBoxTextColourId, ColourPalette::textDim );
        slider.setColour ( juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack );
        slider.setColour ( juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack );
        slider.setComponentID ( componentID );
        addAndMakeVisible ( slider );
    };

    stylePrefix ( mixLabel, "Mix:" );
    styleSlider ( mixSlider, "%", 0, "bottom.mix" );

    stylePrefix ( outputLabel, "Output:" );
    styleSlider ( globalOutputSlider, " dB", 1, "bottom.globalOutput" );

    // ========================= APVTS Attachments =========================
    processingModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParameterIDs::Global::processMode, processingModeCombo );

    oversamplingAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParameterIDs::Global::oversample, oversamplingCombo );

    analyzerModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParameterIDs::Global::analyzerMode, analyzerModeCombo );

    globalOutputAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::Global::globalOutput, globalOutputSlider );

    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::Global::mix, mixSlider );

}

BottomControlStrip::~BottomControlStrip()
{
    globalOutputAttach.reset();
    mixAttach.reset();
    processingModeAttach.reset();
    oversamplingAttach.reset();
    analyzerModeAttach.reset();
}

void BottomControlStrip::paint ( juce::Graphics& g )
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background
    g.setColour ( ColourPalette::panelBottom );
    g.fillRect ( bounds );

    // Subtle top separator
    g.setColour ( ColourPalette::bevelDark );
    g.drawLine ( 0.0f, 0.5f, bounds.getRight(), 0.5f, 1.0f );
}

void BottomControlStrip::resized()
{
    auto bounds = getLocalBounds().reduced ( 10, 0 );
    int vPad = 2;

    // === Right-aligned: Mix & Output ===
    globalOutputSlider.setBounds ( bounds.removeFromRight ( 60 ).reduced ( 0, vPad ) );
    outputLabel.setBounds ( bounds.removeFromRight ( 48 ).reduced ( 0, vPad ) );
    bounds.removeFromRight ( 4 );

    mixSlider.setBounds ( bounds.removeFromRight ( 50 ).reduced ( 0, vPad ) );
    mixLabel.setBounds ( bounds.removeFromRight ( 30 ).reduced ( 0, vPad ) );
    bounds.removeFromRight ( 8 );

    // === Left-aligned: settings ===
    processingModeCombo.setBounds ( bounds.removeFromLeft ( 130 ).reduced ( 0, vPad ) );
    bounds.removeFromLeft ( 4 );

    oversamplingLabel.setBounds ( bounds.removeFromLeft ( 82 ).reduced ( 0, vPad ) );
    oversamplingCombo.setBounds ( bounds.removeFromLeft ( 60 ).reduced ( 0, vPad ) );
    bounds.removeFromLeft ( 4 );

    analyzerLabel.setBounds ( bounds.removeFromLeft ( 58 ).reduced ( 0, vPad ) );
    analyzerModeCombo.setBounds ( bounds.removeFromLeft ( 78 ).reduced ( 0, vPad ) );
    bounds.removeFromLeft ( 4 );

}
