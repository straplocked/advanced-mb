cmake_minimum_required(VERSION 3.25)

set(ADVANCED_MB_VERSION_MAJOR 0)
set(ADVANCED_MB_VERSION_MINOR 1)
set(ADVANCED_MB_VERSION_PATCH 0)

project(AdvancedMB
    VERSION ${ADVANCED_MB_VERSION_MAJOR}.${ADVANCED_MB_VERSION_MINOR}.${ADVANCED_MB_VERSION_PATCH}
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform defaults
include(cmake/PlatformDefaults.cmake)

# JUCE
add_subdirectory(JUCE)

# CLAP support
set(CLAP_JUCE_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions" CACHE STRING "")
if(EXISTS "${CLAP_JUCE_EXTENSIONS_DIR}/CMakeLists.txt")
    add_subdirectory(${CLAP_JUCE_EXTENSIONS_DIR} clap-juce-extensions)
    set(HAS_CLAP TRUE)
else()
    message(STATUS "clap-juce-extensions not found, CLAP format will not be built")
    set(HAS_CLAP FALSE)
endif()

# Plugin formats
set(FORMATS Standalone VST3)
if(APPLE)
    list(APPEND FORMATS AU)
endif()
if(DEFINED AAX_SDK_PATH)
    juce_set_aax_sdk_path(${AAX_SDK_PATH})
    list(APPEND FORMATS AAX)
endif()

# --- Plugin Target ---
juce_add_plugin(AdvancedMB
    COMPANY_NAME "AdvancedMB"
    BUNDLE_ID "com.advancedmb.advancedmb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE AMbP
    PLUGIN_CODE AMb1
    FORMATS ${FORMATS}
    PRODUCT_NAME "Advanced MB"
    VST3_CATEGORIES "Fx" "Dynamics" "Mastering"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    AAX_CATEGORY "AAX_ePlugInCategory_Dynamics"
)

# CLAP extension
if(HAS_CLAP)
    clap_juce_extensions_plugin(
        TARGET AdvancedMB
        CLAP_ID "com.advancedmb.advancedmb"
        CLAP_FEATURES "audio-effect" "compressor" "dynamics" "mastering"
        CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
        CLAP_USE_JUCE_PARAMETER_RANGES DISCRETE
    )
endif()

# --- Binary Data (fonts) ---
juce_add_binary_data(AdvancedMBBinaryData SOURCES
    resources/fonts/SpaceGrotesk-Regular.ttf
    resources/fonts/Audiowide-Regular.ttf
)

# --- Source Files ---
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)
target_sources(AdvancedMB PRIVATE ${SourceFiles})
target_include_directories(AdvancedMB PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source")

# --- JUCE Module Dependencies ---
target_link_libraries(AdvancedMB
    PRIVATE
        AdvancedMBBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- Preprocessor Definitions ---
target_compile_definitions(AdvancedMB
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
    PRIVATE
        ADVANCED_MB_VERSION="${PROJECT_VERSION}"
)

# --- Fast math for DSP performance ---
if(NOT MSVC)
    target_compile_options(AdvancedMB PRIVATE -ffast-math)
else()
    target_compile_options(AdvancedMB PRIVATE /fp:fast)
endif()

# --- Testing ---
option(ADVANCED_MB_BUILD_TESTS "Build tests" OFF)
if(ADVANCED_MB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
