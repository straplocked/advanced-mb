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
    // Main row: rotary knobs ----------------------------------------------
    configureKnob ( thresholdKnob,  thresholdLabel,  "THRESHOLD" );
    configureKnob ( rangeKnob,      rangeLabel,      "RANGE" );
    configureKnob ( attackKnob,     attackLabel,     "ATTACK" );
    configureKnob ( releaseKnob,    releaseLabel,    "RELEASE" );
    configureKnob ( outputKnob,     outputLabel,     "OUTPUT" );

    thresholdKnob.setTextValueSuffix ( " dB" );
    rangeKnob.setTextValueSuffix ( " dB" );
    attackKnob.setTextValueSuffix ( " ms" );
    releaseKnob.setTextValueSuffix ( " ms" );
    outputKnob.setTextValueSuffix ( " dB" );

    // Expert row: horizontal sliders --------------------------------------
    configureSlider ( ratioKnob,      ratioLabel,      "RATIO" );
    configureSlider ( kneeKnob,       kneeLabel,       "KNEE" );
    configureSlider ( lookaheadKnob,  lookaheadLabel,  "LOOK" );
    configureSlider ( stereoLinkKnob, stereoLinkLabel, "ST LINK" );

    ratioKnob.setTextValueSuffix ( " :1" );
    kneeKnob.setTextValueSuffix ( " dB" );
    lookaheadKnob.setTextValueSuffix ( " ms" );
    stereoLinkKnob.setTextValueSuffix ( " %" );

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

    // Delete button with distinct red-tinted colour
    deleteButton.setColour ( juce::TextButton::buttonColourId,
                             juce::Colour ( 0xff2a1018 ) );
    deleteButton.setColour ( juce::TextButton::textColourOnId,
                             ColourPalette::meterRed );
    deleteButton.setColour ( juce::TextButton::textColourOffId,
                             ColourPalette::meterRed.withAlpha ( 0.7f ) );
    addAndMakeVisible ( deleteButton );

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

    setVisible ( false );
}

BandControlPanel::~BandControlPanel() = default;

//==============================================================================

void BandControlPanel::configureKnob ( juce::Slider& knob, juce::Label& label,
                                        const juce::String& text )
{
    knob.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
    knob.setTextBoxStyle ( juce::Slider::TextBoxBelow, false, 68, 14 );
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

void BandControlPanel::configureSlider ( juce::Slider& slider, juce::Label& label,
                                          const juce::String& text )
{
    slider.setSliderStyle ( juce::Slider::LinearHorizontal );
    slider.setTextBoxStyle ( juce::Slider::TextBoxRight, false, 52, 13 );
    slider.setColour ( juce::Slider::trackColourId, ColourPalette::knobIndicator );
    slider.setColour ( juce::Slider::backgroundColourId, ColourPalette::knobEdge );
    slider.setColour ( juce::Slider::thumbColourId, ColourPalette::knobForeground );
    slider.setColour ( juce::Slider::textBoxTextColourId, ColourPalette::textBright );
    slider.setColour ( juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack );
    slider.setColour ( juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack );
    addAndMakeVisible ( slider );

    label.setText ( text, juce::dontSendNotification );
    label.setFont ( Fonts::getBody ( 9.0f ) );
    label.setColour ( juce::Label::textColourId, ColourPalette::textDim );
    label.setJustificationType ( juce::Justification::centredRight );
    addAndMakeVisible ( label );
}

//==============================================================================
// Popup bounds
//==============================================================================

juce::Rectangle<int> BandControlPanel::calculatePopupBounds (
    juce::Rectangle<int> displayArea ) const
{
    int w = juce::jlimit ( 480, 800,
        juce::jlimit (
            static_cast<int> ( displayArea.getWidth() * Constants::kPopupMinWidthRatio ),
            static_cast<int> ( displayArea.getWidth() * Constants::kPopupMaxWidthRatio ),
            Constants::kPopupPanelWidth ) );

    int h = Constants::kPopupPanelHeight;
    int x = displayArea.getX() + ( displayArea.getWidth() - w ) / 2;
    int y = displayArea.getBottom() - h - Constants::kPopupPanelBottomMargin;

    return { x, y, w, h };
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
        rebuildAttachments();
        return;
    }

    int displayNum = ( selectedPosition >= 0 ) ? selectedPosition + 1 : selectedBand + 1;
    bandLabel.setText ( "Band " + juce::String ( displayNum ),
                        juce::dontSendNotification );

    juce::String modeID = ParameterIDs::bandParam (
        selectedBand, ParameterIDs::Band::mode );

    if ( auto* param = apvts.getRawParameterValue ( modeID ) )
        modeButton.setButtonText ( param->load() > 0.5f ? "EXPAND" : "COMP" );

    rebuildAttachments();
    repaint();
}

//==============================================================================
// Rebuild APVTS attachments for the current band
//==============================================================================

