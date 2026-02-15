/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "BandManager.h"
#include "../parameters/ParameterIDs.h"
#include "../utils/MathUtils.h"
#include <algorithm>
#include <cmath>

//==============================================================================
void BandManager::init (juce::AudioProcessorValueTreeState& apvts)
{
    apvtsPtr = &apvts;

    // Identity mapping for slot order
    for (int i = 0; i < Constants::kMaxBands; ++i)
        slotOrder[static_cast<size_t> (i)].store (i, std::memory_order_relaxed);

    for (int i = 0; i < Constants::kMaxRegions; ++i)
        regionSlotOrder[static_cast<size_t> (i)].store (-1, std::memory_order_relaxed);

    for (int i = 0; i < Constants::kMaxRegionCrossovers; ++i)
        regionCrossoverFreqs[static_cast<size_t> (i)].store (1000.0f, std::memory_order_relaxed);

    numActive.store (0, std::memory_order_relaxed);
    numRegionsAtomic.store (0, std::memory_order_relaxed);

    slotUsed.fill (false);
    activeBands.clear();
    regions.clear();
}

//==============================================================================
int BandManager::addBand (float clickFreqHz)
{
    if (apvtsPtr == nullptr)
        return -1;

    if (static_cast<int> (activeBands.size()) >= Constants::kMaxBands)
        return -1;

    // Reject click inside an existing band
    for (const auto& band : activeBands)
    {
        if (clickFreqHz >= band.leftFreqHz && clickFreqHz <= band.rightFreqHz)
            return -1;
    }

    int slot = allocateSlot();
    if (slot < 0)
        return -1;

    resetSlotToDefaults (slot);

    // Compute centered band: default 2 octaves wide (F/2 to F*2)
    float halfWidthOctaves = Constants::kDefaultBandWidthOctaves * 0.5f;
    float proposedLeft  = clickFreqHz / std::pow (2.0f, halfWidthOctaves);
    float proposedRight = clickFreqHz * std::pow (2.0f, halfWidthOctaves);

    // Clamp to [20Hz, 20kHz]
    proposedLeft  = std::max (proposedLeft,  Constants::kMinFrequency);
    proposedRight = std::min (proposedRight, Constants::kMaxFrequency);

    // Find the gap the click is in and clamp to that gap
    float gapLeft  = Constants::kMinFrequency;
    float gapRight = Constants::kMaxFrequency;

    for (const auto& band : activeBands)
    {
        if (band.rightFreqHz <= clickFreqHz)
            gapLeft = std::max (gapLeft, band.rightFreqHz);
        if (band.leftFreqHz >= clickFreqHz)
            gapRight = std::min (gapRight, band.leftFreqHz);
    }

    proposedLeft  = std::max (proposedLeft,  gapLeft);
    proposedRight = std::min (proposedRight, gapRight);

    // Enforce minimum width
    float minWidthFactor = std::pow (2.0f, Constants::kMinBandWidthOctaves);
    if (proposedRight < proposedLeft * minWidthFactor)
    {
        freeSlot (slot);
        return -1;
    }

    Band newBand;
    newBand.slotIndex   = slot;
    newBand.leftFreqHz  = proposedLeft;
    newBand.rightFreqHz = proposedRight;

    // Insert in frequency order
    auto insertIt = std::lower_bound (
        activeBands.begin(), activeBands.end(), newBand,
        [] (const Band& a, const Band& b)
        {
            return a.leftFreqHz < b.leftFreqHz;
        });

    activeBands.insert (insertIt, newBand);

    recomputeRegions();

    // Update the APVTS numBands parameter
    if (auto* param = apvtsPtr->getParameter (ParameterIDs::Global::numBands))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (activeBands.size())));

    updateAtomics();

    return slot;
}

//==============================================================================
int BandManager::removeBand (int bandPosition)
{
    if (apvtsPtr == nullptr)
        return -1;

    if (bandPosition < 0 || bandPosition >= static_cast<int> (activeBands.size()))
        return -1;

    int freedSlot = activeBands[static_cast<size_t> (bandPosition)].slotIndex;

    activeBands.erase (activeBands.begin() + bandPosition);
    freeSlot (freedSlot);

    recomputeRegions();

    if (auto* param = apvtsPtr->getParameter (ParameterIDs::Global::numBands))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (activeBands.size())));

    updateAtomics();

    return freedSlot;
}

