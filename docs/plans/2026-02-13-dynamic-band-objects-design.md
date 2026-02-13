# Dynamic Band Objects Design

## Summary

Replace the fixed-array band parameter model with a dynamic Band object system. Users click on the analyzer to create bands and delete them via the BandControlPanel. Crossover dragging freely resizes adjacent bands. The plugin starts with 0 bands (spectrum only).

## Band Object Architecture

### Band Class
- Encapsulates all per-band state and behavior
- Owns a reference to its assigned APVTS parameter slot (0-5)
- Holds left/right crossover frequency boundaries
- Knows its position in the frequency spectrum (ordered by frequency)
- Manages its own enabled/threshold/ratio/etc. state

### BandManager Class
- Pool of 6 parameter slots — assigns free slots to new bands
- Tracks active bands ordered by frequency
- Handles creation (click on analyzer) and removal (delete button)
- Recomputes crossover positions on add/remove (even split of 20Hz-20kHz, log-scaled)
- Notifies DSP and UI of band topology changes

### Slot Mapping
```
BandManager
├── Band[0] → parameter slot 2
├── Band[1] → parameter slot 0
├── Band[2] → parameter slot 4
└── (slots 1, 3, 5 are unused/available)
```

Bands are ordered by frequency but can map to any underlying parameter slot. A band keeps the same slot for its lifetime (stable DAW automation).

## Click-to-Create Interaction

1. User clicks empty area of MultibandDisplay
2. BandManager checks if below max (6 bands)
3. Allocates next free parameter slot
4. Creates Band object, assigns slot
5. Inserts band in frequency-ordered position based on click location
6. Recomputes crossovers: even split of 20Hz-20kHz (log-scaled) among all active bands
7. New band starts transparent (threshold 0dB, ratio 1:1)
8. BandControlPanel opens for the new band

## Crossover Dragging

- Dragging a crossover handle freely repositions it
- Adjacent bands automatically resize (left band wider/narrower, right band inverse)
- No band removal from dragging — bands can become very narrow but never disappear

## Band Removal

- Delete button on BandControlPanel removes the selected band
- Band object destroyed, parameter slot freed to pool
- Remaining bands' crossovers recalculate (even split)
- Selection clears
- Deleting the last band returns to 0-band state

## Edge Cases

- **0 bands**: Analyzer shows spectrum only, no band regions or crossovers. Clicking creates the first band (full spectrum, no crossovers).
- **1 band**: Full spectrum, no crossovers visible. Delete button removes to 0-band state.
- **6 bands**: Clicks on analyzer ignored (max reached).

## Parameter & DSP Integration

### Parameter Slot Pool
- All 6 sets of band parameters remain pre-allocated in APVTS (JUCE constraint)
- `numBands` parameter default changes from 1 to 0
- Slot mapping stored in processor's ValueTree (not automatable)

### Band Parameter Defaults on Creation
- Threshold: 0 dB (transparent)
- Ratio: 1.0:1 (unity)
- Range: 0 dB
- Attack/Release: 50%
- Output gain: 0 dB
- Enabled: true

### DSP Changes
- MultibandEngine reads slot mapping to know which parameter slots are active and in what order
- CrossoverFilterBank receives crossover frequencies in frequency order regardless of slot
- BandCompressorCore array stays fixed at 6 — only mapped slots process audio

### State Save/Restore
- Slot mapping serialized in getStateInformation / setStateInformation
- On preset load: BandManager reconstructs Band objects from saved mapping
- DAW automation stays consistent per-slot

## UI Changes

### MultibandDisplay
- `mouseDown`: click on empty area creates band via BandManager
- `mouseDrag`: free crossover repositioning, resizes adjacent bands
- Band click selects and opens BandControlPanel
- 0-band state: spectrum curves only

### BandControlPanel
- Add Delete button (removes selected band via BandManager)
- Close button (X) hides panel; Delete removes the band
- Band label shows logical band number (frequency order, 1-based)

### BottomControlStrip
- Remove the `Bands - [value] +` control entirely
- Reclaim space for remaining controls

## Files Changed

| Area | Files | Change |
|------|-------|--------|
| New | `Band.h/cpp` | Band object class |
| New | `BandManager.h/cpp` | Pool management, create/remove/reorder |
| Modified | `MultibandDisplay.h/cpp` | Click-to-create, free crossover drag |
| Modified | `BandControlPanel.h/cpp` | Add delete button |
| Modified | `BottomControlStrip.h/cpp` | Remove band selector |
| Modified | `PluginProcessor.h/cpp` | Own BandManager, serialize slot mapping |
| Modified | `MultibandEngine.h/cpp` | Read slot mapping for DSP routing |
| Modified | `ParameterLayout.cpp` | Default numBands to 0 |
| Modified | `Constants.h` | Default band count to 0 |
