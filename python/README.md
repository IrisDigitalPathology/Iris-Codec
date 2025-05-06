The Iris Codec Community module is a part of the Iris Digital Pathology project. This module allows for:
- Reading and writing of Iris whole slide image (WSI) digital slide files (*.iris*) and 
- Decoding Iris Codec-type compressed tile image data. 

This module was designed to allow for extremely fast slide access using a simple API. We want to simplify access to these files for you.

Iris Codec for Python is available via the Anaconda and PyPi package managers. We prefer the Anaconda enviornment as it includes dynamic libraries if you choose to develop C/C++ applications with Python bindings that dynamically link the C++ Iris-Codec in addition to Python modules. 

## Pip (PyPi)
[![PyPI - Version](https://img.shields.io/pypi/v/Iris-Codec?color=blue&style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Status](https://img.shields.io/pypi/status/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Python Version](https://img.shields.io/pypi/pyversions/Iris-Codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Format](https://img.shields.io/pypi/format/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)
[![PyPI - Downloads](https://img.shields.io/pepy/dt/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)

Iris Codec can also be installed via Pip. The Encoder module dynamically links against OpenSlide to re-encode vendor slide files. This may be removed in the future, but it must be installed presently.

```shell
pip install iris-codec openslide-bin
```


## Anaconda (Conda-Forge)
[![Static Badge](https://img.shields.io/badge/Feedstock-Iris_Codec-g?style=for-the-badge)
](https://github.com/conda-forge/Iris-Codec-feedstock) 
[![Conda Version](https://img.shields.io/conda/vn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Platforms](https://img.shields.io/conda/pn/conda-forge/iris-codec.svg?color=blue&style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec)

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

**NOTE:** The python Conda Forge Encoder does not support OpenSlide on Windows presently as OpenSlide does not support windows with its official Conda-Forge package. We are building in native support for vendor files and DICOM for re-encoding. 

# Python Example API

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
    composite.paste(Image.fromarray(slide.read_slide_tile(layer_index, tile_index)),(256*x,256*y)) #Iris tiles are always 256 px in each dim
composite.show()
```
**CAUTION:** Despite Iris' native fast read speed, higher resolution layers may take substantial time and memory for Pillow to create a full image as it does not create tiled images. I do not recommend doing this above layer 0 or 1 as it may be onerous for PIL.Image 

The API for metadata is designed to be intuitive and pythonic. Below shows how to investigate the metadata attribute array and view a thumbnail image, if one is present within the slide file.
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