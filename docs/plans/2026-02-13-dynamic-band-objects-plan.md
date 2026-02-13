# Dynamic Band Objects Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the fixed-array band model with dynamic Band objects — users click to create bands, delete via BandControlPanel, and bands map to a pool of 6 parameter slots.

**Architecture:** BandManager owns a pool of 6 parameter slots and an ordered list of active Band objects. Each Band wraps one slot. The UI creates/removes bands; BandManager updates APVTS parameters (numBands, crossover frequencies, band defaults) and exposes an atomic slot-order array for the audio thread. MultibandEngine uses slot mapping so each band's compressor state stays tied to its parameter slot.

**Tech Stack:** JUCE 7, C++20, CMake

---

### Task 1: Create Band class

**Files:**
- Create: `source/dsp/Band.h`

**Step 1: Create Band.h**

```cpp
/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "../utils/Constants.h"

/**
 * Represents a single active compressor band mapped to a parameter slot.
 * Bands are ordered by frequency; the slot index is stable for DAW automation.
 */
struct Band
{
    int slotIndex = -1;          // Which APVTS parameter slot (0-5) this band uses
    float leftFreqHz  = Constants::kMinFrequency;
    float rightFreqHz = Constants::kMaxFrequency;

    bool isValid() const { return slotIndex >= 0 && slotIndex < Constants::kMaxBands; }
};
```

**Step 2: Build to verify no errors**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds (Band.h is header-only, discovered by GLOB_RECURSE)

**Step 3: Commit**

```bash
git add source/dsp/Band.h
git commit -m "feat: add Band struct for dynamic band object model"
```

---

### Task 2: Create BandManager class

**Files:**
- Create: `source/dsp/BandManager.h`
- Create: `source/dsp/BandManager.cpp`

**Step 1: Create BandManager.h**

```cpp
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
 * Thread safety:
 * - addBand/removeBand/recomputeCrossovers are called on the message thread only.
 * - slotOrder and numActiveBands are atomic for lock-free audio-thread reads.
 * - APVTS parameter updates are thread-safe by design.
 */
class BandManager
{
public:
    BandManager() = default;

    void init (juce::AudioProcessorValueTreeState& apvts);

    /** Create a new band. clickFreqHz determines insertion position.
     *  Returns the new band's slot index, or -1 if at max. */
    int addBand (float clickFreqHz);

    /** Remove the band at the given frequency-ordered position (0-based).
     *  Returns the freed slot index, or -1 if invalid. */
    int removeBand (int bandPosition);

    /** Number of currently active bands. */
    int getNumActiveBands() const { return numActive.load (std::memory_order_relaxed); }

    /** Get the slot-order array for the audio thread.
     *  slotOrder[position] = parameter slot index for that frequency position. */
    int getSlotForPosition (int position) const;

    /** Get the active bands (message thread only). */
    const std::vector<Band>& getActiveBands() const { return activeBands; }

    /** Serialize/deserialize slot mapping for state save/restore. */
    void writeToState (juce::ValueTree& state) const;
    void readFromState (const juce::ValueTree& state);

private:
    void recomputeCrossovers();
    void resetSlotToDefaults (int slotIndex);
    int  allocateSlot();
    void freeSlot (int slotIndex);
    void updateAtomics();

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;

    // Active bands ordered by frequency (message thread only)
    std::vector<Band> activeBands;

    // Slot availability pool
    std::array<bool, Constants::kMaxBands> slotUsed = {};

    // Atomic state for audio thread
    std::atomic<int> numActive { 0 };
    std::array<std::atomic<int>, Constants::kMaxBands> slotOrder;
};
```

**Step 2: Create BandManager.cpp**

