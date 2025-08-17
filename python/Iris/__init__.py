# Import the core Iris C++ extension 
try:
    from .iris_core import *
    # Get version from the codec functionality - access iris_core directly
    from .iris_core import Codec as _codec_module
    version = _codec_module.get_codec_version()
    __version__ = f'{version.major}.{version.minor}.{version.build}'
    _core_available = True
except ImportError as e:
    # Fallback if core module can't be imported
    print(f"Warning: Could not import iris_core module: {e}")
    print("Note: This may be due to a Python version mismatch or missing dependencies.")
    print("Please ensure you have the correct extension for your Python version.")
    __version__ = "unknown"
    _core_available = False

# Re-export main functionality from core module (if available)
if _core_available:
    __all__ = [
        'Buffer', 'BufferStrength', 'Extent', 'Format', 'LayerExtent', 'Result', 'ResultFlag', 'Version'
    ]
else:
    __all__ = []

# Make submodules available as attributes for convenience (Iris.Codec, Iris.Encoder)
# while still allowing explicit imports (import Iris.Codec)

def __getattr__(name):
    """Provide convenient access to submodules"""
    if not _core_available:
        raise ImportError(f"Cannot access {name} because core iris_core module failed to load. Please ensure you have the correct extension for your Python version.")
    
    if name == 'Codec':
        # Import and cache the module
        import importlib
        codec_module = importlib.import_module('.Codec', package=__name__)
        globals()[name] = codec_module
        return codec_module
    elif name == 'Encoder':
        # Import and cache the module
        import importlib
        encoder_module = importlib.import_module('.Encoder', package=__name__)
        globals()[name] = encoder_module
        return encoder_module
    else:
        raise AttributeError(f"module '{__name__}' has no attribute '{name}'")