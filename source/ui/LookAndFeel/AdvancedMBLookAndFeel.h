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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ColourPalette.h"
#include "Fonts.h"

class AdvancedMBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AdvancedMBLookAndFeel()
    {
        setColour ( juce::ResizableWindow::backgroundColourId, ColourPalette::background );
        setColour ( juce::Slider::rotarySliderFillColourId, ColourPalette::knobForeground );
        setColour ( juce::Slider::rotarySliderOutlineColourId, ColourPalette::knobBackground );
        setColour ( juce::Slider::thumbColourId, ColourPalette::textBright );
        setColour ( juce::Label::textColourId, ColourPalette::textBright );
        setColour ( juce::ComboBox::backgroundColourId, ColourPalette::knobBackground );
        setColour ( juce::ComboBox::textColourId, ColourPalette::textBright );
        setColour ( juce::ComboBox::outlineColourId, ColourPalette::gridLine );
        setColour ( juce::TextButton::buttonColourId, ColourPalette::knobBackground );
        setColour ( juce::TextButton::textColourOnId, ColourPalette::textBright );
        setColour ( juce::TextButton::textColourOffId, ColourPalette::textDim );
        setColour ( juce::PopupMenu::backgroundColourId, ColourPalette::controlBar );
        setColour ( juce::PopupMenu::textColourId, ColourPalette::textBright );
        setColour ( juce::PopupMenu::highlightedBackgroundColourId, ColourPalette::knobBackground );
        setColour ( juce::PopupMenu::highlightedTextColourId, ColourPalette::textBright );
    }

    void drawRotarySlider ( juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider ) override
    {
        auto radius = (float) juce::jmin ( width / 2, height / 2 ) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * ( rotaryEndAngle - rotaryStartAngle );

        // Drop shadow
        g.setColour ( ColourPalette::knobShadow );
        g.fillEllipse ( centreX - radius, centreY - radius + 1.5f, radius * 2.0f, radius * 2.0f );

        // Body
        juce::ColourGradient bodyGrad ( ColourPalette::knobEdge, centreX, centreY,
                                        ColourPalette::knobBackground, centreX, centreY + radius, true );
        g.setGradientFill ( bodyGrad );
        g.fillEllipse ( centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f );

        // Rim highlight
        juce::Path rimHighlight;
        rimHighlight.addCentredArc ( centreX, centreY, radius - 0.5f, radius - 0.5f,
                                     0.0f, -2.4f, -0.7f, true );
        g.setColour ( ColourPalette::bevelLight.withMultipliedAlpha ( 2.0f ) );
        g.strokePath ( rimHighlight, juce::PathStrokeType ( 1.0f ) );

        // Track arc
        juce::Path trackArc;
        trackArc.addCentredArc ( centreX, centreY, radius - 3.0f, radius - 3.0f,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true );
        g.setColour ( ColourPalette::knobBackground.darker ( 0.3f ) );
        g.strokePath ( trackArc, juce::PathStrokeType ( 3.0f ) );

        // Value arc
        if ( std::abs ( sliderPos ) > 0.001f )
        {
            juce::Path valueArc;
            valueArc.addCentredArc ( centreX, centreY, radius - 3.0f, radius - 3.0f,
                                     0.0f, rotaryStartAngle, angle, true );
            g.setColour ( ColourPalette::knobIndicator );
            g.strokePath ( valueArc, juce::PathStrokeType ( 3.0f ) );
        }

        // Pointer dot
        float dotRadius = 2.5f;
        float dotDist = radius - 3.0f;
        float dotX = centreX + dotDist * std::sin ( angle );
        float dotY = centreY - dotDist * std::cos ( angle );
        g.setColour ( ColourPalette::textBright );
        g.fillEllipse ( dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f );

        // Notch marks
        g.setColour ( ColourPalette::axisText.withAlpha ( 0.4f ) );
        for ( float a : { rotaryStartAngle, rotaryEndAngle } )
        {
            float innerR = radius + 1.0f;
            float outerR = radius + 3.0f;
            float nx1 = centreX + innerR * std::sin ( a );
            float ny1 = centreY - innerR * std::cos ( a );
            float nx2 = centreX + outerR * std::sin ( a );
            float ny2 = centreY - outerR * std::cos ( a );
            g.drawLine ( nx1, ny1, nx2, ny2, 1.0f );
        }

        (void) slider;
    }

    void drawButtonBackground ( juce::Graphics& g, juce::Button& button,
                                const juce::Colour& backgroundColour,
                                bool isMouseOverButton, bool isButtonDown ) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced ( 0.5f );

        if ( backgroundColour.getAlpha() == 0 )
        {
            if ( isMouseOverButton )
            {
                g.setColour ( juce::Colour ( 0x08ffffff ) );
                g.fillRect ( bounds );
            }
            return;
        }

        float cornerSize = 4.0f;
        bool isOn = button.getToggleState();

        juce::ColourGradient grad ( ColourPalette::panelTop, 0, bounds.getY(),
                                    ColourPalette::panelBottom, 0, bounds.getBottom(), false );
        if ( isOn )
        {
            auto accent = button.findColour ( juce::TextButton::buttonOnColourId );
            grad = juce::ColourGradient ( accent.brighter ( 0.1f ), 0, bounds.getY(),
                                          accent.darker ( 0.2f ), 0, bounds.getBottom(), false );
        }

        g.setGradientFill ( grad );
        g.fillRoundedRectangle ( bounds, cornerSize );

        g.setColour ( isOn ? ColourPalette::bevelLight.withMultipliedAlpha ( 1.5f )
                           : ColourPalette::bevelLight );
        g.drawLine ( bounds.getX() + cornerSize, bounds.getY(),
                     bounds.getRight() - cornerSize, bounds.getY(), 1.0f );
        g.setColour ( ColourPalette::bevelDark );
        g.drawLine ( bounds.getX() + cornerSize, bounds.getBottom(),
                     bounds.getRight() - cornerSize, bounds.getBottom(), 1.0f );

        if ( isMouseOverButton )
        {
            g.setColour ( juce::Colour ( 0x08ffffff ) );
            g.fillRoundedRectangle ( bounds, cornerSize );
        }
        if ( isButtonDown )
        {
            g.setColour ( juce::Colour ( 0x10000000 ) );
            g.fillRoundedRectangle ( bounds, cornerSize );
        }

        g.setColour ( ColourPalette::gridLine );
        g.drawRoundedRectangle ( bounds, cornerSize, 0.5f );
    }

    void drawComboBox ( juce::Graphics& g, int width, int height, bool isButtonDown,
                        int, int, int, int, juce::ComboBox& box ) override
    {
        auto bounds = juce::Rectangle<float> ( 0, 0, (float) width, (float) height ).reduced ( 0.5f );
        auto bgColour = box.findColour ( juce::ComboBox::backgroundColourId );

        if ( bgColour.getAlpha() > 0 )
        {
            float cornerSize = 4.0f;

            juce::ColourGradient grad ( ColourPalette::panelTop, 0, 0,
                                        ColourPalette::panelBottom, 0, (float) height, false );
            g.setGradientFill ( grad );
            g.fillRoundedRectangle ( bounds, cornerSize );

            g.setColour ( ColourPalette::bevelLight );
            g.drawLine ( bounds.getX() + cornerSize, bounds.getY(),
                         bounds.getRight() - cornerSize, bounds.getY(), 1.0f );
            g.setColour ( ColourPalette::bevelDark );
            g.drawLine ( bounds.getX() + cornerSize, bounds.getBottom(),
                         bounds.getRight() - cornerSize, bounds.getBottom(), 1.0f );

            g.setColour ( ColourPalette::gridLine );
            g.drawRoundedRectangle ( bounds, cornerSize, 0.5f );
        }

        float arrowSize = 5.0f;
        float arrowX = (float) width - 12.0f;
        float arrowY = (float) height * 0.5f;

        juce::Path arrow;
        arrow.addTriangle ( arrowX - arrowSize * 0.5f, arrowY - 1.5f,
                            arrowX + arrowSize * 0.5f, arrowY - 1.5f,
                            arrowX, arrowY + 2.5f );

        auto arrowColour = box.findColour ( juce::ComboBox::arrowColourId );
        g.setColour ( isButtonDown ? arrowColour.brighter ( 0.5f ) : arrowColour );
        g.fillPath ( arrow );
    }

    void drawButtonText ( juce::Graphics& g, juce::TextButton& button,
                          bool, bool ) override
    {
        auto text = button.getButtonText();
        auto bounds = button.getLocalBounds().toFloat();
        float alpha = button.isEnabled() ? 1.0f : 0.5f;

        auto colour = button.findColour ( button.getToggleState()
            ? juce::TextButton::textColourOnId
            : juce::TextButton::textColourOffId ).withMultipliedAlpha ( alpha );
        g.setColour ( colour );

        if ( text.length() == 1 )
        {
            float iconSize = juce::jmin ( bounds.getWidth(), bounds.getHeight() ) * 0.4f;
            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();

            switch ( static_cast<int> ( text[0] ) )
            {
                case 0x25C0: drawIconTriangle ( g, cx, cy, iconSize, true );  return;
                case 0x25B6: drawIconTriangle ( g, cx, cy, iconSize, false ); return;
                case 0x2913: drawIconSave ( g, cx, cy, iconSize );            return;
                case 0x21A9: drawIconUndo ( g, cx, cy, iconSize );            return;
                case 0x21AA: drawIconRedo ( g, cx, cy, iconSize );            return;
                case 0x2665: drawIconHeart ( g, cx, cy, iconSize );           return;
                default: break;
            }
        }

        g.setFont ( Fonts::getBody ( bounds.getHeight() * 0.55f ) );
        g.drawText ( text, bounds, juce::Justification::centred, false );
    }

