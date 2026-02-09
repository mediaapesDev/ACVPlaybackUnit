cmake_minimum_required (VERSION 3.22 FATAL_ERROR)
include(CheckIncludeFiles)

set (PLUGINS_TO_BUILD
	DirectionalSampler
    )

# The following options let you select the plug-ins formats you want to build.
# You can either change them here or use the command line to switch them on/off.
option (IEM_USE_SYSTEM_JUCE "Use JUCE found via CMake rather than the bundled one." OFF)
option (IEM_BUILD_VST2 "Build VST2 version of the plug-ins." OFF)
option (IEM_BUILD_VST3 "Build VST3 version of the plug-ins." OFF)
option (IEM_BUILD_LV2 "Build LV2 version of the plug-ins." OFF)
option (IEM_BUILD_AAX "Build AAX version of the plug-ins." OFF)
option (IEM_BUILD_STANDALONE "Build standalones of the plug-ins." ON)
option (IEM_STANDALONE_JACK_SUPPORT "Build standalones with JACK support." ON)

include (Versions.cmake)

set (IEM_POST_BUILD_INSTALL "USER" CACHE STRING "Define if plug-ins are installed for USER, SYSTEM or if you want NO installation")
set (prefix "/usr/local" CACHE STRING "Path prefix for custom installation")

set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# Actual project configuration
project (ACVPlaybackUnit VERSION ${IEMPluginSuite_VERISON} LANGUAGES C CXX)

set_property (GLOBAL PROPERTY USE_FOLDERS YES)

# set copy paths for built plug-ins
if (IEM_POST_BUILD_INSTALL STREQUAL "NO")
    set_directory_properties (PROPERTIES JUCE_COPY_PLUGIN_AFTER_BUILD OFF)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_directory_properties (PROPERTIES JUCE_COPY_PLUGIN_AFTER_BUILD ON)

    if (IEM_POST_BUILD_INSTALL STREQUAL "SYSTEM")
        set (prefix "/Library/Audio/Plug-Ins")
    else()  # Defaulting to USER
        set (prefix "$ENV{HOME}/Library/Audio/Plug-Ins")
    endif()

    set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "${prefix}/VST/IEM")
    set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "${prefix}/VST3/IEM")
    set_directory_properties (PROPERTIES JUCE_LV2_COPY_DIR "${prefix}/LV2")
    set_directory_properties (PROPERTIES JUCE_AAX_COPY_DIR "/Library/Application Support/Avid/Audio/Plug-Ins/IEM")
    # TODO: I've found no references for a local AAX installation on macOS, so I need to check this

elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_directory_properties (PROPERTIES JUCE_COPY_PLUGIN_AFTER_BUILD ON)

    if(MSVC)
        message (STATUS "MSVC detected, setting runtime library to Multi-threaded (/MT)")
        add_compile_options($<$<CONFIG:Release>:/MT> # Runtime library: Multi-threaded
                            $<$<CONFIG:RelWithDebInfo>:/MT> # Runtime library: Multi-threaded                           
                            $<$<CONFIG:Debug>:/MTd> # Runtime library: Multi-threaded Debug
                            )
    endif()

    if ((IEM_POST_BUILD_INSTALL STREQUAL "USER") AND (prefix STREQUAL "/usr/local"))
        if (IEM_BUILD_VST3 OR IEM_BUILD_VST2 OR IEM_BUILD_AAX)
            message (WARNING "VST, VST3 and AAX plug-ins are installed for all users by default as there is no officially supported consistent directory for local installation. Using SYSTEM for those. You can provide a costum prefix to change this behaviour.")
            set (IEM_POST_BUILD_INSTALL "SYSTEM")
        endif()

        set_directory_properties(PROPERTIES JUCE_LV2_COPY_DIR "$ENV{APPDATA}/LV2")
    endif()

    if (IEM_POST_BUILD_INSTALL STREQUAL "USER")
        set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "${prefix}/VST")
        set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "${prefix}/VST3")
        set_directory_properties(PROPERTIES JUCE_LV2_COPY_DIR "${prefix}/LV2")
        set_directory_properties(PROPERTIES JUCE_AAX_COPY_DIR "${prefix}/AAX")

    else()  # Defaulting to SYSTEM as this is the only valid path on Windows for VST and VST3
        set_directory_properties(PROPERTIES JUCE_LV2_COPY_DIR "$ENV{CommonProgramFiles}/LV2")

        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "$ENV{ProgramW6432}/Steinberg/VSTplugins/IEM")
            set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "$ENV{CommonProgramFiles}/VST3/IEM")
            set_directory_properties (PROPERTIES JUCE_AAX_COPY_DIR "$ENV{CommonProgramFiles}/Avid/Audio/Plug-Ins/IEM")
        else()
            set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "$ENV{ProgramFiles(x86)}/Steinberg/Vstplugins/IEM")
            set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "$ENV{CommonProgramFiles(x86)}/VST3/IEM")
        endif()

    endif()

