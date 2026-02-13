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

#include "BandControlPanel.h"
#include "../LookAndFeel/Fonts.h"

//==============================================================================
// Construction / destruction
//==============================================================================

BandControlPanel::BandControlPanel ( juce::AudioProcessorValueTreeState& vts )
    : apvts ( vts )
{
    // Knobs ---------------------------------------------------------------
    configureKnob ( thresholdKnob,  thresholdLabel,  "THRESHOLD" );
    configureKnob ( rangeKnob,      rangeLabel,      "RANGE" );
    configureKnob ( attackKnob,     attackLabel,     "ATTACK" );
    configureKnob ( releaseKnob,    releaseLabel,    "RELEASE" );
    configureKnob ( outputKnob,     outputLabel,     "OUTPUT" );
    configureKnob ( ratioKnob,      ratioLabel,      "RATIO" );
    configureKnob ( kneeKnob,       kneeLabel,       "KNEE" );
    configureKnob ( lookaheadKnob,  lookaheadLabel,  "LOOKAHEAD" );
    configureKnob ( stereoLinkKnob, stereoLinkLabel, "STEREO LINK" );

    // Buttons -------------------------------------------------------------
    auto setupButton = [this] ( juce::TextButton& btn )
    {
        btn.setColour ( juce::TextButton::buttonColourId, ColourPalette::knobBackground );
        btn.setColour ( juce::TextButton::textColourOnId, ColourPalette::textBright );
        btn.setColour ( juce::TextButton::textColourOffId, ColourPalette::textDim );
        addAndMakeVisible ( btn );
    };

    enableButton.setClickingTogglesState ( true );
    auditionButton.setClickingTogglesState ( true );

    setupButton ( enableButton );
    setupButton ( modeButton );
    setupButton ( auditionButton );
    setupButton ( closeButton );

    // Mode button cycles compress/expand on click
    modeButton.onClick = [this]
    {
        if ( selectedBand < 0 )
            return;

        juce::String paramID = ParameterIDs::bandParam (
            selectedBand, ParameterIDs::Band::mode );

        if ( auto* param = apvts.getParameter ( paramID ) )
        {
            float current = param->getValue();
            param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
        }

        // Update button text
        if ( auto* param = apvts.getRawParameterValue ( paramID ) )
            modeButton.setButtonText ( param->load() > 0.5f ? "EXPAND" : "COMP" );
    };

    // Close
    closeButton.onClick = [this]
    {
        setSelectedBand ( -1 );

        if ( onClose )
            onClose();
    };

    // Band label ----------------------------------------------------------
    bandLabel.setFont ( Fonts::getBold ( 14.0f ) );
    bandLabel.setColour ( juce::Label::textColourId, ColourPalette::textBright );
    bandLabel.setJustificationType ( juce::Justification::centredLeft );
    addAndMakeVisible ( bandLabel );

    // Initially hidden
    setVisible ( false );
}

BandControlPanel::~BandControlPanel() = default;

//==============================================================================

void BandControlPanel::configureKnob ( juce::Slider& knob, juce::Label& label,
                                        const juce::String& text )
{
    knob.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
    knob.setTextBoxStyle ( juce::Slider::TextBoxBelow, false, 56, 14 );
    knob.setColour ( juce::Slider::rotarySliderFillColourId, ColourPalette::knobIndicator );
    knob.setColour ( juce::Slider::rotarySliderOutlineColourId, ColourPalette::knobEdge );
    knob.setColour ( juce::Slider::textBoxTextColourId, ColourPalette::textBright );
    knob.setColour ( juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack );
    knob.setColour ( juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack );
    addAndMakeVisible ( knob );

    label.setText ( text, juce::dontSendNotification );
    label.setFont ( Fonts::getBody ( 10.0f ) );
    label.setColour ( juce::Label::textColourId, ColourPalette::textDim );
    label.setJustificationType ( juce::Justification::centred );
    addAndMakeVisible ( label );
}

//==============================================================================
// Band selection
//==============================================================================

void BandControlPanel::setSelectedBand ( int bandIndex )
{
    if ( bandIndex == selectedBand )
        return;

    selectedBand = bandIndex;

    if ( selectedBand < 0 || selectedBand >= Constants::kMaxBands )
    {
        selectedBand = -1;
        setVisible ( false );
        rebuildAttachments(); // clears attachments
        return;
    }

    bandLabel.setText ( "Band " + juce::String ( selectedBand + 1 ),
                        juce::dontSendNotification );

    // Update mode button text
    juce::String modeID = ParameterIDs::bandParam (
        selectedBand, ParameterIDs::Band::mode );

    if ( auto* param = apvts.getRawParameterValue ( modeID ) )
        modeButton.setButtonText ( param->load() > 0.5f ? "EXPAND" : "COMP" );

    rebuildAttachments();
    setVisible ( true );
    repaint();
}

//==============================================================================
// Rebuild APVTS attachments for the current band
//==============================================================================

