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

#include "HeaderToolbar.h"
#include "../../PluginProcessor.h"

HeaderToolbar::HeaderToolbar ( AdvancedMBProcessor& p )
    : processor ( p ),
      presetManager ( p.getPresetManager() )
{
    presetManager.addListener ( this );

    // ========================= Undo / Redo =========================
    setupFlatButton ( undoButton, "header.undo", ColourPalette::textDim );
    undoButton.onClick = [this] { processor.getUndoManager().undo(); };

    setupFlatButton ( redoButton, "header.redo", ColourPalette::textDim );
    redoButton.onClick = [this] { processor.getUndoManager().redo(); };

    // ========================= Preset navigation =========================
    setupFlatButton ( prevButton, "header.prevPreset", ColourPalette::textDim );
    prevButton.onClick = [this] { presetManager.loadPreviousPreset(); };

    setupFlatButton ( nextButton, "header.nextPreset", ColourPalette::textDim );
    nextButton.onClick = [this] { presetManager.loadNextPreset(); };

    presetNameButton.setComponentID ( "header.presetName" );
    presetNameButton.setColour ( juce::TextButton::buttonColourId, juce::Colours::transparentBlack );
    presetNameButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textBright );
    presetNameButton.onClick = [this]
    {
        if ( onPresetNameClicked )
            onPresetNameClicked();
    };
    addAndMakeVisible ( presetNameButton );

    // Save button
    setupFlatButton ( saveButton, "header.savePreset", ColourPalette::textDim );
    saveButton.onClick = [this] { showSaveDialog(); };

    // ========================= A/B comparison =========================
    setupFlatButton ( abButton, "header.abToggle", ColourPalette::textBright );
    abButton.onClick = [this]
    {
        processor.swapAB();
        abButton.setButtonText ( abButton.getButtonText() == "A" ? "B" : "A" );
    };

    // ========================= MIDI Learn =========================
    midiLearnButton.setComponentID ( "header.midiLearn" );
    midiLearnButton.setClickingTogglesState ( true );
    midiLearnButton.setColour ( juce::TextButton::buttonColourId, juce::Colours::transparentBlack );
    midiLearnButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textDim );
    midiLearnButton.setColour ( juce::TextButton::buttonOnColourId, juce::Colour ( 0x40FF4444 ) );
    midiLearnButton.setColour ( juce::TextButton::textColourOnId, ColourPalette::textBright );
    addAndMakeVisible ( midiLearnButton );

    // ========================= Help =========================
    setupFlatButton ( helpButton, "header.help", ColourPalette::textDim );
    helpButton.onClick = [this]
    {
        if ( onHelpClicked )
            onHelpClicked();
    };

    updatePresetName();
}

HeaderToolbar::~HeaderToolbar()
{
    presetManager.removeListener ( this );
}

void HeaderToolbar::presetChanged()
{
    updatePresetName();
}

void HeaderToolbar::updatePresetName()
{
    presetNameButton.setButtonText ( presetManager.getCurrentPresetName() );
}

void HeaderToolbar::showSaveDialog()
{
    auto callback = [this] ( int result, const juce::String& name )
    {
        if ( result == 1 && name.isNotEmpty() )
            presetManager.saveCurrentAsPreset ( name );
    };

    auto* alertWindow = new juce::AlertWindow ( "Save Preset", "Enter preset name:",
                                                 juce::MessageBoxIconType::NoIcon );
    alertWindow->addTextEditor ( "name", presetManager.getCurrentPresetName(), "Preset Name:" );
    alertWindow->addButton ( "Save", 1 );
    alertWindow->addButton ( "Cancel", 0 );

    alertWindow->enterModalState ( true,
        juce::ModalCallbackFunction::create ( [alertWindow, callback] ( int result )
        {
            juce::String name = alertWindow->getTextEditorContents ( "name" );
            callback ( result, name );
            delete alertWindow;
        } ),
        false );
}