//==============================================================================
int BandManager::getNumActiveBands() const
{
    return numActive.load (std::memory_order_relaxed);
}

int BandManager::getSlotForPosition (int position) const
{
    if (position < 0 || position >= Constants::kMaxBands)
        return -1;

    return slotOrder[static_cast<size_t> (position)].load (std::memory_order_relaxed);
}

const std::vector<Band>& BandManager::getActiveBands() const
{
    return activeBands;
}

const std::vector<Region>& BandManager::getRegions() const
{
    return regions;
}

int BandManager::getNumRegions() const
{
    return static_cast<int> (regions.size());
}

//==============================================================================
// Atomic region data for audio thread
//==============================================================================

int BandManager::getNumRegionsAtomic() const
{
    return numRegionsAtomic.load (std::memory_order_acquire);
}

int BandManager::getRegionSlotForPosition (int regionPos) const
{
    if (regionPos < 0 || regionPos >= Constants::kMaxRegions)
        return -1;

    return regionSlotOrder[static_cast<size_t> (regionPos)].load (std::memory_order_relaxed);
}

float BandManager::getRegionCrossoverFreq (int index) const
{
    if (index < 0 || index >= Constants::kMaxRegionCrossovers)
        return 1000.0f;

    return regionCrossoverFreqs[static_cast<size_t> (index)].load (std::memory_order_relaxed);
}

//==============================================================================
void BandManager::setBandEdge (int bandPosition, bool isLeftEdge, float newFreqHz)
{
    if (bandPosition < 0 || bandPosition >= static_cast<int> (activeBands.size()))
        return;

    newFreqHz = std::clamp (newFreqHz, Constants::kMinFrequency, Constants::kMaxFrequency);

    auto& band = activeBands[static_cast<size_t> (bandPosition)];
    const float minWidthFactor = std::pow (2.0f, Constants::kMinBandWidthOctaves);

    if (isLeftEdge)
    {
        // Check if this left edge is shared with the previous band's right edge
        bool shared = (bandPosition > 0) &&
            std::abs (band.leftFreqHz - activeBands[static_cast<size_t> (bandPosition - 1)].rightFreqHz) < 0.5f;

        if (shared)
        {
            // Shared boundary: move both edges together, clamped by both bands' min widths
            auto& prevBand = activeBands[static_cast<size_t> (bandPosition - 1)];

            float minPos = prevBand.leftFreqHz * minWidthFactor;
            float maxPos = band.rightFreqHz / minWidthFactor;

            // Also can't go past the band before prev or after current
            if (bandPosition > 1)
                minPos = std::max (minPos, activeBands[static_cast<size_t> (bandPosition - 2)].rightFreqHz);

            newFreqHz = std::clamp (newFreqHz, minPos, maxPos);
            prevBand.rightFreqHz = newFreqHz;
            band.leftFreqHz = newFreqHz;
        }
        else
        {
            float minLeft = Constants::kMinFrequency;
            if (bandPosition > 0)
                minLeft = activeBands[static_cast<size_t> (bandPosition - 1)].rightFreqHz;

            float maxLeft = band.rightFreqHz / minWidthFactor;
            newFreqHz = std::clamp (newFreqHz, minLeft, maxLeft);
            band.leftFreqHz = newFreqHz;
        }
    }
    else
    {
        // Check if this right edge is shared with the next band's left edge
        bool shared = (bandPosition + 1 < static_cast<int> (activeBands.size())) &&
            std::abs (band.rightFreqHz - activeBands[static_cast<size_t> (bandPosition + 1)].leftFreqHz) < 0.5f;

        if (shared)
        {
            // Shared boundary: move both edges together
            auto& nextBand = activeBands[static_cast<size_t> (bandPosition + 1)];

            float minPos = band.leftFreqHz * minWidthFactor;
            float maxPos = nextBand.rightFreqHz / minWidthFactor;

            // Also can't go past the band after next
            if (bandPosition + 2 < static_cast<int> (activeBands.size()))
                maxPos = std::min (maxPos, activeBands[static_cast<size_t> (bandPosition + 2)].leftFreqHz);

            newFreqHz = std::clamp (newFreqHz, minPos, maxPos);
            band.rightFreqHz = newFreqHz;
            nextBand.leftFreqHz = newFreqHz;
        }
        else
        {
            float maxRight = Constants::kMaxFrequency;
            if (bandPosition + 1 < static_cast<int> (activeBands.size()))
                maxRight = activeBands[static_cast<size_t> (bandPosition + 1)].leftFreqHz;

            float minRight = band.leftFreqHz * minWidthFactor;
            newFreqHz = std::clamp (newFreqHz, minRight, maxRight);
            band.rightFreqHz = newFreqHz;
        }
    }

    recomputeRegions();
    updateAtomics();
}

