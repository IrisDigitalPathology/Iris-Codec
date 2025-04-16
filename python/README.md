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
[![PyPI - Downloads](https://img.shields.io/pypi/dm/iris-codec?style=for-the-badge)](https://pypi.org/project/Iris-Codec/)

Iris Codec can also be installed via Pip. The Encoder module dynamically links against OpenSlide to re-encode vendor slide files. This may be removed in the future, but it must be installed presently.

```shell
pip install iris-codec openslide-bin
```


## Anaconda (Conda-Forge)
[![Static Badge](https://img.shields.io/badge/Feedstock-Iris_Codec-g?style=for-the-badge)
](https://github.com/conda-forge/Iris-Codec-feedstock) 
[![Conda Version](https://img.shields.io/conda/vn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec) 
[![Conda Platforms](https://img.shields.io/conda/pn/conda-forge/iris-codec.svg?style=for-the-badge)](https://anaconda.org/conda-forge/iris-codec)

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

Open the a slide file. The following conditional will always return True if the slide has already passed validation but you may skip validation and it will return with a null slide object (but without providing the Result debug info).
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

Generate a quick view of a slide tile in the middle of the slide using matplotlib imshow function.
```py
import matplotlib.pyplot as plt
layer_index = 0
x_index = int(extent.layers[layer_index].x_tiles/2)
y_index = int(extent.layers[layer_index].y_tiles/2)
tile_index = extent.layers[layer_index].x_tiles * y_index + x_index
fig = plt.figure()
plt.imshow(slide.read_slide_tile(layer_index,tile_index), interpolation='none')
plt.show()
```
>[!WARNING] If you want to full layer images of higher-res layers, do not use MatPlotLib! Use an alternative like Pillow. MatPlotLib is familiar to most data-scientists. It cannot handle rendering the amount of image data produced by full higher-resolution layers.

Or generate a full layer image.
```py
import matplotlib.pyplot as plt
fig = plt.figure()
layer_extent = extent.layers[0]
for y in range(layer_extent.y_tiles):
  for x in range (layer_extent.x_tiles):
    tile_index = y*layer_extent.x_tiles+x
    plt.subplot(layer_extent.y_tiles,layer_extent.x_tiles,tile_index+1)
    plt.imshow(slide.read_slide_tile(0,tile_index), interpolation='none')
    plt.xticks([])
    plt.yticks([])
    plt.title(f'{tile_index}', y=0.5)
plt.subplots_adjust(wspace=0.0, hspace=0.0)
plt.show()
```