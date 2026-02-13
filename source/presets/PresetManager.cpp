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
    presets.clear();

    // Factory presets
    for ( auto& fp : FactoryPresets::getAll() )
    {
        Preset p;
        p.name = fp.name;
        p.category = fp.category;
        p.isFactory = true;
        presets.push_back ( p );
    }

    // User presets from disk
    auto dir = getUserPresetsDirectory();
    if ( dir.isDirectory() )
    {
        for ( auto& file : dir.findChildFiles ( juce::File::findFiles, false, "*.xml" ) )
        {
            auto name = file.getFileNameWithoutExtension();

            // Don't duplicate factory preset names
            bool duplicate = false;
            for ( auto& existing : presets )
                if ( existing.name == name ) { duplicate = true; break; }

            if ( ! duplicate )
            {
                Preset p;
                p.name = name;
                p.category = "User";
                p.isFactory = false;
                p.file = file;
                presets.push_back ( p );
            }
        }
    }
}

juce::StringArray PresetManager::getPresetList() const
{
    juce::StringArray list;
    for ( auto& p : presets )
        list.add ( p.name );
    return list;
}

juce::String PresetManager::getCurrentPresetName() const
{
    return currentPresetName;
}

//==============================================================================
// Save / Load / Delete (string-based legacy API)
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
    if ( presets.empty() )
        return;

    int idx = getCurrentPresetIndex();
    int next = ( idx + 1 ) % static_cast<int> ( presets.size() );
    applyPreset ( next );
}

void PresetManager::loadPreviousPreset()
{
    if ( presets.empty() )
        return;

    int idx = getCurrentPresetIndex();
    int prev = idx - 1;
    if ( prev < 0 )
        prev = static_cast<int> ( presets.size() ) - 1;
    applyPreset ( prev );
}

//==============================================================================
// Index-based API
//==============================================================================

std::vector<juce::String> PresetManager::getCategories() const
{
    std::vector<juce::String> cats;
    for ( auto& p : presets )
    {
        bool found = false;
        for ( auto& c : cats )
            if ( c == p.category ) { found = true; break; }
        if ( ! found )
            cats.push_back ( p.category );
    }
    return cats;
}

void PresetManager::applyPreset ( int presetIndex )
{
    if ( presetIndex < 0 || presetIndex >= static_cast<int> ( presets.size() ) )
        return;

    auto& preset = presets[static_cast<size_t> ( presetIndex )];
    loadPreset ( preset.name );
}

bool PresetManager::saveCurrentAsPreset ( const juce::String& name, const juce::String& )
{
    if ( name.isEmpty() )
        return false;

    savePreset ( name );
    return true;
}

bool PresetManager::deleteUserPreset ( int presetIndex )
{
    if ( presetIndex < 0 || presetIndex >= static_cast<int> ( presets.size() ) )
        return false;

    auto& preset = presets[static_cast<size_t> ( presetIndex )];
    if ( preset.isFactory )
        return false;

    deletePreset ( preset.name );
    return true;
}

bool PresetManager::exportPresetToJSON ( int presetIndex, const juce::File& file )
{
    if ( presetIndex < 0 || presetIndex >= static_cast<int> ( presets.size() ) )
        return false;

    // Apply preset first so state is current
    applyPreset ( presetIndex );

    auto state = apvts.copyState();
    auto xml = state.createXml();
    if ( xml == nullptr )
        return false;

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ( "name", presets[static_cast<size_t> ( presetIndex )].name );
    obj->setProperty ( "category", presets[static_cast<size_t> ( presetIndex )].category );
    obj->setProperty ( "state", xml->toString() );

    file.replaceWithText ( juce::JSON::toString ( juce::var ( obj.get() ) ) );
    return true;
}

bool PresetManager::importPresetFromJSON ( const juce::File& file )
{
    auto json = juce::JSON::parse ( file.loadFileAsString() );
    if ( ! json.isObject() )
        return false;

    auto* obj = json.getDynamicObject();
    if ( obj == nullptr )
        return false;

    auto name = obj->getProperty ( "name" ).toString();
    auto stateStr = obj->getProperty ( "state" ).toString();

    if ( name.isEmpty() || stateStr.isEmpty() )
        return false;

    auto xml = juce::XmlDocument::parse ( stateStr );
    if ( xml == nullptr )
        return false;

    auto newState = juce::ValueTree::fromXml ( *xml );
    if ( ! newState.isValid() )
        return false;

    // Save as a user preset file
    auto dir = getUserPresetsDirectory();
    dir.createDirectory();

    auto safeName = juce::File::createLegalFileName ( name );
    auto presetFile = dir.getChildFile ( safeName + ".xml" );

    juce::XmlElement wrapper ( "AdvancedMBPreset" );
    wrapper.setAttribute ( "name", name );
    wrapper.addChildElement ( new juce::XmlElement ( *xml ) );
    wrapper.getChildElement ( 0 )->setTagName ( "ParameterState" );
    wrapper.writeTo ( presetFile );

    rebuildPresetList();
    notifyListeners();
    return true;
}

int PresetManager::getCurrentPresetIndex() const
{
    for ( int i = 0; i < static_cast<int> ( presets.size() ); ++i )
        if ( presets[static_cast<size_t> ( i )].name == currentPresetName )
            return i;
    return 0;
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