void BandControlPanel::rebuildAttachments()
{
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
    auto sb = bounds.reduced ( 2.0f );   // shadow-inset bounds
    const float cr = static_cast<float> ( Constants::kPopupCornerRadius );
    const float headerH = static_cast<float> ( Constants::kPopupHeaderHeight );

    // Row geometry (must match resized() layout)
    auto headerRect = juce::Rectangle<float> (
        sb.getX(), sb.getY(), sb.getWidth(), headerH );

    // Slider row: header(28) + gap(2) + knobRow(78) + bottomPad(6) = 114 from top
    float sliderY = sb.getY() + headerH + 2.0f + 78.0f + 6.0f;
    auto sliderRect = juce::Rectangle<float> (
        sb.getX(), sliderY, sb.getWidth(), sb.getBottom() - sliderY );

    // Drop shadow
    juce::DropShadow shadow ( ColourPalette::overlayShadow, 8, { 0, 4 } );
    shadow.drawForRectangle ( g, sb.toNearestInt() );

    // Clip to rounded rect for all interior painting
    juce::Path clip;
    clip.addRoundedRectangle ( sb, cr );
    g.saveState();
    g.reduceClipRegion ( clip );

    // --- Base fill (knob area) ---
    g.setColour ( ColourPalette::overlayBackground );
    g.fillRect ( sb );

    // --- Header strip (darker) ---
    g.setColour ( ColourPalette::overlayBackground.darker ( 0.35f ) );
    g.fillRect ( headerRect );

    // Header bottom bevel: dark line then light line (inset groove)
    g.setColour ( ColourPalette::bevelDark );
    g.fillRect ( sb.getX(), headerRect.getBottom(), sb.getWidth(), 1.0f );
    g.setColour ( ColourPalette::bevelLight );
    g.fillRect ( sb.getX(), headerRect.getBottom() + 1.0f, sb.getWidth(), 1.0f );

    // --- Slider strip (darker) ---
    g.setColour ( ColourPalette::overlayBackground.darker ( 0.25f ) );
    g.fillRect ( sliderRect );

    // Slider row top bevel: dark line then light line
    g.setColour ( ColourPalette::bevelDark );
    g.fillRect ( sb.getX(), sliderRect.getY(), sb.getWidth(), 1.0f );
    g.setColour ( ColourPalette::bevelLight );
    g.fillRect ( sb.getX(), sliderRect.getY() + 1.0f, sb.getWidth(), 1.0f );

    // --- Band colour accent at top (3px) ---
    juce::Colour bandCol = ( selectedBand >= 0 && selectedBand < Constants::kMaxBands )
                               ? bandColours[selectedBand]
                               : ColourPalette::textDim;
    g.setColour ( bandCol );
    g.fillRect ( sb.getX(), sb.getY(), sb.getWidth(), 3.0f );

    g.restoreState();

    // --- Outer border (drawn outside clip for clean corners) ---
    g.setColour ( ColourPalette::overlayBorder );
    g.drawRoundedRectangle ( sb, cr, 1.0f );
}

//==============================================================================
// Layout — header + rotary knobs + horizontal sliders
//==============================================================================

void BandControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced ( 2 );  // match shadow inset
    const int headerH = Constants::kPopupHeaderHeight;

    // === Header row ===
    auto header = bounds.removeFromTop ( headerH );
    header.reduce ( 10, 4 );

    bandLabel.setBounds ( header.removeFromLeft ( 70 ) );

    // Right-to-left: close, delete, audition, enable, mode
    closeButton.setBounds ( header.removeFromRight ( 28 ) );
    header.removeFromRight ( 4 );
    deleteButton.setBounds ( header.removeFromRight ( 56 ) );
    header.removeFromRight ( 4 );
    auditionButton.setBounds ( header.removeFromRight ( 50 ) );
    header.removeFromRight ( 4 );
    enableButton.setBounds ( header.removeFromRight ( 36 ) );
    header.removeFromRight ( 4 );
    modeButton.setBounds ( header.removeFromRight ( 60 ) );

    // === Main knob row: 5 rotary knobs ===
    bounds.removeFromTop ( 2 );
    auto knobRow = bounds.removeFromTop ( 78 );
    knobRow.reduce ( 8, 0 );
    bounds.removeFromTop ( 6 );   // bottom padding before slider border

    const int numKnobs = 5;
    int knobW = knobRow.getWidth() / numKnobs;
    const int labelH = 14;

    auto placeKnob = [&] ( juce::Slider& knob, juce::Label& label )
    {
        auto cell = knobRow.removeFromLeft ( knobW );
        label.setBounds ( cell.removeFromTop ( labelH ) );
        knob.setBounds ( cell );
    };

    placeKnob ( thresholdKnob, thresholdLabel );
    placeKnob ( rangeKnob,     rangeLabel );
    placeKnob ( attackKnob,    attackLabel );
    placeKnob ( releaseKnob,   releaseLabel );
    placeKnob ( outputKnob,    outputLabel );

    // === Single slider row: 4 horizontal sliders across ===
    bounds.removeFromTop ( 6 );   // top padding after slider border
    auto sliderRow = bounds.removeFromTop ( 16 );
    sliderRow.reduce ( 8, 0 );

    const int numSliders = 4;
    int sliderCellW = sliderRow.getWidth() / numSliders;
    const int sliderLabelW = 42;

    auto placeSlider = [&] ( juce::Slider& slider, juce::Label& label )
    {
        auto cell = sliderRow.removeFromLeft ( sliderCellW );
        label.setBounds ( cell.removeFromLeft ( sliderLabelW ) );
        slider.setBounds ( cell );
    };

    placeSlider ( ratioKnob,      ratioLabel );
    placeSlider ( kneeKnob,       kneeLabel );
    placeSlider ( lookaheadKnob,  lookaheadLabel );
    placeSlider ( stereoLinkKnob, stereoLinkLabel );
}
