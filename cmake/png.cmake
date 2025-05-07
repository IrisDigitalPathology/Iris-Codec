set (ZLIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/zlib)
set (PNG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/png)
if(WIN32)
    set(STATIC_LIB_SUFFIX .lib)
    set (PNG_LIB_NAME libpng16_static${STATIC_LIB_SUFFIX})
elseif(UNIX)
    set(STATIC_LIB_SUFFIX .a)
    set (PNG_LIB_NAME libpng${STATIC_LIB_SUFFIX})
endif()

# Start by trying to find turbojpeg locally
if (NOT IRIS_BUILD_DEPENDENCIES)
    message(STATUS "libPNG dependency set to system search: attempting to dynamically link")
    message(STATUS "Looking for libPNG...")
    find_package(PNG)
    if (PNG_FOUND)
        message(STATUS "libPNG FOUND: version ${PNG_VERSION_STRING}")
        set(PNG_LIBRARY  $<IF:$<TARGET_EXISTS:PNG::PNG>,PNG::PNG,PNG::PNG-static>)
    else ()
        find_library(PNG_LIBRARY png)
    endif()
    find_path(PNG_INCLUDE png.h)
else ()
    if (NOT PNG_LIBRARY OR NOT PNG_INCLUDE)
        find_file (PNG_LIBRARY ${PNG_LIB_NAME} ${PNG_INSTALL_DIR}/lib)
        find_path (PNG_INCLUDE png.h HINTS ${PNG_INSTALL_DIR})
        if (PNG_LIBRARY)
            MESSAGE(STATUS "libpng found from previous build attempt: ${PNG_LIBRARY}")
        endif()
    endif()
endif()

if (NOT PNG_LIBRARY OR NOT PNG_INCLUDE)
    message(STATUS "PNG NOT FOUND. Set to clone and build during the build process.")
    if(WIN32)
        set (ZLIB_LIBRARY ${ZLIB_INSTALL_DIR}/lib/zlibstatic${STATIC_LIB_SUFFIX})
    elseif(UNIX)
        set (ZLIB_LIBRARY ${ZLIB_INSTALL_DIR}/lib/libz${STATIC_LIB_SUFFIX})
    endif()
    ExternalProject_Add(
        Zlib-ng
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
        GIT_TAG "2.2.4"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${ZLIB_LIBRARY} # Ninja compatability
        CMAKE_ARGS 
            -D CMAKE_INSTALL_PREFIX:PATH=${ZLIB_INSTALL_DIR}
            -D CMAKE_INSTALL_LIBDIR=${ZLIB_INSTALL_DIR}/lib/
            -D CMAKE_POSITION_INDEPENDENT_CODE=ON
            -D BUILD_SHARED_LIBS=OFF
            -D ZLIB_COMPAT=ON
            -D ZLIB_ENABLE_TESTS=OFF
            -D WITH_RUNTIME_CPU_DETECTION=ON
    )
    set(PNG_EXTERNAL_PROJECT_ADD ON)
    set(PNG_LIBRARY ${PNG_INSTALL_DIR}/lib/${PNG_LIB_NAME})
    set(PNG_INCLUDE ${PNG_INSTALL_DIR}/include/)
    ExternalProject_Add(
        Png
        GIT_REPOSITORY https://github.com/pnggroup/libpng.git
        GIT_TAG "51f5bd68b9b806d2c92b4318164d28b49357da31" #"origin/main"
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED ON
        BUILD_BYPRODUCTS ${PNG_LIBRARY} # Ninja compatability
        CMAKE_ARGS 
            -D CMAKE_INSTALL_PREFIX:PATH=${PNG_INSTALL_DIR}
            -D CMAKE_POSITION_INDEPENDENT_CODE=ON
            -D BUILD_SHARED_LIBS=OFF 
            -D ZLIB_ROOT=${ZLIB_INSTALL_DIR}
            -D PNG_TESTS=OFF
            -D PNG_TOOLS=OFF
            -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -D CMAKE_INSTALL_LIBDIR=${PNG_INSTALL_DIR}/lib/
    )
    add_dependencies(Png Zlib-ng)
    # Add ZLIB to the PNG. It MUST come afterwards d/t static linking conv
    set(PNG_LIBRARY ${PNG_LIBRARY} ${ZLIB_LIBRARY})
endif()
include_directories(
    ${PNG_INCLUDE}
)