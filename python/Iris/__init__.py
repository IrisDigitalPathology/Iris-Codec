from . import Iris
from .Iris import *
from .Iris import Codec
__all__ = ['Codec']
version = Codec.get_codec_version()
__version__=f'{version.major}.{version.minor}.{version.build}'