```cpp
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

void BandManager::init (juce::AudioProcessorValueTreeState& apvts)
{
    apvtsPtr = &apvts;

    // Initialize slot order to identity (unused but safe)
    for (int i = 0; i < Constants::kMaxBands; ++i)
        slotOrder[i].store (i, std::memory_order_relaxed);
}

int BandManager::addBand (float clickFreqHz)
{
    if (apvtsPtr == nullptr)
        return -1;

    if (static_cast<int> (activeBands.size()) >= Constants::kMaxBands)
        return -1;

    int slot = allocateSlot();
    if (slot < 0)
        return -1;

    // Reset this slot's parameters to transparent defaults
    resetSlotToDefaults (slot);

    // Create band and insert in frequency order
    Band newBand;
    newBand.slotIndex = slot;

    // Find insertion position based on click frequency
    int insertPos = 0;
    for (int i = 0; i < static_cast<int> (activeBands.size()); ++i)
    {
        float bandCenter = std::sqrt (activeBands[i].leftFreqHz * activeBands[i].rightFreqHz);
        if (clickFreqHz > bandCenter)
            insertPos = i + 1;
    }

    activeBands.insert (activeBands.begin() + insertPos, newBand);

    // Recompute crossover positions (even split)
    recomputeCrossovers();

    // Update APVTS numBands
    if (auto* param = apvtsPtr->getParameter (ParameterIDs::Global::numBands))
        param->setValueNotifyingHost (param->convertTo0to1 (
            static_cast<float> (activeBands.size())));

    // Update atomics for audio thread
    updateAtomics();

    return slot;
}

int BandManager::removeBand (int bandPosition)
{
    if (apvtsPtr == nullptr)
        return -1;

    if (bandPosition < 0 || bandPosition >= static_cast<int> (activeBands.size()))
        return -1;

    int freedSlot = activeBands[bandPosition].slotIndex;
    activeBands.erase (activeBands.begin() + bandPosition);
    freeSlot (freedSlot);

    // Recompute crossovers
    recomputeCrossovers();

    // Update APVTS numBands
    if (auto* param = apvtsPtr->getParameter (ParameterIDs::Global::numBands))
        param->setValueNotifyingHost (param->convertTo0to1 (
            static_cast<float> (activeBands.size())));

    // Update atomics
    updateAtomics();

    return freedSlot;
}

int BandManager::getSlotForPosition (int position) const
{
    if (position < 0 || position >= Constants::kMaxBands)
        return 0;
    return slotOrder[position].load (std::memory_order_relaxed);
}

void BandManager::recomputeCrossovers()
{
    if (apvtsPtr == nullptr)
        return;

    const int n = static_cast<int> (activeBands.size());
    if (n <= 0)
        return;

    // Even log-spaced split of 20Hz - 20kHz
    const float logMin = std::log (Constants::kMinFrequency);
    const float logMax = std::log (Constants::kMaxFrequency);
    const float logStep = (logMax - logMin) / static_cast<float> (n);

    for (int i = 0; i < n; ++i)
    {
        activeBands[i].leftFreqHz  = std::exp (logMin + logStep * static_cast<float> (i));
        activeBands[i].rightFreqHz = std::exp (logMin + logStep * static_cast<float> (i + 1));
    }

    // Set crossover frequencies in APVTS (crossover i = right edge of band i)
    for (int i = 0; i < n - 1; ++i)
    {
        float freq = activeBands[i].rightFreqHz;
        juce::String paramID = ParameterIDs::crossoverParam (i, ParameterIDs::Crossover::frequency);

        if (auto* param = apvtsPtr->getParameter (paramID))
            param->setValueNotifyingHost (param->convertTo0to1 (freq));
    }
}

void BandManager::resetSlotToDefaults (int slotIndex)
{
    if (apvtsPtr == nullptr || slotIndex < 0 || slotIndex >= Constants::kMaxBands)
        return;

    // Transparent defaults: threshold 0 dB, ratio 1:1
    auto setFloat = [&] (const juce::String& suffix, float value)
    {
        juce::String id = ParameterIDs::bandParam (slotIndex, suffix);
        if (auto* param = apvtsPtr->getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    auto setBool = [&] (const juce::String& suffix, bool value)
    {
        juce::String id = ParameterIDs::bandParam (slotIndex, suffix);
        if (auto* param = apvtsPtr->getParameter (id))
            param->setValueNotifyingHost (value ? 1.0f : 0.0f);
    };

    setBool  (ParameterIDs::Band::enabled,    true);
    setFloat (ParameterIDs::Band::threshold,   0.0f);   // transparent
    setFloat (ParameterIDs::Band::ratio,       1.0f);   // unity
    setFloat (ParameterIDs::Band::range,       0.0f);
    setFloat (ParameterIDs::Band::attack,      Constants::kDefaultAttackPct);
    setFloat (ParameterIDs::Band::release,     Constants::kDefaultReleasePct);
    setFloat (ParameterIDs::Band::outputGain,  0.0f);
    setFloat (ParameterIDs::Band::knee,        Constants::kDefaultKnee);
    setFloat (ParameterIDs::Band::lookahead,   0.0f);
    setFloat (ParameterIDs::Band::stereoLink,  Constants::kDefaultStereoLink);
    setBool  (ParameterIDs::Band::audition,    false);
}

int BandManager::allocateSlot()
{
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        if (!slotUsed[i])
        {
            slotUsed[i] = true;
            return i;
        }
    }
    return -1;
}

void BandManager::freeSlot (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < Constants::kMaxBands)
        slotUsed[slotIndex] = false;
}

void BandManager::updateAtomics()
{
    const int n = static_cast<int> (activeBands.size());
    numActive.store (n, std::memory_order_relaxed);

    for (int i = 0; i < n; ++i)
        slotOrder[i].store (activeBands[i].slotIndex, std::memory_order_relaxed);

    // Fill remaining with identity to avoid stale reads
    for (int i = n; i < Constants::kMaxBands; ++i)
        slotOrder[i].store (i, std::memory_order_relaxed);
}

void BandManager::writeToState (juce::ValueTree& state) const
{
    juce::ValueTree bandMap ("BandSlotMapping");

    for (int i = 0; i < static_cast<int> (activeBands.size()); ++i)
    {
        juce::ValueTree entry ("Band");
        entry.setProperty ("position", i, nullptr);
        entry.setProperty ("slot", activeBands[i].slotIndex, nullptr);
        entry.setProperty ("leftFreq", activeBands[i].leftFreqHz, nullptr);
        entry.setProperty ("rightFreq", activeBands[i].rightFreqHz, nullptr);
        bandMap.addChild (entry, -1, nullptr);
    }

    // Remove existing mapping if present
    auto existing = state.getChildWithName ("BandSlotMapping");
    if (existing.isValid())
        state.removeChild (existing, nullptr);

    state.addChild (bandMap, -1, nullptr);
}

void BandManager::readFromState (const juce::ValueTree& state)
{
    auto bandMap = state.getChildWithName ("BandSlotMapping");
    if (!bandMap.isValid())
        return;

    // Clear current state
    activeBands.clear();
    slotUsed.fill (false);

    for (int i = 0; i < bandMap.getNumChildren(); ++i)
    {
        auto entry = bandMap.getChild (i);
        Band band;
        band.slotIndex   = entry.getProperty ("slot", -1);
        band.leftFreqHz  = entry.getProperty ("leftFreq", Constants::kMinFrequency);
        band.rightFreqHz = entry.getProperty ("rightFreq", Constants::kMaxFrequency);

        if (band.isValid())
        {
            activeBands.push_back (band);
            slotUsed[band.slotIndex] = true;
        }
    }

    updateAtomics();
}
```

