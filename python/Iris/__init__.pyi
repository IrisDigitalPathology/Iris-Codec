from __future__ import annotations
import numpy
import typing
import typing_extensions
from . import Codec
__all__ = ['Buffer', 'BufferStrength', 'Codec', 'Extent', 'Format', 'LayerExtent', 'Result', 'ResultFlag', 'Version']
class Buffer:
    @typing.overload
    def __init__(self: typing_extensions.Buffer, arg0: BufferStrength) -> None:
        ...
    @typing.overload
    def __init__(self: typing_extensions.Buffer, arg0: BufferStrength, arg1: int) -> None:
        ...
    def data(self: typing_extensions.Buffer) -> numpy.ndarray[numpy.uint8]:
        ...
    def get_strength(self: typing_extensions.Buffer) -> BufferStrength:
        ...
class BufferStrength:
    """
    Members:
    
      REFERENCE_WEAK
    
      REFERENCE_STRONG
    """
    REFERENCE_STRONG: typing.ClassVar[BufferStrength]  # value = <BufferStrength.REFERENCE_STRONG: 1>
    REFERENCE_WEAK: typing.ClassVar[BufferStrength]  # value = <BufferStrength.REFERENCE_WEAK: 0>
    __members__: typing.ClassVar[dict[str, BufferStrength]]  # value = {'REFERENCE_WEAK': <BufferStrength.REFERENCE_WEAK: 0>, 'REFERENCE_STRONG': <BufferStrength.REFERENCE_STRONG: 1>}
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
class Extent:
    def __repr__(self) -> str:
        ...
    @property
    def height(self) -> int:
        ...
    @property
    def layers(self) -> list[LayerExtent]:
        ...
    @property
    def width(self) -> int:
        ...
class Format:
    """
    Members:
    
      FORMAT_B8G8R8
    
      FORMAT_R8G8B8
    
      FORMAT_B8G8R8A8
    
      FORMAT_R8G8B8A8
    """
    FORMAT_B8G8R8: typing.ClassVar[Format]  # value = <Format.FORMAT_B8G8R8: 1>
    FORMAT_B8G8R8A8: typing.ClassVar[Format]  # value = <Format.FORMAT_B8G8R8A8: 3>
    FORMAT_R8G8B8: typing.ClassVar[Format]  # value = <Format.FORMAT_R8G8B8: 2>
    FORMAT_R8G8B8A8: typing.ClassVar[Format]  # value = <Format.FORMAT_R8G8B8A8: 4>
    __members__: typing.ClassVar[dict[str, Format]]  # value = {'FORMAT_B8G8R8': <Format.FORMAT_B8G8R8: 1>, 'FORMAT_R8G8B8': <Format.FORMAT_R8G8B8: 2>, 'FORMAT_B8G8R8A8': <Format.FORMAT_B8G8R8A8: 3>, 'FORMAT_R8G8B8A8': <Format.FORMAT_R8G8B8A8: 4>}
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
class LayerExtent:
    def __repr__(self) -> str:
        ...
    @property
    def downsample(self) -> float:
        ...
    @property
    def scale(self) -> float:
        ...
    @property
    def x_tiles(self) -> int:
        ...
    @property
    def y_tiles(self) -> int:
        ...
class Result:
    def __repr__(self) -> str:
        ...
    def success(self) -> bool:
        ...
    @property
    def message(self) -> str:
        ...
    @property
    def value(self) -> ResultFlag:
        ...
class ResultFlag:
    """
    Members:
    
      Success
    
      Failure
    
      Uninitialized
    
      ValidationFailure
    
      UNDEFINED_ERROR
    """
    Failure: typing.ClassVar[ResultFlag]  # value = <ResultFlag.Failure: 65535>
    Success: typing.ClassVar[ResultFlag]  # value = <ResultFlag.Success: 0>
    UNDEFINED_ERROR: typing.ClassVar[ResultFlag]  # value = <ResultFlag.UNDEFINED_ERROR: 4294967295>
    Uninitialized: typing.ClassVar[ResultFlag]  # value = <ResultFlag.Uninitialized: 1>
    ValidationFailure: typing.ClassVar[ResultFlag]  # value = <ResultFlag.ValidationFailure: 2>
    __members__: typing.ClassVar[dict[str, ResultFlag]]  # value = {'Success': <ResultFlag.Success: 0>, 'Failure': <ResultFlag.Failure: 65535>, 'Uninitialized': <ResultFlag.Uninitialized: 1>, 'ValidationFailure': <ResultFlag.ValidationFailure: 2>, 'UNDEFINED_ERROR': <ResultFlag.UNDEFINED_ERROR: 4294967295>}
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
class Version:
    """
    Iris version (major, minor, and build number)
    """
    def __repr__(self) -> str:
        ...
    @property
    def build(self) -> int:
        ...
    @property
    def major(self) -> int:
        ...
    @property
    def minor(self) -> int:
        ...
