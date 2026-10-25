cmake_minimum_required(VERSION 3.22)
project(PadsAccords VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# Instrument qui produit du MIDI : c'est la seule forme que Live accepte pour un générateur d'accords en plugin.
juce_add_plugin(PadsAccords
    COMPANY_NAME "PYR"
    PLUGIN_MANUFACTURER_CODE Pyrd
    PLUGIN_CODE Padc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Pads Accords"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(PadsAccords PRIVATE
    Source/ChordEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(PadsAccords PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Les accords contiennent des ♯ et des ♭ : les sources sont en UTF-8.
target_compile_options(PadsAccords PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(PadsAccords
    PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Test du moteur de théorie, sans JUCE : cmake --build build --target engine_test
enable_testing()
add_executable(engine_test tests/engine_test.cpp Source/ChordEngine.cpp)
add_test(NAME engine COMMAND engine_test)
