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
#include <map>
#include <atomic>

/**
 * @brief Maps MIDI CC numbers to APVTS parameters for the multiband compressor.
 *
 * Thread-safe: processMidi() runs on the audio thread while learn mode
 * is toggled from the UI thread.  Mappings are stored as ValueTree
 * children so they persist across sessions.
 */
class MIDILearnManager
{
public:
    explicit MIDILearnManager ( juce::AudioProcessorValueTreeState& apvts )
        : apvts ( apvts )
    {
    }

    //==============================================================================
    // Learn mode (called from UI thread)
    //==============================================================================

    bool isLearning() const { return learning.load(); }

    void startLearning ( const juce::String& parameterID )
    {
        learningParamID = parameterID;
        learning.store ( true );
    }

    void stopLearning()
    {
        learning.store ( false );
        learningParamID.clear();
    }

    juce::String getLearningParamID() const { return learningParamID; }

    //==============================================================================
    // Mapping management (called from UI thread)
    //==============================================================================

    void assignCC ( int ccNumber, const juce::String& parameterID )
    {
        juce::ScopedLock sl ( mappingLock );
        ccToParam[ccNumber] = parameterID;
    }

    void removeCC ( int ccNumber )
    {
        juce::ScopedLock sl ( mappingLock );
        ccToParam.erase ( ccNumber );
    }

    void clearAll()
    {
        juce::ScopedLock sl ( mappingLock );
        ccToParam.clear();
    }

    juce::String getParamForCC ( int ccNumber ) const
    {
        juce::ScopedLock sl ( mappingLock );
        auto it = ccToParam.find ( ccNumber );
        if ( it != ccToParam.end() )
            return it->second;
        return {};
    }

    std::map<int, juce::String> getAllMappings() const
    {
        juce::ScopedLock sl ( mappingLock );
        return ccToParam;
    }

    //==============================================================================
    // Audio thread processing
    //==============================================================================

    void processMidi ( juce::MidiBuffer& midiMessages )
    {
        for ( const auto metadata : midiMessages )
        {
            auto msg = metadata.getMessage();

            if ( msg.isController() )
            {
                int cc    = msg.getControllerNumber();
                int value = msg.getControllerValue();

                // Learn mode: assign this CC to the learning parameter
                if ( learning.load() )
                {
                    juce::ScopedLock sl ( mappingLock );
                    ccToParam[cc] = learningParamID;
                    learning.store ( false );
                }

                // Apply mapping
                juce::String paramID;
                {
                    juce::ScopedLock sl ( mappingLock );
                    auto it = ccToParam.find ( cc );
                    if ( it != ccToParam.end() )
                        paramID = it->second;
                }

                if ( paramID.isNotEmpty() )
                {
                    if ( auto* param = apvts.getParameter ( paramID ) )
                    {
                        float normalized = (float) value / 127.0f;
                        param->setValueNotifyingHost ( normalized );
                    }
                }
            }
        }
    }

    //==============================================================================
    // State persistence
    //==============================================================================

    void saveToState ( juce::ValueTree& parentState ) const
    {
        auto existing = parentState.getChildWithName ( "MIDILearnMappings" );
        if ( existing.isValid() )
            parentState.removeChild ( existing, nullptr );

        juce::ValueTree mappingTree ( "MIDILearnMappings" );

        juce::ScopedLock sl ( mappingLock );
        for ( auto& [cc, paramID] : ccToParam )
        {
            juce::ValueTree entry ( "Mapping" );
            entry.setProperty ( "cc", cc, nullptr );
            entry.setProperty ( "param", paramID, nullptr );
            mappingTree.addChild ( entry, -1, nullptr );
        }

        parentState.addChild ( mappingTree, -1, nullptr );
    }

    void loadFromState ( const juce::ValueTree& parentState )
    {
        auto mappingTree = parentState.getChildWithName ( "MIDILearnMappings" );
        if ( ! mappingTree.isValid() )
            return;

        juce::ScopedLock sl ( mappingLock );
        ccToParam.clear();

        for ( int i = 0; i < mappingTree.getNumChildren(); ++i )
        {
            auto entry = mappingTree.getChild ( i );
            int cc = entry.getProperty ( "cc", -1 );
            juce::String paramID = entry.getProperty ( "param", "" ).toString();
            if ( cc >= 0 && cc <= 127 && paramID.isNotEmpty() )
                ccToParam[cc] = paramID;
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    mutable juce::CriticalSection mappingLock;
    std::map<int, juce::String> ccToParam;

    std::atomic<bool> learning { false };
    juce::String learningParamID;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( MIDILearnManager )
};
