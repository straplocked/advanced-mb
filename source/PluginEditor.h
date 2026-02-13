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

#include "PluginProcessor.h"
#include "ui/LookAndFeel/AdvancedMBLookAndFeel.h"
#include "ui/components/HeaderToolbar.h"
#include "ui/components/MultibandDisplay.h"
#include "ui/components/BandControlPanel.h"
#include "ui/components/BottomControlStrip.h"
#include "ui/components/LevelMeterComponent.h"
#include "utils/Constants.h"

class AdvancedMBEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit AdvancedMBEditor ( AdvancedMBProcessor& p );
    ~AdvancedMBEditor() override;

    void paint ( juce::Graphics& g ) override;
    void resized() override;

private:
    void timerCallback() override;

    AdvancedMBProcessor& processor;

    AdvancedMBLookAndFeel lookAndFeel;

    HeaderToolbar headerToolbar;
    MultibandDisplay multibandDisplay;
    BandControlPanel bandControlPanel;
    BottomControlStrip bottomControlStrip;
    LevelMeterComponent levelMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( AdvancedMBEditor )
};
