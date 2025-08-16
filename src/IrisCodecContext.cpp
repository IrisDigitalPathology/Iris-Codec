//
//  IrisCodecContext.cpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//
#include <sstream>
#include "IrisCodecPriv.hpp"
#include "IrisCoreVulkan.hpp"
#include <png.h>
#include <turbojpeg.h>
#include <avif/avif.h>
static const avifRGBImage AVIF_RGB_BLANK_IMAGE {
    .width              = TILE_PIX_LENGTH,
    .height             = TILE_PIX_LENGTH,
    .depth              = 0,
    .format             = AVIF_RGB_FORMAT_COUNT,
    .chromaUpsampling   = AVIF_CHROMA_UPSAMPLING_AUTOMATIC,
    .chromaDownsampling = AVIF_CHROMA_DOWNSAMPLING_AUTOMATIC,
    .avoidLibYUV        = AVIF_FALSE,
    .ignoreAlpha        = AVIF_FALSE,
    .alphaPremultiplied = AVIF_FALSE,
    .isFloat            = AVIF_FALSE,
    .maxThreads         = 1,
    .pixels             = NULL,
    .rowBytes           = 0,
};

Iris::Version IrisCodec::get_codec_version() noexcept {
    return {
        .major          = CODEC_MAJOR_VERSION,
        .minor          = CODEC_MINOR_VERSION,
        .build          = CODEC_BUILD_NUMBER,
    };
}
namespace IrisCodec {
Context create_context() noexcept
{
    return create_context({
        .device = nullptr
    });
}
Context create_context(const ContextCreateInfo &info) noexcept
{
    return std::make_shared<__INTERNAL__Context>(info);
}
inline Quality CHECK_QUALITY_BOUNDS (Quality quality)
{
    if (quality > 100) {
        std::cerr << "Quality exceeds the definined maximum of 100\n";
        return 100;
    } return quality;
}
inline int BITS_PER_PIXEL (Format format)
{
    switch (format) {
        case Iris::FORMAT_UNDEFINED:
        std::cerr << "Undefined format provided. Returning 0 bpp\n";
            return 0;
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:       return 3;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:     return 4;
    }
    std::cerr << "Invalid format provided. Returning 0 bpp\n";
    return 0;
}
inline int BIT_DEPTH (Format format)
{
    switch (format) {
            
        case Iris::FORMAT_UNDEFINED:
            std::cerr << "Undefined format provided. Returning 0 bit depth\n";
                return 0;
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8: return 8;
    }
    std::cerr << "Invalid format provided. Returning 0 bit depth\n";
    return 0;
}
inline Subsampling CHECK_SUBSAMPLING (Subsampling subsample)
{
    switch (subsample) {
        case SUBSAMPLE_444:
        case SUBSAMPLE_422:
        case SUBSAMPLE_420:
            return subsample;
    }
    std::cerr << "Invalid subsampling provided, using default";
    return SUBSAMPLE_DEFAULT;
}
inline Format CHECK_PIXEL_FORMAT (Format format)
{
    switch (format) {
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:     return format;
        case Iris::FORMAT_UNDEFINED:    break;
    }
    std::cerr << "Pixel format check failed. Invalid pixel format provided.";
    return FORMAT_UNDEFINED;
}
inline TJSAMP CONVERT_TO_TJSAMP (Subsampling subsample)
{
    switch (subsample) {
        case SUBSAMPLE_444: return TJSAMP_444;
        case SUBSAMPLE_422: return TJSAMP_422;
        case SUBSAMPLE_420: return TJSAMP_420;
    }
    std::cerr << "Invalid subsampling provided, using TJSAMP_444";
    return TJSAMP_444;
}
inline TJPF CONVERT_TO_TJPIXEL_FORMAT(Format format)
{
    switch (format) {
        case Iris::FORMAT_UNDEFINED:
            std::cerr << "FORMAT_UNDEFINED provided, returning TJPF_UNKNOWN";
            return TJPF_UNKNOWN;
            
        case Iris::FORMAT_B8G8R8:       return TJPF_BGR;
        case Iris::FORMAT_R8G8B8:       return TJPF_RGB;
        case Iris::FORMAT_B8G8R8A8:     return TJPF_BGRA;
        case Iris::FORMAT_R8G8B8A8:     return TJPF_RGBA;
    }
    std::cerr << "Invalid format provided, returning TJPF_UNKNOWN";
    return TJPF_UNKNOWN;
}
inline avifPixelFormat CONVERT_TO_AVIF_SAMP (Subsampling subsample)
{
    switch (subsample) {
        case SUBSAMPLE_444: return AVIF_PIXEL_FORMAT_YUV444;
        case SUBSAMPLE_422: return AVIF_PIXEL_FORMAT_YUV422;
        case SUBSAMPLE_420: return AVIF_PIXEL_FORMAT_YUV420;
    }
    std::cerr << "Invalid subsampling provided, using AVIF_PIXEL_FORMAT_YUV444";
    return AVIF_PIXEL_FORMAT_YUV444;
}
inline avifRGBFormat CONVERT_TO_AVIF_RGBFORMAT (Format format)
{
    switch (format) {
        case Iris::FORMAT_UNDEFINED:
            std::cerr << "FORMAT_UNDEFINED provided, returning AVIF_RGB_FORMAT_COUNT";
            return AVIF_RGB_FORMAT_COUNT;
            
        case Iris::FORMAT_B8G8R8:       return AVIF_RGB_FORMAT_BGR;
        case Iris::FORMAT_R8G8B8:       return AVIF_RGB_FORMAT_RGB;
        case Iris::FORMAT_B8G8R8A8:     return AVIF_RGB_FORMAT_BGRA;
        case Iris::FORMAT_R8G8B8A8:     return AVIF_RGB_FORMAT_RGBA;
    }
    std::cerr << "Invalid format provided, returning AVIF_RGB_FORMAT_COUNT";
    return AVIF_RGB_FORMAT_COUNT;
}
//static Buffer CPU_SIMD_CONVERT_FORMAT (const Buffer& src, Format s_format, Format d_format)
//{
//
//}
inline void SIMPLY_COPY (const Buffer& src, Buffer& dst)
{
    memcpy(dst->append(src->size()), src->data(), src->size());
}
static void PNGCBAPI PRINT_PNG_ERROR (png_structp pp, png_const_charp error)
{
    // This should be in the call stack...
    // Throw runtime error to be caught within (DE)COMPRESS_PNG calls
    throw std::runtime_error(error);
}
static void PNGCBAPI PRINT_PNG_WARNING (png_structp pp, png_const_charp warning)
{
    std::cout << "libPNG WARNING: " << warning << "\n";
}
inline Buffer COMPRESS_PNG  (const Buffer &src,
                             Format format,
                             Quality quality,
                             uint32_t width,
                             uint32_t height)
{
    Buffer dst                  = Create_strong_buffer(src->size());
    png_structp __png_encoder   = NULL;
    png_infop   __png_info      = NULL;
    
    try {
        __png_encoder = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                PRINT_PNG_ERROR,
                                                PRINT_PNG_WARNING);
        
        if (__png_encoder == NULL) throw std::runtime_error
            ("Failed to create PNG encoder");
        
        __png_info = png_create_info_struct(__png_encoder);
        if (__png_info == NULL) throw std::runtime_error
            ("Failed to create PNG encode info");
        
        int bit_depth   = -1;
        int color_type  = -1;
        int bpp         = 0;
        switch (format) {
            case Iris::FORMAT_UNDEFINED:
                break;
            case Iris::FORMAT_B8G8R8:
            case Iris::FORMAT_R8G8B8:
                bit_depth   = 8;
                color_type  = PNG_COLOR_TYPE_RGB;
                bpp         = 3;
                break;
            case Iris::FORMAT_B8G8R8A8:
            case Iris::FORMAT_R8G8B8A8:
                bit_depth   = 8;
                color_type  = PNG_COLOR_TYPE_RGBA;
                bpp         = 4;
                break;
        } if (bit_depth < 0 || color_type < 0) throw std::runtime_error
            ("Invalid pixel format supplied.");
        
        // Convert 0->100 quality (AVIF/JPEG) to 0-9uint zlib compression level
        int zlib_lvl = static_cast<int>(round((static_cast<float>(quality)/100.f) * 9.f));
        png_set_compression_level(__png_encoder, zlib_lvl);
        
        // Load a reference to the byte rows into the encoder structure
        // for access to the raw pixel values
        std::vector<png_bytep>row_ptrs(height);
        for (auto row = 0, offset = 0; row < height; ++row, offset+=(width*bpp))
            row_ptrs[row] = (png_bytep)src->data()+offset;
        png_set_rows(__png_encoder, __png_info, row_ptrs.data());
        
        
        // Set the data handling using Iris::Buffer as a source rather than file IO
        // I just use a lambda here for ease instead of ref a callback function.
        // See above WRITE_PNG_CALLBACK fn with required libPNG prototype args
        png_set_write_fn(__png_encoder, (png_voidp)(&dst),[]
                         (png_structp png_ptr, png_bytep src, size_t req_bytes)
        {
            // Get the destination buffer saved in the png_structp
            auto dst = static_cast<Buffer*>(png_get_io_ptr(png_ptr));
            if (!dst) throw std::runtime_error
                ("WRITE_PNG_CALLBACK failed no valid dst buffer provided");
            // Expand and get the pointer to the next available byte in the buffer
            auto dst_ptr = (*dst)->append(req_bytes);
            // Check the Iris Buffer destination. If buffer expansion error, this will be NULL
            if (!dst_ptr) throw std::runtime_error
                ("Invalid destination pointer. Output buffer expansion error");
            // And copy into the buffer
            memcpy(dst_ptr, src, req_bytes);
        }, NULL);
        
        // Write the information about the PNG to the stream
        png_set_IHDR(__png_encoder, __png_info,
                     width, height,
                     bit_depth, color_type,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);
        
        // Write the PNG to the dst buffer
        png_write_png(__png_encoder, __png_info, PNG_TRANSFORM_IDENTITY, NULL);
        
        // Shrink wrap the buffer.
        dst->shrink_to_fit();
        
        // Cleanup
        png_destroy_info_struct(__png_encoder, &__png_info);
        png_destroy_write_struct(&__png_encoder, NULL);
    } catch (std::runtime_error &e) {
        if (__png_info)
            png_destroy_info_struct(__png_encoder, &__png_info);
        if (__png_encoder)
            png_destroy_write_struct(&__png_encoder, NULL);
        std::stringstream log;
        log << "Failed to compress PNG image: "
            << e.what() << "\n";
        throw std::runtime_error(log.str());
    }   return dst; // Place after try-catch to satisfy MSVC
}
inline Buffer DECOMPRESS_PNG (const Buffer &src,
                              Buffer dst_buffer,
                              Format sourceFormat,
                              Format desiredFormat,
                              uint32_t width,
                              uint32_t height)
{
    png_structp __png_decoder   = NULL;
    png_infop   __png_info      = NULL;
    png_infop   __end_info      = NULL;
    
    try {
        if(png_sig_cmp((png_const_bytep)src->data(), 0, 8))
            throw std::runtime_error("byte stream does not contain valid PNG signature");
        
        __png_decoder = png_create_read_struct (PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                PRINT_PNG_ERROR,
                                                PRINT_PNG_WARNING);
        if (__png_decoder == NULL) throw std::runtime_error
            ("Failed to create PNG decider");
        
        __png_info = png_create_info_struct (__png_decoder);
        if (__png_info == NULL) throw std::runtime_error
            ("Failed to create PNG decode info");
        
        __end_info = png_create_info_struct (__png_decoder);
        if (__end_info == NULL) throw std::runtime_error
            ("Failed to create PNG decode end info");

        size_t pixel_extent     = width*height;
        size_t bpp              = 0;
        switch (sourceFormat) {
            case Iris::FORMAT_UNDEFINED: break;
            case Iris::FORMAT_B8G8R8:
            case Iris::FORMAT_R8G8B8:
                bpp             = 3;
                break;
            case Iris::FORMAT_B8G8R8A8:
            case Iris::FORMAT_R8G8B8A8:
                bpp             = 4;
                break;
        } if (bpp == 0) throw std::runtime_error
            ("Failed to calculate destination buffer size. Invalid pixel format provided");

        // Create the output image buffer
        if (!dst_buffer || bpp*pixel_extent > dst_buffer->size())
            dst_buffer = Create_strong_buffer(bpp*pixel_extent);
        
        std::vector<png_bytep>row_ptrs(height);
        for (auto row = 0, offset = 0; row < height; ++row, offset+=(width*bpp))
            row_ptrs[row] = (png_bytep)dst_buffer->data()+offset;
        
        // Set the data handling using Iris::Buffer as a source rather than file IO
        // I just use a lambda here for ease instead of ref a callback function.
        // See libPNG manual for required libPNG prototype args
        std::pair<Buffer,Offset> src_stream (src,0);
        png_set_read_fn(__png_decoder, (png_voidp)(&src_stream),[/*I just use Lambda instead*/]
                        (png_structp png_ptr, png_bytep dst, size_t req_bytes){
            auto src_stream = static_cast<std::pair<Buffer,Offset>*>(png_get_io_ptr(png_ptr));
            auto& src       = src_stream->first;
            auto& offset    = src_stream->second;
            if (!src) throw std::runtime_error
                ("READ_PNG_CALLBACK failed no valid source buffer provided");
            if (offset + req_bytes > src->size()) throw std::runtime_error
                ("READ_PNG_CALLBACK failed with more bytes requested than are available");
            memcpy(dst, (char*)src->data()+offset, req_bytes);
            offset += req_bytes;
        });
        png_set_rows(__png_decoder, __png_info, row_ptrs.data());
        
        png_read_png(__png_decoder, __png_info, PNG_TRANSFORM_IDENTITY, NULL);
        png_destroy_read_struct(&__png_decoder, &__png_info, &__end_info);
        
        // TODO: Enable SIMD extension to convert format once stable
        int enable_convert_format = 0; // Cause compiler warning.
//        dst_buffer = Iris::SIMD::convert_format ({
//            .source     = dst_buffer,
//            .initial    = sourceFormat,
//            .desired    = desiredFormat,
//        });
        
    } catch (std::runtime_error &e) {
        if (__png_decoder)
            png_destroy_read_struct(&__png_decoder, &__png_info, &__end_info);
        std::stringstream log;
        log << "Failed to decompress PNG image: "
            << e.what() << "\n";
        throw std::runtime_error(log.str());
    }   return dst_buffer; // Place after try-catch to satisfy MSVC
}
inline Buffer COMPRESS_JPEG (const Buffer &src,
                             Format format,
                             Quality quality,
                             Subsampling subsampling,
                             uint32_t width,
                             uint32_t height)
{
    tjhandle turbo_handle = NULL;
    auto dst = Create_strong_buffer(tjBufSize(width, height,
                                              CONVERT_TO_TJSAMP(subsampling)));
    try {
        size_t size     = dst->capacity();
        auto dst_ptr    = (BYTE*)dst->data();
        turbo_handle    = tj3Init (TJINIT_COMPRESS);
        if (turbo_handle == NULL)
            throw std::runtime_error("Failed to create a TURBO_JPEG Context");
        // Set the desired image quality
        if (tj3Set(turbo_handle, TJPARAM_QUALITY, quality))
            throw std::runtime_error("Failed to configure TURBO_JPEG Context -- " +
                                     std::string(tj3GetErrorStr(turbo_handle)));
        // Set the chromatic image subsampling level
        if (tj3Set(turbo_handle, TJPARAM_SUBSAMP, CONVERT_TO_TJSAMP(subsampling)))
            throw std::runtime_error("Failed to configure TURBO_JPEG Context -- " +
                                     std::string(tj3GetErrorStr(turbo_handle)));
        // Compress the image
        if (tj3Compress8(turbo_handle, static_cast<BYTE*>(src->data()),
                         width, 0, height,
                         CONVERT_TO_TJPIXEL_FORMAT(format),
                         &dst_ptr, &size))
            throw std::runtime_error("TURBO_JPEG failed to compress tile data --" +
                                     std::string(tj3GetErrorStr(turbo_handle)));
        tj3Destroy  (turbo_handle);
        
        // If JPEG_TURBO reallocated the pointer, copy the data out of the new pointer
        // and return that instead. This should never happen...
        if (dst_ptr != dst->data())
            dst = Copy_strong_buffer_from_data(dst_ptr, size);
        // Make sure to update the size of the buffer and return.
        else dst->set_size(size);
        return dst;
    } catch (std::runtime_error &e) {
        if (turbo_handle) tjDestroy(turbo_handle);
        std::stringstream log;
        log << "Failed to compress JPEG tile: "
            << e.what() << "\n";
        throw std::runtime_error(log.str());
        return Buffer();
    }   return Buffer();
}
inline Buffer DECOMPRESS_JPEG (const Buffer &compressed,
                               Buffer dst_buffer,
                               Format desired_format,
                               uint32_t width,
                               uint32_t height)
{
    auto&       src_buffer  = compressed;
    TJPF        format      = CONVERT_TO_TJPIXEL_FORMAT(desired_format);
    tjhandle    tjhandle    = NULL;
    size_t      buffer_size = width*height*BITS_PER_PIXEL(desired_format);

    if (format == TJPF_UNKNOWN || !buffer_size) throw std::runtime_error
        ("DECOMPRESS_JPEG failed due to undefined destination pixel format");
    
    if (!dst_buffer || buffer_size > dst_buffer->capacity())
        dst_buffer = Create_strong_buffer(buffer_size);
    
    try {
        tjhandle = tj3Init(TJINIT_DECOMPRESS);
        tj3Set(tjhandle, TJPARAM_JPEGWIDTH,  width);
        tj3Set(tjhandle, TJPARAM_JPEGHEIGHT, height);
        int result = tj3Decompress8
        (tjhandle, static_cast<const BYTE*>(src_buffer->data()),
         src_buffer->size(),
         static_cast<BYTE*>(dst_buffer->data()),
         0, format);
        
        if (result) throw std::runtime_error
            ("DECOMPRESS_JPEG failed with tj3error " +
             std::string(tj3GetErrorStr(tjhandle)));
        
        dst_buffer->set_size(buffer_size);
        
    } catch (std::runtime_error& error) {
        std::cerr   << "Failed to decompress JPEG tile: "
                    << error.what() << "\n";
        dst_buffer  = NULL;
    }
    
    if (tjhandle) tj3Destroy (tjhandle);
    return dst_buffer;
}
inline Buffer COMPRESS_AVIF_CPU (const Buffer &src_buffer,
                                 Format format,
                                 Quality quality,
                                 Subsampling subsampling,
                                 uint32_t width,
                                 uint32_t height)
{
    Buffer          dst_buffer  = nullptr;
    avifImage*      image       = nullptr;
    avifEncoder*    encoder     = NULL;
    avifRWData      avifOutput  = AVIF_DATA_EMPTY;
    avifRGBImage    rgb         = AVIF_RGB_BLANK_IMAGE;
    
    try {
        image                   = avifImageCreate
        (width, height,
         BIT_DEPTH(format),
         CONVERT_TO_AVIF_SAMP(subsampling));
        
        if (!image) throw std::runtime_error
            ("failed to create AVIF dst image");
        
        avifRGBImageSetDefaults(&rgb, image);
        rgb.ignoreAlpha = true;
        rgb.format      = CONVERT_TO_AVIF_RGBFORMAT(format);
        rgb.maxThreads  = 1;
        rgb.pixels      = (uint8_t*)src_buffer->data();
        rgb.rowBytes    = width * BITS_PER_PIXEL(format);
        
        auto result = avifImageRGBToYUV(image, &rgb);
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("failed to convert to RGB image YUV -- "+
             std::string(avifResultToString(result)));
        
        encoder = avifEncoderCreate();
        if (encoder == NULL) throw std::runtime_error
            ("Failed to create AVIF encoder");
        encoder->maxThreads = 1;
        encoder->quality    = (int)quality;
        encoder->speed      = 10;
        
        result = avifEncoderAddImage(encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("failed to add image to encoder: --"+
             std::string(avifResultToString(result)));
        
        
        result = avifEncoderFinish(encoder, &avifOutput);
        if (result != AVIF_RESULT_OK)throw std::runtime_error
            ("Failed to finish encode: "+
             std::string(avifResultToString(result)));
        
        
        // Transfer control of the data to an Iris buffer
        dst_buffer = Wrap_weak_buffer_fom_data(avifOutput.data, avifOutput.size);
        dst_buffer->change_strength(REFERENCE_STRONG);
        avifOutput = AVIF_DATA_EMPTY;
        
    } catch (std::runtime_error& error) {
        std::cerr   << "Failed to compress AVIF tile: "
                    << error.what() << "\n";
        dst_buffer = NULL;
    }

    if (image)   avifImageDestroy   (image);
    if (encoder) avifEncoderDestroy (encoder);
    avifRWDataFree (&avifOutput);
    return dst_buffer;
}
inline Buffer DECOMPRESS_AVIF_CPU (const Buffer &compressed,
                                   Buffer dst_buffer,
                                   Format desired_format,
                                   uint32_t width,
                                   uint32_t height)
{
    auto&           src_buffer  = compressed;
    avifDecoder*    decoder     = NULL;
    size_t          buffer_size = width*height*BITS_PER_PIXEL(desired_format);
    
    // Reallocate buffer if insufficient space provided
    if (!dst_buffer || dst_buffer->size() < buffer_size)
        dst_buffer = Create_strong_buffer (buffer_size);
    
    try {
        avifRGBImage rgb    = AVIF_RGB_BLANK_IMAGE;
        rgb.format          = CONVERT_TO_AVIF_RGBFORMAT(desired_format);
        rgb.rowBytes        = width * BITS_PER_PIXEL(desired_format);
        rgb.depth           = BIT_DEPTH(desired_format);
        rgb.pixels          = (uint8_t*)dst_buffer->data();
        
        if (rgb.format == AVIF_RGB_FORMAT_COUNT || !buffer_size) throw std::runtime_error
            ("Failed due to undefined destination pixel format");

        decoder = avifDecoderCreate();
        if (decoder == NULL) throw std::runtime_error
            ("Failed to create AVIF encoder");
        decoder->maxThreads = 1;
        decoder->imageDimensionLimit = TILE_PIX_LENGTH;
        
        avifResult result = avifDecoderSetIOMemory
        (decoder, (uint8_t*)src_buffer->data(), src_buffer->size());
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("Failed to set memory IO for AVIF decoder -- "+
             std::string(avifResultToString(result)));
        
        result = avifDecoderParse(decoder);
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("Failed to decode AVIF tile -- "+
             std::string(avifResultToString(result)));
        
        result = avifDecoderNextImage(decoder);
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("Failed to read AVIF tile -- "+
             std::string(avifResultToString(result)));
        
        result = avifImageYUVToRGB(decoder->image, &rgb);
        if (result != AVIF_RESULT_OK) throw std::runtime_error
            ("Failed to convert YUV formatted tile -- "+
             std::string(avifResultToString(result)));
        
    } catch (std::runtime_error& error) {
        std::cerr   << "DECOMPRESS_AVIF_CPU error: "
                    << error.what() << "\n";
        dst_buffer = NULL;
    }
    if (decoder) avifDecoderDestroy(decoder);
    return dst_buffer;
}
__INTERNAL__Context::__INTERNAL__Context    (const ContextCreateInfo& info) :
_device                                     (nullptr)
{
    
}
__INTERNAL__Context::~__INTERNAL__Context ()
{
    
}
Buffer __INTERNAL__Context::compress_tile(const CompressTileInfo &info) const
{
    switch (info.encoding) {
        case TILE_ENCODING_UNDEFINED:
            throw std::runtime_error("Encoding format in CompressTileInfo is undefined");
            return Buffer();
        case TILE_ENCODING_JPEG:
            return COMPRESS_JPEG        (info.pixelArray,
                                         info.format,
                                         info.quality,
                                         info.subsampling,
                                         TILE_PIX_LENGTH,
                                         TILE_PIX_LENGTH);
        case TILE_ENCODING_AVIF:
            if (_gpuAV1Encode) {
                assert(false && "HARDWARE ENCODER AV1 IMPLEMENTATION NOT YET BUILT");
            } return COMPRESS_AVIF_CPU (info.pixelArray,
                                        info.format,
                                        info.quality,
                                        info.subsampling,
                                        TILE_PIX_LENGTH,
                                        TILE_PIX_LENGTH);
            break;
        case TILE_ENCODING_IRIS:
            assert(false && "IMPLEMENTATION NOT YET BUILT");
            break;
    } throw std::runtime_error("compress_tile failed with nonsense encoding format in CompressTileInfo");
}
Buffer __INTERNAL__Context::decompress_tile(const DecompressTileInfo &info) const
{
    if (info.compressed == NULL) throw std::runtime_error
        ("Cannot decompress tile without a valid compressed source buffer");
    switch (info.encoding) {
        case TILE_ENCODING_UNDEFINED:
            throw std::runtime_error("Encoding format in DecompressTileInfo is undefined");
        case TILE_ENCODING_JPEG:
            return DECOMPRESS_JPEG      (info.compressed,
                                         info.optionalDestination,
                                         info.desiredFormat,
                                         TILE_PIX_LENGTH,
                                         TILE_PIX_LENGTH);
        case TILE_ENCODING_AVIF:
            if (_gpuAV1Decode) {
                assert(false && "HARDWARE ENCODER AV1 IMPLEMENTATION NOT YET BUILT");
            } return DECOMPRESS_AVIF_CPU (info.compressed,
                                          info.optionalDestination,
                                          info.desiredFormat,
                                          TILE_PIX_LENGTH,
                                          TILE_PIX_LENGTH);
        case TILE_ENCODING_IRIS:
            assert(false && "IMPLEMENTATION NOT YET BUILT");
            break;
    } throw std::runtime_error("decompress_tile failed with nonsense encoding format in DecompressTileInfo");
}
Buffer __INTERNAL__Context::compress_image(const CompressImageInfo &info) const
{
    switch (info.encoding) {
        case IMAGE_ENCODING_UNDEFINED:
            throw std::runtime_error("Encoding format in CompressImageInfo is undefined");
        case IMAGE_ENCODING_PNG:
            return COMPRESS_PNG         (info.pixelArray,
                                         info.format,
                                         info.quality,
                                         info.width,
                                         info.height);
            
            break;
        case IMAGE_ENCODING_JPEG:
            return COMPRESS_JPEG        (info.pixelArray,
                                         info.format,
                                         info.quality,
                                         info.subsampling,
                                         info.width,
                                         info.height);
            
        case IMAGE_ENCODING_AVIF:
            if (_gpuAV1Decode) {
                assert(false && "HARDWARE ENCODER AV1 IMPLEMENTATION NOT YET BUILT");
            }
            return COMPRESS_AVIF_CPU    (info.pixelArray,
                                         info.format,
                                         info.quality,
                                         info.subsampling,
                                         info.width,
                                         info.height);
    } throw std::runtime_error("compress_image failed with nonsense encoding format in CompressImageInfo");
}
Buffer __INTERNAL__Context::decompress_image(const DecompressImageInfo &info) const
{
    switch (info.encoding) {
        case IMAGE_ENCODING_UNDEFINED:
            throw std::runtime_error("Encoding format in DecompressImageInfo is undefined");
        case IMAGE_ENCODING_PNG:
            return DECOMPRESS_PNG       (info.compressed,
                                         info.optionalDestination,
                                         info.sourceFormat,
                                         info.desiredFormat,
                                         info.width,
                                         info.height);
            
        case IMAGE_ENCODING_JPEG:
            return DECOMPRESS_JPEG      (info.compressed,
                                         info.optionalDestination,
                                         info.desiredFormat,
                                         info.width,
                                         info.height);
            
        case IMAGE_ENCODING_AVIF:
            if (_gpuAV1Decode) {
                assert(false && "HARDWARE ENCODER AV1 IMPLEMENTATION NOT YET BUILT");
            } return DECOMPRESS_AVIF_CPU (info.compressed,
                                          info.optionalDestination,
                                          info.desiredFormat,
                                          info.width,
                                          info.height);
    } throw std::runtime_error("decompress_tile failed with nonsense encoding format in DecompressImageInfo");
}
} // END IRIS CODEC NAMESPACE
