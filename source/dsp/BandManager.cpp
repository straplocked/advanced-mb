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

    numActive.store (0, std::memory_order_relaxed);

    slotUsed.fill (false);
    activeBands.clear();
}

//==============================================================================
int BandManager::addBand (float clickFreqHz)
{
    if (apvtsPtr == nullptr)
        return -1;

    if (static_cast<int> (activeBands.size()) >= Constants::kMaxBands)
        return -1;

    int slot = allocateSlot();
    if (slot < 0)
        return -1;

    resetSlotToDefaults (slot);

    Band newBand;
    newBand.slotIndex   = slot;
    newBand.leftFreqHz  = Constants::kMinFrequency;
    newBand.rightFreqHz = Constants::kMaxFrequency;

    // Find insertion position: compare clickFreqHz to the geometric centre of
    // each existing band to keep the list ordered by frequency.
    auto insertIt = std::lower_bound (
        activeBands.begin(), activeBands.end(), clickFreqHz,
        [] (const Band& band, float freq)
        {
            float centre = std::sqrt (band.leftFreqHz * band.rightFreqHz);
            return centre < freq;
        });

    activeBands.insert (insertIt, newBand);

    recomputeCrossovers();

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

    recomputeCrossovers();

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

    updateAtomics();
}

//==============================================================================
void BandManager::recomputeCrossovers()
{
    const int n = static_cast<int> (activeBands.size());
    if (n == 0)
        return;

    const float logMin  = std::log (Constants::kMinFrequency);
    const float logMax  = std::log (Constants::kMaxFrequency);
    const float logStep = (logMax - logMin) / static_cast<float> (n);

    for (int i = 0; i < n; ++i)
    {
        activeBands[static_cast<size_t> (i)].leftFreqHz  = std::exp (logMin + logStep * static_cast<float> (i));
        activeBands[static_cast<size_t> (i)].rightFreqHz = std::exp (logMin + logStep * static_cast<float> (i + 1));
    }

    // Write crossover frequencies to APVTS (N-1 crossovers for N bands)
    if (apvtsPtr != nullptr)
    {
        for (int i = 0; i < n - 1; ++i)
        {
            auto paramID = ParameterIDs::crossoverParam (i, ParameterIDs::Crossover::frequency);
            if (auto* param = apvtsPtr->getParameter (paramID))
                param->setValueNotifyingHost (
                    param->convertTo0to1 (activeBands[static_cast<size_t> (i)].rightFreqHz));
        }
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
    setFloat (ParameterIDs::Band::attack,     Constants::kDefaultAttackPct);
    setFloat (ParameterIDs::Band::release,    Constants::kDefaultReleasePct);
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

    for (int i = 0; i < n; ++i)
        slotOrder[static_cast<size_t> (i)].store (
            activeBands[static_cast<size_t> (i)].slotIndex, std::memory_order_relaxed);

    // Fill remaining positions with -1
    for (int i = n; i < Constants::kMaxBands; ++i)
        slotOrder[static_cast<size_t> (i)].store (-1, std::memory_order_relaxed);

    numActive.store (n, std::memory_order_release);
}