**Step 3: Build to verify compilation**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add source/dsp/BandManager.h source/dsp/BandManager.cpp
git commit -m "feat: add BandManager for dynamic band slot pool management"
```

---

### Task 3: Update Constants and ParameterLayout for 0-band default

**Files:**
- Modify: `source/utils/Constants.h:14` — change `kDefaultNumBands` from 1 to 0
- Modify: `source/parameters/ParameterLayout.cpp:39` — change min from 1 to 0

**Step 1: Update Constants.h**

Change line 14:
```cpp
constexpr int kDefaultNumBands = 0;
```

**Step 2: Update ParameterLayout.cpp**

Change line 38-39 to allow 0 bands:
```cpp
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParameterIDs::Global::numBands, "Num Bands", 0, Constants::kMaxBands, Constants::kDefaultNumBands));
```

**Step 3: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add source/utils/Constants.h source/parameters/ParameterLayout.cpp
git commit -m "feat: change default band count to 0, allow 0-band state"
```

---

### Task 4: Add slot mapping to MultibandParameters and MultibandEngine

**Files:**
- Modify: `source/dsp/MultibandEngine.h:15-31` — add `slotOrder` to `MultibandParameters`
- Modify: `source/dsp/MultibandEngine.cpp:52-74,76-124` — use slot mapping in `updateParameters` and `process`

**Step 1: Add slotOrder to MultibandParameters struct**

In `source/dsp/MultibandEngine.h`, add after line 22 (`float mix = 100.0f;`):
```cpp
    std::array<int, Constants::kMaxBands> slotOrder = {0, 1, 2, 3, 4, 5};
```

**Step 2: Update MultibandEngine::updateParameters to use slot mapping**

In `source/dsp/MultibandEngine.cpp`, replace `updateParameters` (lines 52-74) with:
```cpp
void MultibandEngine::updateParameters(const MultibandParameters& params)
{
    numBands = std::clamp(params.numBands, 0, Constants::kMaxBands);

    globalOutputGain = MathUtils::dbToLinear(params.globalOutput);
    mixAmount = std::clamp(params.mix, 0.0f, 200.0f) * 0.01f;

    // Store slot mapping
    for (int i = 0; i < Constants::kMaxBands; ++i)
        currentSlotOrder[i] = params.slotOrder[i];

    // Update crossover bank
    crossoverBank.setNumBands(std::max(numBands, 1));

    for (int i = 0; i < Constants::kMaxCrossovers; ++i)
    {
        crossoverBank.setCrossoverFrequency(i, params.crossovers[i].frequency);
        crossoverBank.setCrossoverSlope(i, params.crossovers[i].slope);
    }

    // Update band compressors using slot mapping
    for (int pos = 0; pos < numBands; ++pos)
    {
        int slot = currentSlotOrder[pos];
        bandCompressors[slot].updateParameters(params.bands[slot]);
    }

    oversampler.setFactor(params.oversampleIndex);
}
```

**Step 3: Update MultibandEngine::process to use slot mapping**

Replace `process` method (lines 76-124) with:
```cpp
void MultibandEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), currentChannels);

    // 0 bands = pass through (no processing)
    if (numBands <= 0)
        return;

    // 1. Store dry copy for dry/wet mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 2. Split via crossover bank into band buffers
    crossoverBank.process(buffer, bandBuffers);

    // 3. Process each active band using slot mapping
    for (int pos = 0; pos < numBands; ++pos)
    {
        int slot = currentSlotOrder[pos];
        bandCompressors[slot].process(bandBuffers[pos]);
    }

    // 4. Sum all band outputs back into the main buffer
    buffer.clear();
    for (int pos = 0; pos < numBands; ++pos)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0,
                           bandBuffers[pos].getReadPointer(ch),
                           numSamples);
        }
    }

    // 5. Apply dry/wet mix
    if (mixAmount < 1.999f)
    {
        float dryAmount = 1.0f - mixAmount;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.applyGain(ch, 0, numSamples, mixAmount);
            buffer.addFrom(ch, 0, dryBuffer.getReadPointer(ch), numSamples, dryAmount);
        }
    }

    // 6. Apply global output gain
    if (std::abs(globalOutputGain - 1.0f) > 1.0e-6f)
        buffer.applyGain(0, numSamples, globalOutputGain);
}
```

**Step 4: Add currentSlotOrder member to MultibandEngine**

In `source/dsp/MultibandEngine.h`, add after line 57 (`int numBands = 1;`):
```cpp
    std::array<int, Constants::kMaxBands> currentSlotOrder = {0, 1, 2, 3, 4, 5};
```

And change line 57 from `int numBands = 1;` to:
```cpp
    int numBands = 0;
```

**Step 5: Update getBandGainReduction to accept slot index**

In `source/dsp/MultibandEngine.cpp`, the existing `getBandGainReduction` already takes a `bandIndex` and reads from `bandCompressors[bandIndex]`. This works correctly when the caller passes a slot index. No change needed, but update the comment in the header (line 47):
```cpp
    // Per-band gain reduction for metering (pass slot index)
    float getBandGainReduction(int slotIndex) const;
```

**Step 6: Update CrossoverFilterBank to handle 0 bands gracefully**

