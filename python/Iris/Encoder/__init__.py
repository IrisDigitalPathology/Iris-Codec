from __future__ import absolute_import
import os, platform, sys, time, multiprocessing

# Import core types from parent Iris module FIRST to register types
from .. import iris_core
from ..iris_core import Result, Format

# Import codec functionality 
from .. import Codec
from ..Codec import Context

# Import all encoder functionality from the renamed C++ extension
# This must come AFTER the core types are imported to avoid type registration issues
from .encoder import *

__all__ = ['encode_slide_file']

def encode_slide_file(source: str, 
                      outdir: str = '', 
                      desired_encoding: Codec.Encoding = Codec.Encoding.TILE_ENCODING_JPEG, 
                      desired_byte_format: Format = Format.FORMAT_R8G8B8, 
                      strip_metadata: bool = False, 
                      anonymize: bool = False, 
                      derivation: EncoderDerivation = EncoderDerivation.layer_2x, 
                      concurrency: int = None, 
                      codec_context: Context = None) -> Result:
    encoder = None
    
    # Default concurrency to CPU count if not specified
    if concurrency is None:
        concurrency = multiprocessing.cpu_count()
    
    try: 
        # Check the source file to see if it is a valid OS file path
        if (os.path.isfile(source) == False):
            raise Exception(f"Invalid slide source path provided: {source}")
        
        # Check the destination directory path and generate the output file
        if (os.path.isdir(outdir) == False):
            outdir = os.path.dirname(source)
            print(f"No destination or invalid directory provided. Selecting source directory instead: {outdir}")
        
        # Create the Iris Codec Encoder
        encoder = create_encoder(source, outdir, desired_encoding, desired_byte_format, anonymize, derivation, concurrency, codec_context)
        if (encoder == None): 
            raise Exception("Failed to create an Iris Encoder")
        
        # Dispatch the encoder
        result = dispatch_encoder(encoder)
        if (result.success() == False): 
            raise Exception(f"Failed to dispatch encoder: {result.message}")

        result, progress = get_encoder_progress(encoder)

        # Try and make the loading progress bar 50% of width. Revert to 40
        try:    bar_width = int(os.get_terminal_size().columns / 2)
        except: bar_width = 40

        # Monitor the progress of the encoding
        start_time = time.time()
        prog       = 0.001
        while (progress.status == EncoderStatus.ENCODER_ACTIVE):
            result, progress = get_encoder_progress(encoder)
            if (result.success() == False):
                raise Exception(f'Error during encoding progress check: {result.message}')
            
            # Get the current encoding progress, avoiding div/0 faults
            # Then calculate the remaining time (ETA) and write the progress bar to stdout
            prog        = progress.progress if progress.progress > prog else prog 
            duration    = time.time()-start_time
            mins, sec   = divmod(duration/prog - duration, 60)
            time_str    = f"{int(mins):02}:{sec:02.0f}"
            print(f"\r[{u'█'*int(prog*bar_width)}{'.'*int((1-prog)*bar_width)}] {prog*100:.1f}% ETA: {time_str}", end='', file=sys.stdout, flush=True)
            time.sleep(1)
        print ("\n", file=sys.stdout, flush=True)

        result, progress = get_encoder_progress(encoder)
        if (result.success() == False):
            raise Exception (f'Error during final progress check {result.message}')
        elif (progress.status == EncoderStatus.ENCODER_ERROR):
            raise Exception (f'Error during final progress check {progress.errorMsg}')
        else: print(f'Iris Encoder completed successfully\nSlide written to {outdir+"/"+os.path.splitext(os.path.basename(source))[0]+".iris"}')
    except Exception as e:
        if (encoder): interrupt_encoder(encoder)
        print(f"Encoder raised exception: {e}")



    
    
    



    
