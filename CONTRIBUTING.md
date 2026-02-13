# Contributing to Advanced MB

Thank you for your interest in contributing! This guide will help you get started.

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## Getting Started

### Prerequisites

- CMake 3.25+
- C++20 compiler (Clang 13+, GCC 11+, MSVC 2022+)
- Git

### Building from Source

```bash
git clone --recurse-submodules https://github.com/straplocked/advanced-mb.git
cd advanced-mb
cmake -B build
cmake --build build
```

On Linux, install dependencies first:

```bash
sudo apt-get install libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev libxcomposite-dev
```

### Running the Plugin

After building, the Standalone app is in `build/AdvancedMB_artefacts/`. On macOS, AU and VST3 are automatically copied to system plugin directories.

## Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: Allman-ish style (opening brace on new line for functions/classes)
- **Spacing**: Spaces inside parentheses: `if ( condition )`, `foo( bar, baz )`
- **File naming**: PascalCase matching class names (`CrossoverFilterBank.h`, `CrossoverFilterBank.cpp`)
- **Includes**: JUCE headers via module includes, project headers via relative paths

### DSP Classes

All DSP classes follow the pattern:

```cpp
void prepare( double sampleRate, int samplesPerBlock );
void reset();
void process( juce::AudioBuffer<float>& buffer );
```

Header-only for simple processors (e.g., `MidSideProcessor.h`, `CompressorMath.h`).

### Parameter Attachments

Use JUCE `AudioProcessorValueTreeState` attachments (`SliderAttachment`, `ComboBoxAttachment`, `ButtonAttachment`). Do not use `Slider::Listener` or `Button::Listener` for parameter-connected controls.

### Audio Thread Safety

Never allocate memory, lock mutexes, or perform I/O on the audio thread. Parameters are read via `std::atomic<float>*` pointers in `MultibandParameterAttachment`.

## Pull Request Process

1. **Fork** the repository and create a feature branch from `main`
2. **Make your changes** following the code style above
3. **Test** your changes:
   - Build successfully on your platform
   - Test in at least one DAW (or standalone)
   - Verify no audio glitches or crashes
4. **Commit** with clear, descriptive messages
5. **Open a PR** against `main` with a description of what changed and why

### What Makes a Good PR

- Focused on a single change or feature
- Includes a clear description of the problem and solution
- Follows existing code style and patterns
- Does not introduce new compiler warnings
- Has been tested with real audio

## Reporting Issues

Use [GitHub Issues](https://github.com/straplocked/advanced-mb/issues) to report bugs or request features. Please include:

- **Bug reports**: OS, DAW, plugin format, sample rate, buffer size, steps to reproduce
- **Feature requests**: Use case description and proposed solution

## Questions?

Open a [Discussion](https://github.com/straplocked/advanced-mb/discussions) for questions about the codebase, architecture, or development.
