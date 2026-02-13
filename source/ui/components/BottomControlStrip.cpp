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
    // ========================= Helper lambdas =========================

    auto setupCombo = [this] ( juce::ComboBox& combo, juce::Label& label,
                               const juce::String& labelText, const juce::StringArray& items,
                               const juce::String& componentID )
    {
        combo.addItemList ( items, 1 );
        combo.setJustificationType ( juce::Justification::centred );
        combo.setComponentID ( componentID );
        addAndMakeVisible ( combo );

        label.setText ( labelText, juce::dontSendNotification );
        label.setJustificationType ( juce::Justification::centred );
        label.setFont ( Fonts::getBody ( 10.0f ) );
        label.setColour ( juce::Label::textColourId, ColourPalette::textDim );
        addAndMakeVisible ( label );
    };

    auto setupKnob = [this] ( juce::Slider& slider, juce::Label& title, juce::Label& value,
                               const juce::String& titleText, const juce::String& componentID )
    {
        slider.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
        slider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        slider.setComponentID ( componentID );
        slider.onValueChange = [this] { updateValueLabels(); };
        addAndMakeVisible ( slider );

        title.setText ( titleText, juce::dontSendNotification );
        title.setJustificationType ( juce::Justification::centred );
        title.setFont ( Fonts::getBody ( 10.0f ) );
        title.setColour ( juce::Label::textColourId, ColourPalette::textDim );
        addAndMakeVisible ( title );

        value.setJustificationType ( juce::Justification::centred );
        value.setFont ( Fonts::getBody ( 11.0f ) );
        value.setColour ( juce::Label::textColourId, ColourPalette::axisText );
        addAndMakeVisible ( value );
    };

    // ========================= Processing Mode =========================
    setupCombo ( processingModeCombo, processingModeLabel, "Mode",
                 { "Dynamic Phase", "Linear Phase", "Minimum Phase" },
                 "bottom.processMode" );

    // ========================= Oversampling =========================
    setupCombo ( oversamplingCombo, oversamplingLabel, "Oversample",
                 { "1x", "2x", "4x" },
                 "bottom.oversample" );

    // ========================= Analyzer Mode =========================
    setupCombo ( analyzerModeCombo, analyzerModeLabel, "Analyzer",
                 { "Off", "Pre", "Post", "Pre+Post" },
                 "bottom.analyzerMode" );

    // ========================= Global Output knob =========================
    setupKnob ( globalOutputSlider, globalOutputTitle, globalOutputValue,
                "Output", "bottom.globalOutput" );

    // ========================= Mix knob =========================
    setupKnob ( mixSlider, mixTitle, mixValue,
                "Mix", "bottom.mix" );

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

    // ========================= Band count control =========================
    bandsTitle.setText ( "Bands", juce::dontSendNotification );
    bandsTitle.setJustificationType ( juce::Justification::centred );
    bandsTitle.setFont ( Fonts::getBody ( 10.0f ) );
    bandsTitle.setColour ( juce::Label::textColourId, ColourPalette::textDim );
    addAndMakeVisible ( bandsTitle );

    bandsValueLabel.setJustificationType ( juce::Justification::centred );
    bandsValueLabel.setFont ( Fonts::getBold ( 14.0f ) );
    bandsValueLabel.setColour ( juce::Label::textColourId, ColourPalette::textBright );
    addAndMakeVisible ( bandsValueLabel );

    bandsMinusButton.setComponentID ( "global.numBandsMinus" );
    bandsMinusButton.setColour ( juce::TextButton::buttonColourId, ColourPalette::knobBackground );
    bandsMinusButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textBright );
    bandsMinusButton.onClick = [this]
    {
        if ( auto* param = apvts.getParameter ( ParameterIDs::Global::numBands ) )
        {
            float current = param->convertFrom0to1 ( param->getValue() );
            float newVal = juce::jmax ( 1.0f, current - 1.0f );
            param->setValueNotifyingHost ( param->convertTo0to1 ( newVal ) );
            updateBandsLabel();
        }
    };
    addAndMakeVisible ( bandsMinusButton );

    bandsPlusButton.setComponentID ( "global.numBandsPlus" );
    bandsPlusButton.setColour ( juce::TextButton::buttonColourId, ColourPalette::knobBackground );
    bandsPlusButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textBright );
    bandsPlusButton.onClick = [this]
    {
        if ( auto* param = apvts.getParameter ( ParameterIDs::Global::numBands ) )
        {
            float current = param->convertFrom0to1 ( param->getValue() );
            float newVal = juce::jmin ( static_cast<float> ( Constants::kMaxBands ), current + 1.0f );
            param->setValueNotifyingHost ( param->convertTo0to1 ( newVal ) );
            updateBandsLabel();
        }
    };
    addAndMakeVisible ( bandsPlusButton );

    // Initial value update
    updateValueLabels();
    updateBandsLabel();
}

