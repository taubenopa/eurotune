cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
endif()

project(EuroTune VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Bild wird fest ins Plugin eingebettet (BinaryData::figure_png)
juce_add_binary_data(EuroTuneAssets SOURCES Assets/figure.png)
set_target_properties(EuroTuneAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(EuroTune
    COMPANY_NAME "EuroJunkie"
    PLUGIN_MANUFACTURER_CODE Ejnk
    PLUGIN_CODE Etun
    FORMATS VST3 Standalone
    PRODUCT_NAME "EuroTune"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx "Pitch Shift"
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(EuroTune)

target_sources(EuroTune PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(EuroTune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(EuroTune
    PRIVATE
        EuroTuneAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
