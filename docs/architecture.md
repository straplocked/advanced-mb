# Architecture

## Signal Flow

```
                          +------------------+
                          |      Input       |
                          +--------+---------+
                                   |
                          +--------v---------+
                          |   Oversampler    |  (1x / 2x / 4x polyphase IIR)
                          +--------+---------+
                                   |
                          +--------v---------+
                          | CrossoverFilter  |  Linkwitz-Riley filter bank
                          |       Bank       |  splits into 1-6 bands
                          +--+--+--+--+--+---+
                             |  |  |  |  |
               +-------------+  |  |  |  +-------------+
               |             |  |  |  |                 |
         +-----v----+  +----v--v--v----+          +-----v----+
         |  Band 1  |  |  Band 2 .. 5 |          |  Band 6  |
         |Compressor|  |  Compressors  |          |Compressor|
         +-----+----+  +------+-------+          +-----+----+
               |               |                        |
               +-------+-------+------------------------+
                       |
              +--------v---------+
              |       Sum        |  Sum all band outputs
              +--------+---------+
                       |
              +--------v---------+
              |    Dry/Wet Mix   |  0-200%
              +--------+---------+
                       |
              +--------v---------+
              |  Global Output   |  -30 to +30 dB
              +--------+---------+
                       |
              +--------v---------+
              |      Output      |
              +------------------+
```

Each **BandCompressorCore** internally runs this per-sample chain:

```
Band Signal -> LevelDetector -> Stereo Linking -> GainComputer -> EnvelopeFollower -> Apply Gain -> Output Gain
```

## Class Hierarchy

### Top Level

```
AdvancedMBProcessor (juce::AudioProcessor)
  |-- AudioProcessorValueTreeState (apvts)
  |-- MultibandParameterAttachment  (reads APVTS, fills MultibandParameters struct)
  |-- MultibandEngine               (owns all DSP)
  |-- FFTProcessor x2               (pre/post analyzer)
  |-- LevelMeterData                (input/output/GR metering)
  |-- UndoManager
```

### MultibandEngine Internals

```
MultibandEngine
  |-- Oversampler                       (JUCE polyphase IIR, 1x/2x/4x)
  |-- CrossoverFilterBank
  |     +-- LinkwitzRileyFilter[5]      (one per crossover point, per channel)
  |           +-- SVFFilter[]           (cascaded Cytomic TPT sections)
  |-- BandCompressorCore[6]             (one per band, max 6)
        |-- LevelDetector (L)
        |-- LevelDetector (R)
        |-- GainComputer
        |-- EnvelopeFollower (L)
        +-- EnvelopeFollower (R)
```

### UI

```
AdvancedMBEditor (juce::AudioProcessorEditor)
  |-- HeaderToolbar          (preset selector, undo/redo, A/B, settings)
  |-- MultibandDisplay       (spectrum analyzer, crossover handles, GR meters)
  |-- BandControlPanel       (per-band knobs and controls)
  |-- BottomControlStrip     (global controls: mix, output, oversampling)
  |-- LevelMeterComponent    (input/output level meters)
  |-- HelpOverlay            (interactive tooltip overlay)
  +-- AdvancedMBLookAndFeel  (custom fonts, colours, drawing routines)
```

## Threading Model

### Two-Thread Architecture

The plugin operates with two principal threads:

1. **Audio Thread** -- calls `AdvancedMBProcessor::processBlock()` at the host's buffer rate.
2. **UI Thread** -- calls `AdvancedMBEditor::timerCallback()` at 30 Hz (`Constants::kUITimerHz`).

### Parameter Flow

```
UI Thread                              Audio Thread
---------                              ------------
User moves knob
  -> APVTS writes atomic float           APVTS reads atomic float
                                           -> MultibandParameterAttachment
                                              reads all params into
                                              MultibandParameters struct
                                           -> MultibandEngine::updateParameters()
                                           -> MultibandEngine::process()
```

