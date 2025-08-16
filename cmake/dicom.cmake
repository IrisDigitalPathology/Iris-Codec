include(FindPkgConfig)
include(ExternalProject)

set (LIBDICOM_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/libdicom)
if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (LIBDICOM_LIB_NAME dicom${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (LIBDICOM_LIB_NAME libdicom${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find libdicom locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "libdicom dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for libdicom...")
    pkg_check_modules(DICOM libdicom)
    if (DICOM_FOUND)
        message(STATUS "libdicom FOUND: version ${DICOM_VERSION}")
        # Use LINK_LIBRARIES instead of LIBRARIES for full paths
        set(LIBDICOM_LIBRARY ${DICOM_LINK_LIBRARIES})
        set(LIBDICOM_INCLUDE ${DICOM_INCLUDE_DIRS})
        # If LINK_LIBRARIES is empty, try to find the library manually
        if (NOT LIBDICOM_LIBRARY)
            find_library(LIBDICOM_LIBRARY dicom PATHS ${DICOM_LIBRARY_DIRS})
        endif()
    else()
        # Fallback search without pkg-config
        find_library(LIBDICOM_LIBRARY dicom)
        find_path(LIBDICOM_INCLUDE dicom/dicom.h)
        if (LIBDICOM_LIBRARY)
            message(STATUS "libdicom found without pkg-config: ${LIBDICOM_LIBRARY}")
        endif()
    endif()
else ()
    if (NOT LIBDICOM_LIBRARY OR NOT LIBDICOM_INCLUDE)
        find_file (LIBDICOM_LIBRARY ${LIBDICOM_LIB_NAME} ${LIBDICOM_INSTALL_DIR}/lib)
        find_path (LIBDICOM_INCLUDE dicom/dicom.h HINTS ${LIBDICOM_INSTALL_DIR})
        if (LIBDICOM_LIBRARY)
            MESSAGE(STATUS "libdicom found from previous build attempt: ${LIBDICOM_LIBRARY}")
        endif()
    endif()
endif()

if (NOT LIBDICOM_LIBRARY OR NOT LIBDICOM_INCLUDE)
    MESSAGE(STATUS "libdicom NOT FOUND. Set to clone and build during the build process.")
    
    # Check if meson is available
    find_program(MESON_EXECUTABLE meson)
    if (NOT MESON_EXECUTABLE)
        message(FATAL_ERROR "Meson build system not found. Please install meson to build libdicom.")
    endif()
    
    set(LIBDICOM_EXTERNAL_PROJECT_ADD ON)
    set(LIBDICOM_LIBRARY ${LIBDICOM_INSTALL_DIR}/lib/${LIBDICOM_LIB_NAME})
    set(LIBDICOM_INCLUDE ${LIBDICOM_INSTALL_DIR}/include/)
    
    ExternalProject_Add(
        libdicom
        GIT_REPOSITORY https://github.com/ImagingDataCommons/libdicom.git
        GIT_TAG main
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${LIBDICOM_LIBRARY} # Ninja compatibility
        CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup 
            --prefix=${LIBDICOM_INSTALL_DIR}
            --buildtype=release
            --default-library=static
            <SOURCE_DIR> <BINARY_DIR>
        BUILD_COMMAND ${MESON_EXECUTABLE} compile -C <BINARY_DIR>
        INSTALL_COMMAND ${MESON_EXECUTABLE} install -C <BINARY_DIR>
    )
endif()

include_directories(
    ${LIBDICOM_INCLUDE}
)