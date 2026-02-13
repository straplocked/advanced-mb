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
 */
class BandManager
{
public:
    BandManager() = default;

    /** Store the APVTS pointer and initialise slot order to identity. */
    void init (juce::AudioProcessorValueTreeState& apvts);

    /** Allocate a free slot, reset it to transparent defaults, create a Band,
        insert in frequency order, recompute crossovers, and update atomics.
        @return the allocated slot index, or -1 if no slot is available. */
    int addBand (float clickFreqHz);

    /** Remove the band at the given position (index into the frequency-ordered
        list), free its slot, recompute crossovers, and update atomics.
        @return the freed slot index, or -1 on failure. */
    int removeBand (int bandPosition);

    /** Atomic read of the number of currently active bands. */
    int getNumActiveBands() const;

    /** Atomic read of which slot a given frequency-ordered position maps to. */
    int getSlotForPosition (int position) const;

    /** Direct access to the frequency-ordered band list (message thread only). */
    const std::vector<Band>& getActiveBands() const;

    /** Serialize the band mapping into a "BandSlotMapping" child of @p state. */
    void writeToState (juce::ValueTree& state) const;

    /** Deserialize from a "BandSlotMapping" child of @p state. */
    void readFromState (const juce::ValueTree& state);

private:
    void recomputeCrossovers();
    void resetSlotToDefaults (int slotIndex);
    int  allocateSlot();
    void freeSlot (int slotIndex);
    void updateAtomics();

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;

    std::vector<Band> activeBands;                              // ordered by frequency
    std::array<bool, Constants::kMaxBands> slotUsed {};         // pool tracking

    std::atomic<int> numActive { 0 };
    std::array<std::atomic<int>, Constants::kMaxBands> slotOrder;
};
