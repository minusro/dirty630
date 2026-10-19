cmake_minimum_required(VERSION 3.22)

# Build universal binaries on macOS unless the caller overrides it.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(dirty630 VERSION 0.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(DIRTY630_BUILD_PLUGIN "Build the JUCE plugin targets" ON)
option(DIRTY630_BUILD_TESTS  "Build the plain-C++ DSP tests"   ON)
set(DIRTY630_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (empty = fetch JUCE 8.0.4)")

# ---------------------------------------------------------------------------
# Pure C++ DSP core: no JUCE dependency, so the model can be unit tested and
# reused outside the plugin.
# ---------------------------------------------------------------------------
add_library(dirty630_dsp INTERFACE)
target_include_directories(dirty630_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(DIRTY630_BUILD_TESTS)
    enable_testing()
    add_executable(dirty630_tests tests/dsp_tests.cpp)
    target_link_libraries(dirty630_tests PRIVATE dirty630_dsp)
    target_compile_options(dirty630_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -O2>)
    add_test(NAME dsp_tests COMMAND dirty630_tests)
endif()

# ---------------------------------------------------------------------------
# JUCE plugin
# ---------------------------------------------------------------------------
if(DIRTY630_BUILD_PLUGIN)
    if(DIRTY630_JUCE_DIR)
        add_subdirectory(${DIRTY630_JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.4
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    set(DIRTY630_FORMATS VST3)
    if(APPLE)
        list(APPEND DIRTY630_FORMATS AU Standalone)
    endif()

    juce_add_plugin(dirty630
        COMPANY_NAME                "minus"
        PLUGIN_MANUFACTURER_CODE    Minu
        PLUGIN_CODE                 D630
        PRODUCT_NAME                "DIRTY630"
        BUNDLE_ID                   "com.minus.dirty630"
        FORMATS                     ${DIRTY630_FORMATS}
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        VST3_CATEGORIES             Fx Dynamics
        AU_MAIN_TYPE                kAudioUnitType_Effect
        COPY_PLUGIN_AFTER_BUILD     TRUE)

    target_sources(dirty630 PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

    # BuildInfo.h carries the displayed version (major.minor, policy in
    # CLAUDE.md) and is re-stamped with the build time on every build.
    add_custom_target(dirty630_buildinfo
        COMMAND ${CMAKE_COMMAND}
            -DOUT=${CMAKE_BINARY_DIR}/generated/BuildInfo.h
            -DVERSION=${PROJECT_VERSION}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildInfo.cmake
        BYPRODUCTS ${CMAKE_BINARY_DIR}/generated/BuildInfo.h)
    add_dependencies(dirty630 dirty630_buildinfo)
    target_include_directories(dirty630 PRIVATE ${CMAKE_BINARY_DIR}/generated)

    target_compile_definitions(dirty630 PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    if(UNIX AND NOT APPLE)
        # Keep the Linux sanity build free of optional X11/ALSA extensions.
        target_compile_definitions(dirty630 PUBLIC
            JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XSHM=0
            JUCE_ALSA=0 JUCE_JACK=0)
    endif()

    target_link_libraries(dirty630
        PRIVATE
            dirty630_dsp
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()
