/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../presets/PresetManager.h"
#include "../../presets/PresetMetadata.h"
#include "../LookAndFeel/ColourPalette.h"

/**
 * Enhanced preset browser panel with search, tags, favorites, and categories.
 */
class PresetBrowserPanel : public juce::Component,
                           public juce::ListBoxModel,
                           public juce::KeyListener
{
public:
    explicit PresetBrowserPanel ( PresetManager& pm )
        : presetManager ( pm )
    {
        // Search box
        searchBox.setTextToShowWhenEmpty ( "Search presets...", ColourPalette::textDim );
        searchBox.onTextChange = [this] { rebuildFilteredList(); };
        addAndMakeVisible ( searchBox );

        // Category filter
        categoryFilter.addItem ( "All Categories", 1 );
        auto cats = presetManager.getCategories();
        for ( int i = 0; i < static_cast<int> ( cats.size() ); ++i )
            categoryFilter.addItem ( cats[static_cast<size_t> ( i )], i + 2 );
        categoryFilter.setSelectedId ( 1 );
        categoryFilter.onChange = [this] { rebuildFilteredList(); };
        addAndMakeVisible ( categoryFilter );

        // Favorites toggle
        favToggle.setClickingTogglesState ( true );
        favToggle.onClick = [this] { rebuildFilteredList(); };
        addAndMakeVisible ( favToggle );

        // Preset list
        listBox.setModel ( this );
        listBox.setRowHeight ( 28 );
        listBox.setColour ( juce::ListBox::backgroundColourId, ColourPalette::background );
        addAndMakeVisible ( listBox );

        // Import/Export buttons
        importButton.setColour ( juce::TextButton::buttonColourId, ColourPalette::background.brighter ( 0.1f ) );
        importButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textBright );
        importButton.onClick = [this] { showImportDialog(); };
        addAndMakeVisible ( importButton );

        exportButton.setColour ( juce::TextButton::buttonColourId, ColourPalette::background.brighter ( 0.1f ) );
        exportButton.setColour ( juce::TextButton::textColourOffId, ColourPalette::textBright );
        exportButton.onClick = [this] { showExportDialog(); };
        addAndMakeVisible ( exportButton );

        // Close button
        closeButton.onClick = [this] { setVisible ( false ); };
        addAndMakeVisible ( closeButton );

        addKeyListener ( this );
        rebuildFilteredList();
    }

    ~PresetBrowserPanel() override
    {
        removeKeyListener ( this );
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( ColourPalette::background.withAlpha ( 0.97f ) );
        g.setColour ( ColourPalette::bevelLight );
        g.drawRect ( getLocalBounds(), 1 );

        g.setColour ( ColourPalette::textBright );
        g.setFont ( 15.0f );
        g.drawText ( "Preset Browser", 10, 6, 200, 20, juce::Justification::centredLeft );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 8 );
        bounds.removeFromTop ( 24 ); // title area

        auto topRow = bounds.removeFromTop ( 28 );
        searchBox.setBounds ( topRow.removeFromLeft ( topRow.getWidth() / 2 - 4 ) );
        topRow.removeFromLeft ( 8 );
        categoryFilter.setBounds ( topRow.removeFromLeft ( topRow.getWidth() - 50 ) );
        topRow.removeFromLeft ( 4 );
        favToggle.setBounds ( topRow );

        importButton.setBounds ( getWidth() - 150, 6, 50, 22 );
        exportButton.setBounds ( getWidth() - 94, 6, 50, 22 );
        closeButton.setBounds ( getWidth() - 30, 6, 22, 22 );

        bounds.removeFromTop ( 6 );
        listBox.setBounds ( bounds );
    }

    // ListBoxModel
    int getNumRows() override { return static_cast<int> ( filteredEntries.size() ); }

    void paintListBoxItem ( int rowNumber, juce::Graphics& g, int width, int height,
                            bool rowIsSelected ) override
    {
        if ( rowNumber < 0 || rowNumber >= static_cast<int> ( filteredEntries.size() ) )
            return;

        auto& entry = filteredEntries[static_cast<size_t> ( rowNumber )];

        if ( rowIsSelected )
            g.fillAll ( ColourPalette::band1.withAlpha ( 0.25f ) );

        // Favorite heart
        {
            float hx = 14.0f, hy = static_cast<float> ( height ) * 0.5f;
            float hs = 10.0f;
            juce::Path heart;
            heart.startNewSubPath ( hx, hy + hs * 0.35f );
            heart.cubicTo ( hx - hs * 0.7f, hy - hs * 0.1f,
                            hx - hs * 0.4f, hy - hs * 0.55f,
                            hx, hy - hs * 0.2f );
            heart.cubicTo ( hx + hs * 0.4f, hy - hs * 0.55f,
                            hx + hs * 0.7f, hy - hs * 0.1f,
                            hx, hy + hs * 0.35f );
            heart.closeSubPath();
            g.setColour ( entry.isFavorite ? juce::Colour ( 0xffff4466 ) : ColourPalette::textDim );
            if ( entry.isFavorite )
                g.fillPath ( heart );
            else
                g.strokePath ( heart, juce::PathStrokeType ( 1.0f ) );
        }

        // Name
        g.setColour ( ColourPalette::textBright );
        g.setFont ( 13.0f );
        g.drawText ( entry.name, 26, 0, width / 2, height, juce::Justification::centredLeft );

        // Category
        g.setColour ( ColourPalette::textDim );
        g.setFont ( 11.0f );
        g.drawText ( entry.category, width / 2, 0, width / 4, height,
                     juce::Justification::centredLeft );

        // Tags
        juce::String tagStr = entry.tags.joinIntoString ( ", " );
        g.drawText ( tagStr, width * 3 / 4, 0, width / 4 - 4, height,
                     juce::Justification::centredRight );
    }

    void listBoxItemClicked ( int row, const juce::MouseEvent& e ) override
    {
        if ( row < 0 || row >= static_cast<int> ( filteredEntries.size() ) )
            return;

        auto& entry = filteredEntries[static_cast<size_t> ( row )];

        // Right-click context menu
        if ( e.mods.isPopupMenu() )
        {
            juce::PopupMenu contextMenu;
            contextMenu.addItem ( 1, entry.isFavorite ? "Unfavorite" : "Favorite" );
            if ( ! presetManager.getAllPresets()[static_cast<size_t> ( entry.presetIndex )].isFactory )
                contextMenu.addItem ( 2, "Delete" );

            contextMenu.showMenuAsync ( {}, [this, row] ( int result )
            {
                if ( row < 0 || row >= static_cast<int> ( filteredEntries.size() ) ) return;
                auto& fe = filteredEntries[static_cast<size_t> ( row )];
                if ( result == 1 )
                {
                    fe.isFavorite = ! fe.isFavorite;
                    metadataStore.setFavorite ( fe.name, fe.isFavorite );
                    listBox.repaint();
                }
                else if ( result == 2 )
                {
                    presetManager.deleteUserPreset ( fe.presetIndex );
                    rebuildFilteredList();
                }
            } );
            return;
        }

        // Left-click to load preset
        if ( entry.presetIndex >= 0 )
            presetManager.applyPreset ( entry.presetIndex );
    }

    void listBoxItemDoubleClicked ( int row, const juce::MouseEvent& ) override
    {
        if ( row >= 0 && row < static_cast<int> ( filteredEntries.size() ) )
        {
            auto& entry = filteredEntries[static_cast<size_t> ( row )];
            if ( entry.presetIndex >= 0 )
            {
                presetManager.applyPreset ( entry.presetIndex );
                setVisible ( false );
            }
        }
    }

    bool keyPressed ( const juce::KeyPress& key, juce::Component* ) override
    {
        if ( key == juce::KeyPress::escapeKey )
        {
            setVisible ( false );
            return true;
        }
        if ( key == juce::KeyPress::returnKey )
        {
            int sel = listBox.getSelectedRow();
            if ( sel >= 0 && sel < static_cast<int> ( filteredEntries.size() ) )
            {
                auto& entry = filteredEntries[static_cast<size_t> ( sel )];
                if ( entry.presetIndex >= 0 )
                    presetManager.applyPreset ( entry.presetIndex );
                setVisible ( false );
            }
            return true;
        }
        return false;
    }

