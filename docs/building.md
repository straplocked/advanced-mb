# Building Advanced MB

## Prerequisites

| Tool     | Minimum Version | Notes                                      |
|----------|----------------|--------------------------------------------|
| CMake    | 3.25           | Build system generator                     |
| C++      | C++20          | Standard required (`CMAKE_CXX_STANDARD 20`) |
| Git      | Any recent     | With submodule support                     |

The project uses JUCE (included as a Git submodule) and optionally supports CLAP via `clap-juce-extensions` (also a submodule under `libs/`).

## Cloning the Repository

Always clone with submodules so that JUCE and any extension libraries are pulled in automatically:

```bash
git clone --recurse-submodules https://github.com/advanced-mb/advanced-mb-compressor.git
cd advanced-mb-compressor
```

If you already cloned without `--recurse-submodules`, initialize them manually:

```bash
git submodule update --init --recursive
```

## macOS

### Requirements

- **Xcode Command Line Tools** (or full Xcode)
- macOS 12+ recommended

Install the command line tools if you have not already:

```bash
xcode-select --install
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

By default the macOS build produces **Standalone**, **VST3**, and **AU** plugin formats. After a successful build, the plugin binaries are copied to the standard plugin directories automatically (`COPY_PLUGIN_AFTER_BUILD` is enabled in CMakeLists.txt).

To open the project in Xcode instead of building from the command line:

```bash
cmake -B build -G Xcode
open build/AdvancedMB.xcodeproj
```

### Optional: AAX format

If you have the Avid AAX SDK, pass its path to CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DAAX_SDK_PATH=/path/to/AAX_SDK
cmake --build build --config Release
```

## Windows

### Requirements

- **Visual Studio 2022** (Community edition or higher) with the "Desktop development with C++" workload installed

### Build

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

This produces **Standalone** and **VST3** formats. The `-G` flag selects the Visual Studio generator and `-A x64` targets 64-bit.

To build from within Visual Studio, open the generated `build/AdvancedMB.sln` solution file.

### Optional: AAX format

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64 -DAAX_SDK_PATH=C:\path\to\AAX_SDK
cmake --build build --config Release
```

## Linux

### Requirements

Install the development packages required by JUCE:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    libasound2-dev \
    libcurl4-openssl-dev \
    libfreetype6-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxcomposite-dev \
    mesa-common-dev \
    libgl1-mesa-dev \
    libwebkit2gtk-4.0-dev \
    pkg-config
```

On Fedora/RHEL-based distributions, substitute the equivalent `-devel` packages.

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

This produces **Standalone** and **VST3** formats on Linux.

## Build Options

| CMake Option               | Default | Description                          |
|----------------------------|---------|--------------------------------------|
| `CMAKE_BUILD_TYPE`         | (empty) | `Debug`, `Release`, `RelWithDebInfo` |
| `AAX_SDK_PATH`             | (unset) | Path to the Avid AAX SDK             |
| `ADVANCED_MB_BUILD_TESTS`  | `OFF`   | Enable building of unit tests        |

### Building with Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DADVANCED_MB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

## Plugin Formats

| Format     | macOS | Windows | Linux | Notes                           |
|------------|:-----:|:-------:|:-----:|--------------------------------|
| Standalone |  Yes  |   Yes   |  Yes  | Always built                   |
| VST3       |  Yes  |   Yes   |  Yes  | Always built                   |
| AU         |  Yes  |    --   |   --  | macOS only, automatic          |
| AAX        |  Yes  |   Yes   |   --  | Requires AAX SDK path          |
| CLAP       |  Yes  |   Yes   |  Yes  | Requires `clap-juce-extensions` submodule |

## Troubleshooting

**Submodules are empty / JUCE not found:**
Run `git submodule update --init --recursive` from the repository root.

**CMake version too old:**
The project requires CMake 3.25 or later. On Ubuntu, install a newer CMake from Kitware's APT repository or use `pip install cmake`.

**CLAP format not building:**
Ensure the `libs/clap-juce-extensions` submodule is checked out. If it is missing, the build will skip CLAP with a status message.
