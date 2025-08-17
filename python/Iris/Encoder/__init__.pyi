from __future__ import annotations
from ..iris_core import Result, Format, Context
from .. import Codec
from .encoder import (
    Encoder as Encoder,
    EncoderDerivation as EncoderDerivation,
    EncoderProgress as EncoderProgress,
    EncoderStatus as EncoderStatus,
    create_encoder as create_encoder,
    dispatch_encoder as dispatch_encoder,
    get_encoder_dst_path as get_encoder_dst_path,
    get_encoder_progress as get_encoder_progress,
    get_encoder_src as get_encoder_src,
    interrupt_encoder as interrupt_encoder,
    reset_encoder as reset_encoder,
    set_encoder_dst_path as set_encoder_dst_path,
    set_encoder_src as set_encoder_src,
)

__all__ = ['encode_slide_file']

def encode_slide_file(source: str, outdir: str = ..., desired_encoding: Codec.Encoding = ..., desired_byte_format: Format = ..., strip_metadata: bool = ..., anonymize: bool = ..., derivation: EncoderDerivation = ..., concurrency: int = None, codec_context: Context = ...) -> Result: ...
