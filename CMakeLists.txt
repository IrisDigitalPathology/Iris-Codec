# 2025 Copyright Ryan Landvater
# Created 3/3/2025

cmake_minimum_required(VERSION 3.27)
include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

function(get_codec_version)
    set(codec_priv_header "${CMAKE_CURRENT_SOURCE_DIR}/src/IrisCodecPriv.hpp")
    if (NOT EXISTS ${codec_priv_header})
        message(FATAL_ERROR "Could NOT locate Iris Codec Private Header (IrisCodecPriv.hpp)")
    endif()

    file(READ ${codec_priv_header} header)

    if (header MATCHES "#define CODEC_MAJOR_VERSION[ ]*([0-9]+)")
        set(MAJOR_VERSION "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec major version")
    endif()

    if (header MATCHES "#define CODEC_MINOR_VERSION[ ]*([0-9]+)")
        set(MINOR_VERSION "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec minor version")
    endif()

    if (header MATCHES "#define CODEC_BUILD_NUMBER[ ]*([0-9]+)")
        set(BUILD_NUMBER "${CMAKE_MATCH_1}")
    else () 
        message(FATAL_ERROR "Couldn't parse the Iris Codec build iteration")
    endif()

    set(VERSION_STRING "${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}" PARENT_SCOPE)
endfunction()

get_codec_version()
message(STATUS "BUILDING IRIS CODEC COMMUNITY MODULE VERSION v" ${VERSION_STRING})
if (IRIS_BUILD_DEPENDENCIES)
    message(STATUS "Iris Codec is configured to build all dependencies and statically link into a single self-contained binary.")
endif()
if (IRIS_BUILD_PYTHON)
    message(STATUS "Iris Codec is configured to build Python Module")
endif()

FetchContent_Declare (
    VulkanHeaders
    URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/vulkan-sdk-1.4.309.0.tar.gz
)
FetchContent_Declare (
    VkMemoryAllocator
    URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.2.1.tar.gz
    URL_HASH SHA256=5e7749504cb802427ffb7bec38a0b6a15db46ae253f00560acb3e624d9fe695c
)
set(GLM_BUILD_LIBRARY OFF)
set(GLM_BUILD_INSTALL OFF)
set(GLM_BUILD_TESTS   OFF)
FetchContent_Declare (
    GLM
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
    URL_HASH SHA256=9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c
)
FetchContent_MakeAvailable(
    VulkanHeaders 
    VkMemoryAllocator
    GLM
)

PROJECT (
    IrisCodec
    LANGUAGES CXX
    VERSION ${VERSION_STRING}
)

option(IRIS_BUILD_SHARED "Build IrisCodec Shared Library" ON)
option(IRIS_BUILD_STATIC "Build IrisCodec Static Library" ON)
option(IRIS_BUILD_ENCODER "Build the IrisCodec Encoder executable" ON)
option(IRIS_BUILD_PYTHON "Build IrisCodec Python modules" OFF)
option(IRIS_BUILD_DEPENDENCIES "Build all dependencies and statically link into self-contained binary" OFF)
option(IRIS_USE_OPENSLIDE "Use openslide in the encoder (currently not supported on Windows Arm64)" ON)


