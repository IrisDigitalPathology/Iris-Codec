# 2025 Copyright Ryan Landvater
# Created 2025-08-09

cmake_minimum_required(VERSION 3.27)
include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# if(NOT DEFINED EMSCRIPTEN)
#     message(FATAL_ERROR "This is a JavaScript build ONLY. It must be built with Emscripten.")
# endif()

function(get_codec_version)
    set(codec_priv_header "${CMAKE_CURRENT_SOURCE_DIR}/../src/IrisCodecPriv.hpp")
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

    set(IRIS_CODEC_VERSION_STRING "${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}" PARENT_SCOPE)
endfunction()

get_codec_version()
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
message(STATUS "BUILDING IRIS CODEC JS COMMUNITY MODULE VERSION v" ${IRIS_CODEC_VERSION_STRING})
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")

PROJECT (
    IrisCodecJS
    LANGUAGES CXX
    VERSION ${IRIS_CODEC_VERSION_STRING}
)

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
# NOTE: The following IFE_Export definition
# is IFE_EXPORT= to blank out import/export
# declarations. This is important as we use
# intermediate objects rather than link to
# the Iris File Extension Library (MSVC)
add_compile_definitions(IFE_EXPORT=)
FetchContent_Declare (
    IrisFileExtension
    GIT_REPOSITORY https://github.com/IrisDigitalPathology/Iris-File-Extension.git
    GIT_TAG "origin/main"
    GIT_SHALLOW ON
    FETCHCONTENT_UPDATES_DISCONNECTED ON
    FETCHCONTENT_QUIET ON
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
FetchContent_MakeAvailable(
    IrisHeaders
    IrisFileExtension
)
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Iris Codec Universal Object Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set(IFE_SOURCE_DIR ${irisfileextension_SOURCE_DIR}/src)
add_executable(
    iris-codec
    $<TARGET_OBJECTS:IrisFileExtensionLib>
    IrisCodecJavascript.cpp
)
target_include_directories (
    iris-codec PRIVATE 
    ${irisheaders_SOURCE_DIR}/include
    ${irisheaders_SOURCE_DIR}/priv
    ${IFE_SOURCE_DIR} 
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(
        iris-codec PRIVATE
        -sFETCH
        -sASYNCIFY
        --emit-tsd iris-codec.d.ts
        --bind
        -O0
        -gsource-map 
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=2
        -sSAFE_HEAP=1
        -sSTACK_OVERFLOW_CHECK=2
    )
else () 
    target_link_options(
        iris-codec PRIVATE
        -sFETCH
        -sASYNCIFY
        --emit-tsd iris-codec.d.ts
        --bind
        -O3
    )
endif()
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/iris-codec.js"
        "${CMAKE_CURRENT_BINARY_DIR}/iris-codec.wasm"
        "${CMAKE_CURRENT_BINARY_DIR}/iris-codec.d.ts"
    DESTINATION
        "."
)