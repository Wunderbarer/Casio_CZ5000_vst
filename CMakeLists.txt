cmake_minimum_required(VERSION 3.22)
project(CZ5000VST VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# JUCE wird automatisch via CMake FetchContent heruntergeladen.
# Internetzugang beim Konfigurieren (cmake -B build) ist dafuer noetig.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CZ5000VST
    COMPANY_NAME             "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yman
    PLUGIN_CODE              Cz50
    FORMATS                  VST3 Standalone
    PRODUCT_NAME             "CZ-5000 Virtual"
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        TRUE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD  TRUE
    VST3_CATEGORIES          Instrument Synth
)

juce_generate_juce_header(CZ5000VST)

target_sources(CZ5000VST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/DSP/CZVoice.cpp
    Source/MIDI/CZSysEx.cpp
    Source/GUI/HardwarePanel.cpp
)

target_include_directories(CZ5000VST PRIVATE Source)

target_compile_definitions(CZ5000VST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CZ5000VST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