FetchContent_Declare (
    IrisHeaders
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-Headers.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
)
# Do not export the IFE API. 
set(IFE_BUILD_SHARED OFF)
set(IFE_BUILD_STATIC OFF)
add_compile_definitions(IFE_EXPORT=)
FetchContent_Declare (
    IrisFileExtension
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-File-Extension.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
FetchContent_MakeAvailable(
    IrisHeaders
    IrisFileExtension
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")

# Include external projects, if needed
include(./cmake/turbo-jpeg.cmake)
include(./cmake/avif.cmake)
include(./cmake/png.cmake)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Universal Object Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set(IFE_SOURCE_DIR ${irisfileextension_SOURCE_DIR}/src)
set(CODEC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set (
    IrisCodecSources
    ${CODEC_SOURCE_DIR}/IrisCodecContext.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecFile.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecCache.cpp
    ${CODEC_SOURCE_DIR}/IrisCodecSlide.cpp
)
set (
    IrisCodecEncoderSources
    ${CODEC_SOURCE_DIR}/IrisCodecEncoder.cpp
)
set (
    IrisCodecInclude
    ${CODEC_SOURCE_DIR}
    ${IFE_SOURCE_DIR}
    ${glm_SOURCE_DIR}
    ${irisheaders_SOURCE_DIR}/priv
    ${vkmemoryallocator_SOURCE_DIR}/include
    ${vulkanheaders_SOURCE_DIR}/include
)
set (
    IrisCodecDependencies
    ${TURBOJPEG_LIBRARY}
    ${AVIF_LIBRARY}
    ${PNG_LIBRARY}
)
if (IRIS_BUILD_ENCODER)
    if (IRIS_USE_OPENSLIDE)
        add_compile_definitions(IRIS_INCLUDE_OPENSLIDE=1)
        message(STATUS "BUILDING IRIS CODEC ENCODER: Encoding from vendor files requires Openslide")
        include(./cmake/openslide.cmake)
        message(STATUS "Found OPENSLIDE_LIB: ${OPENSLIDE_LIB}")
    else ()
        add_compile_definitions(IRIS_INCLUDE_OPENSLIDE=0)
    endif()
endif(IRIS_BUILD_ENCODER)

add_library(
    IrisCodecLib OBJECT
    ${IrisCodecSources}
)
target_include_directories (
    IrisCodecLib PRIVATE
    ${IrisCodecInclude}
)
target_compile_definitions (
    IrisCodecLib
    PRIVATE IRIS_EXPORT_API=true 
)
set_target_properties(
    IrisCodecLib 
    PROPERTIES CXX_VISIBILITY_PRESET hidden
)
target_link_libraries (
    IrisCodecLib
    PUBLIC IrisHeaders
)
if (TURBOJPEG_EXTERNAL_PROJECT_ADD) 
    add_dependencies(IrisCodecLib TurboJpeg)
endif()
if (AVIF_EXTERNAL_PROJECT_ADD) 
    add_dependencies(IrisCodecLib Avif)
endif()
if (PNG_EXTERNAL_PROJECT_ADD)
    add_dependencies(IrisCodecLib Png)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Targets (ie what we are installing)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set (
    IrisCodecTargets
    IrisCodecLib
)
if (IRIS_BUILD_SHARED)
    add_library(
        IrisCodec SHARED 
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisCodecLib>
    )
    target_link_libraries(
        IrisCodec
        PRIVATE ${IrisCodecDependencies}
    )
    set(IrisCodecTargets ${IrisCodecTargets} IrisCodec)
endif()

if (IRIS_BUILD_STATIC)
    add_library(
        IrisCodecStatic STATIC 
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisCodecLib>
    )
    target_link_libraries(
        IrisCodecStatic
        PRIVATE ${IrisCodecDependencies}
    )
    set(IrisCodecTargets ${IrisCodecTargets} IrisCodecStatic)
endif(IRIS_BUILD_STATIC)

if (IRIS_BUILD_ENCODER AND (IRIS_BUILD_SHARED OR IRIS_BUILD_STATIC))
    add_executable (
        IrisCodecEncoder
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisCodecLib>
        ${IrisCodecEncoderSources}
        ${CODEC_SOURCE_DIR}/EncoderMain.cpp
    )
    target_include_directories(
        IrisCodecEncoder 
        PRIVATE ${IrisCodecInclude}
        PRIVATE ${OPENSLIDE_DIR}
    )
    target_compile_definitions (
        IrisCodecEncoder
        PRIVATE IRIS_EXPORT_API=true 
    )
    target_link_libraries (
        IrisCodecEncoder
        PRIVATE IrisHeaders
        PRIVATE ${IrisCodecDependencies}
        PRIVATE ${OPENSLIDE_LIB}
    )
    set(IrisCodecTargets ${IrisCodecTargets} IrisCodecEncoder)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Installation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if (${PROJECT_IS_TOP_LEVEL} AND (IRIS_BUILD_SHARED OR IRIS_BUILD_STATIC))
    install(
        TARGETS IrisHeaders
        EXPORT IrisHeadersConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisHeadersConfig
        FILE IrisHeadersConfig.cmake
        NAMESPACE "Iris::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
    install(
        TARGETS ${IrisCodecTargets}
        EXPORT IrisCodecConfig
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Iris
    )
    install(
        EXPORT IrisCodecConfig
        FILE IrisCodecConfig.cmake
        NAMESPACE "IrisCodec::"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Iris
    )
endif(${PROJECT_IS_TOP_LEVEL} AND (IRIS_BUILD_SHARED OR IRIS_BUILD_STATIC))

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Python support (including installation)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if(IRIS_BUILD_PYTHON)
    set(PYBIND11_FINDPYTHON ON)
    FetchContent_Declare (
        Pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG "a2e59f0" #"origin/main"
        GIT_SHALLOW ON
        FETCHCONTENT_QUIET ON
    )
    FetchContent_MakeAvailable (
        Pybind11
    )
    if (NOT CMAKE_INSTALL_PYTHON_LIBDIR)
        set(CMAKE_INSTALL_PYTHON_LIBDIR
        "${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
    endif ()
    string (REPLACE "\\" "/" IRIS_INSTALL_PYTHON_LIBDIR ${CMAKE_INSTALL_PYTHON_LIBDIR})
    message(STATUS "Installing python modules at: ${IRIS_INSTALL_PYTHON_LIBDIR}")
    pybind11_add_module(
        IrisCodecPython
        $<TARGET_OBJECTS:IrisFileExtensionLib>
        $<TARGET_OBJECTS:IrisCodecLib>
        ${PROJECT_SOURCE_DIR}/python/IrisCodecPython.cpp
    )
    target_include_directories (
        IrisCodecPython 
        PRIVATE ${IrisCodecInclude}
    )
    set_property(
        TARGET IrisCodecPython
        PROPERTY OUTPUT_NAME Iris
    )
    target_compile_definitions (
        IrisCodecPython
        PRIVATE IRIS_EXPORT_API=true 
    )
    target_link_libraries (
        IrisCodecPython
        PRIVATE IrisHeaders
        PRIVATE ${IrisCodecDependencies}
    )
    install(
        TARGETS IrisCodecPython
        DESTINATION ${IRIS_INSTALL_PYTHON_LIBDIR}/Iris
    )
    if (IRIS_BUILD_ENCODER)
        pybind11_add_module(
            IrisCodecEncoderPython
            $<TARGET_OBJECTS:IrisFileExtensionLib>
            $<TARGET_OBJECTS:IrisCodecLib>
            ${IrisCodecEncoderSources}
            ${PROJECT_SOURCE_DIR}/python/IrisEncoderPython.cpp
        )
        target_include_directories (
            IrisCodecEncoderPython 
            PRIVATE ${IrisCodecInclude}
            PRIVATE ${OPENSLIDE_DIR}
        )
        set_property(
            TARGET IrisCodecEncoderPython
            PROPERTY OUTPUT_NAME Encoder
        )
        target_compile_definitions (
            IrisCodecEncoderPython
            PRIVATE IRIS_EXPORT_API=true 
        )
        target_link_libraries (
            IrisCodecEncoderPython
            PRIVATE IrisHeaders
            PRIVATE ${IrisCodecDependencies}
            PRIVATE ${OPENSLIDE_LIB}
        )
        install(
            TARGETS IrisCodecEncoderPython
            DESTINATION ${IRIS_INSTALL_PYTHON_LIBDIR}/Iris/Encoder
        )
    endif()
    
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/python/Iris 
        DESTINATION ${IRIS_INSTALL_PYTHON_LIBDIR}
    )
        
endif()
