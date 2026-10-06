cmake_minimum_required(VERSION 3.22)
project(GenericNeuralAudioPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE CMake integration
find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# RTNeural header-only library
include(FetchContent)
FetchContent_Declare(
    RTNeural
    GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
    GIT_TAG v1.3.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(RTNeural)

juce_add_plugin(GenericNeuralAudioPlugin
    COMPANY_NAME "AnalogTensor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE ATen
    PLUGIN_CODE GnAp
    FORMATS VST3 Standalone
    PRODUCT_NAME "Neural Audio Plugin"
)

target_sources(GenericNeuralAudioPlugin
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/NeuralModelEngine.h
        Source/EmbeddedModel.h
)

target_compile_definitions(GenericNeuralAudioPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        RTNEURAL_DEFAULT_MATHS=1
)

target_link_libraries(GenericNeuralAudioPlugin
    PRIVATE
        RTNeural
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
