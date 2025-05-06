# Iris Codec Community Module

Copyright &copy; 2025 Iris Developers; MIT Software License
> [!WARNING]
>  The Iris Codec module is still in active development. We do not anticipate altering the established API functions in the [header files](https://github.com/IrisDigitalPathology/Iris-Headers) but as we add in new features, some elements of the API may change slightly. Please check in regularly if you intend to update your dynamically linked libraries to ensure no API breaking changes have been merged. 


The Iris Codec Community module is a part of the Iris Digital Pathology project. This module allows for:
- Reading and writing of Iris whole slide image (WSI) digital slide files (*.iris*) and 
- Decoding Iris Codec-type compressed tile image data. 

This repository was designed to allow for extremely fast slide access using a simple API. We want to simplify access to these files for you. This module [may be installed](README.md#installation) in the following forms:
- Pre-compiled binaries in the [releases tab](https://github.com/IrisDigitalPathology/Iris-Codec/releases),
- Source files with [CMake build scripts](README.md#c-and-c-implementations).
- Prebuilt python modules are avilable via [Python package managers](README.md#python).

This module has reliatively limited dependencies. As our encoder builds shift away from using OpenSlide, we will add additional library dependencies for reading vendor files. 

> [!NOTE]
> **If you are a scanning device manufacturer or programmer developing a custom encoder/decoder, the [Iris File Extension (IFE) repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) will provide the necessary calls to read, write, and validate slide files in accordance with the Iris File Extension Specification.** The current repository (Iris Codec Module) applies higher level abstractions for slide access and incorporates image codecs for image compression. The IFE repository does not. It is limited to (de)serialization and validation. The Iris Codec Module incorporates the [IFE repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) as a dependency, so if you use the IFE respository instead, the Iris Codec module source files may be a helpful guide in how we choose to read and write to Iris files using the IFE's API.

*If you are a software engineer looking to help with Iris, we are always looking for additional passionate engineers to help in developing the Iris Project.*

# Installation
The Iris Codec Community module is available via:
- [Building From Source](README.md#building-from-source)
- [Python Package Managers](README.md#python)
- [JavaScript WASM Module](README.md#javascript)

The standard module (decoder) requires:
- C++ 20 Standard Library
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo)
- [libavif](https://github.com/AOMediaCodec/libavif)

The Encoder (optional) additionally requires:
- [OpenSlide](https://github.com/openslide/openslide) <p> *We hope to shift away from openslide in future releases*



## Building From Source

This library can be built from source using CMake. 

[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/cmake-macos-CI.yml?style=for-the-badge&logo=github&label=MacOS%20CMake%20CI)](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-macos-CI.yml)\
[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/cmake-linux-CI.yml?style=for-the-badge&logo=github&label=Ubuntu%20CMake%20CI)
](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-linux-CI.yml)\
[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/cmake-win64-CI.yml?style=for-the-badge&logo=github&label=Windows%20CMake%20CI)
](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-win64-CI.yml)


Clone the repository. 
```sh
git clone --depth 1 https://github.com/IrisDigitalPathology/Iris-Codec.git
```

CMake bash build commands. Remember to `-DCMAKE_INSTALL_PREFIX` your chosen install directory if not installing system-wide. Additionally, Iris Codec CMake script is designed to look for and dynamically link [turbo-jpeg](https://github.com/libjpeg-turbo/libjpeg-turbo) and [AVIF](https://github.com/AOMediaCodec/libavif) by default; however, some implementations would rather simply build a self-contained statically linked binary without the need to dynamically load libraries. **In some instances where reliablity is key, this may be the most secure option.** Some architectures, such as iOS require this. To enable static dependency linkage, instead set `-DIRIS_BUILD_DEPENDENCIES=ON`.
 

```shell
#/bin/bash
cmake -B build \
    -D IRIS_BUILD_SHARED=ON \
    -D IRIS_BUILD_STATIC=ON \
    -D IRIS_BUILD_ENCODER=ON \
    -D IRIS_BUILD_DEPENDENCIES=OFF \
    -D IRIS_BUILD_PYTHON=OFF \
    -D IRIS_USE_OPENSLIDE=ON \
    ./Iris-Codec
cmake --build ./Iris-Codec/build --config Release -j$CPU_COUNT
cmake --install ./Iris-Codec/build
```

Or Windows
```bat
cmake -B build -GNinja ^
    -D CMAKE_BUILD_TYPE=Release ^
    -D IRIS_BUILD_SHARED=ON ^
    -D IRIS_BUILD_STATIC=ON ^
    -D IRIS_BUILD_ENCODER=ON ^
    -D IRIS_BUILD_DEPENDENCIES=OFF ^
    -D IRIS_BUILD_PYTHON=OFF ^
    -D IRIS_USE_OPENSLIDE=ON ^
    .\Iris-Codec
if errorlevel 1 exit 1
cmake --build .\Iris-Codec\build --config Release
if errorlevel 1 exit 1
cmake --install .\Iris-Codec\build
```

 More info on the dependencies lookup and **cross compiling** Iris Codec in the [cmake directory](./cmake/).


## Python
Iris Codec is available via the Anaconda and PyPi package managers. We prefer the Anaconda enviornment as it includes dynamic libraries if you choose to develop C/C++ applications with Python bindings that dynamically link the C++ Iris-Codec in addition to Python modules. 

>[!NOTE] In addition to the below package managers, The Python module may also be built from source by setting `-DIRIS_BUILD_PYTHON=ON` in the above [CMake command](README.md#building-from-source).


### Anaconda (Conda-Forge)
[![Static Badge](https://img.shields.io/badge/Feedstock-Iris_Codec-g?style=for-the-badge)
](https://github.com/conda-forge/Iris-Codec-feedstock) 
[![Conda Version](https://img.shields.io/conda/vn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Platforms](https://img.shields.io/conda/pn/conda-forge/iris-codec.svg?style=for-the-badge&color=blue)](https://anaconda.org/conda-forge/iris-codec)

You may configure your conda enviornment in the following way
```shell
conda config --add channels conda-forge
conda install iris-codec
```
Or directly install it in a single command

```shell
conda install -c conda-forge Iris-Codec 
```

or install it with `mamba`:
```shell
mamba install iris-codec
```

>[!NOTE]
> The python Conda Forge Encoder does not support OpenSlide on Windows presently as OpenSlide does not support windows with its official Conda-Forge package. We are building in native support for vendor files and DICOM for re-encoding. 

### Pip (PyPi)
[![PyPI - Version](https://img.shields.io/pypi/v/Iris-Codec?color=blue&style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Status](https://img.shields.io/pypi/status/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Python Version](https://img.shields.io/pypi/pyversions/Iris-Codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Format](https://img.shields.io/pypi/format/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Downloads](https://img.shields.io/pepy/dt/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)

Iris Codec can also be installed via Pip. The Encoder module dynamically links against OpenSlide to re-encode vendor slide files. This may be removed in the future, but it must be installed presently.

```shell
pip install iris-codec openslide-bin
```

## Javascript
[![Iris Codec Emscripten Webassembly Build](https://github.com/IrisDigitalPathology/Iris-Codec-JavaScript/actions/workflows/emcmake.yml/badge.svg)](https://github.com/IrisDigitalPathology/Iris-Codec-JavaScript/actions/workflows/emcmake.yml)

The [Iris-Codec-JavaScript repository](https://github.com/IrisDigitalPathology/Iris-Codec-JavaScript) contains the WebAssembly (WASM) build of the Iris Codec library, allowing it to be used in web browsers and Node.js applications. This implementation does not have the same dependencies, as image decoding is performed in the browser with JavaScript native codec tools. 

# Implementations
We provide introduction implementation examples for the following languages below:
- [C++ Example API](README.md#c-example-api)
- [Python Example API](README.md#python-example-api)

Please refer to the Iris Codec API documentation for a more through explaination.

## C++ Example API
Iris is natively a C++ program and the majority of features will first be supported in C++ followed by the other language bindings as we find time to write bindings. 

Begin by importing the [Iris Codec Core header](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisCodecCore.hpp); it contains references to the [Iris Codec specific type definitions](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisCodecTypes.hpp) as well as the general [Iris Core type definitions](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisTypes.hpp). 
```cpp
// Import the Iris Codec header
// This import includes the types header automatically
#import <filesystem>
#import <Iris/IrisCodecCore.hpp>
```
You may chose to perform your own file system validations and recovery routines. Iris will, however catch all of these (and the main API methods are declared `noexcept`).
```cpp
if (!std::filesystem::exists(file_path)) {
    printf(file_path.string() + " file does not exist\n");
    return EXIT_FAILURE;
}
if (!is_iris_codec_file(file_path.string())) {
    printf(file_path.string() + " is not a valid Iris slide file\n");
    return EXIT_FAILURE;
}
IrisResult result = validate_slide (SlideOpenInfo {
    .filePath = file_path.string()
    // Default values for any undefined parameters
});

if (result != IRIS_SUCCESS) {
    printf (result.message);
    return EXIT_FAILURE;
}
```
 Should an runtime error occur, it will be reported in the form of an `IrisResult` message, as seen in the `IrisResult validate_slide (const SlideOpenInfo&) noexcept;` call. 
 
 Successful loading of a slide file will return a valid `IrisCodec::Slide` object; failure will return a `nullptr`. 
```cpp
auto slide = open_slide (SlideOpenInfo {
    .filePath       = file_path.string(),
    .context        = nullptr,
    .writeAccess    = false
});
if (!slide) return EXIT_FAILURE;
```

Once opened, the slide `IrisCodec::SlideInfo` structure can be loaded using the `Result get_slide_info (const Slide&, SlideInfo&) noexcept` call and used as an initialized structure containing all the information needed to navigate the slide file and read elements.
```cpp
SlideInfo info;
IrisResult result = get_slide_info (slide, info);
if (result != IRIS_SUCCESS) {
    printf (result.message);
    return EXIT_FAILURE;
}
```
The `SlideTileReadInfo` struct provides a simple mechanism for reading slide image data. The `info.extent` struct is extremely simple to navigate. Please refer to the `struct Extent` type in the [IrisTypes.hpp](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/include/IrisTypes.hpp) core header file for more information about slide extents.
```cpp
struct SlideTileReadInfo read_info {
    .slide                  = slide,
    .layer                  = 0,
    .optionalDestination    = NULL, /*wrapper can go here*/
    .desiredFormat          = Iris::FORMAT_R8G8B8A8,
};
for (auto& layer : info.extent.layers) {
    for (int y_index = 0; y_index < layer.yTiles; ++y_index) {
        for (int x_index = 0; x_index < layer.xTiles; ++x_index) {
            // Read the tile slide tile
            Iris::Buffer rgba = read_slide_tile (read_info);
            
            // Do something with the tile pixel values in rgba

            // Do not worry about clean up;
            // The Iris::Buffer will deallocate it.
        }
    }
    read_info.layer++;
}
if (optional_buffer) free (optional_buffer);
```
Decompressed slide data can be optionally read into preallocated memory. If the optional destination buffer is insufficiently sized, Iris will instead allocate a new buffer and return that new buffer with the pixel data. 

>[!NOTE]
> If writing into externally managed memory, `Iris::Buffer` should weakly reference the underlying memory using `Wrap_weak_buffer_fom_data()` as strongly referenced `Iris::Buffer` objects deallocate underlying memory on deletion.
```cpp
// In this example we have some preallocated buffer we want
// to write our slide pixel data into. A GPU buffer is a great
// example and the GPU API manages that memory:
char* GPU_DST;

// We will write in R8G8B8A8 format for simplicity
Iris::Format format = Iris::FORMAT_R8G8B8A8;
size_t tile_bytes   = 256*256*4; 
Iris::Buffer wrapper = Wrap_weak_buffer_fom_data (GPU_DST, tile_bytes);

// Read the data
struct SlideTileReadInfo read_info {
    .slide                  = slide,
    .optionalDestination    = wrapper,
    .desiredFormat          = format,
};
Buffer result = read_slide_tile (read_info);

// If there was insufficient space in the provided
// destination buffer, a new buffer will be allocated.
if (wrapper != result) {
    printf ("Insufficient sized buffer, new buffer was allocated");
}
```
Iris can decompress into different pixel byte orderings and exchange data ownership via the `Iris::Buffer` strength. For more information about the Iris Buffer, which was designed primarily as a networking buffer, please see [IrisBuffer.hpp](https://github.com/IrisDigitalPathology/Iris-Headers/blob/main/priv/IrisBuffer.hpp) in the core headers. 

## Python Example API

Import the Python API and Iris Codec Module.

```python
#Import the Iris Codec Module
from Iris import Codec
slide_path = 'path/to/slide_file.iris'
```

Perform a deep validation of the slide file structure. This will navigate the internal offset-chain and check for violations of the IFE standard.
```python
result = Codec.validate_slide_path(slide_path)
if (result.success() == False):
    raise Exception(f'Invalid slide file path: {result.message()}')
print(f"Slide file '{slide_path}' successfully passed validation")
```

Open a slide file. The following conditional will always return True if the slide has already passed validation but you may skip validation and it will return with a null slide object (but without providing the Result debug info).
```python
slide = Codec.open_slide(slide_path)
if (not slide): 
    raise Exception('Failed to open slide file')
```
Get the slide abstraction, read off the slide dimensions, and then print it to the console.  
```py
# Get the slide abstraction
result, info = slide.get_info()
if (result.success() == False):
    raise Exception(f'Failed to read slide information: {result.message()}')

# Print the slide extent to the console
extent = info.extent
print(f"Slide file {extent.width} px by {extent.height}px with an encoding of {info.encoding}. The layer extents are as follows:")
print(f'There are {len(extent.layers)} layers comprising the following dimensions:')
for i, layer in enumerate(extent.layers):
    print(f' Layer {i}: {layer.x_tiles} x-tiles, {layer.y_tiles} y-tiles, {layer.scale:0.0f}x scale')
```

Generate a quick low-power view of the slide using Pillow images.
```py
from PIL import Image
layer_index = 0 # Lowest power layer is layer zero (0)
scale = int(extent.layers[layer_index].scale)
composite = Image.new('RGBA',(extent.width * scale, extent.height * scale))
layer_extent = extent.layers[layer_index]
for y in range(layer_extent.y_tiles):
  for x in range (layer_extent.x_tiles):
    tile_index = y*layer_extent.x_tiles+x
    composite.paste(Image.fromarray(slide.read_slide_tile(layer_index, tile_index)),(256*x,256*y))
composite.show()
```
>[!CAUTION]
>Despite Iris' native fast read speed, higher resolution layers may take substantial time and memory for Pillow to create a full image as it does not create tiled images. I do not recommend doing this above layer 0 or 1 as it may be onerous for PIL.Image 

Investigate the metadata attribute array and view a thumbnail image
```py
result, info = slide.get_info()
if (result.success() == False):
    raise Exception(f'Failed to read slide information: {result.message()}')

print ("Slide metadata attributes")
for attribute in info.metadata.attributes:
    print(f"{attribute}: {info.metadata.attributes[attribute]}")

from PIL import Image
if ('thumbnail' in info.metadata.associated_images):
    image = Image.fromarray(slide.read_associated_image('thumbnail'))
    image.show()
```