//==============================================================================
void BandManager::writeToState (juce::ValueTree& state) const
{
    // Remove any existing mapping
    auto existing = state.getChildWithName ("BandSlotMapping");
    if (existing.isValid())
        state.removeChild (existing, nullptr);

    juce::ValueTree mapping ("BandSlotMapping");

    for (int i = 0; i < static_cast<int> (activeBands.size()); ++i)
    {
        juce::ValueTree bandEntry ("Band");
        bandEntry.setProperty ("position",  i,                          nullptr);
        bandEntry.setProperty ("slot",      activeBands[static_cast<size_t> (i)].slotIndex,    nullptr);
        bandEntry.setProperty ("leftFreq",  activeBands[static_cast<size_t> (i)].leftFreqHz,   nullptr);
        bandEntry.setProperty ("rightFreq", activeBands[static_cast<size_t> (i)].rightFreqHz,  nullptr);
        mapping.addChild (bandEntry, -1, nullptr);
    }

    state.addChild (mapping, -1, nullptr);
}

//==============================================================================
void BandManager::readFromState (const juce::ValueTree& state)
{
    auto mapping = state.getChildWithName ("BandSlotMapping");
    if (! mapping.isValid())
        return;

    activeBands.clear();
    slotUsed.fill (false);

    for (int i = 0; i < mapping.getNumChildren(); ++i)
    {
        auto bandEntry = mapping.getChild (i);
        if (! bandEntry.isValid())
            continue;

        Band band;
        band.slotIndex   = static_cast<int> (bandEntry.getProperty ("slot",      -1));
        band.leftFreqHz  = static_cast<float> (bandEntry.getProperty ("leftFreq",  Constants::kMinFrequency));
        band.rightFreqHz = static_cast<float> (bandEntry.getProperty ("rightFreq", Constants::kMaxFrequency));

        if (band.isValid())
        {
            activeBands.push_back (band);
            slotUsed[static_cast<size_t> (band.slotIndex)] = true;
        }
    }

    // Keep ordered by frequency (should already be, but be safe)
    std::sort (activeBands.begin(), activeBands.end(),
               [] (const Band& a, const Band& b)
               { return a.leftFreqHz < b.leftFreqHz; });

    recomputeRegions();
    updateAtomics();
}

//==============================================================================
void BandManager::recomputeRegions()
{
    regions.clear();

    const int n = static_cast<int> (activeBands.size());

    if (n == 0)
    {
        // No bands: single gap region spanning entire spectrum
        Region gap;
        gap.leftFreqHz  = Constants::kMinFrequency;
        gap.rightFreqHz = Constants::kMaxFrequency;
        gap.slotIndex   = -1;
        regions.push_back (gap);
        return;
    }

    // Build regions: [gap?] [band, gap?]... in frequency order
    float cursor = Constants::kMinFrequency;

    for (int i = 0; i < n; ++i)
    {
        const auto& band = activeBands[static_cast<size_t> (i)];

        // Gap before this band (if any)
        if (band.leftFreqHz > cursor + 0.1f) // avoid zero-width gaps
        {
            Region gap;
            gap.leftFreqHz  = cursor;
            gap.rightFreqHz = band.leftFreqHz;
            gap.slotIndex   = -1;
            regions.push_back (gap);
        }

        // The band itself
        Region userRegion;
        userRegion.leftFreqHz  = band.leftFreqHz;
        userRegion.rightFreqHz = band.rightFreqHz;
        userRegion.slotIndex   = band.slotIndex;
        regions.push_back (userRegion);

        cursor = band.rightFreqHz;
    }

    // Gap after last band (if any)
    if (cursor < Constants::kMaxFrequency - 0.1f)
    {
        Region gap;
        gap.leftFreqHz  = cursor;
        gap.rightFreqHz = Constants::kMaxFrequency;
        gap.slotIndex   = -1;
        regions.push_back (gap);
    }
}

