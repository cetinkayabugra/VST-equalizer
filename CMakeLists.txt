cmake_minimum_required(VERSION 3.16)
project(VSTEqualizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory (assumes JUCE is in the parent directory)
# You can modify this path if JUCE is installed elsewhere
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
    message(FATAL_ERROR "JUCE framework not found. Please ensure JUCE is installed in the parent directory.")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

# Create plugin target
juce_add_plugin(VSTEqualizer
    PRODUCT_NAME "VST Equalizer"
    COMPANY_NAME "Audio DSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "ADSP"
    PLUGIN_CODE "VEql"
    FORMATS VST3
    VST3_CATEGORIES "EQ"
)

# Add source files
target_sources(VSTEqualizer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/BiQuadFilter.h
    Source/DSP/FilterBank.h
    Source/UI/SpectrumAnalyzer.h
)

# Link JUCE
target_link_libraries(VSTEqualizer
    PRIVATE
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# Set compile options
target_compile_options(VSTEqualizer PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g>
)

# Disable VST2 (we only use VST3)
set_target_properties(VSTEqualizer PROPERTIES
    JUCE_VST2_SDK_PATH ""
)

# Enable AU if on macOS
if(APPLE)
    set_target_properties(VSTEqualizer PROPERTIES
        JUCE_AU_SDK_PATH ""
    )
endif()

# Installation
install(TARGETS VSTEqualizer
    RUNTIME DESTINATION "."
)