private:
    void rebuildFilteredList()
    {
        filteredEntries.clear();

        auto& allPresets = presetManager.getAllPresets();
        juce::String query = searchBox.getText();
        bool favOnly = favToggle.getToggleState();
        int catId = categoryFilter.getSelectedId();
        juce::String selectedCat = ( catId > 1 ) ? categoryFilter.getItemText ( catId - 1 ) : "";

        for ( int i = 0; i < static_cast<int> ( allPresets.size() ); ++i )
        {
            auto& preset = allPresets[static_cast<size_t> ( i )];

            PresetMetadataEntry entry;
            entry.name = preset.name;
            entry.category = preset.category;
            entry.presetIndex = i;
            entry.isFavorite = metadataStore.isFavorite ( preset.name );

            // Category filter
            if ( selectedCat.isNotEmpty() && entry.category != selectedCat )
                continue;

            // Favorites filter
            if ( favOnly && ! entry.isFavorite )
                continue;

            // Search filter
            if ( query.isNotEmpty() && ! entry.matchesSearch ( query ) )
                continue;

            filteredEntries.push_back ( entry );
        }

        // Sort: favorites first, then alphabetical
        std::sort ( filteredEntries.begin(), filteredEntries.end(),
                    [] ( const PresetMetadataEntry& a, const PresetMetadataEntry& b )
                    {
                        if ( a.isFavorite != b.isFavorite )
                            return a.isFavorite;
                        return a.name.compareIgnoreCase ( b.name ) < 0;
                    } );

        listBox.updateContent();
        listBox.repaint();
    }

    void showImportDialog()
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Import Preset", juce::File(), "*.json" );

        fileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] ( const juce::FileChooser& fc )
            {
                auto result = fc.getResult();
                if ( result == juce::File() )
                    return;
                if ( presetManager.importPresetFromJSON ( result ) )
                {
                    categoryFilter.clear();
                    categoryFilter.addItem ( "All Categories", 1 );
                    auto cats = presetManager.getCategories();
                    for ( int i = 0; i < static_cast<int> ( cats.size() ); ++i )
                        categoryFilter.addItem ( cats[static_cast<size_t> ( i )], i + 2 );
                    categoryFilter.setSelectedId ( 1 );
                    rebuildFilteredList();
                }
            } );
    }

    void showExportDialog()
    {
        int sel = listBox.getSelectedRow();
        if ( sel < 0 || sel >= static_cast<int> ( filteredEntries.size() ) )
            return;

        auto& entry = filteredEntries[static_cast<size_t> ( sel )];
        auto suggestedName = juce::File::createLegalFileName ( entry.name ) + ".json";

        fileChooser = std::make_unique<juce::FileChooser> (
            "Export Preset",
            juce::File::getSpecialLocation ( juce::File::userDesktopDirectory )
                .getChildFile ( suggestedName ),
            "*.json" );

        auto presetIdx = entry.presetIndex;
        fileChooser->launchAsync (
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this, presetIdx] ( const juce::FileChooser& fc )
            {
                auto result = fc.getResult();
                if ( result == juce::File() )
                    return;
                presetManager.exportPresetToJSON ( presetIdx, result );
            } );
    }

    PresetManager& presetManager;
    PresetMetadataStore metadataStore;
    std::vector<PresetMetadataEntry> filteredEntries;

    juce::TextEditor searchBox;
    juce::ComboBox categoryFilter;
    juce::TextButton favToggle { juce::String::charToString ( 0x2665 ) }; // heart
    juce::TextButton importButton { "Import" };
    juce::TextButton exportButton { "Export" };
    juce::TextButton closeButton { "X" };
    juce::ListBox listBox;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( PresetBrowserPanel )
};
