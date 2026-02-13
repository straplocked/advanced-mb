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

#include "PresetManager.h"
#include "FactoryPresets.h"

//==============================================================================
PresetManager::PresetManager ( juce::AudioProcessorValueTreeState& vts )
    : apvts ( vts )
{
    rebuildPresetList();
}

//==============================================================================
// Directory helpers
//==============================================================================

juce::File PresetManager::getUserPresetsDirectory() const
{
    auto appData = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory );

#if JUCE_MAC
    auto dir = appData.getChildFile ( "Application Support" )
                      .getChildFile ( "AdvancedMB" )
                      .getChildFile ( "User Presets" );
#else
    auto dir = appData.getChildFile ( "AdvancedMB" )
                      .getChildFile ( "User Presets" );
#endif
    return dir;
}

//==============================================================================
// Preset list management
//==============================================================================

void PresetManager::rebuildPresetList()
{
    presetList.clear();

    // Factory presets
    for ( auto& fp : FactoryPresets::getAll() )
        presetList.add ( fp.name );

    // User presets from disk
    auto dir = getUserPresetsDirectory();
    if ( dir.isDirectory() )
    {
        for ( auto& file : dir.findChildFiles ( juce::File::findFiles, false, "*.xml" ) )
        {
            auto name = file.getFileNameWithoutExtension();
            if ( ! presetList.contains ( name ) )
                presetList.add ( name );
        }
    }
}

juce::StringArray PresetManager::getPresetList() const
{
    return presetList;
}

juce::String PresetManager::getCurrentPresetName() const
{
    return currentPresetName;
}

//==============================================================================
// Save / Load / Delete
//==============================================================================

void PresetManager::savePreset ( const juce::String& name )
{
    if ( name.isEmpty() )
        return;

    auto dir = getUserPresetsDirectory();
    dir.createDirectory();

    auto safeName = juce::File::createLegalFileName ( name );
    auto file = dir.getChildFile ( safeName + ".xml" );

    saveStateToFile ( file );

    currentPresetName = name;
    rebuildPresetList();
    notifyListeners();
}

void PresetManager::loadPreset ( const juce::String& name )
{
    if ( name.isEmpty() )
        return;

    // Check factory presets first
    for ( auto& fp : FactoryPresets::getAll() )
    {
        if ( fp.name == name )
        {
            applyFactoryPresetValues ( name );
            currentPresetName = name;
            notifyListeners();
            return;
        }
    }

    // Try user preset file
    auto dir = getUserPresetsDirectory();
    auto safeName = juce::File::createLegalFileName ( name );
    auto file = dir.getChildFile ( safeName + ".xml" );

    if ( file.existsAsFile() && loadStateFromFile ( file ) )
    {
        currentPresetName = name;
        notifyListeners();
    }
}

void PresetManager::deletePreset ( const juce::String& name )
{
    if ( name.isEmpty() )
        return;

    // Only allow deleting user presets (not factory ones)
    for ( auto& fp : FactoryPresets::getAll() )
        if ( fp.name == name )
            return;

    auto dir = getUserPresetsDirectory();
    auto safeName = juce::File::createLegalFileName ( name );
    auto file = dir.getChildFile ( safeName + ".xml" );

    if ( file.existsAsFile() )
        file.deleteFile();

    if ( currentPresetName == name )
        currentPresetName = "Init";

    rebuildPresetList();
    notifyListeners();
}

//==============================================================================
// Navigation
//==============================================================================

void PresetManager::loadNextPreset()
{
    if ( presetList.isEmpty() )
        return;

    int idx = presetList.indexOf ( currentPresetName );
    int next = ( idx + 1 ) % presetList.size();
    loadPreset ( presetList[next] );
}

void PresetManager::loadPreviousPreset()
{
    if ( presetList.isEmpty() )
        return;

    int idx = presetList.indexOf ( currentPresetName );
    int prev = idx - 1;
    if ( prev < 0 )
        prev = presetList.size() - 1;
    loadPreset ( presetList[prev] );
}

//==============================================================================
// Factory preset helpers
//==============================================================================

juce::StringArray PresetManager::getFactoryCategories() const
{
    juce::StringArray cats;
    for ( auto& fp : FactoryPresets::getAll() )
        if ( ! cats.contains ( fp.category ) )
            cats.add ( fp.category );
    return cats;
}

juce::StringArray PresetManager::getFactoryPresetsInCategory ( const juce::String& category ) const
{
    juce::StringArray result;
    for ( auto& fp : FactoryPresets::getAll() )
        if ( fp.category == category )
            result.add ( fp.name );
    return result;
}

void PresetManager::loadFactoryPreset ( const juce::String& name )
{
    loadPreset ( name );
}

//==============================================================================
// State serialisation
//==============================================================================

void PresetManager::saveStateToFile ( const juce::File& file )
{
    auto state = apvts.copyState();
    auto paramXml = state.createXml();
    if ( paramXml == nullptr )
        return;

    juce::XmlElement wrapper ( "AdvancedMBPreset" );
    wrapper.setAttribute ( "name", currentPresetName );
    wrapper.addChildElement ( new juce::XmlElement ( *paramXml ) );
    wrapper.getChildElement ( 0 )->setTagName ( "ParameterState" );

    wrapper.writeTo ( file );
}

bool PresetManager::loadStateFromFile ( const juce::File& file )
{
    auto xml = juce::XmlDocument::parse ( file );
    if ( xml == nullptr )
        return false;

    // Wrapper format
    if ( xml->getTagName() == "AdvancedMBPreset" )
    {
        if ( auto* paramXml = xml->getChildByName ( "ParameterState" ) )
        {
            auto newState = juce::ValueTree::fromXml ( *paramXml );
            if ( newState.isValid() )
            {
                apvts.replaceState ( newState );
                return true;
            }
        }
        return false;
    }

    // Legacy: raw APVTS ValueTree
    auto newState = juce::ValueTree::fromXml ( *xml );
    if ( newState.isValid() )
    {
        apvts.replaceState ( newState );
        return true;
    }

    return false;
}

void PresetManager::applyFactoryPresetValues ( const juce::String& name )
{
    auto setParam = [this] ( const juce::String& id, float value )
    {
        if ( auto* p = apvts.getParameter ( id ) )
            p->setValueNotifyingHost ( p->convertTo0to1 ( value ) );
    };

    for ( auto& fp : FactoryPresets::getAll() )
    {
        if ( fp.name != name )
            continue;

        for ( auto& [paramID, value] : fp.values )
            setParam ( paramID, value );

        return;
    }
}

//==============================================================================
void PresetManager::notifyListeners()
{
    listeners.call ( [] ( Listener& l ) { l.presetChanged(); } );
}