In `source/dsp/CrossoverFilterBank.cpp`, change line 39:
```cpp
    numBands = std::clamp(n, 0, Constants::kMaxBands);
```

And in `process` (line 86), change the early return condition:
```cpp
    if (numBands <= 1)
    {
        if (numBands == 1)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                bandOutputs[0].copyFrom(ch, 0, input, ch, 0, numSamples);
        }
        return;
    }
```

**Step 7: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 8: Commit**

```bash
git add source/dsp/MultibandEngine.h source/dsp/MultibandEngine.cpp source/dsp/CrossoverFilterBank.cpp
git commit -m "feat: add slot mapping to MultibandEngine for dynamic band routing"
```

---

### Task 5: Update MultibandParameterAttachment to read slot order

**Files:**
- Modify: `source/parameters/MultibandParameterAttachment.h:63-96` — read slot order from BandManager

**Step 1: Add BandManager reference and update getCurrentParameters**

Add include at top of file (after line 9):
```cpp
#include "../dsp/BandManager.h"
```

Add BandManager pointer after line 108 (`std::atomic<float>* midiLearn = nullptr;`):
```cpp
    // Band manager (optional, set by processor)
    BandManager* bandManager = nullptr;
```

Add setter in public section after `init` (after line 61):
```cpp
    void setBandManager (BandManager* bm) { bandManager = bm; }
```

Update `getCurrentParameters` (lines 63-96) to populate slotOrder:

After line 72 (`p.mix = mix->load();`), add:
```cpp
    // Populate slot order from BandManager
    if (bandManager != nullptr)
    {
        p.numBands = bandManager->getNumActiveBands();
        for (int i = 0; i < Constants::kMaxBands; ++i)
            p.slotOrder[i] = bandManager->getSlotForPosition (i);
    }
```

**Step 2: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add source/parameters/MultibandParameterAttachment.h
git commit -m "feat: wire BandManager slot order into MultibandParameterAttachment"
```

---

### Task 6: Integrate BandManager into PluginProcessor

**Files:**
- Modify: `source/PluginProcessor.h:18-22,76-93` — add BandManager member and accessor
- Modify: `source/PluginProcessor.cpp:16-25,136-152` — init BandManager, serialize/deserialize

**Step 1: Add BandManager include and member to PluginProcessor.h**

Add include after line 18 (`#include "dsp/MultibandEngine.h"`):
```cpp
#include "dsp/BandManager.h"
```

Add accessor after line 62 (`PresetManager& getPresetManager() { return presetManager; }`):
```cpp
    BandManager& getBandManager() { return bandManager; }
```

Add member after line 80 (`MultibandEngine multibandEngine;`):
```cpp
    BandManager bandManager;
```

**Step 2: Init BandManager in constructor**

In `source/PluginProcessor.cpp`, after line 24 (`paramAttachment.init ( apvts );`), add:
```cpp
    bandManager.init ( apvts );
    paramAttachment.setBandManager ( &bandManager );
```

**Step 3: Update getStateInformation to include band mapping**

Replace `getStateInformation` (lines 136-141):
```cpp
void AdvancedMBProcessor::getStateInformation ( juce::MemoryBlock& destData )
{
    auto state = apvts.copyState();
    bandManager.writeToState ( state );
    std::unique_ptr<juce::XmlElement> xml ( state.createXml() );
    copyXmlToBinary ( *xml, destData );
}
```

**Step 4: Update setStateInformation to restore band mapping**

Replace `setStateInformation` (lines 143-152):
```cpp
void AdvancedMBProcessor::setStateInformation ( const void* data, int sizeInBytes )
{
    std::unique_ptr<juce::XmlElement> xml ( getXmlFromBinary ( data, sizeInBytes ) );

    if ( xml != nullptr && xml->hasTagName ( apvts.state.getType() ) )
    {
        auto state = juce::ValueTree::fromXml ( *xml );
        bandManager.readFromState ( state );
        apvts.replaceState ( state );
    }
}
```

**Step 5: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 6: Commit**

```bash
git add source/PluginProcessor.h source/PluginProcessor.cpp
git commit -m "feat: integrate BandManager into PluginProcessor with state serialization"
```

---

### Task 7: Remove band selector from BottomControlStrip

**Files:**
- Modify: `source/ui/components/BottomControlStrip.h:53-57,66` — remove band count members
- Modify: `source/ui/components/BottomControlStrip.cpp:92-134,146-153,194-197` — remove band count code

**Step 1: Remove band count members from header**

In `source/ui/components/BottomControlStrip.h`, delete lines 53-57:
```
    // Band count control
    juce::Label      bandsLabel;
    juce::TextButton bandsMinusButton { "-" };
    juce::Label      bandsValueLabel;
    juce::TextButton bandsPlusButton  { "+" };
```

Delete line 66:
```
    void updateBandsLabel();
```

**Step 2: Remove band count code from cpp**

In `source/ui/components/BottomControlStrip.cpp`:

Delete the entire band count section (lines 92-134, from `// ========================= Band count =========================` through the end of `addAndMakeVisible ( bandsPlusButton );` and `updateBandsLabel();`).

Delete the `updateBandsLabel` method (lines 146-153).

Delete the band count layout lines from `resized()` (lines 194-197):
```
    bandsLabel.setBounds ( bounds.removeFromLeft ( 42 ).reduced ( 0, vPad ) );
    bandsMinusButton.setBounds ( bounds.removeFromLeft ( 20 ).reduced ( 0, 4 ) );
    bandsValueLabel.setBounds ( bounds.removeFromLeft ( 18 ).reduced ( 0, vPad ) );
    bandsPlusButton.setBounds ( bounds.removeFromLeft ( 20 ).reduced ( 0, 4 ) );
```

