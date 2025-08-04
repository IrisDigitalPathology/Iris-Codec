set (hwy_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/hwy)
if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (hwy_LIB_NAME avif${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (hwy_LIB_NAME libavif${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find Avif locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "Google Highways dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for google/hwy...")
    find_package(hwy)
    if (hwy_FOUND)
        message(STATUS "hwy FOUND: version ${hwy_VERSION}")
        set(hwy_LIBRARY hwy::hwy)
    endif()
    find_path(hwy_INCLUDE hwy/highway.h)
else ()
    if (NOT hwy_LIBRARY OR NOT hwy_INCLUDE)
        find_file (hwy_LIBRARY ${hwy_LIB_NAME} ${hwy_INSTALL_DIR}/lib)
        find_path (AVIF_INCLUDE hwy/highwyas.h HINTS ${hwy_INSTALL_DIR})
        if (hwy_LIBRARY)
            MESSAGE(STATUS "Google highwyas found from previous build attempt: ${hwy_LIBRARY}")
        endif()
    endif()
endif()

if (NOT hwy_LIBRARY OR NOT hwy_INCLUDE)
    message(STATUS "Google Highways NOT FOUND. Set to clone and build during the build process.")
    set(hwy_EXTERNAL_PROJECT_ADD ON)
    set(hwy_LIBRARY ${hwy_INSTALL_DIR}/lib/${hwy_LIB_NAME})
    set(hwy_INCLUDE ${hwy_INSTALL_DIR}/include/)
    ExternalProject_Add (
        hwy
        GIT_REPOSITORY https://github.com/google/highway.git
        GIT_TAG "1.2.0"
        GIT_SHALLOW ON
        FETCHCONTENT_UPDATES_DISCONNECTED ON
        FETCHCONTENT_QUIET ON
        BUILD_BYPRODUCTS ${hwy_LIBRARY} # Ninja compatability
        CMAKE_ARGS 
            -D CMAKE_INSTALL_PREFIX:PATH=""
            -D BUILD_SHARED_LIBS=OFF 
            -D BUILD_TESTING=OFF
            -D HWY_ENABLE_EXAMPLES=OFF
            -D HWY_ENABLE_CONTRIB=OFF
            -D CMAKE_POSITION_INDEPENDENT_CODE=ON
            -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -D CMAKE_INSTALL_LIBDIR=${hwy_INSTALL_DIR}/lib/
    )
endif()
include_directories(
    ${hwy_INCLUDE}
)