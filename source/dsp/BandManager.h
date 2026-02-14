/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "Band.h"
#include "../utils/Constants.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

/**
 * Manages dynamic band creation/removal with a pool of 6 APVTS parameter slots.
 *
 * Active bands are kept ordered by frequency. Each band maps to one of the
 * pre-allocated parameter slots (band0-band5) in the APVTS.  Atomic slot-order
 * and band-count members allow the audio thread to read the current mapping
 * without locks.
 *
 * Region model: For N user bands, there are up to 2N+1 regions (user bands +
 * gaps between them). The audio thread reads region data via atomic arrays.
 */
class BandManager
{
public:
    BandManager() = default;

    /** Store the APVTS pointer and initialise slot order to identity. */
    void init (juce::AudioProcessorValueTreeState& apvts);

    /** Allocate a free slot, reset it to transparent defaults, create a Band
        centered on clickFreqHz (~2 octaves wide, clamped to gaps), insert in
        frequency order, recompute regions, and update atomics.
        @return the allocated slot index, or -1 if no slot is available or
                the click is inside an existing band. */
    int addBand (float clickFreqHz);

    /** Remove the band at the given position (index into the frequency-ordered
        list), free its slot, recompute regions, and update atomics.
        @return the freed slot index, or -1 on failure. */
    int removeBand (int bandPosition);

    /** Atomic read of the number of currently active bands. */
    int getNumActiveBands() const;

    /** Atomic read of which slot a given frequency-ordered position maps to. */
    int getSlotForPosition (int position) const;

    /** Direct access to the frequency-ordered band list (message thread only). */
    const std::vector<Band>& getActiveBands() const;

    /** Direct access to the computed region list (message thread only). */
    const std::vector<Region>& getRegions() const;
    int getNumRegions() const;

    // --- Atomic region data for audio thread ---
    int getNumRegionsAtomic() const;
    int getRegionSlotForPosition (int regionPos) const;
    float getRegionCrossoverFreq (int index) const;

    /** Serialize the band mapping into a "BandSlotMapping" child of @p state. */
    void writeToState (juce::ValueTree& state) const;

    /** Deserialize from a "BandSlotMapping" child of @p state. */
    void readFromState (const juce::ValueTree& state);

    /** Update a band's left or right edge. Called during crossover handle drag.
        @param bandPosition  the frequency-ordered band index
        @param isLeftEdge    true to move left edge, false for right edge
        @param newFreqHz     the new frequency for the edge */
    void setBandEdge (int bandPosition, bool isLeftEdge, float newFreqHz);

private:
    void recomputeRegions();
    void resetSlotToDefaults (int slotIndex);
    int  allocateSlot();
    void freeSlot (int slotIndex);
    void updateAtomics();

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;

    std::vector<Band> activeBands;                              // ordered by frequency
    std::vector<Region> regions;                                // computed from activeBands
    std::array<bool, Constants::kMaxBands> slotUsed {};         // pool tracking

    // Atomic data for audio thread — bands
    std::atomic<int> numActive { 0 };
    std::array<std::atomic<int>, Constants::kMaxBands> slotOrder;

    // Atomic data for audio thread — regions
    std::atomic<int> numRegionsAtomic { 0 };
    std::array<std::atomic<int>, Constants::kMaxRegions> regionSlotOrder;
    std::array<std::atomic<float>, Constants::kMaxRegionCrossovers> regionCrossoverFreqs;
};
