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

    // BandControlPanel is a floating overlay (child, not always visible)
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
            showBandControlPanel();
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
            showBandControlPanel();
        }
    };

    // When the band control panel is closed, deselect the band
    bandControlPanel.onClose = [this]
    {
        hideBandControlPanel();
        multibandDisplay.setSelectedBand ( -1 );
    };

    // When the delete button is clicked on the band control panel
    bandControlPanel.onDeleteBand = [this] ( int bandPosition )
    {
        processor.getBandManager().removeBand ( bandPosition );
        hideBandControlPanel();
        bandControlPanel.setSelectedBand ( -1 );
        multibandDisplay.setSelectedBand ( -1 );
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

    // Level meter on the right edge of the remaining centre area
    levelMeter.setBounds ( bounds.removeFromRight ( Constants::kMeterWidth ) );

    // Multiband display always fills the full remaining space (no docking)
    multibandDisplay.setBounds ( bounds );

    // Reposition floating panel overlay if visible
    if ( bandControlPanel.isVisible() && ! panelAnimator.isAnimating ( &bandControlPanel ) )
    {
        bandControlPanel.setBounds (
            bandControlPanel.calculatePopupBounds ( multibandDisplay.getBounds() ) );
        bandControlPanel.toFront ( false );
    }

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

//==============================================================================
// Floating panel show/hide with animation
//==============================================================================

void AdvancedMBEditor::showBandControlPanel()
{
    // Cancel any in-progress hide animation
    panelAnimator.cancelAnimation ( &bandControlPanel, false );

    auto displayBounds = multibandDisplay.getBounds();
    auto targetBounds = bandControlPanel.calculatePopupBounds ( displayBounds );

    // Fully visible and opaque — just reposition (e.g. switching bands)
    if ( bandControlPanel.isVisible() && bandControlPanel.getAlpha() >= 1.0f )
    {
        bandControlPanel.setBounds ( targetBounds );
        bandControlPanel.toFront ( false );
        return;
    }

    // Animate in from below
    auto startBounds = targetBounds.translated ( 0, targetBounds.getHeight() + Constants::kPopupPanelBottomMargin );
    bandControlPanel.setBounds ( startBounds );
    bandControlPanel.setAlpha ( 0.0f );
    bandControlPanel.setVisible ( true );
    bandControlPanel.toFront ( false );

    panelAnimator.animateComponent ( &bandControlPanel,
                                      targetBounds, 1.0f,
                                      Constants::kPopupAnimationMs,
                                      false, 1.0, 1.0 );
}

void AdvancedMBEditor::hideBandControlPanel()
{
    if ( ! bandControlPanel.isVisible() )
        return;

    panelAnimator.cancelAnimation ( &bandControlPanel, false );

    auto currentBounds = bandControlPanel.getBounds();
    auto offScreenBounds = currentBounds.translated ( 0,
        currentBounds.getHeight() + Constants::kPopupPanelBottomMargin );

    panelAnimator.animateComponent ( &bandControlPanel,
                                      offScreenBounds, 0.0f,
                                      Constants::kPopupAnimationMs,
                                      false, 1.0, 1.0 );

    // Hide after animation completes — only if still faded out
    juce::Timer::callAfterDelay ( Constants::kPopupAnimationMs + 20, [this]
    {
        if ( bandControlPanel.getAlpha() < 0.05f )
            bandControlPanel.setVisible ( false );
    });
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