void HeaderToolbar::paint ( juce::Graphics& g )
{
    auto bounds = getLocalBounds().toFloat();

    // Panel gradient fill
    juce::ColourGradient grad ( ColourPalette::panelTop, 0, bounds.getY(),
                                ColourPalette::panelBottom, 0, bounds.getBottom(), false );
    g.setGradientFill ( grad );
    g.fillRect ( bounds );

    // Bottom bevel line pair
    float bottomY = bounds.getBottom();
    g.setColour ( ColourPalette::bevelLight );
    g.drawLine ( 0.0f, bottomY - 1.5f, bounds.getRight(), bottomY - 1.5f, 1.0f );
    g.setColour ( ColourPalette::bevelDark );
    g.drawLine ( 0.0f, bottomY - 0.5f, bounds.getRight(), bottomY - 0.5f, 1.0f );

    // Title text with drop shadow (Audiowide font)
    g.setFont ( Fonts::getTitle ( 17.5f ) );
    g.setColour ( juce::Colour ( 0x40000000 ) );
    g.drawText ( "ADVANCED MB", 13, 1, 200, getHeight(), juce::Justification::centredLeft, true );
    g.setColour ( ColourPalette::textBright );
    g.drawText ( "ADVANCED MB", 12, 0, 200, getHeight(), juce::Justification::centredLeft, true );
}

void HeaderToolbar::resized()
{
    auto bounds = getLocalBounds().reduced ( 8, 4 );

    int btnH = 22;
    int iconW = 36;

    // === Left side: title gap, then undo/redo ===
    bounds.removeFromLeft ( 210 ); // space for title text

    auto undoArea = bounds.removeFromLeft ( iconW );
    undoButton.setBounds ( undoArea.withHeight ( btnH ).withCentre ( undoArea.getCentre() ) );

    auto redoArea = bounds.removeFromLeft ( iconW );
    redoButton.setBounds ( redoArea.withHeight ( btnH ).withCentre ( redoArea.getCentre() ) );

    bounds.removeFromLeft ( 8 );

    // A/B toggle
    auto abArea = bounds.removeFromLeft ( 22 );
    abButton.setBounds ( abArea.withHeight ( btnH ).withCentre ( abArea.getCentre() ) );

    bounds.removeFromLeft ( 8 );

    // === Right side: Help, MIDI Learn ===
    auto helpArea = bounds.removeFromRight ( 28 );
    helpButton.setBounds ( helpArea.withHeight ( btnH ).withCentre ( helpArea.getCentre() ) );
    bounds.removeFromRight ( 4 );

    auto midiArea = bounds.removeFromRight ( 36 );
    midiLearnButton.setBounds ( midiArea.withHeight ( btnH ).withCentre ( midiArea.getCentre() ) );
    bounds.removeFromRight ( 4 );

    // === Center: preset navigation ===
    auto presetBounds = bounds;
    auto presetArea = presetBounds.withSizeKeepingCentre (
        juce::jmin ( 300, presetBounds.getWidth() ), presetBounds.getHeight() );

    auto prevBtnArea = presetArea.removeFromLeft ( 24 );
    prevButton.setBounds ( prevBtnArea.withHeight ( btnH ).withCentre ( prevBtnArea.getCentre() ) );
    presetArea.removeFromLeft ( 4 );

    auto nextBtnArea = presetArea.removeFromRight ( 24 );
    nextButton.setBounds ( nextBtnArea.withHeight ( btnH ).withCentre ( nextBtnArea.getCentre() ) );
    presetArea.removeFromRight ( 4 );

    // Save button right of next
    auto saveBtnArea = presetArea.removeFromRight ( 38 );
    saveButton.setBounds ( saveBtnArea.withHeight ( btnH ).withCentre ( saveBtnArea.getCentre() ) );
    presetArea.removeFromRight ( 4 );

    presetNameButton.setBounds ( presetArea.withHeight ( btnH ).withCentre ( presetArea.getCentre() ) );
}

void HeaderToolbar::setupFlatButton ( juce::TextButton& button, const juce::String& componentID,
                                      juce::Colour textColour )
{
    button.setComponentID ( componentID );
    button.setColour ( juce::TextButton::buttonColourId, juce::Colours::transparentBlack );
    button.setColour ( juce::TextButton::textColourOffId, textColour );
    addAndMakeVisible ( button );
}