private:
    static void drawIconTriangle ( juce::Graphics& g, float cx, float cy, float s, bool left )
    {
        juce::Path p;
        float hw = s * 0.35f;
        float hh = s * 0.45f;
        if ( left )
            p.addTriangle ( cx + hw, cy - hh, cx + hw, cy + hh, cx - hh, cy );
        else
            p.addTriangle ( cx - hw, cy - hh, cx - hw, cy + hh, cx + hh, cy );
        g.fillPath ( p );
    }

    static void drawIconSave ( juce::Graphics& g, float cx, float cy, float s )
    {
        float stemW = s * 0.15f;
        g.fillRect ( cx - stemW, cy - s * 0.45f, stemW * 2.0f, s * 0.5f );
        juce::Path p;
        p.addTriangle ( cx - s * 0.4f, cy + s * 0.05f,
                        cx + s * 0.4f, cy + s * 0.05f,
                        cx, cy + s * 0.45f );
        g.fillPath ( p );
        g.drawLine ( cx - s * 0.5f, cy + s * 0.6f,
                     cx + s * 0.5f, cy + s * 0.6f, 1.5f );
    }

    static void drawIconUndo ( juce::Graphics& g, float cx, float cy, float s )
    {
        juce::Path curve;
        curve.startNewSubPath ( cx + s * 0.4f, cy + s * 0.15f );
        curve.cubicTo ( cx + s * 0.4f, cy - s * 0.5f,
                        cx - s * 0.4f, cy - s * 0.5f,
                        cx - s * 0.4f, cy + s * 0.15f );
        g.strokePath ( curve, juce::PathStrokeType ( 1.5f ) );
        juce::Path ah;
        ah.addTriangle ( cx - s * 0.4f, cy + s * 0.4f,
                         cx - s * 0.15f, cy,
                         cx - s * 0.65f, cy );
        g.fillPath ( ah );
    }

    static void drawIconRedo ( juce::Graphics& g, float cx, float cy, float s )
    {
        juce::Path curve;
        curve.startNewSubPath ( cx - s * 0.4f, cy + s * 0.15f );
        curve.cubicTo ( cx - s * 0.4f, cy - s * 0.5f,
                        cx + s * 0.4f, cy - s * 0.5f,
                        cx + s * 0.4f, cy + s * 0.15f );
        g.strokePath ( curve, juce::PathStrokeType ( 1.5f ) );
        juce::Path ah;
        ah.addTriangle ( cx + s * 0.4f, cy + s * 0.4f,
                         cx + s * 0.15f, cy,
                         cx + s * 0.65f, cy );
        g.fillPath ( ah );
    }

    static void drawIconHeart ( juce::Graphics& g, float cx, float cy, float s )
    {
        juce::Path p;
        float w = s * 0.52f;
        float h = s * 0.48f;
        // Left lobe
        p.startNewSubPath ( cx, cy + h * 0.85f );
        p.cubicTo ( cx - w * 1.2f, cy + h * 0.1f,
                    cx - w * 1.0f, cy - h * 1.0f,
                    cx, cy - h * 0.3f );
        // Right lobe
        p.cubicTo ( cx + w * 1.0f, cy - h * 1.0f,
                    cx + w * 1.2f, cy + h * 0.1f,
                    cx, cy + h * 0.85f );
        p.closeSubPath();
        g.fillPath ( p );
    }
};
