# Iris Codec Community Module

Copyright &copy; 2025 Iris Developers; MIT Software License
<!-- > [!WARNING]
>  The Iris Codec module is still in active development. We do not anticipate introducting breaking API alterations within the [header files](https://github.com/IrisDigitalPathology/Iris-Headers) but as we add in new features, some elements of the API may change slightly. Please check in regularly if you intend to update your dynamically linked libraries to ensure no critial API changes have been merged.  -->


The Iris Codec Community module is a part of the Iris Digital Pathology project. This module allows for:
- Reading and writing of Iris whole slide image (WSI) digital slide files (*.iris*) and 
- Decoding Iris Codec-type compressed tile image data. 

This repository was designed to allow for extremely fast slide access using a simple API. We want to simplify access to these files for you. This module [may be installed](README.md#installation) in the following forms:
- Pre-compiled binaries in the [releases tab](https://github.com/IrisDigitalPathology/Iris-Codec/releases),
- Source files with [CMake build scripts](README.md#c-and-c-implementations).
- Prebuilt python modules are avilable via [Python package managers](README.md#python).
- JavaScript WASM module via [jsDelivr](https://cdn.jsdelivr.net/npm/iris-codec@latest/)

This module has relatively limited dependencies. As our encoder builds shift away from using OpenSlide, we will add additional library dependencies for reading vendor files. 

> [!TIP]
> **Iris Slide Files may immediately be used instead of deep zoom images (DZI)** within your image management and viewer stacks. This is because we provide [Iris RESTful Server](https://github.com/IrisDigitalPathology/Iris-RESTful-Server) builds and [OpenSeaDragon IrisTileSource](https://github.com/openseadragon/openseadragon/blob/master/src/iristilesource.js) that may be deployed using docker containers from our pre-build images as well as pre-built binaries (for those who do not wish to use containers). We also provide the [examples.restful.irisdigitalpathology.org](https://examples.restful.irisdigitalpathology.org) subdomain as a developer service to the community. **This domain responds directly to Iris RESTful API calls.** Use it to integrate and evaluate your HTTPS implementations with the Iris RESTful API, utilizing example Iris test slide files and accelerating your development process. See [JavaScript](#javascript) for more information on using Iris Files with web viewers.

> [!NOTE]
> **If you are a scanning device manufacturer or programmer developing a custom encoder/decoder, the [Iris File Extension (IFE) repository](https://github.com/IrisDigitalPathology/Iris-File-Extension) will provide the necessary calls to read, write, and validate slide files in accordance with the Iris File Extension Specification.** We are adding **'stream encoding'** support for scanners within this repository. However, if you wish to write your own encoder using the IFE respository instead, the Iris Codec module source files may be a helpful guide in how we choose to read and write to Iris files using the IFE's API.

*If you are a software engineer looking to help with Iris, we are always looking for additional passionate engineers to help in developing the Iris Project.*

# Example Slides
The following example WSI files are publically available from AWS S3:
* [cervix_2x_jpeg.iris (jpeg encoded at 2x downsampling)](https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_2x_jpeg.iris)
* [cervix_4x_jpeg.iris (jpeg encoded at 4x downsampling)](https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_4x_jpeg.iris)
# Installation
The Iris Codec Community module is available via:
- [Building From Source](README.md#building-from-source)
- [Python Package Managers](README.md#python)
- [JavaScript NPM / JSDelivr](README.md#javascript)

The standard module (decoder) requires:
- C++ 20 Standard Library
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo)
- [libavif](https://github.com/AOMediaCodec/libavif)

The Encoder (optional) additionally requires:
- [OpenSlide](https://github.com/openslide/openslide) <p> *We hope to shift away from openslide in future releases*

## Building From Source

This library can be built from source using CMake. 

[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/cmake-macos-CI.yml?style=for-the-badge&logo=github&label=MacOS%20CMake%20CI)](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-macos-CI.yml)
[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/cmake-linux-CI.yml?style=for-the-badge&logo=github&label=Ubuntu%20CMake%20CI)
](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-linux-CI.yml)
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

### Pip (PyPi)
[![PyPI - Version](https://img.shields.io/pypi/v/Iris-Codec?color=blue&style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Python Version](https://img.shields.io/pypi/pyversions/Iris-Codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Format](https://img.shields.io/pypi/format/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Downloads](https://img.shields.io/pepy/dt/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)

Iris Codec can also be installed via Pip. The Encoder module dynamically links against OpenSlide to re-encode vendor slide files. This may be removed in the future, but it must be installed presently.

```shell
pip install iris-codec openslide-bin
```

## Javascript
[![Iris Codec WASM CI](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/emcmake-wasm-CI.yml?style=for-the-badge&logo=github&label=Iris%20Codec%20WebAssembly%20CI)](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-macos-CI.yml)
[![NPM Version](https://img.shields.io/npm/v/iris-codec?style=for-the-badge&logo=npm)](https://www.npmjs.com/package/iris-codec)
[![jsDelivr hits (npm)](https://img.shields.io/jsdelivr/npm/hw/iris-codec?style=for-the-badge&logo=jsDelivr&link=https%3A%2F%2Fcdn.jsdelivr.net%2Fnpm%2Firis-codec%2F)](https://cdn.jsdelivr.net/npm/iris-codec@latest/)

There are two mechanisms by which Iris Codec / Iris slide files can be incorporated into a JavaScript project. Unlike the other builds in this repository, JavaScript implementations rely upon the browser enviornment to perform image decompression. 

The two options are as follows:
1. <strong>[Iris RESTful Server]()</strong>: This is the most efficient method of streaming image data to a client. This is a <u>**server side serialization**</u> tool. It will map Iris Slides into virtual memory and return image data quickly. This is only compatible with HTTP servers that have a true file system. This solution is very easy to implement using the [OpenSeaDragon IrisTileSource](https://github.com/openseadragon/openseadragon/blob/master/src/iristilesource.js) in about 4 lines of code (see below).
2. <strong>[Iris-Codec WebAssembly Module](https://www.npmjs.com/package/iris-codec)</strong>: This is a slower method of streaming image data; however it is fully compatible with bucked-based storage solutions such as AWS S3 or Google Cloud Storage. It is slower because it is a <u>**client side serialization**</u> tool that validates and loads the full slide metadata in a series of small HTTP 206 'partial read' responses (including offset tables) upfront; following validation and offset generation however, reads are fast as they use HTTP ranged reads for tile data, hitting the same bucket file over and over again. Because of this limitation, we **STRONGLY** recommend <u>abstracting the metadata for all slides within a case set simultaneously</u> as it requires a small footprint (1-2 MB/slide) and allows for fast access following this file metadata abstraction. 

>[!NOTE] [Iris RESTful]() has <u>substantially better performance</u>. The Iris-Codec WebAssembly module is primary useful when image data is stored in low-cost bucket storage where a custom server instance (Irist RESTful) cannot be deployed - AWS S3, for example. If you choose to use this route, we **STRONGLY** recommend <u>loading all slides within a case set simultaneously</u> as the major overhead with this module comes from slide validation and offset table generation. This can even take seconds, but all slides can be loaded simultaneously (ie all slides in a case can should be loaded at once), and only consumes 1-2 MB per slide.

### Installation of Iris RESTful Server
Please refer to the <strong>[Iris RESTful Server documentation](https://github.com/IrisDigitalPathology/Iris-RESTful-Server?tab=readme-ov-file#deployment-introduction)</strong> for a description of deploying the Iris RESTful Server. The easiest way is through deployment of our [official container images](https://ghcr.io/irisdigitalpathology/iris-restful:latest).

### Installation of Iris-Codec WebAssembly
When used in a client side system, jsDelivr will distribute the WebAssembly module from NPM. There is no installation needed from you.
```html
<!DOCTYPE html>
<script type="module">
    import createModule 
    from 'https://cdn.jsdelivr.net/npm/iris-codec@latest';
    
    // Compile the WASM module and stall execution until ready
    const irisCodec = await createModule();
    console.log("Iris-Codec has been loaded");
    
    // ...Your code goes here...
</script>
```

The general use case is in web applications. If you wish to use it with NodeJS to access remote slides or for node based testing it can be installed from NPM. Please do not try to build a server with this. It is designed for client-side applications and will bottleneck your server.

```sh
npm i iris-codec
```

>[!WARNING] Do not use this package from NPM for a server deployment that serves Iris encoded slides. This is a client slide tool and <u>the webassembly module was not designed for use in a server</u> and it will not work well for this purpose. Trust me. If you need a server, <strong>use Iris RESTful server instead</strong>; frankly, it is significantly faster and more robust than what Node will give you for slide tile serving.



# Implementations
We provide introduction implementation examples for the following languages below:
- [C++ Example API](README.md#c-example-api)
- [Python Example API](README.md#python-example-api)
- [JavaScript Example API](README.md#javascript-webassembly-api)

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

## JavaScript Iris Codec API
### Iris RESTful API
Iris RESTful has a simple API (and supports DICOMweb WADO-RS), outlined here, and explained in greater detail within the [Iris RESTful API Exlained Section](https://github.com/IrisDigitalPathology/Iris-RESTful-Server/blob/main/README.md#api-explained). This will return the slide metadata in JSON format and slide tile image data.

```
Iris RESTful
GET <URL>/slides/<slide-name>/metadata
GET <URL>/slides/<slide-name>/layers/<layer>/tiles/<tile>

Supported WADO-RS
GET <URL>/studies/<study>/series/<UID>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/frames/<tile>
```


### Iris JavaScript WASM API
Load the Iris-Codec NPM WebAssembly module via jsDelivr (or download the [latest javascript release](https://github.com/IrisDigitalPathology/Iris-Codec/releases/latest) and include your local copy)
```html
<!DOCTYPE html>
<script type="module">
    import createModule 
    from 'https://cdn.jsdelivr.net/npm/iris-codec@latest';
    
    // Compile the WASM module and stall execution until ready
    const irisCodec = await createModule();
    console.log("Iris-Codec has been loaded");
    
    // ...Your code goes here...
</script>
```

Once loaded, you can access image data in a manner similar to the C++ and Python Iris-Codec API. Importantly, the metadata will be returned in IrisCodec::Abstraction C++ types (file metadata abstractions) exposed using Emscripten bindings. Refer to the included TypeScript file for those definitions. Image tile data will be returned as an image (MIME) source and can be used directly as an image data source.

We support callback notation presently as it is both common/well established with JS programmers and easy to construct promises from a callback; however it is more challening to reformat a promise into a callback structure for legacy support. We may simply move to promises in the future. If you wish to wrap file access in promise structures, here are example definitions:
```js
// Wraps Module.validateFileStructure (url, callback) in a Promise
function validateFileStructureAsync(Module, fileUrl) {
  return new Promise((resolve, reject) => {
    Module.validateFileStructure(fileUrl, (result) => {
        const SUCCESS = Module.ResultFlag.IRIS_SUCCESS.value;
        if (result.flag.value === SUCCESS) {
            resolve();
        } else {
            // result.message is guaranteed to be a JS string
            reject(new Error(result.message));
        }
    });
  });
}
// Wraps Module.openIrisSlide(url, callback) in a Promise
function openIrisSlideAsync(url) {
  return new Promise((resolve, reject) => {
    Module.openIrisSlide(url, slide => {
      if (!slide) {
        reject(new Error("Failed to validate"));
      } else {
        resolve(slide);
      }
    });
  });
}
// Wraps slide.getSlideTile(layer, tileIndex, callback) in a Promise
function getSlideTileAsync(slide, layer, tileIndex) {
  return new Promise((resolve, reject) => {
    slide.getSlideTile(layer, tileIndex, tile_image => {
      if (tile_image) {
        resolve(tile_image);
      } else {
        reject(new Error("Failed to get tile"));
      }
    });
  });
}
```

Perform a deep validation of the slide file structure. This will navigate the internal offset-chain and check for violations of the IFE standard. This can be omitted if you are confident of the source.

```js
const irisCodec = await createModule();
console.log("Iris-Codec has been loaded");
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_2x_jpeg.iris";
try {
    await validateFileStructureAsync(irisCodec, url);
    console.log(`Slide file at ${url} successfully passed validation`);
} catch (error) {
    console.log(`Slide file at ${url} failed validation: ${error}`);
}
```
Open a slide file. The following conditional will succeed without throwing an exception if the slide has already passed validation but you may skip validation to reduce server requests.
```js
const irisCodec = await createModule();
console.log("Iris-Codec has been loaded");
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_2x_jpeg.iris";
try {
    const slide = await openIrisSlideAsync(irisCodec, url);

    // ...Do something with slide

    slide.delete();
} catch (error) {
    console.error(error);
}
```
Get the slide abstraction, read off the slide dimensions, and then print it to the console.
```js
const irisCodec = await createModule();
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_4x_jpeg.iris";
try {
    const slide = await openIrisSlideAsync (irisCodec, url);

    // Let's get the slide dimensions and print them to the console.
    const info = slide.getSlideInfo();
    const extent = info.extent;
    console.log(`Slide file ${extent.width} px by ${extent.height}px at lowest resolution layer. The layer extents are as follows:`);
    console.log(`There are ${extent.layers.size()} layers comprising the following dimensions:`)
    for (var i = 0; i < extent.layers.size(); i++) {
        const layer = extent.layers.get(i);
        console.log(`  Layer ${i}: ${layer.xTiles} x-tiles, ${layer.xTiles} y-tiles, ${layer.scale}x scale`);
    }
    slide.delete();
} catch (error) {
    console.error(error);
}
```
Generate a quick view of the one of the images (`tileImage`) somewhere earlier in the HTML page.
```html
<img id="tileImage" width="128" height="128" alt="Loading..." style="border: 1px solid black;"/>
<!-- ... Somewhere Earlier -->
<script type="module">
    // Earlier Promise definitions 
    try {
        const slide = await openIrisSlideAsync (irisCodec, url);
        const layer = 0;
        const tile = 0;
        const tileData = await getSlideTileAsync(slide, layer, tile);

        // Now pass the image off to the 'tileImage' element.
        const url = URL.createObjectURL(tileData);
        const imgElement = document.getElementById("tileImage");
        imgElement.src = url;

        // Clean up after the image is loaded
        imgElement.onload = () => {
            URL.revokeObjectURL(objectUrl);
            slide.delete();
        };
    } catch (error) {
        console.error(error);
    }
    
</script>
```
<!-- result, info = slide.get_info()
if (result.success() == False):
    raise Exception(f'Failed to read slide information: {result.message()}') -->