BottomControlStrip::~BottomControlStrip()
{
    processingModeAttach.reset();
    oversamplingAttach.reset();
    analyzerModeAttach.reset();
    globalOutputAttach.reset();
    mixAttach.reset();
}

void BottomControlStrip::updateValueLabels()
{
    globalOutputValue.setText (
        juce::String ( globalOutputSlider.getValue(), 1 ) + " dB",
        juce::dontSendNotification );

    mixValue.setText (
        juce::String ( (int) mixSlider.getValue() ) + "%",
        juce::dontSendNotification );
}

void BottomControlStrip::updateBandsLabel()
{
    if ( auto* param = apvts.getRawParameterValue ( ParameterIDs::Global::numBands ) )
    {
        int numBands = juce::roundToInt ( param->load() );
        bandsValueLabel.setText ( juce::String ( numBands ), juce::dontSendNotification );
    }
}

void BottomControlStrip::paint ( juce::Graphics& g )
{
    auto bounds = getLocalBounds().toFloat();

    // Panel gradient fill
    juce::ColourGradient grad ( ColourPalette::panelTop, 0, bounds.getY(),
                                ColourPalette::panelBottom, 0, bounds.getBottom(), false );
    g.setGradientFill ( grad );
    g.fillRect ( bounds );

    // Top bevel line pair
    g.setColour ( ColourPalette::bevelLight );
    g.drawLine ( 0.0f, bounds.getY() + 0.5f, bounds.getRight(), bounds.getY() + 0.5f, 1.0f );
    g.setColour ( ColourPalette::bevelDark );
    g.drawLine ( 0.0f, bounds.getY() + 1.5f, bounds.getRight(), bounds.getY() + 1.5f, 1.0f );

    // Vertical separator between combos and knobs
    float midX = bounds.getCentreX();
    g.setColour ( ColourPalette::gridLine.withAlpha ( 0.5f ) );
    g.drawLine ( midX, bounds.getY() + 10.0f, midX, bounds.getBottom() - 10.0f, 0.5f );
}

void BottomControlStrip::resized()
{
    auto bounds = getLocalBounds().reduced ( 12, 8 );

    int comboW = 130;
    int comboH = 22;
    int labelH = 14;
    int knobW = 80;
    int knobH = bounds.getHeight();
    int gap = 12;

    // === Left half: combo boxes + band count ===
    auto leftHalf = bounds.removeFromLeft ( bounds.getWidth() / 2 );
    int bandCtrlW = 100;
    int comboContentW = comboW * 3 + gap * 2 + gap + bandCtrlW;
    auto comboRow = leftHalf.withSizeKeepingCentre ( comboContentW, leftHalf.getHeight() );

    auto layoutCombo = [] ( juce::ComboBox& combo, juce::Label& label,
                            juce::Rectangle<int>& area, int w, int h, int lblH )
    {
        auto col = area.removeFromLeft ( w );
        label.setBounds ( col.removeFromTop ( lblH ) );
        col.removeFromTop ( 2 );
        combo.setBounds ( col.removeFromTop ( h ) );
    };

    layoutCombo ( processingModeCombo, processingModeLabel, comboRow, comboW, comboH, labelH );
    comboRow.removeFromLeft ( gap );
    layoutCombo ( oversamplingCombo, oversamplingLabel, comboRow, comboW, comboH, labelH );
    comboRow.removeFromLeft ( gap );
    layoutCombo ( analyzerModeCombo, analyzerModeLabel, comboRow, comboW, comboH, labelH );

    // Band count control
    comboRow.removeFromLeft ( gap );
    auto bandCol = comboRow.removeFromLeft ( bandCtrlW );
    bandsTitle.setBounds ( bandCol.removeFromTop ( labelH ) );
    bandCol.removeFromTop ( 2 );
    auto bandBtnRow = bandCol.removeFromTop ( comboH );
    int btnW = 26;
    bandsMinusButton.setBounds ( bandBtnRow.removeFromLeft ( btnW ) );
    bandsValueLabel.setBounds ( bandBtnRow.removeFromLeft ( bandBtnRow.getWidth() - btnW ) );
    bandsPlusButton.setBounds ( bandBtnRow );

    // === Right half: rotary knobs ===
    auto rightHalf = bounds;
    int knobContentW = knobW * 2 + gap;
    auto knobRow = rightHalf.withSizeKeepingCentre ( knobContentW, knobH );

    auto layoutKnob = [] ( juce::Slider& slider, juce::Label& title, juce::Label& value,
                           juce::Rectangle<int>& area, int w )
    {
        auto col = area.removeFromLeft ( w );
        title.setBounds ( col.removeFromTop ( 14 ) );
        value.setBounds ( col.removeFromBottom ( 14 ) );
        slider.setBounds ( col );
    };

    layoutKnob ( globalOutputSlider, globalOutputTitle, globalOutputValue, knobRow, knobW );
    knobRow.removeFromLeft ( gap );
    layoutKnob ( mixSlider, mixTitle, mixValue, knobRow, knobW );
}
