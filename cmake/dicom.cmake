include(FindPkgConfig)
include(ExternalProject)

set(DICOM_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/libdicom)
# Meson produces a '.a' archive regarless of windows vs unix
set(DICOM_LIB_NAME libdicom.a)

# Start by trying to find libdicom locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "libdicom dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for libdicom...")
    pkg_check_modules(DICOM libdicom)
    if (DICOM_FOUND)
        message(STATUS "libdicom FOUND: version ${DICOM_VERSION}")
        set(DICOM_LIBRARY ${DICOM_LINK_LIBRARIES})
        set(DICOM_INCLUDE ${DICOM_INCLUDE_DIRS})
        if (NOT DICOM_LIBRARY)
            find_library(DICOM_LIBRARY dicom PATHS ${DICOM_LIBRARY_DIRS})
        endif()
    endif()
    find_path(DICOM_INCLUDE dicom/dicom.h)
else ()
    if (NOT DICOM_LIBRARY OR NOT DICOM_INCLUDE)
        find_file (DICOM_LIBRARY ${DICOM_LIB_NAME} ${DICOM_INSTALL_DIR}/lib)
        find_path (DICOM_INCLUDE dicom/dicom.h HINTS ${DICOM_INSTALL_DIR}/include)
        if (DICOM_LIBRARY)
            MESSAGE(STATUS "libdicom found from previous build attempt: ${DICOM_LIBRARY}")
        endif()
    endif()
endif()

if (NOT DICOM_LIBRARY OR NOT DICOM_INCLUDE)
    MESSAGE(STATUS "libdicom NOT FOUND. Set to clone and build during the build process.")
    
    find_program(MESON_EXECUTABLE meson)
    if (NOT MESON_EXECUTABLE)
        message(FATAL_ERROR "Meson build system not found. Please install meson to build libdicom.")
    endif()

    set(DICOM_EXTERNAL_PROJECT_ADD ON)
    set(DICOM_LIBRARY ${DICOM_INSTALL_DIR}/lib/${DICOM_LIB_NAME})
    set(DICOM_INCLUDE ${DICOM_INSTALL_DIR}/include/)
    ExternalProject_Add(
        libdicom
        GIT_REPOSITORY https://github.com/ImagingDataCommons/libdicom.git
        GIT_TAG main
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${DICOM_LIBRARY}
        CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup
            --prefix=${DICOM_INSTALL_DIR}
            --buildtype=release
            --default-library=static
            -Dlibdir=lib
            -Db_staticpic=true
            <SOURCE_DIR> <BINARY_DIR>
        BUILD_COMMAND ${MESON_EXECUTABLE} compile -C <BINARY_DIR>
        INSTALL_COMMAND ${MESON_EXECUTABLE} install -C <BINARY_DIR>
    )
endif()

message(STATUS "DICOM_LIBRARY: ${DICOM_LIBRARY}")
message(STATUS "DICOM_INCLUDE: ${DICOM_INCLUDE}")

include_directories(
    ${DICOM_INCLUDE}
)