JUCE's `AudioProcessorValueTreeState` stores each parameter as an `std::atomic<float>`. The UI thread writes new values; the audio thread reads them at the start of each `processBlock()` call via `MultibandParameterAttachment::getCurrentParameters()`. No locks are acquired on the audio thread.

### Metering Data Flow (Audio -> UI)

Metering data (spectrum FFT bins, level meter values, per-band gain reduction) flows from the audio thread to the UI thread via lock-free structures:

- **`TripleBuffer<float>`** -- Used by `FFTProcessor` to publish spectrum data. The audio thread writes into one buffer and calls `finishWrite()`. The UI thread calls `swapRead()` to atomically obtain the latest completed buffer without blocking.
- **`LevelMeterData`** -- Stores peak/RMS levels and per-band gain reduction using atomic floats, read by `LevelMeterComponent` on the UI thread.

### Real-Time Safety Guarantees

The audio thread path (`processBlock` -> `MultibandEngine::process`) adheres to these constraints:

- **No heap allocations.** All buffers (`bandBuffers`, `dryBuffer`, filter state arrays) are pre-allocated in `prepare()`.
- **No locks or mutexes.** Parameter reads use atomics; metering writes use lock-free triple buffers or atomic stores.
- **No system calls.** No file I/O, no logging, no thread creation.
- **`ScopedNoDenormals`** is engaged at the top of `processBlock` to prevent denormalized floating-point slowdowns.
- **`-ffast-math`** (GCC/Clang) or `/fp:fast` (MSVC) is enabled for DSP performance.

## Key DSP Classes

| Class                  | File                           | Responsibility                                                    |
|------------------------|--------------------------------|-------------------------------------------------------------------|
| `SVFFilter`            | `source/dsp/SVFFilter.h`       | Cytomic TPT state variable filter. 1st or 2nd order. Core building block. |
| `LinkwitzRileyFilter`  | `source/dsp/LinkwitzRileyFilter.h` | Cascades SVFFilters to produce LR crossover pairs (LP + HP) at configurable slopes. |
| `CrossoverFilterBank`  | `source/dsp/CrossoverFilterBank.h` | Manages up to 5 LinkwitzRileyFilters to split audio into 1-6 bands. |
| `GainComputer`         | `source/dsp/GainComputer.h`    | Soft-knee static transfer function. Compress and expand modes.    |
| `EnvelopeFollower`     | `source/dsp/EnvelopeFollower.h`| One-pole attack/release smoother with frequency-dependent timing. |
| `LevelDetector`        | `source/dsp/LevelDetector.h`   | Peak and RMS level measurement, returns dB.                      |
| `BandCompressorCore`   | `source/dsp/BandCompressorCore.h` | Complete single-band compressor: detection -> gain computation -> envelope -> gain application. |
| `MultibandEngine`      | `source/dsp/MultibandEngine.h` | Top-level DSP: oversampling, crossover splitting, per-band compression, summing, mixing. |
| `Oversampler`          | `source/dsp/Oversampler.h`     | Wraps JUCE's polyphase IIR oversampling (1x, 2x, 4x).           |
| `MidSideProcessor`     | `source/dsp/MidSideProcessor.h`| Encode/decode between L/R and Mid/Side for per-band stereo mode. |
| `LinearPhaseCrossover`  | `source/dsp/LinearPhaseCrossover.h` | FFT-based linear-phase crossover alternative.               |
| `CompressorMath`       | `source/utils/CompressorMath.h`| Static utility functions for gain reduction, time-to-coefficient conversion. |
| `TripleBuffer<T>`      | `source/analyzer/TripleBuffer.h` | Lock-free single-producer/single-consumer triple buffer for metering. |
| `FFTProcessor`         | `source/analyzer/FFTProcessor.h` | Windowed FFT for spectrum analysis (4096 bins).                 |

## State Persistence

Plugin state is serialized as XML via JUCE's `ValueTree`. `getStateInformation()` exports the APVTS tree to binary XML; `setStateInformation()` restores it. An A/B comparison feature stores two independent state snapshots (`stateSlotA`, `stateSlotB`) that can be swapped at runtime.
