cmake_minimum_required(VERSION 3.22)
project(CleanMixEQ VERSION 1.0.0)

# Install JUCE 8 and make its CMake package available to CMake.
find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(CleanMixEQ
    COMPANY_NAME "CleanMix Audio"
    BUNDLE_ID com.cleanmixaudio.cleanmitsiaq
    PLUGIN_MANUFACTURER_CODE Clmx
    PLUGIN_CODE Cln1
    FORMATS VST3
    PRODUCT_NAME "CleanMix EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(CleanMixEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_features(CleanMixEQ PRIVATE cxx_std_17)
target_link_libraries(CleanMixEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
