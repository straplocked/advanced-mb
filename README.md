# Advanced MB

An open-source multiband compressor audio plugin inspired by FabFilter Pro-MB. Built with JUCE 7 and C++20.

Advanced MB is part of a family of sibling plugins: **advanced-eq**, **advanced-compressor**, and **advanced-limiter**.

## Features

- Up to 6 bands with dynamic add/remove via an interactive spectrum display
- Linkwitz-Riley crossover filters with selectable slopes (6/12/18/24/36/48 dB/oct)
- Compress and Expand (downward expansion) modes per band
- Percentage-based attack/release (0--100%, Pro-MB style) with frequency-dependent scaling
- Three processing modes: Dynamic Phase (IIR), Linear Phase (FIR), Minimum Phase
- Band/Free sidechain with configurable bandpass filter
- Full spectrum analyzer (pre, post, or both)
- Per-band gain reduction metering
- 15 factory presets across Mastering, Mixing, Creative, and Starting Points categories
- Stereo and Mid/Side processing with adjustable stereo link per band
- Oversampling (1x / 2x / 4x)
- Dry/wet mix (0--200%)
- MIDI Learn for CC mapping
- A/B state comparison
- Keyboard shortcuts for common operations
- Resizable UI

## Plugin Formats

| Format     | macOS | Windows | Linux |
|------------|:-----:|:-------:|:-----:|
| VST3       | Yes   | Yes     | Yes   |
| AU         | Yes   | --      | --    |
| CLAP       | Yes   | Yes     | Yes   |
| Standalone | Yes   | Yes     | Yes   |

## Build Instructions

### Prerequisites

- C++20-compatible compiler (Clang 14+, GCC 12+, MSVC 2022+)
- [CMake](https://cmake.org/) 3.25 or later
- Platform SDK (Xcode on macOS, Visual Studio on Windows, system dev packages on Linux)

### Clone and Build

```bash
git clone --recurse-submodules https://github.com/your-org/advanced-mb-compressor.git
cd advanced-mb-compressor
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

#### macOS / Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Windows

```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

#### Linux Dependencies

On Debian/Ubuntu you will need:

```bash
sudo apt install libasound2-dev libcurl4-openssl-dev libfreetype6-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libgl1-mesa-dev
```

### Build Options

| Option                    | Default | Description              |
|---------------------------|---------|--------------------------|
| `ADVANCED_MB_BUILD_TESTS` | `OFF`   | Build the test suite     |
| `AAX_SDK_PATH`            | --      | Path to AAX SDK for AAX builds |

Built plugins are copied to the system plugin folder automatically (`COPY_PLUGIN_AFTER_BUILD` is enabled).

## Architecture

### Signal Flow

```
Input -> Crossover Filter Bank -> Per-Band Compression -> Sum -> Dry/Wet Mix -> Global Output
               |                        |
        Linkwitz-Riley              Threshold
        split (2-6 bands)           Ratio / Knee
                                    Attack / Release
                                    Gain Computer
                                    Envelope Follower
```

### Processing Modes

- **Dynamic Phase** -- IIR Linkwitz-Riley crossover with minimal latency
- **Linear Phase** -- FIR crossover that preserves phase relationships across bands
- **Minimum Phase** -- FIR crossover with minimum-phase response

## Project Structure

```
advanced-mb-compressor/
├── CMakeLists.txt
├── JUCE/                          # JUCE framework (submodule)
├── libs/
│   └── clap-juce-extensions/      # CLAP format support (submodule)
├── cmake/
│   └── PlatformDefaults.cmake
├── resources/
│   └── fonts/
├── source/
│   ├── PluginProcessor.cpp/.h     # Main audio processor
│   ├── PluginEditor.cpp/.h        # Main editor
│   ├── dsp/
│   │   ├── CrossoverFilterBank     # Linkwitz-Riley band splitting
│   │   ├── LinearPhaseCrossover    # FIR linear-phase crossover
│   │   ├── LinkwitzRileyFilter     # Per-slope LR filter
│   │   ├── SVFFilter               # State-variable filter
│   │   ├── BandCompressorCore      # Per-band compression engine
│   │   ├── GainComputer            # Threshold/ratio/knee math
│   │   ├── EnvelopeFollower        # Attack/release envelope
│   │   ├── LevelDetector           # RMS/peak detection
│   │   ├── MidSideProcessor        # M/S encoding/decoding
│   │   ├── MultibandEngine         # Top-level DSP orchestrator
│   │   └── Oversampler             # 2x/4x oversampling
│   ├── analyzer/
│   │   ├── FFTProcessor            # Spectrum analysis
│   │   └── TripleBuffer            # Lock-free UI data exchange
│   ├── metering/
│   │   └── LevelMeterData          # Thread-safe meter values
│   ├── parameters/
│   │   ├── ParameterIDs            # Canonical parameter ID strings
│   │   ├── ParameterLayout         # APVTS parameter creation
│   │   └── MultibandParameterAttachment
│   ├── presets/
│   │   ├── PresetManager           # Save/load/factory presets
│   │   └── FactoryPresets          # 15 built-in presets
│   ├── midi/
│   │   └── MIDILearnManager        # CC-to-parameter mapping
│   ├── ui/
│   │   ├── components/
│   │   │   ├── MultibandDisplay    # Interactive spectrum/band editor
│   │   │   ├── BandControlPanel    # Per-band knobs and controls
│   │   │   ├── HeaderToolbar       # Preset selector, A/B, menus
│   │   │   ├── BottomControlStrip  # Global controls
│   │   │   └── LevelMeterComponent # I/O and GR meters
│   │   ├── help/
│   │   │   ├── HelpOverlay         # On-screen help system
│   │   │   └── HelpTextDatabase    # Contextual help strings
│   │   └── LookAndFeel/
│   │       ├── AdvancedMBLookAndFeel
│   │       ├── ColourPalette
│   │       └── Fonts
│   └── utils/
│       ├── Constants               # Global constants and enums
│       ├── CompressorMath          # DSP math utilities
│       ├── FrequencyUtils          # Hz/log conversion helpers
│       └── MathUtils               # General math helpers
└── tests/                          # Unit tests (optional build)
```

## Built With

- [JUCE 7](https://juce.com/) -- Cross-platform C++ audio framework
- C++20
- [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) -- CLAP format support

## Contributing

Contributions are welcome. Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on how to get started.

## License

This project is licensed under the **GNU General Public License v3.0 or later** -- see the [LICENSE](LICENSE) file for details.

```
SPDX-License-Identifier: GPL-3.0-or-later
```
