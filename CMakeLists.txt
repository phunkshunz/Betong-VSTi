cmake_minimum_required(VERSION 3.22)

project(BETONG VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: bygg in C++-runtime i pluginet, så att köparen inte behöver installera något extra
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE hämtas automatiskt vid bygget
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Betong
    COMPANY_NAME "phunkshunz"
    PRODUCT_NAME "BETONG"
    BUNDLE_ID "com.phunkshunz.betong"
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Btng
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Sampler Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone)

# Ljudbiblioteket (103 ljud, 24-bit FLAC) byggs in i pluginet
file(GLOB BETONG_LIBRARY CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/library/*.flac)
juce_add_binary_data(BetongData SOURCES ${BETONG_LIBRARY})

target_sources(Betong
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/Library.cpp)

target_compile_definitions(Betong
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Betong
    PRIVATE
        BetongData
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
