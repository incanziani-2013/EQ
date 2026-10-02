cmake_minimum_required(VERSION 3.22)
project(AQEQ VERSION 1.0.0)

# Si pasas JUCE_DIR, usa tu JUCE local, por ejemplo:
#   cmake -S . -B build -DJUCE_DIR="C:/JUCE"
# Si no, descarga JUCE solo (usado por GitHub Actions).
if(DEFINED JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" JUCE)
    set(AQEQ_COPY_AFTER_BUILD TRUE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(AQEQ_COPY_AFTER_BUILD FALSE)
endif()

juce_add_plugin(AQEQ
    COMPANY_NAME "AQWAX"
    BUNDLE_ID "com.aqwax.eq"
    PLUGIN_MANUFACTURER_CODE AqWx
    PLUGIN_CODE Aqe1
    FORMATS VST3
    PRODUCT_NAME "AQEQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD ${AQEQ_COPY_AFTER_BUILD}
)

target_sources(AQEQ PRIVATE
    Source/EQDsp.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_features(AQEQ PRIVATE cxx_std_17)

target_link_libraries(AQEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(AQEQ)