elseif ((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME MATCHES ".*BSD"))
    set_directory_properties (PROPERTIES JUCE_COPY_PLUGIN_AFTER_BUILD ON)

    if (IEM_POST_BUILD_INSTALL STREQUAL "SYSTEM")
        set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "${prefix}/lib/vst/IEM")
        set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "${prefix}/lib/vst3/IEM")
        set_directory_properties (PROPERTIES JUCE_LV2_COPY_DIR "${prefix}/lib/lv2")
    else()  # Defaulting to USER
        set_directory_properties (PROPERTIES JUCE_VST_COPY_DIR "$ENV{HOME}/.vst/IEM")
        set_directory_properties (PROPERTIES JUCE_VST3_COPY_DIR "$ENV{HOME}/.vst3/IEM")
        set_directory_properties (PROPERTIES JUCE_LV2_COPY_DIR "$ENV{HOME}/.lv2")
    endif()
endif()

# add JUCE dependency
if (IEM_USE_SYSTEM_JUCE)
    set (CMAKE_WARN_DEPRECATED TRUE)
    message (DEPRECATION "Using a system JUCE installation may be deprecated in future releases. Please use the fetched version instead.")
    find_package (JUCE CONFIG REQUIRED)
else()
    add_subdirectory (JUCE)
endif()

# formats
set (IEM_FORMATS "")

if (IEM_BUILD_VST2)
    SET (VST2SDKPATH "${CMAKE_CURRENT_SOURCE_DIR}/src/VST_SDK/VST2_SDK/" CACHE STRING "Path to the VST2 SDK")

    # checking VST2 SDK
    if (VST2SDKPATH STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/src/VST_SDK/VST2_SDK/")
        set(CMAKE_REQUIRED_INCLUDES "${VST2SDKPATH}")
        include(CheckIncludeFiles)
        CHECK_INCLUDE_FILES(pluginterfaces/vst2.x/aeffectx.h HAVE_VST2_HEADERS)

        IF (NOT HAVE_VST2_HEADERS)
            message (FATAL_ERROR "You have to specify the VST2SDKPATH variable with the path to the VST2 SDK if you want to build VST2 versions.")
        endif()
    endif()
    juce_set_vst2_sdk_path (${VST2SDKPATH})
    list (APPEND IEM_FORMATS VST)
    message ("-- IEM: Building VST2 versions")
endif()

if (IEM_BUILD_VST3)
    message ("-- IEM: Building VST3 versions")
    list (APPEND IEM_FORMATS VST3)
endif()

if (IEM_BUILD_LV2)
    message ("-- IEM: Building LV2 versions")
    list (APPEND IEM_FORMATS LV2)
endif()

if (IEM_BUILD_AAX)
    message ("-- IEM: Building AAX versions")
    list (APPEND IEM_FORMATS AAX)
endif()

if (IEM_BUILD_STANDALONE)
    message ("-- IEM: Building Standalone versions")
    list (APPEND IEM_FORMATS Standalone)
endif()

include_directories (resources/ /usr/local/include)
add_compile_definitions (DONT_SET_USING_JUCE_NAMESPACE=1
                         JUCE_MODAL_LOOPS_PERMITTED=1)

juce_add_binary_data (LAF_fonts SOURCES
    resources/lookAndFeel/Roboto-Bold.ttf
    resources/lookAndFeel/Roboto-Light.ttf
    resources/lookAndFeel/Roboto-Medium.ttf
    resources/lookAndFeel/Roboto-Regular.ttf)
    
juce_add_binary_data (IEM_logo 
    HEADER_NAME "IEMLogo.h"
    NAMESPACE "IEMLogo"
    SOURCES
    resources/Standalone/IEMPluginSuiteSmall.png)

foreach (subproject IN LISTS PLUGINS_TO_BUILD)
    add_subdirectory (${subproject})
endforeach()


if (DirectionalSampler IN_LIST PLUGINS_TO_BUILD AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if (CMAKE_SYSTEM_NAME STREQUAL "Windows") # build fftw3f on windows
        set (BUILD_SHARED_LIBS OFF)
        set (BUILD_TESTS OFF)
        set (ENABLE_FLOAT ON)
        set (ENABLE_SSE ON)

        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        add_subdirectory (fftw)
        set_target_properties ("fftw3f" PROPERTIES POSITION_INDEPENDET_CODE ON)
    endif()

    if (IEM_FORMATS)
        target_compile_definitions ("DirectionalSampler" PUBLIC JUCE_DSP_USE_STATIC_FFTW=1)
        target_link_libraries ("DirectionalSampler" PRIVATE fftw3f)
    endif()
endif()



if (IEM_BUILD_STANDALONE)
    foreach (subproject IN LISTS PLUGINS_TO_BUILD)
        target_sources(${subproject} PRIVATE
            resources/Standalone/StandaloneApp.cpp
            resources/Standalone/MyStandaloneFilterWindow.h
            resources/Standalone/IEM_JackAudio.h
            resources/Standalone/IEM_AudioDeviceSelectorComponent.cpp
            resources/Standalone/IEM_AudioDeviceSelectorComponent.h)
    endforeach()

    if (NOT IEM_STANDALONE_JACK_SUPPORT)
        foreach (subproject IN LISTS PLUGINS_TO_BUILD)
            target_compile_definitions(${subproject} PUBLIC DONT_BUILD_WITH_JACK_SUPPORT=1)
        endforeach()
    endif()
endif()
