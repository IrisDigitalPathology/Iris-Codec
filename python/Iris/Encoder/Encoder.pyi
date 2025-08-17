from __future__ import annotations
import typing
from ..iris_core import Encoding, Format, Result, Context

__all__ = ['Encoder', 'EncoderDerivation', 'EncoderProgress', 'EncoderStatus', 'create_encoder', 'dispatch_encoder', 'get_encoder_dst_path', 'get_encoder_progress', 'get_encoder_src', 'interrupt_encoder', 'reset_encoder', 'set_encoder_dst_path', 'set_encoder_src']
class Encoder:
    """
    Slide Encoder class used internally to generate a slide file. This class is stateful, thread-safe, and encodes a single slide at a time in parrallel using a massive amount of the machine's cores. Dispatching an encoder will bring your machine up to about 90-95% of the full CPU capacity so it is not recommended to use more than a single encoder at a time.
    """
    def __repr__(self) -> str:
        ...
class EncoderDerivation:
    """
    Members:
    
      layer_2x
    
      layer_4x
    
      use_source
    """
    __members__: typing.ClassVar[dict[str, EncoderDerivation]]  # value = {'layer_2x': <EncoderDerivation.layer_2x: 1>, 'layer_4x': <EncoderDerivation.layer_4x: 2>, 'use_source': <EncoderDerivation.use_source: 0>}
    use_source: typing.ClassVar[EncoderDerivation]  # value = <EncoderDerivation.use_source: 0>
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class EncoderProgress:
    """
    Encoder progress monitoring structure that returns a snapshot containing the  status of the encoder's progress through an encoding session at the time the get_encoder_progress method was called. If the encoder returns an error status (ENCODER_ERROR), the errorMsg should contain any explaination or explainations
    """
    @property
    def dstFilePath(self) -> str:
        ...
    @property
    def errorMsg(self) -> str:
        ...
    @property
    def progress(self) -> float:
        ...
    @property
    def status(self) -> EncoderStatus:
        ...
class EncoderStatus:
    """
    Members:
    
      ENCODER_INACTIVE
    
      ENCODER_ACTIVE
    
      ENCODER_ERROR
    
      ENCODER_SHUTDOWN
    """
    ENCODER_ACTIVE: typing.ClassVar[EncoderStatus]  # value = <EncoderStatus.ENCODER_ACTIVE: 1>
    ENCODER_ERROR: typing.ClassVar[EncoderStatus]  # value = <EncoderStatus.ENCODER_ERROR: 2>
    ENCODER_INACTIVE: typing.ClassVar[EncoderStatus]  # value = <EncoderStatus.ENCODER_INACTIVE: 0>
    ENCODER_SHUTDOWN: typing.ClassVar[EncoderStatus]  # value = <EncoderStatus.ENCODER_SHUTDOWN: 3>
    __members__: typing.ClassVar[dict[str, EncoderStatus]]  # value = {'ENCODER_INACTIVE': <EncoderStatus.ENCODER_INACTIVE: 0>, 'ENCODER_ACTIVE': <EncoderStatus.ENCODER_ACTIVE: 1>, 'ENCODER_ERROR': <EncoderStatus.ENCODER_ERROR: 2>, 'ENCODER_SHUTDOWN': <EncoderStatus.ENCODER_SHUTDOWN: 3>}
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
def create_encoder(source: str, outdir: str = '', desired_encoding: Encoding = ..., desired_byte_format: Format = ..., anonymize: bool = False, derivation: EncoderDerivation = ..., concurrency: int = 8, codec_context: Context = None) -> Encoder:
    """
    Iris Codec encoding context for GPU based compression
    """
def dispatch_encoder(encoder: Encoder) -> Result:
    """
    The endcoder object to dispatch. Once dispatched the encoder parameters cannot be changed.
    """
def get_encoder_dst_path(arg0: Encoder) -> tuple[Result, str]:
    """
    Get the encoder destination directory path string
    """
def get_encoder_progress(encoder: Encoder) -> tuple[Result, EncoderProgress]:
    """
    the encoder object to check
    """
def get_encoder_src(arg0: Encoder) -> tuple[Result, str]:
    """
    Get the source file path string
    """
def interrupt_encoder(encoder: Encoder) -> Result:
    """
    The encoder object for which to interrupt execution.
    """
def reset_encoder(encoder: Encoder) -> Result:
    """
    The encoder object ro reset.
    """
def set_encoder_dst_path(encoder: Encoder, destination_path: str) -> Result:
    """
    String containing the destination directory path
    """
def set_encoder_src(encoder: Encoder, source_path: str) -> Result:
    """
    String containing the source slide file path
    """
