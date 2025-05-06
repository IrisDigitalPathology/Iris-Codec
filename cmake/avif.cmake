set (AVIF_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/avif)
if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (AVIF_LIB_NAME avif${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (AVIF_LIB_NAME libavif${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find Avif locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "Avif dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for Avif...")
    find_package(libavif)
    if (libavif_FOUND)
        message(STATUS "Avif FOUND: version ${libavif_VERSION}")
        set(AVIF_LIBRARY avif)
    endif()
    find_path(AVIF_INCLUDE avif/avif.h)
else ()
    if (NOT AVIF_LIBRARY OR NOT AVIF_INCLUDE)
        find_file (AVIF_LIBRARY ${AVIF_LIB_NAME} ${AVIF_INSTALL_DIR}/lib)
        find_path (AVIF_INCLUDE avif/avif.h HINTS ${AVIF_INSTALL_DIR})
        if (AVIF_LIBRARY)
            MESSAGE(STATUS "Avif found from previous build attempt: ${AVIF_LIBRARY}")
        endif()
    endif()
endif()

if (NOT AVIF_LIBRARY OR NOT AVIF_INCLUDE)
    message(STATUS "AVIF NOT FOUND. Set to clone and build during the build process.")
    set(AVIF_EXTERNAL_PROJECT_ADD ON)
    set(AVIF_LIBRARY ${AVIF_INSTALL_DIR}/lib/${AVIF_LIB_NAME})
    set(AVIF_INCLUDE ${AVIF_INSTALL_DIR}/include/)
    ExternalProject_Add(
        Avif
        GIT_REPOSITORY https://github.com/AOMediaCodec/libavif.git
        GIT_TAG "a28899a" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${AVIF_LIBRARY} # Ninja compatability
        CMAKE_ARGS 
            -D CMAKE_INSTALL_PREFIX:PATH=${AVIF_INSTALL_DIR}
            -D BUILD_SHARED_LIBS=OFF 
            -D ENABLE_TOOLS=OFF
            -D ENABLE_EXAMPLES=OFF
            -D AVIF_LIBYUV=LOCAL
            -D AVIF_CODEC_AOM=LOCAL 
            -D CMAKE_POSITION_INDEPENDENT_CODE=ON
            -D CMAKE_POLICY_VERSION_MINIMUM=3.5
            -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -D CMAKE_INSTALL_LIBDIR=${AVIF_INSTALL_DIR}/lib/
    )
    
endif()
include_directories(
    ${AVIF_INCLUDE}
)