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

#===============================================================================
# Maskinerna.
#
# Alla maskiner i MASKINHALLEN byggs ur samma källor och har samma funktioner.
# Det som skiljer är namnet och identiteten i DAW:en: produktnamn, plugin-kod,
# bundle-id, raderna i gränssnittet och vilket mönster maskinen öppnar med.
# Alla ligger under samma tillverkare, så de sorteras ihop i DAW:ens lista.
#
# En maskin per rad: mål | produktnamn | plugin-kod (4 tecken) | bundle-id | rad på om-sidan | tagline | öppningsmönster
#
# Produktnamnet syns i DAW:en och i loggan: håll det kort (högst 8 tecken),
# annars klipps det i loggrutan. Plugin-koden måste vara fyra tecken och unik
# per tillverkare, annars kan DAW:en blanda ihop maskinerna.
# tools/check_machines.py kontrollerar allt detta, och körs av bygget.
set(BETONG_MACHINES
    "Betong|BETONG|Btng|com.phunkshunz.betong|techno-sampler|TECHNO SAMPLER|Grund 130")

if(APPLE)
    set(BETONG_PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(BETONG_PLUGIN_FORMATS VST3 Standalone)
endif()

# Ljudbiblioteket (215 ljud, 24-bit FLAC) byggs in en gång och delas av maskinerna
file(GLOB BETONG_LIBRARY CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/library/*.flac)
juce_add_binary_data(BetongData SOURCES ${BETONG_LIBRARY})

function(betong_add_machine target product code bundle about tagline pattern)
    juce_add_plugin(${target}
        COMPANY_NAME "Phunkshunz"
        PRODUCT_NAME "${product}"
        BUNDLE_ID "${bundle}"
        PLUGIN_MANUFACTURER_CODE Phnk
        PLUGIN_CODE ${code}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Sampler Drum
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        FORMATS ${BETONG_PLUGIN_FORMATS})

    target_sources(${target}
        PRIVATE
            src/PluginProcessor.cpp
            src/PluginEditor.cpp
            src/Library.cpp)

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_DISPLAY_SPLASH_SCREEN=0
        PRIVATE
            BETONG_PRODUCT_NAME="${product}"
            BETONG_ABOUT_LINE="${about}"
            BETONG_TAGLINE="${tagline}"
            BETONG_DEFAULT_PATTERN="${pattern}")

    target_link_libraries(${target}
        PRIVATE
            BetongData
            juce::juce_audio_utils
            juce::juce_audio_formats
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

foreach(machine IN LISTS BETONG_MACHINES)
    string(REPLACE "|" ";" parts "${machine}")
    list(GET parts 0 target)
    list(GET parts 1 product)
    list(GET parts 2 code)
    list(GET parts 3 bundle)
    list(GET parts 4 about)
    list(GET parts 5 tagline)
    list(GET parts 6 pattern)
    betong_add_machine(${target} "${product}" "${code}" "${bundle}" "${about}" "${tagline}" "${pattern}")
endforeach()

# MSVC: läs källkoden som UTF-8, så att å, ä och ö blir rätt
if(MSVC)
    if(TARGET Betong)
        target_compile_options(Betong PRIVATE /utf-8 /bigobj)
    endif()
    if(TARGET Smidet)
        target_compile_options(Smidet PRIVATE /utf-8 /bigobj)
    endif()
endif()
