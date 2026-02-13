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

#include "PluginEditor.h"
#include "ui/LookAndFeel/ColourPalette.h"

AdvancedMBEditor::AdvancedMBEditor ( AdvancedMBProcessor& p )
    : AudioProcessorEditor ( &p ),
      processor ( p ),
      headerToolbar ( p ),
      bandControlPanel ( p.getAPVTS() ),
      bottomControlStrip ( p.getAPVTS() ),
      presetBrowser ( p.getPresetManager() )
{
    setLookAndFeel ( &lookAndFeel );

    multibandDisplay.setProcessor ( &p );
    multibandDisplay.setBandManager ( &p.getBandManager() );

    addAndMakeVisible ( headerToolbar );
    addAndMakeVisible ( multibandDisplay );
    addAndMakeVisible ( bottomControlStrip );
    addAndMakeVisible ( levelMeter );

    // BandControlPanel is a sibling docked at the bottom (not an overlay)
    addChildComponent ( bandControlPanel );

    // PresetBrowserPanel overlays the right side of the display
    addChildComponent ( presetBrowser );

    // HelpOverlay covers the entire editor
    addChildComponent ( helpOverlay );

    // When a band is clicked in the display, show its controls
    multibandDisplay.onBandSelected = [this] ( int bandPosition )
    {
        auto& bm = processor.getBandManager();
        const auto& bands = bm.getActiveBands();
        if ( bandPosition >= 0 && bandPosition < static_cast<int> ( bands.size() ) )
        {
            int slot = bands[bandPosition].slotIndex;
            bandControlPanel.setSelectedPosition ( bandPosition );
            bandControlPanel.setSelectedBand ( slot );
            bandControlPanel.setVisible ( true );
            resized();
        }
    };

    // When a new band is created by clicking empty space
    multibandDisplay.onBandCreated = [this] ( int bandPosition )
    {
        auto& bm = processor.getBandManager();
        const auto& bands = bm.getActiveBands();
        if ( bandPosition >= 0 && bandPosition < static_cast<int> ( bands.size() ) )
        {
            int slot = bands[bandPosition].slotIndex;
            bandControlPanel.setSelectedPosition ( bandPosition );
            bandControlPanel.setSelectedBand ( slot );
            bandControlPanel.setVisible ( true );
            resized();
        }
    };

    // When the same band is clicked again, deselect it
    multibandDisplay.onBandDeselected = [this]
    {
        bandControlPanel.setVisible ( false );
        resized();
    };

    // When the band control panel is closed, deselect the band
    bandControlPanel.onClose = [this]
    {
        bandControlPanel.setVisible ( false );
        multibandDisplay.setSelectedBand ( -1 );
        resized();
    };

    // When the delete button is clicked on the band control panel
    bandControlPanel.onDeleteBand = [this] ( int bandPosition )
    {
        processor.getBandManager().removeBand ( bandPosition );
        bandControlPanel.setVisible ( false );
        bandControlPanel.setSelectedBand ( -1 );
        multibandDisplay.setSelectedBand ( -1 );
        resized();
    };

    // Clicking preset name toggles preset browser
    headerToolbar.onPresetNameClicked = [this]
    {
        presetBrowser.setVisible ( ! presetBrowser.isVisible() );
        resized();
    };

    // Help button toggles help overlay
    headerToolbar.onHelpClicked = [this]
    {
        helpOverlay.setVisible ( ! helpOverlay.isVisible() );
        if ( helpOverlay.isVisible() )
            helpOverlay.toFront ( false );
    };

    setSize ( Constants::kDefaultEditorWidth, Constants::kDefaultEditorHeight );
    setResizable ( true, true );
    setResizeLimits ( Constants::kMinEditorWidth,  Constants::kMinEditorHeight,
                      Constants::kMaxEditorWidth,  Constants::kMaxEditorHeight );

    startTimerHz ( Constants::kUITimerHz );
}

AdvancedMBEditor::~AdvancedMBEditor()
{
    stopTimer();
    setLookAndFeel ( nullptr );
}

void AdvancedMBEditor::paint ( juce::Graphics& g )
{
    g.fillAll ( ColourPalette::background );
}

void AdvancedMBEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header toolbar across the top
    headerToolbar.setBounds ( bounds.removeFromTop ( Constants::kHeaderHeight ) );

    // Bottom control strip across the bottom
    bottomControlStrip.setBounds ( bounds.removeFromBottom ( Constants::kBottomStripHeight ) );

    // Band control panel docked above the bottom strip (only when visible)
    if ( bandControlPanel.isVisible() )
        bandControlPanel.setBounds ( bounds.removeFromBottom ( Constants::kBandControlPanelHeight ) );

    // Level meter on the right edge of the remaining centre area
    levelMeter.setBounds ( bounds.removeFromRight ( Constants::kMeterWidth ) );

    // Multiband display fills the rest
    multibandDisplay.setBounds ( bounds );

    // Preset browser overlays the right side of the display area
    if ( presetBrowser.isVisible() )
    {
        int browserW = juce::jmin ( 350, bounds.getWidth() );
        presetBrowser.setBounds ( bounds.getRight() - browserW,
                                   bounds.getY(),
                                   browserW, bounds.getHeight() );
    }

    // Help overlay covers everything
    helpOverlay.setBounds ( getLocalBounds() );
    helpOverlay.toFront ( false );
}

void AdvancedMBEditor::timerCallback()
{
    auto& meterData = processor.getLevelMeterData();

    if ( meterData.getBuffer().swapRead() )
    {
        const auto& frame = meterData.getBuffer().getReadPointer()[0];
        levelMeter.setLevels ( frame.outputL, frame.outputR,
                               frame.outputL, frame.outputR );
    }

    multibandDisplay.repaint();
    levelMeter.repaint();
}
