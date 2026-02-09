
set (PLUGIN_TARGET "")

if (IEM_FORMATS)
    juce_add_plugin (PluginTemplate
        PLUGIN_MANUFACTURER_CODE IEMA
        PLUGIN_CODE PlTe
        ICON_BIG "../resources/Standalone/IEMPluginSuiteSmall.png"
        ICON_SMALL "../resources/Standalone/IEMPluginSuiteSmall.png"
        COMPANY_NAME "IEM"
        PRODUCT_NAME "PluginTemplate"
        FORMATS ${IEM_FORMATS}
        VERSION ${PluginTemplate_VERSION}  # Add version number in Versions.cmake in root directory
        LV2URI "http://plugins.iem.at/PluginTemplate"
        VST2_CATEGORY kPlugSurroundFx kPlugCategSpacializer
        VST3_CATEGORIES Spatial Surround  # Add more categories if applicable
        AAX_CATEGORY AAX_ePlugInCategory_SoundField)

    list (APPEND PLUGIN_TARGET PluginTemplate)
endif()

foreach (TARGET IN LISTS PLUGIN_TARGET)
    juce_generate_juce_header (${TARGET})


    target_sources (${TARGET} PRIVATE
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h

        ../resources/OSC/OSCInputStream.h
        ../resources/OSC/OSCParameterInterface.cpp
        ../resources/OSC/OSCParameterInterface.h
        ../resources/OSC/OSCStatus.cpp
        ../resources/OSC/OSCStatus.h
        ../resources/OSC/OSCUtilities.h

        ../resources/efficientSHvanilla.cpp
        )

    target_compile_definitions (${TARGET} PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_USE_FLAC=0
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_include_directories (${TARGET} PRIVATE Source)

    target_link_libraries (${TARGET} PRIVATE
        LAF_fonts
        IEM_logo
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_osc
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)
    endforeach()
    