**Step 3: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add source/ui/components/BottomControlStrip.h source/ui/components/BottomControlStrip.cpp
git commit -m "refactor: remove band count selector from BottomControlStrip"
```

---

### Task 8: Update MultibandDisplay for click-to-create and slot-aware rendering

**Files:**
- Modify: `source/ui/components/MultibandDisplay.h:60-64` — add onBandCreated callback, BandManager pointer
- Modify: `source/ui/components/MultibandDisplay.cpp:215-251,257-303,309-368,374-408,451-545,588-643` — slot-aware rendering and click-to-create

**Step 1: Add BandManager support to MultibandDisplay.h**

Add forward declaration after line 20 (`class AdvancedMBProcessor;`):
```cpp
class BandManager;
```

Add after line 64 (`std::function<void()> onBandDeselected;`):
```cpp
    /** Callback invoked when a new band is created by clicking empty space. */
    std::function<void ( int )> onBandCreated;

    void setBandManager ( BandManager* bm ) { bandManager = bm; }
```

Add member after line 91 (`AdvancedMBProcessor* processor = nullptr;`):
```cpp
    BandManager* bandManager = nullptr;
```

**Step 2: Update drawBandRegions to use slot mapping**

Replace `drawBandRegions` (lines 215-251) with:
```cpp
void MultibandDisplay::drawBandRegions ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    if ( numBands < 1 )
        return;

    const auto& bands = bandManager->getActiveBands();
    const float h = static_cast<float> ( getHeight() );

    for ( int b = 0; b < numBands; ++b )
    {
        float x0 = frequencyToX ( bands[b].leftFreqHz );
        float x1 = frequencyToX ( bands[b].rightFreqHz );
        int slot = bands[b].slotIndex;

        juce::Colour fill = bandFillColours[slot % Constants::kMaxBands];

        if ( b == selectedBand )
            fill = fill.brighter ( 0.3f );

        g.setColour ( fill );
        g.fillRect ( x0, 0.0f, x1 - x0, h );
    }
}
```

**Step 3: Update drawGainReduction to use slot mapping**

Replace `drawGainReduction` (lines 257-303) with:
```cpp
void MultibandDisplay::drawGainReduction ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    if ( numBands < 1 )
        return;

    const auto& bands = bandManager->getActiveBands();
    const float zeroY = dbToY ( 0.0f );

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[b].slotIndex;
        float grDB = processor->getBandGainReduction ( slot );
        if ( std::abs ( grDB ) < 0.1f )
            continue;

        float grY = dbToY ( grDB );
        float x0 = frequencyToX ( bands[b].leftFreqHz );
        float x1 = frequencyToX ( bands[b].rightFreqHz );
        float centerX = ( x0 + x1 ) * 0.5f;
        float barWidth = juce::jmin ( ( x1 - x0 ) * 0.5f, 40.0f );

        g.setColour ( ColourPalette::gainReductionFill );
        g.fillRect ( centerX - barWidth * 0.5f, zeroY, barWidth, grY - zeroY );

        g.setColour ( ColourPalette::gainReduction );
        g.drawRect ( centerX - barWidth * 0.5f, zeroY, barWidth, grY - zeroY, 1.0f );
    }
}
```

**Step 4: Update drawCrossovers to use BandManager**

Replace `drawCrossovers` (lines 309-368) with:
```cpp
void MultibandDisplay::drawCrossovers ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    const int numCrossovers = numBands - 1;
    const float h = static_cast<float> ( getHeight() );

    if ( numCrossovers <= 0 )
        return;

    const auto& bands = bandManager->getActiveBands();

    for ( int i = 0; i < numCrossovers; ++i )
    {
        float freq = bands[i].rightFreqHz;
        float x = frequencyToX ( freq );

        // Dashed vertical line
        g.setColour ( ColourPalette::crossoverLine );
        const float dashLengths[] = { 6.0f, 4.0f };
        g.drawDashedLine ( juce::Line<float> ( x, 0.0f, x, h ),
                           dashLengths, 2, 1.5f );

        // Handle circle at top
        bool isDragging = ( draggingCrossover == i );
        bool isHovered  = ( hoveredCrossover == i );
        juce::Colour handleCol = ColourPalette::crossoverHandle;

        if ( isDragging )
            handleCol = handleCol.brighter ( 0.4f );
        else if ( isHovered )
            handleCol = handleCol.brighter ( 0.3f );

        g.setColour ( handleCol );
        g.fillEllipse ( x - kHandleRadius, kHandleRadius * 0.5f,
                        kHandleRadius * 2.0f, kHandleRadius * 2.0f );
        g.setColour ( ColourPalette::curveArea );
        g.fillEllipse ( x - kHandleRadius + 2.0f, kHandleRadius * 0.5f + 2.0f,
                        kHandleRadius * 2.0f - 4.0f, kHandleRadius * 2.0f - 4.0f );
    }

    // Frequency tooltip during crossover drag
    if ( showingFreqTooltip && draggingCrossover >= 0 )
    {
        juce::String freqText = FrequencyUtils::formatFrequency ( tooltipFreq );
        g.setFont ( Fonts::getBody ( 11.0f ) );

        int textW = g.getCurrentFont().getStringWidth ( freqText ) + 10;
        int textH = 18;
        int tx = juce::roundToInt ( tooltipX ) - textW / 2;
        int ty = juce::roundToInt ( tooltipY ) - textH - 8;

        tx = juce::jlimit ( 2, getWidth() - textW - 2, tx );
        ty = juce::jmax ( 2, ty );

        g.setColour ( ColourPalette::panelTop.withAlpha ( 0.9f ) );
        g.fillRoundedRectangle ( static_cast<float> ( tx ), static_cast<float> ( ty ),
                                  static_cast<float> ( textW ), static_cast<float> ( textH ), 3.0f );
        g.setColour ( ColourPalette::textBright );
        g.drawText ( freqText, tx, ty, textW, textH, juce::Justification::centred, false );
    }
}
```

**Step 5: Update drawThresholds to use slot mapping**

Replace `drawThresholds` (lines 374-408) with:
```cpp
void MultibandDisplay::drawThresholds ( juce::Graphics& g )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const int numBands = bandManager->getNumActiveBands();
    if ( numBands < 1 )
        return;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const auto& bands = bandManager->getActiveBands();

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[b].slotIndex;
        if ( ! params.bands[slot].enabled )
            continue;

        float threshY = dbToY ( params.bands[slot].threshold );
        float x0 = frequencyToX ( bands[b].leftFreqHz );
        float x1 = frequencyToX ( bands[b].rightFreqHz );

        g.setColour ( ( b == selectedBand ) ? ColourPalette::thresholdAccent
                                            : ColourPalette::thresholdLine );
        g.drawLine ( x0, threshY, x1, threshY, 1.5f );
    }
}
```

**Step 6: Update hit-testing helpers for BandManager**

Replace `crossoverHandleAtPoint` (lines 451-472) with:
```cpp
int MultibandDisplay::crossoverHandleAtPoint ( juce::Point<float> point ) const
{
    if ( bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    const int numCrossovers = numBands - 1;

    if ( numCrossovers <= 0 )
        return -1;

    const auto& bands = bandManager->getActiveBands();

    for ( int i = 0; i < numCrossovers; ++i )
    {
        float cx = frequencyToX ( bands[i].rightFreqHz );
        float cy = kHandleRadius * 0.5f + kHandleRadius;

        float dx = point.x - cx;
        float dy = point.y - cy;

        if ( ( dx * dx + dy * dy ) < ( kHandleHitRadius * kHandleHitRadius ) )
            return i;
    }

    return -1;
}
```

Replace `thresholdAtPoint` (lines 474-514) with:
```cpp
int MultibandDisplay::thresholdAtPoint ( juce::Point<float> point ) const
{
    if ( processor == nullptr || bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    if ( numBands < 1 )
        return -1;

    auto params = processor->getParameterAttachment().getCurrentParameters();
    const auto& bands = bandManager->getActiveBands();
    constexpr float hitTolerance = 8.0f;

    for ( int b = 0; b < numBands; ++b )
    {
        int slot = bands[b].slotIndex;
        if ( ! params.bands[slot].enabled )
            continue;

        float x0 = frequencyToX ( bands[b].leftFreqHz );
        float x1 = frequencyToX ( bands[b].rightFreqHz );

        if ( point.x < x0 || point.x > x1 )
            continue;

        float threshY = dbToY ( params.bands[slot].threshold );

        if ( std::abs ( point.y - threshY ) < hitTolerance )
            return b;
    }

    return -1;
}
```

Replace `bandAtX` (lines 516-545) with:
```cpp
int MultibandDisplay::bandAtX ( float x ) const
{
    if ( bandManager == nullptr )
        return -1;

    const int numBands = bandManager->getNumActiveBands();
    if ( numBands < 1 )
        return -1;

    const auto& bands = bandManager->getActiveBands();

    for ( int b = 0; b < numBands; ++b )
    {
        float x0 = frequencyToX ( bands[b].leftFreqHz );
        float x1 = frequencyToX ( bands[b].rightFreqHz );

        if ( x >= x0 && x < x1 )
            return b;
    }

    return -1;
}
```

**Step 7: Update mouseDown for click-to-create**

Replace `mouseDown` (lines 588-643) with:
```cpp
void MultibandDisplay::mouseDown ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    auto pos = e.position;

    // Check crossover handles first
    int handle = crossoverHandleAtPoint ( pos );

    if ( handle >= 0 )
    {
        draggingCrossover = handle;
        showingFreqTooltip = true;
        return;
    }

    // Check threshold lines
    int thresh = thresholdAtPoint ( pos );

    if ( thresh >= 0 )
    {
        draggingThresholdBand = thresh;
        return;
    }

    // Right-click context menu on band region
    if ( e.mods.isPopupMenu() )
    {
        int band = bandAtX ( pos.x );
        if ( band >= 0 )
            showBandContextMenu ( band );
        return;
    }

    // Left-click on existing band: toggle selection
    int band = bandAtX ( pos.x );

    if ( band >= 0 )
    {
        if ( band == selectedBand )
        {
            selectedBand = -1;

            if ( onBandDeselected )
                onBandDeselected();

            return;
        }

        selectedBand = band;

        if ( onBandSelected )
            onBandSelected ( selectedBand );

        return;
    }

    // Left-click on empty space: create a new band
    if ( bandManager != nullptr )
    {
        float clickFreq = xToFrequency ( pos.x );
        int newSlot = bandManager->addBand ( clickFreq );

        if ( newSlot >= 0 )
        {
            // Find position of newly created band
            const auto& bands = bandManager->getActiveBands();
            for ( int i = 0; i < static_cast<int> ( bands.size() ); ++i )
            {
                if ( bands[i].slotIndex == newSlot )
                {
                    selectedBand = i;

                    if ( onBandCreated )
                        onBandCreated ( i );

                    break;
                }
            }

            repaint();
        }
    }
}
```

**Step 8: Update mouseDrag for crossover dragging with BandManager**

Replace the crossover dragging section of `mouseDrag` (lines 645-668) with:
```cpp
void MultibandDisplay::mouseDrag ( const juce::MouseEvent& e )
{
    if ( processor == nullptr )
        return;

    // Crossover dragging — directly update band boundaries
    if ( draggingCrossover >= 0 && bandManager != nullptr )
    {
        float newFreq = xToFrequency ( e.position.x );
        newFreq = juce::jlimit ( Constants::kMinFrequency, Constants::kMaxFrequency, newFreq );

        // Update the crossover APVTS parameter
        juce::String paramID = ParameterIDs::crossoverParam (
            draggingCrossover, ParameterIDs::Crossover::frequency );

        if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
            param->setValueNotifyingHost ( param->convertTo0to1 ( newFreq ) );

        // Update band boundaries in BandManager
        auto& bands = const_cast<std::vector<Band>&> ( bandManager->getActiveBands() );
        if ( draggingCrossover < static_cast<int> ( bands.size() ) )
        {
            bands[draggingCrossover].rightFreqHz = newFreq;
            if ( draggingCrossover + 1 < static_cast<int> ( bands.size() ) )
                bands[draggingCrossover + 1].leftFreqHz = newFreq;
        }

        tooltipFreq = newFreq;
        tooltipX = e.position.x;
        tooltipY = e.position.y;
        showingFreqTooltip = true;

        repaint();
        return;
    }

    // Threshold dragging — use slot mapping
    if ( draggingThresholdBand >= 0 && bandManager != nullptr )
    {
        float newDB = yToDb ( e.position.y );
        newDB = juce::jlimit ( Constants::kMinThreshold, Constants::kMaxThreshold, newDB );

        const auto& bands = bandManager->getActiveBands();
        if ( draggingThresholdBand < static_cast<int> ( bands.size() ) )
        {
            int slot = bands[draggingThresholdBand].slotIndex;
            juce::String paramID = ParameterIDs::bandParam (
                slot, ParameterIDs::Band::threshold );

            if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                param->setValueNotifyingHost ( param->convertTo0to1 ( newDB ) );
        }

        repaint();
    }
}
```

**Step 9: Update showBandContextMenu to use slot mapping**

Replace `showBandContextMenu` (lines 699-737) with:
```cpp
void MultibandDisplay::showBandContextMenu ( int bandPosition )
{
    if ( processor == nullptr || bandManager == nullptr )
        return;

    const auto& bands = bandManager->getActiveBands();
    if ( bandPosition < 0 || bandPosition >= static_cast<int> ( bands.size() ) )
        return;

    int slot = bands[bandPosition].slotIndex;
    auto params = processor->getParameterAttachment().getCurrentParameters();
    bool enabled = params.bands[slot].enabled;

    juce::PopupMenu menu;
    menu.addItem ( 1, enabled ? "Disable Band" : "Enable Band" );
    menu.addItem ( 2, "Audition Band" );

    menu.showMenuAsync ( juce::PopupMenu::Options(),
        [this, slot] ( int result )
        {
            if ( result == 1 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    slot, ParameterIDs::Band::enabled );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
            else if ( result == 2 )
            {
                juce::String paramID = ParameterIDs::bandParam (
                    slot, ParameterIDs::Band::audition );

                if ( auto* param = processor->getAPVTS().getParameter ( paramID ) )
                {
                    float current = param->getValue();
                    param->setValueNotifyingHost ( current > 0.5f ? 0.0f : 1.0f );
                }
            }
        } );
}
```

**Step 10: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 11: Commit**

```bash
git add source/ui/components/MultibandDisplay.h source/ui/components/MultibandDisplay.cpp
git commit -m "feat: click-to-create bands and slot-aware rendering in MultibandDisplay"
```

---

### Task 9: Add delete button to BandControlPanel

**Files:**
- Modify: `source/ui/components/BandControlPanel.h:51,75-78` — add delete button, onDelete callback
- Modify: `source/ui/components/BandControlPanel.cpp:19-87,116-144,150-191,227-274` — add delete button, slot-aware attachments

**Step 1: Add delete callback and button to header**

In `source/ui/components/BandControlPanel.h`, add after line 51 (`std::function<void()> onClose;`):
```cpp
    /** Called when the user clicks the delete button to remove a band. */
    std::function<void ( int )> onDeleteBand;
```

Add a `selectedSlot` member after line 58 (`int selectedBand = -1;`):
```cpp
    int selectedSlot = -1;  // The APVTS parameter slot for the selected band
```

Add delete button after line 78 (`juce::TextButton closeButton    { "X" };`):
```cpp
    juce::TextButton deleteButton   { "DEL" };
```

Add setter for slot in public section after `getSelectedBand()` (after line 45):
```cpp
    /** Set which APVTS slot the selected band maps to. */
    void setSelectedSlot ( int slotIndex ) { selectedSlot = slotIndex; }
```

**Step 2: Wire up delete button in constructor**

In `source/ui/components/BandControlPanel.cpp`, add after `setupButton ( closeButton );` (after line 48):
```cpp
    setupButton ( deleteButton );
```

Add delete button click handler after the close button handler (after line 77):
```cpp
    deleteButton.onClick = [this]
    {
        if ( selectedBand >= 0 && onDeleteBand )
            onDeleteBand ( selectedBand );
    };
```

**Step 3: Update rebuildAttachments to use selectedSlot**

In `rebuildAttachments` (line 168), change:
```cpp
    const int b = selectedSlot >= 0 ? selectedSlot : selectedBand;
```

**Step 4: Update setSelectedBand to use slot for band label**

In `setSelectedBand`, the band label should show the position (1-based), which it already does. The slot is set separately by the editor. Keep line 131 as-is.

**Step 5: Layout the delete button**

In `resized()`, add delete button to the top row. Replace line 241 (`closeButton.setBounds ( leftTopRow.removeFromRight ( 28 ) );`) with:
```cpp
    closeButton.setBounds ( leftTopRow.removeFromRight ( 28 ) );
    deleteButton.setBounds ( leftTopRow.removeFromRight ( 36 ) );
    leftTopRow.removeFromRight ( 4 );
```

**Step 6: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 7: Commit**

```bash
git add source/ui/components/BandControlPanel.h source/ui/components/BandControlPanel.cpp
git commit -m "feat: add delete button to BandControlPanel for band removal"
```

---

### Task 10: Wire everything together in PluginEditor

**Files:**
- Modify: `source/PluginEditor.cpp:15-85` — connect BandManager to display, handle band creation/deletion

**Step 1: Pass BandManager to MultibandDisplay**

In `source/PluginEditor.cpp`, after line 25 (`multibandDisplay.setProcessor ( &p );`), add:
```cpp
    multibandDisplay.setBandManager ( &p.getBandManager() );
```

**Step 2: Update onBandSelected to pass slot info**

Replace the `onBandSelected` lambda (lines 42-47) with:
```cpp
    multibandDisplay.onBandSelected = [this] ( int bandPosition )
    {
        auto& bm = processor.getBandManager();
        const auto& bands = bm.getActiveBands();
        if ( bandPosition >= 0 && bandPosition < static_cast<int> ( bands.size() ) )
        {
            int slot = bands[bandPosition].slotIndex;
            bandControlPanel.setSelectedSlot ( slot );
            bandControlPanel.setSelectedBand ( slot );
            bandControlPanel.setVisible ( true );
            resized();
        }
    };
```

**Step 3: Add onBandCreated handler**

After the `onBandDeselected` lambda (after line 54), add:
```cpp
    multibandDisplay.onBandCreated = [this] ( int bandPosition )
    {
        auto& bm = processor.getBandManager();
        const auto& bands = bm.getActiveBands();
        if ( bandPosition >= 0 && bandPosition < static_cast<int> ( bands.size() ) )
        {
            int slot = bands[bandPosition].slotIndex;
            bandControlPanel.setSelectedSlot ( slot );
            bandControlPanel.setSelectedBand ( slot );
            bandControlPanel.setVisible ( true );
            resized();
        }
    };
```

**Step 4: Add onDeleteBand handler**

After the `onClose` lambda (after line 62), add:
```cpp
    bandControlPanel.onDeleteBand = [this] ( int bandPosition )
    {
        processor.getBandManager().removeBand ( bandPosition );
        bandControlPanel.setVisible ( false );
        bandControlPanel.setSelectedBand ( -1 );
        multibandDisplay.setSelectedBand ( -1 );
        resized();
    };
```

**Step 5: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 6: Commit**

```bash
git add source/PluginEditor.cpp
git commit -m "feat: wire BandManager, click-to-create, and delete into PluginEditor"
```

---

### Task 11: Update processBlock for 0-band passthrough

**Files:**
- Modify: `source/PluginProcessor.cpp:82-127` — handle 0 bands in processBlock

**Step 1: Update processBlock**

In `source/PluginProcessor.cpp`, replace lines 115-116 (the per-band GR loop) with:
```cpp
    int activeBands = params.numBands;
    for ( int pos = 0; pos < activeBands; ++pos )
    {
        int slot = params.slotOrder[pos];
        levelMeterData.pushBandGainReduction ( pos, multibandEngine.getBandGainReduction ( slot ) );
    }
```

**Step 2: Build to verify**

Run: `cmake --build build --target AdvancedMB 2>&1 | tail -5`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add source/PluginProcessor.cpp
git commit -m "feat: update processBlock for slot-mapped gain reduction metering"
```

---

### Task 12: Build, run, and smoke test

**Step 1: Full clean rebuild**

Run: `cmake -B build && cmake --build build --target AdvancedMB 2>&1 | tail -20`
Expected: Build succeeds with no errors

**Step 2: Run standalone and verify**

Run: `open build/AdvancedMB_artefacts/*/Standalone/AdvancedMB.app`

Manual smoke test checklist:
- [ ] Plugin opens with empty analyzer (0 bands, spectrum only)
- [ ] Clicking on analyzer creates a band (colored fill appears)
- [ ] Band selector no longer visible in bottom strip
- [ ] BandControlPanel opens when band is created
- [ ] Creating additional bands evenly splits the frequency range
- [ ] Crossover handles appear between bands and are draggable
- [ ] Dragging crossovers resizes adjacent bands
- [ ] DEL button on BandControlPanel removes the selected band
- [ ] Deleting last band returns to 0-band state
- [ ] Maximum of 6 bands can be created
- [ ] Threshold dragging works correctly for each band
- [ ] Audio passes through in 0-band state (no processing)

**Step 3: Commit any fixes**

```bash
git add -A
git commit -m "fix: address issues found during smoke testing"
```
