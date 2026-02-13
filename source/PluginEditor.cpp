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
      bottomControlStrip ( p.getAPVTS() )
{
    setLookAndFeel ( &lookAndFeel );

    multibandDisplay.setProcessor ( &p );

    addAndMakeVisible ( headerToolbar );
    addAndMakeVisible ( multibandDisplay );
    addAndMakeVisible ( bottomControlStrip );
    addAndMakeVisible ( levelMeter );

    // BandControlPanel is an overlay inside the multiband display area
    multibandDisplay.addChildComponent ( bandControlPanel );

    // When a band is clicked in the display, show its controls
    multibandDisplay.onBandSelected = [this] ( int bandIndex )
    {
        bandControlPanel.setSelectedBand ( bandIndex );
        bandControlPanel.setVisible ( true );
    };

    // When the band control panel is closed, deselect the band
    bandControlPanel.onClose = [this]
    {
        bandControlPanel.setVisible ( false );
        multibandDisplay.setSelectedBand ( -1 );
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

    // Multiband display fills the rest
    multibandDisplay.setBounds ( bounds );

    // BandControlPanel is positioned within the multiband display's local bounds
    auto displayLocal = multibandDisplay.getLocalBounds();
    int panelW = juce::jmin ( 360, displayLocal.getWidth() );
    int panelH = juce::jmin ( 280, displayLocal.getHeight() );
    bandControlPanel.setBounds ( displayLocal.getCentreX() - panelW / 2,
                                 displayLocal.getCentreY() - panelH / 2,
                                 panelW, panelH );
}

void AdvancedMBEditor::timerCallback()
{
    multibandDisplay.repaint();
    levelMeter.repaint();
}