//==============================================================================
void BandManager::resetSlotToDefaults (int slotIndex)
{
    if (apvtsPtr == nullptr)
        return;

    auto setFloat = [&] (const juce::String& suffix, float value)
    {
        auto paramID = ParameterIDs::bandParam (slotIndex, suffix);
        if (auto* param = apvtsPtr->getParameter (paramID))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    auto setBool = [&] (const juce::String& suffix, bool value)
    {
        auto paramID = ParameterIDs::bandParam (slotIndex, suffix);
        if (auto* param = apvtsPtr->getParameter (paramID))
            param->setValueNotifyingHost (value ? 1.0f : 0.0f);
    };

    // Transparent defaults: threshold 0 dB, ratio 1:1, range 0 dB means
    // no compression/expansion is applied.
    setFloat (ParameterIDs::Band::threshold,  0.0f);
    setFloat (ParameterIDs::Band::ratio,      1.0f);
    setFloat (ParameterIDs::Band::range,      0.0f);
    setFloat (ParameterIDs::Band::attack,     Constants::kDefaultAttackMs);
    setFloat (ParameterIDs::Band::release,    Constants::kDefaultReleaseMs);
    setFloat (ParameterIDs::Band::outputGain, 0.0f);
    setFloat (ParameterIDs::Band::knee,       Constants::kDefaultKnee);
    setFloat (ParameterIDs::Band::lookahead,  0.0f);
    setFloat (ParameterIDs::Band::stereoLink, Constants::kDefaultStereoLink);

    setBool (ParameterIDs::Band::enabled,  true);
    setBool (ParameterIDs::Band::audition, false);
}

//==============================================================================
int BandManager::allocateSlot()
{
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        if (! slotUsed[static_cast<size_t> (i)])
        {
            slotUsed[static_cast<size_t> (i)] = true;
            return i;
        }
    }

    return -1;
}

void BandManager::freeSlot (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < Constants::kMaxBands)
        slotUsed[static_cast<size_t> (slotIndex)] = false;
}

//==============================================================================
void BandManager::updateAtomics()
{
    const int n = static_cast<int> (activeBands.size());

    // Update band slot order
    for (int i = 0; i < n; ++i)
        slotOrder[static_cast<size_t> (i)].store (
            activeBands[static_cast<size_t> (i)].slotIndex, std::memory_order_relaxed);

    for (int i = n; i < Constants::kMaxBands; ++i)
        slotOrder[static_cast<size_t> (i)].store (-1, std::memory_order_relaxed);

    numActive.store (n, std::memory_order_release);

    // Update region atomics
    const int numR = static_cast<int> (regions.size());

    for (int i = 0; i < numR; ++i)
        regionSlotOrder[static_cast<size_t> (i)].store (
            regions[static_cast<size_t> (i)].slotIndex, std::memory_order_relaxed);

    for (int i = numR; i < Constants::kMaxRegions; ++i)
        regionSlotOrder[static_cast<size_t> (i)].store (-1, std::memory_order_relaxed);

    // Region crossover frequencies (boundaries between adjacent regions)
    int numRC = numR - 1;
    for (int i = 0; i < numRC; ++i)
        regionCrossoverFreqs[static_cast<size_t> (i)].store (
            regions[static_cast<size_t> (i)].rightFreqHz, std::memory_order_relaxed);

    for (int i = numRC; i < Constants::kMaxRegionCrossovers; ++i)
        regionCrossoverFreqs[static_cast<size_t> (i)].store (1000.0f, std::memory_order_relaxed);

    numRegionsAtomic.store (numR, std::memory_order_release);
}
