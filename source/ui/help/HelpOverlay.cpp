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

#include "HelpOverlay.h"
#include "../LookAndFeel/Fonts.h"

HelpOverlay::HelpOverlay()
{
    setInterceptsMouseClicks ( false, false );
}

void HelpOverlay::parentHierarchyChanged()
{
    if ( getParentComponent() != nullptr && ! isTimerRunning() )
        startTimerHz ( 15 );
}

//==============================================================================
void HelpOverlay::timerCallback()
{
    auto* parent = getParentComponent();
    if ( parent == nullptr )
        return;

    auto mouseScreenPos = juce::Desktop::getMousePosition();
    auto localPos = getLocalPoint ( nullptr, mouseScreenPos );

    if ( ! getLocalBounds().contains ( localPos ) )
    {
        if ( lastHoveredID.isNotEmpty() || tooltipVisible )
        {
            lastHoveredID = {};
            currentHelpText = {};
            tooltipVisible = false;
            hoverStartTime = 0;
            repaint();
        }
        return;
    }

    // Walk the parent's component tree (not ourselves)
    auto parentPos = parent->getLocalPoint ( nullptr, mouseScreenPos );
    auto* hoveredComp = findDeepestComponentWithID ( parent, parentPos );

    juce::String hoveredID;
    if ( hoveredComp != nullptr )
        hoveredID = hoveredComp->getComponentID();

    if ( hoveredID != lastHoveredID )
    {
        lastHoveredID = hoveredID;
        tooltipVisible = false;

        if ( hoveredID.isNotEmpty() )
        {
            currentHelpText = HelpText::getHelpText ( hoveredID );
            if ( currentHelpText.isNotEmpty() && hoveredComp != nullptr )
            {
                currentTargetBounds = getLocalArea ( hoveredComp, hoveredComp->getLocalBounds() );
                hoverStartTime = juce::Time::getMillisecondCounter();
            }
            else
            {
                hoverStartTime = 0;
            }
        }
        else
        {
            currentHelpText = {};
            hoverStartTime = 0;
        }

        repaint();
        return;
    }

    // Check if hover delay has elapsed
    if ( ! tooltipVisible && hoverStartTime > 0 && currentHelpText.isNotEmpty() )
    {
        auto elapsed = juce::Time::getMillisecondCounter() - hoverStartTime;
        if ( elapsed >= (juce::uint32) kHoverDelayMs )
        {
            tooltipVisible = true;
            repaint();
        }
    }
}

//==============================================================================
void HelpOverlay::paint ( juce::Graphics& g )
{
    // Draw semi-transparent dark overlay across the entire area
    g.setColour ( juce::Colour ( 0x60000000 ) );
    g.fillRect ( getLocalBounds() );

    if ( ! tooltipVisible || currentHelpText.isEmpty() )
        return;

    drawHelpTooltip ( g, currentHelpText, currentTargetBounds );
}

//==============================================================================
void HelpOverlay::drawHelpTooltip ( juce::Graphics& g, const juce::String& text,
                                    juce::Rectangle<int> targetBounds )
{
    const float maxWidth    = 280.0f;
    const float padding     = 8.0f;
    const float vertPadding = 5.0f;

    g.setFont ( 12.0f );

    // Measure text with wrapping
    juce::AttributedString attrText;
    attrText.append ( text, Fonts::getBody ( 12.0f ), ColourPalette::textBright );
    attrText.setJustification ( juce::Justification::centredLeft );

    juce::TextLayout layout;
    layout.createLayout ( attrText, maxWidth );

    float textWidth  = juce::jmin ( layout.getWidth() + padding * 2.0f, maxWidth + padding * 2.0f );
    float textHeight = layout.getHeight() + vertPadding * 2.0f;

    // Position: above the component, centered horizontally
    float x = targetBounds.getCentreX() - textWidth / 2.0f;
    float y = targetBounds.getY() - textHeight - 6.0f;

    // If tooltip would go above the top, show it below
    if ( y < 0.0f )
        y = (float) targetBounds.getBottom() + 6.0f;

    // Clamp horizontal
    x = juce::jlimit ( 4.0f, (float) getWidth() - textWidth - 4.0f, x );

    auto tooltipBounds = juce::Rectangle<float> ( x, y, textWidth, textHeight );

    // Frosted glass background
    juce::ColourGradient tooltipGrad (
        ColourPalette::controlBar.withAlpha ( 0.92f ), 0, tooltipBounds.getY(),
        ColourPalette::panelBottom.withAlpha ( 0.88f ), 0, tooltipBounds.getBottom(), false );
    g.setGradientFill ( tooltipGrad );
    g.fillRoundedRectangle ( tooltipBounds, 4.0f );

    // 3D bevel border edges
    g.setColour ( ColourPalette::bevelLight );
    g.drawLine ( tooltipBounds.getX() + 4.0f, tooltipBounds.getY(),
                 tooltipBounds.getRight() - 4.0f, tooltipBounds.getY(), 0.5f );
    g.drawLine ( tooltipBounds.getX(), tooltipBounds.getY() + 4.0f,
                 tooltipBounds.getX(), tooltipBounds.getBottom() - 4.0f, 0.5f );
    g.setColour ( ColourPalette::bevelDark );
    g.drawLine ( tooltipBounds.getX() + 4.0f, tooltipBounds.getBottom(),
                 tooltipBounds.getRight() - 4.0f, tooltipBounds.getBottom(), 0.5f );
    g.drawLine ( tooltipBounds.getRight(), tooltipBounds.getY() + 4.0f,
                 tooltipBounds.getRight(), tooltipBounds.getBottom() - 4.0f, 0.5f );

    // Subtle outline
    g.setColour ( ColourPalette::gridLine.withAlpha ( 0.5f ) );
    g.drawRoundedRectangle ( tooltipBounds, 4.0f, 0.5f );

    // Draw wrapped text
    layout.draw ( g, tooltipBounds.reduced ( padding, vertPadding ) );
}

//==============================================================================
juce::Component* HelpOverlay::findDeepestComponentWithID ( juce::Component* root,
                                                           juce::Point<int> pos )
{
    if ( root == nullptr || root == this )
        return nullptr;

    // Check children in reverse order (topmost first)
    for ( int i = root->getNumChildComponents() - 1; i >= 0; --i )
    {
        auto* child = root->getChildComponent ( i );
        if ( child == this || ! child->isVisible() )
            continue;

        auto localPos = child->getLocalPoint ( root, pos );
        if ( child->getLocalBounds().contains ( localPos ) )
        {
            // Recurse to find deepest
            auto* deeper = findDeepestComponentWithID ( child, localPos );
            if ( deeper != nullptr && deeper->getComponentID().isNotEmpty() )
                return deeper;
            if ( child->getComponentID().isNotEmpty() )
                return child;
        }
    }

    return nullptr;
}
