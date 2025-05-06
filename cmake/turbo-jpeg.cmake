set (TURBOJPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/turbojpeg)
if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (TURBOJPEG_LIB_NAME turbojpeg-static${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (TURBOJPEG_LIB_NAME libturbojpeg${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find turbojpeg locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "Turbo-Jpeg dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for Turbo-Jpeg...")
    find_package(libjpeg-turbo)
    if (libjpeg-turbo_FOUND)
        message(STATUS "Turbo-Jpeg FOUND: version ${libjpeg-turbo_VERSION}")
        set(TURBOJPEG_LIBRARY  $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)
    endif()
    find_path(TURBOJPEG_INCLUDE turbojpeg.h)
else ()
    if (NOT TURBOJPEG_LIBRARY OR NOT TURBOJPEG_INCLUDE)
        find_file (TURBOJPEG_LIBRARY ${TURBOJPEG_LIB_NAME} ${TURBOJPEG_INSTALL_DIR}/lib)
        find_path (TURBOJPEG_INCLUDE turbojpeg.h HINTS ${TURBOJPEG_INSTALL_DIR})
        if (TURBOJPEG_LIBRARY)
            MESSAGE(STATUS "Turbo-jpeg found from previous build attempt: ${TURBOJPEG_LIBRARY}")
        endif()
    endif()
endif()


if (NOT TURBOJPEG_LIBRARY OR NOT TURBOJPEG_INCLUDE)
    MESSAGE(STATUS "Turbo-Jpeg NOT FOUND. Set to clone and build during the build process.")
    set(TURBOJPEG_EXTERNAL_PROJECT_ADD ON)
    set(TURBOJPEG_LIBRARY ${TURBOJPEG_INSTALL_DIR}/lib/${TURBOJPEG_LIB_NAME})
    set(TURBOJPEG_INCLUDE ${TURBOJPEG_INSTALL_DIR}/include/)
    ExternalProject_Add(
        TurboJpeg
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG "20ade4d" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${TURBOJPEG_LIBRARY} # Ninja compatability
        CMAKE_ARGS
            -D CMAKE_INSTALL_PREFIX:PATH=${TURBOJPEG_INSTALL_DIR}
            -D ENABLE_SHARED=OFF
            -D CMAKE_POSITION_INDEPENDENT_CODE=ON
            -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -D CMAKE_INSTALL_LIBDIR=${TURBOJPEG_INSTALL_DIR}/lib
    )
endif()
include_directories(
    ${TURBOJPEG_INCLUDE}
)

