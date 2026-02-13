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

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdvancedMBProcessor::AdvancedMBProcessor()
    : AudioProcessor ( BusesProperties()
                       .withInput  ( "Input",    juce::AudioChannelSet::stereo(), true )
                       .withOutput ( "Output",   juce::AudioChannelSet::stereo(), true )
                       .withInput  ( "Sidechain", juce::AudioChannelSet::stereo(), false ) ),
      apvts ( *this, &undoManager, "AdvancedMBState", ParameterLayout::createLayout() ),
      presetManager ( apvts )
{
    paramAttachment.init ( apvts );
}

AdvancedMBProcessor::~AdvancedMBProcessor() = default;

//==============================================================================
const juce::String AdvancedMBProcessor::getName() const
{
    return JucePlugin_Name;
}

//==============================================================================
void AdvancedMBProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
    multibandEngine.prepare ( sampleRate, samplesPerBlock, getTotalNumOutputChannels() );
    fftProcessorPre.prepare ( sampleRate, samplesPerBlock );
    fftProcessorPost.prepare ( sampleRate, samplesPerBlock );
    levelMeterData.prepare ( sampleRate );
}

void AdvancedMBProcessor::releaseResources()
{
    multibandEngine.reset();
    fftProcessorPre.reset();
    fftProcessorPost.reset();
    levelMeterData.reset();
}

//==============================================================================
bool AdvancedMBProcessor::isBusesLayoutSupported ( const BusesLayout& layouts ) const
{
    auto mainOutput = layouts.getMainOutputChannelSet();
    auto mainInput  = layouts.getMainInputChannelSet();

    // Only support mono or stereo main buses
    if ( mainOutput != juce::AudioChannelSet::mono()
         && mainOutput != juce::AudioChannelSet::stereo() )
        return false;

    // Input and output must match
    if ( mainOutput != mainInput )
        return false;

    // Sidechain can be disabled, mono, or stereo
    if ( layouts.inputBuses.size() > 1 )
    {
        auto sc = layouts.getChannelSet ( true, 1 );

        if ( ! sc.isDisabled()
             && sc != juce::AudioChannelSet::mono()
             && sc != juce::AudioChannelSet::stereo() )
            return false;
    }

    return true;
}

//==============================================================================
void AdvancedMBProcessor::processBlock ( juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& )
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for ( auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i )
        buffer.clear ( i, 0, buffer.getNumSamples() );

    // Read parameters from the APVTS via the attachment
    auto params = paramAttachment.getCurrentParameters();
    multibandEngine.updateParameters ( params );

    // Determine analyzer state
    bool analyzerPre  = ( params.analyzerMode == Constants::AnalyzerPre
                          || params.analyzerMode == Constants::AnalyzerPrePost );
    bool analyzerPost = ( params.analyzerMode == Constants::AnalyzerPost
                          || params.analyzerMode == Constants::AnalyzerPrePost );

    // Push pre-compression spectrum data
    if ( analyzerPre )
        fftProcessorPre.pushSamples ( buffer, buffer.getNumSamples() );

    // Push input metering
    levelMeterData.pushInputLevel ( buffer );

    // Run the multiband compression engine
    multibandEngine.process ( buffer );

    // Push per-band gain reduction to meter data
    for ( int b = 0; b < params.numBands; ++b )
        levelMeterData.pushBandGainReduction ( b, multibandEngine.getBandGainReduction ( b ) );

    // Push output metering
    levelMeterData.pushOutputLevel ( buffer );

    // Push post-compression spectrum data
    if ( analyzerPost )
        fftProcessorPost.pushSamples ( buffer, buffer.getNumSamples() );

    // Report latency from the oversampler / linear-phase crossover
    setLatencySamples ( multibandEngine.getLatencySamples() );
}

//==============================================================================
juce::AudioProcessorEditor* AdvancedMBProcessor::createEditor()
{
    return new AdvancedMBEditor ( *this );
}

//==============================================================================
void AdvancedMBProcessor::getStateInformation ( juce::MemoryBlock& destData )
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml ( state.createXml() );
    copyXmlToBinary ( *xml, destData );
}

void AdvancedMBProcessor::setStateInformation ( const void* data, int sizeInBytes )
{
    std::unique_ptr<juce::XmlElement> xml ( getXmlFromBinary ( data, sizeInBytes ) );

    if ( xml != nullptr && xml->hasTagName ( apvts.state.getType() ) )
    {
        auto state = juce::ValueTree::fromXml ( *xml );
        apvts.replaceState ( state );
    }
}

//==============================================================================
// FFT access
//==============================================================================

FFTProcessor& AdvancedMBProcessor::getPreFFT()
{
    return fftProcessorPre;
}

FFTProcessor& AdvancedMBProcessor::getPostFFT()
{
    return fftProcessorPost;
}

//==============================================================================
// Per-band gain reduction
//==============================================================================

float AdvancedMBProcessor::getBandGainReduction ( int bandIndex ) const
{
    return multibandEngine.getBandGainReduction ( bandIndex );
}

//==============================================================================
// A/B state comparison
//==============================================================================

void AdvancedMBProcessor::copyToA()
{
    stateSlotA.reset();
    getStateInformation ( stateSlotA );
}

void AdvancedMBProcessor::copyToB()
{
    stateSlotB.reset();
    getStateInformation ( stateSlotB );
}

void AdvancedMBProcessor::swapAB()
{
    // Save current state into a temporary block
    juce::MemoryBlock current;
    getStateInformation ( current );

    // If A has data, load A as the current state, then store the old current into A
    // and the old A into B (effectively swapping the active state with the stored one)
    // Simpler approach: swap A and B, then load whichever was previously the other slot.
    std::swap ( stateSlotA, stateSlotB );

    // Load slot A as the active state (convention: A is the "active" comparison slot)
    if ( stateSlotA.getSize() > 0 )
        setStateInformation ( stateSlotA.getData(), static_cast<int> ( stateSlotA.getSize() ) );
}

//==============================================================================
// Plugin instantiation
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdvancedMBProcessor();
}
