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
    setupButton ( deleteButton );
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

    // Delete band
    deleteButton.onClick = [this]
    {
        if ( selectedPosition >= 0 && onDeleteBand )
            onDeleteBand ( selectedPosition );
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

    // Show position-based label if set, otherwise slot-based
    int displayNum = ( selectedPosition >= 0 ) ? selectedPosition + 1 : selectedBand + 1;
    bandLabel.setText ( "Band " + juce::String ( displayNum ),
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

    // Flat panel gradient (matching BottomControlStrip style)
    g.setGradientFill ( juce::ColourGradient (
        ColourPalette::panelTop, 0.0f, bounds.getY(),
        ColourPalette::panelBottom, 0.0f, bounds.getBottom(),
        false ) );
    g.fillRect ( bounds );

    // Top bevel line pair
    g.setColour ( ColourPalette::bevelLight );
    g.drawLine ( 0.0f, bounds.getY() + 0.5f, bounds.getRight(), bounds.getY() + 0.5f, 1.0f );
    g.setColour ( ColourPalette::bevelDark );
    g.drawLine ( 0.0f, bounds.getY() + 1.5f, bounds.getRight(), bounds.getY() + 1.5f, 1.0f );

    // Band colour indicator line at very top (3px)
    juce::Colour bandCol = ( selectedBand >= 0 && selectedBand < Constants::kMaxBands )
                               ? bandColours[selectedBand]
                               : ColourPalette::textDim;

    g.setColour ( bandCol );
    g.fillRect ( bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f );
}

//==============================================================================
// Layout
//==============================================================================

void BandControlPanel::resized()
{
    auto bounds = getLocalBounds();

    // Band colour bar is purely painted (3px), skip it in layout
    bounds.removeFromTop ( 3 );

    // Left section (~200px): band label + buttons
    auto leftSection = bounds.removeFromLeft ( 200 );
    leftSection.reduce ( 8, 4 );

    // Top row of left section: Band label + close + delete
    auto leftTopRow = leftSection.removeFromTop ( 24 );
    bandLabel.setBounds ( leftTopRow.removeFromLeft ( 70 ) );
    closeButton.setBounds ( leftTopRow.removeFromRight ( 28 ) );
    deleteButton.setBounds ( leftTopRow.removeFromRight ( 36 ) );
    leftTopRow.removeFromRight ( 4 );

    // Bottom row of left section: mode, enable, audition
    leftSection.removeFromTop ( 4 );
    auto leftBtnRow = leftSection.removeFromTop ( 24 );
    modeButton.setBounds ( leftBtnRow.removeFromLeft ( 60 ) );
    leftBtnRow.removeFromLeft ( 4 );
    enableButton.setBounds ( leftBtnRow.removeFromLeft ( 36 ) );
    leftBtnRow.removeFromLeft ( 4 );
    auditionButton.setBounds ( leftBtnRow.removeFromLeft ( 50 ) );

    // Right section: all 9 knobs in a single horizontal row
    auto knobArea = bounds.reduced ( 4, 2 );
    const int numKnobs = 9;
    int knobW = knobArea.getWidth() / numKnobs;
    const int labelH = 14;

    auto placeKnob = [&] ( juce::Slider& knob, juce::Label& label )
    {
        auto cell = knobArea.removeFromLeft ( knobW );
        label.setBounds ( cell.removeFromTop ( labelH ) );
        knob.setBounds ( cell );
    };

    placeKnob ( thresholdKnob,  thresholdLabel );
    placeKnob ( rangeKnob,      rangeLabel );
    placeKnob ( attackKnob,     attackLabel );
    placeKnob ( releaseKnob,    releaseLabel );
    placeKnob ( outputKnob,     outputLabel );
    placeKnob ( ratioKnob,      ratioLabel );
    placeKnob ( kneeKnob,       kneeLabel );
    placeKnob ( lookaheadKnob,  lookaheadLabel );
    placeKnob ( stereoLinkKnob, stereoLinkLabel );
}