void BandControlPanel::rebuildAttachments()
{
    // Destroy all existing attachments first (order matters: detach before re-attach)
    thresholdAttach.reset();
    rangeAttach.reset();
    attackAttach.reset();
    releaseAttach.reset();
    outputAttach.reset();
    ratioAttach.reset();
    kneeAttach.reset();
    lookaheadAttach.reset();
    stereoLinkAttach.reset();
    enableAttach.reset();
    auditionAttach.reset();

    if ( selectedBand < 0 )
        return;

    const int b = selectedBand;

    auto makeSlider = [&] ( juce::Slider& knob, const juce::String& suffix )
    {
        return std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, ParameterIDs::bandParam ( b, suffix ), knob );
    };

    thresholdAttach  = makeSlider ( thresholdKnob,  ParameterIDs::Band::threshold );
    rangeAttach      = makeSlider ( rangeKnob,      ParameterIDs::Band::range );
    attackAttach     = makeSlider ( attackKnob,     ParameterIDs::Band::attack );
    releaseAttach    = makeSlider ( releaseKnob,    ParameterIDs::Band::release );
    outputAttach     = makeSlider ( outputKnob,     ParameterIDs::Band::outputGain );
    ratioAttach      = makeSlider ( ratioKnob,      ParameterIDs::Band::ratio );
    kneeAttach       = makeSlider ( kneeKnob,       ParameterIDs::Band::knee );
    lookaheadAttach  = makeSlider ( lookaheadKnob,  ParameterIDs::Band::lookahead );
    stereoLinkAttach = makeSlider ( stereoLinkKnob, ParameterIDs::Band::stereoLink );

    enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::bandParam ( b, ParameterIDs::Band::enabled ), enableButton );

    auditionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::bandParam ( b, ParameterIDs::Band::audition ), auditionButton );
}

//==============================================================================
// Paint
//==============================================================================

void BandControlPanel::paint ( juce::Graphics& g )
{
    auto bounds = getLocalBounds().toFloat();

    // Drop shadow
    g.setColour ( ColourPalette::screenShadow );
    g.fillRoundedRectangle ( bounds.expanded ( 2.0f ), 6.0f );

    // Panel background
    g.setGradientFill ( juce::ColourGradient (
        ColourPalette::panelTop, 0.0f, bounds.getY(),
        ColourPalette::panelBottom, 0.0f, bounds.getBottom(),
        false ) );
    g.fillRoundedRectangle ( bounds, 4.0f );

    // Bevel highlight
    g.setColour ( ColourPalette::bevelLight );
    g.drawRoundedRectangle ( bounds.reduced ( 0.5f ), 4.0f, 1.0f );

    // Coloured header bar
    const float headerH = 28.0f;
    auto headerBounds = bounds.removeFromTop ( headerH );

    juce::Colour bandCol = ( selectedBand >= 0 && selectedBand < Constants::kMaxBands )
                               ? bandColours[selectedBand]
                               : ColourPalette::textDim;

    g.setColour ( bandCol.withAlpha ( 0.25f ) );
    g.fillRoundedRectangle ( headerBounds.getX(), headerBounds.getY(),
                              headerBounds.getWidth(), headerH,
                              4.0f );

    // Bottom corners are not rounded - overlay a small rect to square them off
    g.fillRect ( headerBounds.getX(), headerBounds.getY() + headerH - 4.0f,
                 headerBounds.getWidth(), 4.0f );

    // Band colour indicator line at very top
    g.setColour ( bandCol );
    g.fillRoundedRectangle ( headerBounds.getX(), headerBounds.getY(),
                              headerBounds.getWidth(), 3.0f, 4.0f );
}

//==============================================================================
// Layout
//==============================================================================

void BandControlPanel::resized()
{
    auto bounds = getLocalBounds();

    // Header row (28px) ===================================================
    const int headerH = 28;
    auto headerRow = bounds.removeFromTop ( headerH );
    headerRow.reduce ( 6, 2 );

    bandLabel.setBounds ( headerRow.removeFromLeft ( 60 ) );
    closeButton.setBounds ( headerRow.removeFromRight ( 28 ) );
    auditionButton.setBounds ( headerRow.removeFromRight ( 50 ) );
    headerRow.removeFromRight ( 4 );
    enableButton.setBounds ( headerRow.removeFromRight ( 36 ) );
    headerRow.removeFromRight ( 4 );
    modeButton.setBounds ( headerRow.removeFromRight ( 60 ) );

    // Knob area -----------------------------------------------------------
    auto knobArea = bounds.reduced ( 4, 2 );

    const int knobW = 80;
    const int knobH = 60;
    const int labelH = 14;

    // Row 1 (5 knobs): Threshold, Range, Attack, Release, Output
    auto row1 = knobArea.removeFromTop ( knobH + labelH );
    auto placeKnob = [&] ( juce::Slider& knob, juce::Label& label, juce::Rectangle<int>& row )
    {
        auto cell = row.removeFromLeft ( knobW );
        label.setBounds ( cell.removeFromTop ( labelH ) );
        knob.setBounds ( cell );
    };

    placeKnob ( thresholdKnob,  thresholdLabel,  row1 );
    placeKnob ( rangeKnob,      rangeLabel,      row1 );
    placeKnob ( attackKnob,     attackLabel,     row1 );
    placeKnob ( releaseKnob,    releaseLabel,    row1 );
    placeKnob ( outputKnob,     outputLabel,     row1 );

    // Row 2 (4 knobs): Ratio, Knee, Lookahead, Stereo Link
    knobArea.removeFromTop ( 2 );
    auto row2 = knobArea.removeFromTop ( knobH + labelH );

    placeKnob ( ratioKnob,      ratioLabel,      row2 );
    placeKnob ( kneeKnob,       kneeLabel,       row2 );
    placeKnob ( lookaheadKnob,  lookaheadLabel,  row2 );
    placeKnob ( stereoLinkKnob, stereoLinkLabel, row2 );
}
