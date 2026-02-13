# Platform-specific compiler flags and settings
# Note: CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_ARCHITECTURES are set in
# CMakeLists.txt before project() so they propagate to all sub-builds.

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Suppress common JUCE warnings
if(NOT MSVC)
    add_compile_options(-Wno-unused-parameter)
endif()
