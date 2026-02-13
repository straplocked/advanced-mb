/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <map>

/**
 * Metadata for a preset entry in the enhanced browser.
 */
struct PresetMetadataEntry
{
    juce::String name;
    juce::String category;
    juce::String author;
    juce::String description;
    juce::StringArray tags;
    bool isFavorite = false;
    int presetIndex = -1;

    bool matchesSearch ( const juce::String& query ) const
    {
        auto q = query.toLowerCase();
        if ( name.toLowerCase().contains ( q ) ) return true;
        if ( category.toLowerCase().contains ( q ) ) return true;
        if ( description.toLowerCase().contains ( q ) ) return true;
        if ( author.toLowerCase().contains ( q ) ) return true;
        for ( auto& tag : tags )
            if ( tag.toLowerCase().contains ( q ) ) return true;
        return false;
    }

    bool hasTag ( const juce::String& tag ) const
    {
        for ( auto& t : tags )
            if ( t.equalsIgnoreCase ( tag ) ) return true;
        return false;
    }
};

/**
 * Persistence layer for preset metadata (favorites, tags, etc.)
 */
class PresetMetadataStore
{
public:
    PresetMetadataStore()
    {
        metadataFile = juce::File::getSpecialLocation ( juce::File::userApplicationDataDirectory )
#if JUCE_MAC
                          .getChildFile ( "Application Support" )
#endif
                          .getChildFile ( "AdvancedMB" )
                          .getChildFile ( "metadata.json" );
        load();
    }

    void load()
    {
        if ( ! metadataFile.existsAsFile() )
            return;

        auto json = juce::JSON::parse ( metadataFile.loadFileAsString() );
        if ( ! json.isArray() )
            return;

        auto* arr = json.getArray();
        for ( auto& item : *arr )
        {
            if ( auto* obj = item.getDynamicObject() )
            {
                auto name = obj->getProperty ( "name" ).toString();
                bool fav = (bool) obj->getProperty ( "favorite" );
                favorites[name] = fav;
            }
        }
    }

    void save()
    {
        metadataFile.getParentDirectory().createDirectory();

        juce::Array<juce::var> arr;
        for ( auto& [name, fav] : favorites )
        {
            juce::DynamicObject::Ptr obj = new juce::DynamicObject();
            obj->setProperty ( "name", name );
            obj->setProperty ( "favorite", fav );
            arr.add ( juce::var ( obj.get() ) );
        }

        metadataFile.replaceWithText ( juce::JSON::toString ( juce::var ( arr ) ) );
    }

    bool isFavorite ( const juce::String& name ) const
    {
        auto it = favorites.find ( name );
        return it != favorites.end() && it->second;
    }

    void setFavorite ( const juce::String& name, bool fav )
    {
        favorites[name] = fav;
        save();
    }

private:
    juce::File metadataFile;
    std::map<juce::String, bool> favorites;
};
