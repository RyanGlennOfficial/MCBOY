cmake_minimum_required(VERSION 3.22)
project(McBoy VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
if(BUILD_TESTING)
 add_subdirectory(Tests)
endif()
option(MCBOY_BUILD_PLUGIN "Build JUCE VST3 and Standalone targets" ON)
option(MCBOY_FETCH_JUCE "Fetch JUCE from GitHub (requires internet)" OFF)
if(MCBOY_BUILD_PLUGIN)
 if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
   add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
 else()
   find_package(JUCE CONFIG QUIET)
   if(NOT TARGET juce::juce_audio_processors AND MCBOY_FETCH_JUCE)
     include(FetchContent)
     FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.16-security)
     FetchContent_MakeAvailable(JUCE)
   endif()
 endif()
 if(NOT TARGET juce::juce_audio_processors)
   message(FATAL_ERROR "JUCE not found. Configure with -DJUCE_DIR=/path/to/JUCE or -DMCBOY_FETCH_JUCE=ON. To run DSP tests alone use -DMCBOY_BUILD_PLUGIN=OFF")
 endif()
 juce_add_plugin(McBoyPlugin
  COMPANY_NAME "RG Audio"
  BUNDLE_ID "audio.rgaudio.mcboy"
  # Legacy manufacturer four-character ID retained to preserve existing VST3
  # component/controller class IDs and previously saved host sessions.
  PLUGIN_MANUFACTURER_CODE HWMG
  PLUGIN_CODE MCBY
  FORMATS VST3 Standalone
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PRODUCT_NAME "McBoy"
  VST3_CATEGORIES Fx
 )
 target_sources(McBoyPlugin PRIVATE
   Source/PluginProcessor.cpp
   Source/PluginEditor.cpp
 )
 target_include_directories(McBoyPlugin PRIVATE Source)
 target_compile_definitions(McBoyPlugin PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
 target_link_libraries(McBoyPlugin PRIVATE
  juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

 # Compiles the same JUCE processor/editor code without a DAW to exercise
 # parameter automation and state/preset round-trips during macOS builds.
 if(BUILD_TESTING)
  juce_add_console_app(mcboy_juce_smoke PRODUCT_NAME "McBoy JUCE Smoke Test")
  target_sources(mcboy_juce_smoke PRIVATE
   Tests/JuceProcessorSmoke.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_include_directories(mcboy_juce_smoke PRIVATE Source)
  target_compile_definitions(mcboy_juce_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(mcboy_juce_smoke PRIVATE
   juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
   juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  add_test(NAME JuceProcessorSmoke COMMAND mcboy_juce_smoke)
 endif()
endif()
