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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "parameters/ParameterLayout.h"
#include "parameters/MultibandParameterAttachment.h"
#include "dsp/MultibandEngine.h"
#include "analyzer/FFTProcessor.h"
#include "metering/LevelMeterData.h"
#include "presets/PresetManager.h"
#include "utils/Constants.h"

class AdvancedMBProcessor : public juce::AudioProcessor
{
public:
    AdvancedMBProcessor();
    ~AdvancedMBProcessor() override;

    void prepareToPlay ( double sampleRate, int samplesPerBlock ) override;
    void releaseResources() override;
    bool isBusesLayoutSupported ( const BusesLayout& layouts ) const override;
    void processBlock ( juce::AudioBuffer<float>&, juce::MidiBuffer& ) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram ( int ) override {}
    const juce::String getProgramName ( int ) override { return {}; }
    void changeProgramName ( int, const juce::String& ) override {}

    void getStateInformation ( juce::MemoryBlock& destData ) override;
    void setStateInformation ( const void* data, int sizeInBytes ) override;

    //==============================================================================
    // Accessors for the editor and other components
    //==============================================================================

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    MultibandEngine& getMultibandEngine() { return multibandEngine; }
    MultibandParameterAttachment& getParameterAttachment() { return paramAttachment; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    LevelMeterData& getLevelMeterData() { return levelMeterData; }
    PresetManager& getPresetManager() { return presetManager; }

    // FFT data access for analyzer
    FFTProcessor& getPreFFT();
    FFTProcessor& getPostFFT();

    // Per-band gain reduction for metering in the editor
    float getBandGainReduction ( int bandIndex ) const;

    // A/B state comparison
    void copyToA();
    void copyToB();
    void swapAB();

private:
    juce::UndoManager undoManager { 30000, 30 };
    juce::AudioProcessorValueTreeState apvts;

    MultibandEngine multibandEngine;
    MultibandParameterAttachment paramAttachment;

    FFTProcessor fftProcessorPre;
    FFTProcessor fftProcessorPost;

    LevelMeterData levelMeterData;

    PresetManager presetManager;

    // A/B state storage
    juce::MemoryBlock stateSlotA;
    juce::MemoryBlock stateSlotB;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( AdvancedMBProcessor )
};
