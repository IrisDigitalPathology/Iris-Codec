"""
Codec submodule that provides access to Iris Encoded Slide Files.
This module re-exports the codec functionality from the main Iris C++ extension.
"""

# Import the core module directly
from .iris_core import Codec as _codec_module

# Re-export codec functionality
AssociatedImageEncoding = _codec_module.AssociatedImageEncoding
AssociatedImageInfo = _codec_module.AssociatedImageInfo
Attributes = _codec_module.Attributes
Context = _codec_module.Context
Encoding = _codec_module.Encoding
Metadata = _codec_module.Metadata
MetadataType = _codec_module.MetadataType
Slide = _codec_module.Slide
SlideInfo = _codec_module.SlideInfo
get_codec_version = _codec_module.get_codec_version
open_slide = _codec_module.open_slide
validate_slide_path = _codec_module.validate_slide_path

__all__ = [
    'AssociatedImageEncoding',
    'AssociatedImageInfo', 
    'Attributes',
    'Context',
    'Encoding',
    'Metadata',
    'MetadataType',
    'Slide',
    'SlideInfo',
    'get_codec_version',
    'open_slide',
    'validate_slide_path'
]
