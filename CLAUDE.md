# CLAUDE.md - Advanced MB

## Project Overview

Advanced MB is a professional multiband compressor audio plugin inspired by FabFilter Pro-MB. Built with C++20 and JUCE 7, it targets VST3, AU, CLAP, AAX, and Standalone formats across macOS, Windows, and Linux.

**Version:** 0.1.0 (active development)

## Build Commands

```bash
# Configure
cmake -B build

# Build (debug)
cmake --build build

# Build (release)
cmake --build build --config Release

# Build with tests
cmake -B build -DADVANCED_MB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build

# Linux prerequisites
sudo apt-get install libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev libxcomposite-dev

# Clone with submodules
git clone --recurse-submodules https://github.com/straplocked/advanced-mb.git
```

Build output: `build/AdvancedMB_artefacts/` (Standalone, VST3, AU, CLAP depending on platform).

## Architecture

### Signal Flow

```
Input → Oversampler (1x/2x/4x)
      → CrossoverFilterBank (Linkwitz-Riley, up to 6 bands)
      → Per-band: LevelDetector → GainComputer → EnvelopeFollower → Gain
      → Sum bands → Dry/wet mix (0-200%) → Global output (±30 dB)
```

### Threading Model

- **Audio thread**: `processBlock()` — no allocations, no locks, no I/O. Parameters read via `std::atomic<float>*`.
- **UI thread**: 30 Hz timer. Metering via `TripleBuffer` / atomic stores. Never blocks audio.

### Source Layout

```
source/
├── PluginProcessor.h/cpp       # Main AudioProcessor
├── PluginEditor.h/cpp          # Main editor window (30Hz timer)
├── dsp/
│   ├── MultibandEngine          # Top-level DSP orchestrator
│   ├── CrossoverFilterBank      # Band splitting (Linkwitz-Riley)
│   ├── LinkwitzRileyFilter      # Dual LP/HP crossover pair
│   ├── SVFFilter                # Cytomic TPT state-variable filter
│   ├── BandCompressorCore       # Single-band compressor
│   ├── GainComputer             # Soft-knee transfer function
│   ├── EnvelopeFollower         # Attack/release with freq-dependent scaling
│   ├── LevelDetector            # Peak/RMS measurement
│   ├── BandManager              # Dynamic band pool (up to 6 slots)
│   ├── Band.h                   # Band data struct
│   ├── LinearPhaseCrossover     # FFT-based crossover alternative
│   ├── MidSideProcessor.h       # L/R ↔ M/S (header-only)
│   └── Oversampler              # Polyphase IIR oversampling
├── ui/
│   ├── components/
│   │   ├── MultibandDisplay     # Interactive spectrum + band visualization
│   │   ├── BandControlPanel     # Floating per-band control popup
│   │   ├── HeaderToolbar        # Presets, undo/redo, A/B
│   │   ├── BottomControlStrip   # Global controls (mix, output, OS)
│   │   ├── LevelMeterComponent  # I/O + GR meters
│   │   ├── PresetBrowserPanel   # Preset browser
│   │   └── HelpOverlay          # Contextual tooltips
│   └── AdvancedMBLookAndFeel    # Custom styling + fonts
├── parameters/
│   ├── ParameterLayout          # APVTS parameter creation
│   ├── ParameterIDs.h           # Parameter ID string constants
│   └── MultibandParameterAttachment  # Lock-free APVTS→DSP bridge
├── analyzer/
│   ├── FFTProcessor             # Spectrum analysis (4096-point FFT)
│   └── TripleBuffer.h           # Lock-free triple buffer
├── metering/LevelMeterData      # Lock-free metering data
├── presets/PresetManager         # Factory/user presets
├── midi/MIDILearnManager.h      # MIDI CC mapping
└── utils/
    ├── Constants.h              # All constants, enums, limits
    ├── CompressorMath.h         # DSP math helpers (header-only)
    ├── MathUtils.h              # General math (header-only)
    └── FrequencyUtils.h         # Hz/log conversion (header-only)
```

### Key Design Patterns

- **Parameter Pool**: 6 fixed APVTS slots with dynamic band assignment via `BandManager`. Slot indices are stable for DAW automation.
- **Region Model**: User bands + gap regions between them — up to 13 regions total (6 bands + 7 gaps).
- **Atomic Bridge**: `MultibandParameterAttachment` reads all APVTS atomics into a struct each `processBlock`.
- **Triple Buffer**: Lock-free audio→UI data transfer for spectrum/metering.
- **DSP Lifecycle**: All DSP classes implement `prepare(sampleRate, blockSize)` / `reset()` / `process(buffer)`.

## Code Conventions

### Naming

- **Classes/files**: PascalCase (`CrossoverFilterBank.h`)
- **Functions**: camelCase (`processSample`, `setFrequency`)
- **Constants**: `kPascalCase` (`kMaxBands`, `kFFTSize`)
- **Enums**: PascalCase values in `Constants` namespace
- **Member variables**: camelCase, no prefix

### Formatting

- 4-space indentation, no tabs
- Spaces inside parentheses: `if ( condition )`, `foo( bar, baz )`
- Allman-ish braces (opening brace on new line for functions/classes)

### Audio Thread Rules

- No heap allocation (`new`, `malloc`, container resize)
- No mutex/lock
- No system calls or I/O
- Use `ScopedNoDenormals` at top of `processBlock`
- Read parameters only via `std::atomic<float>*` from APVTS

### Parameter Conventions

- Always use JUCE APVTS attachments (`SliderAttachment`, `ComboBoxAttachment`, `ButtonAttachment`) — never raw `Slider::Listener`
- Parameter IDs: `bandParam(index, suffix)` → `"band0_threshold"`, `crossoverParam(index, suffix)` → `"xover2_freq"`
- Global params: simple strings (`"process_mode"`, `"mix"`, `"global_output"`)
- Header-only for simple/stateless processors (`CompressorMath.h`, `MidSideProcessor.h`)

## Dependencies

- **JUCE 7** — git submodule at `JUCE/`
- **clap-juce-extensions** — git submodule at `libs/clap-juce-extensions/`
- **C++20** required (Clang 13+, GCC 11+, MSVC 2022+)
- CMake 3.25+
- Compiler flags: `-ffast-math` (GCC/Clang), `/fp:fast` (MSVC)

JUCE modules: `juce_audio_utils`, `juce_audio_processors`, `juce_dsp`, `juce_gui_extra`, `juce_opengl`

## Documentation

- `docs/architecture.md` — Signal flow, class hierarchy, threading model
- `docs/dsp.md` — Filter design, gain computer math, envelope follower
- `docs/parameters.md` — Complete parameter reference with ranges
- `docs/building.md` — Platform-specific build instructions
- `docs/plans/` — Design documents for architectural changes
