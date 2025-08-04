//
//  IrisCodecEncoder.cpp
//  Iris
//
//  Created by Ryan Landvater on 8/2/22.
//
#include <cmath>
#include <filesystem>
#include "IrisCodecPriv.hpp"

namespace IrisCodec {
inline void CHECK_ENCODER (const Encoder& encoder) {
    if (!encoder)               throw std::runtime_error ("No valid encoder provided");
}
inline void CHECK_MUTABLE (const Encoder& encoder) {
    CHECK_ENCODER(encoder);
    switch (encoder->get_status()) {
        case ENCODER_INACTIVE:  return;
        case ENCODER_ACTIVE:    throw std::runtime_error("Encoder currently active and thus immutable");
        case ENCODER_ERROR:     throw std::runtime_error("Encoder failed in error and must be reset first.");
        case ENCODER_SHUTDOWN:  throw std::runtime_error("Encoder undergoing destruction and thus immutable");
    }
}
inline std::string TO_STRING (EncoderStatus status)
{
    switch (status) {
        case ENCODER_INACTIVE:  return "ENCODER_INACTIVE";
        case ENCODER_ACTIVE:    return "ENCODER_ACTIVE";
        case ENCODER_ERROR:     return "ENCODER_ERROR";
        case ENCODER_SHUTDOWN:  return "ENCODER_SHUTDOWN";
    }   return "CORRUPT ENCODER STATUS IDENTIFIED";
}
Encoder create_encoder(EncodeSlideInfo &info) noexcept
{
    try {
        // Check the context status; If no context, create one.
        Context& context = info.context;
        if (context == nullptr) {
            ContextCreateInfo context_info {nullptr};
            context = std::make_shared<__INTERNAL__Context>(context_info);
        } if (context == nullptr)
            throw std::runtime_error("No valid context created or available");
        
        // Set the destination file directory to the source directory
        // if it does not exist
        std::filesystem::path source_file_path = info.srcFilePath;
        auto source_name = source_file_path.filename();
        auto source_dir  = source_file_path.parent_path();
        
        if (std::filesystem::exists(source_file_path) == false)
            throw std::runtime_error("source slide file "+info.srcFilePath+" does not exist");
        
        if (info.dstFilePath.size() == 0 ||
            std::filesystem::is_directory(info.dstFilePath) == false)
            info.dstFilePath = source_dir.string();
        
        // Return the newly constructed encoder object
        return std::make_shared<__INTERNAL__Encoder>(info);
        
    } catch (std::runtime_error&e) {
        std::cerr   << "Failed to create a valid slide encoder: "
        << e.what() << "\n";
        return NULL;
    }   return NULL;
}
Result reset_encoder(Encoder &encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->reset_encoder();
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
Result dispatch_encoder (const Encoder& encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->dispatch_encoder();
    } catch (std::runtime_error& e) {
        encoder->reset_encoder();
        return Iris::Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result interrupt_encoder(const Encoder & encoder) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->interrupt_encoder();
    } catch (std::runtime_error&e) {
        return Iris::Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_progress (const Encoder &encoder, EncoderProgress &progress) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        return encoder->get_encoder_progress(progress);
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_src(const Encoder &encoder, std::string &src_string) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        src_string = encoder->get_src_path();
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result get_encoder_dst_path(const Encoder &encoder, std::string &dst_string) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        dst_string = encoder->get_dst_path();
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return Result {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result set_encoder_src(const Encoder &encoder, const std::string &source_path) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        CHECK_MUTABLE(encoder);
        encoder->set_src_path(source_path);
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
Result set_encoder_dst_path(const Encoder &encoder, const std::string &dst_path) noexcept
{
    try {
        CHECK_ENCODER(encoder);
        CHECK_MUTABLE(encoder);
        encoder->set_dst_path(dst_path);
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    }
}
__INTERNAL__Encoder::__INTERNAL__Encoder    (const EncodeSlideInfo& __i) :
_derive                                     (__i.derviation),
_context                                    (__i.context),
_srcPath                                    (__i.srcFilePath),
_dstPath                                    (__i.dstFilePath),
_encoding                                   (__i.desiredEncoding),
_derivation                                 (_derive?*__i.derviation:EncoderDerivation()),
_status                                     (ENCODER_INACTIVE)
{
    
}
__INTERNAL__Encoder::~__INTERNAL__Encoder   ()
{
    while (_status == ENCODER_ACTIVE) {
        _status.wait(ENCODER_ACTIVE);
        auto STATUS = ENCODER_INACTIVE;
        _status.compare_exchange_strong(STATUS, ENCODER_SHUTDOWN);
        switch (STATUS) {
            case ENCODER_INACTIVE:
                _status = ENCODER_SHUTDOWN;
            case ENCODER_SHUTDOWN:
            case ENCODER_ERROR:
                break;
            case ENCODER_ACTIVE:
                continue;
        }
    } for (auto& thread : _threads)
        if (thread.joinable()) thread.join();
}
// MARK: Getters
EncoderStatus __INTERNAL__Encoder::get_status() const
{
    return _status;
}
Context __INTERNAL__Encoder::get_context() const
{
    return _context;
}
std::string __INTERNAL__Encoder::get_src_path() const
{
    return _srcPath;
}
std::string __INTERNAL__Encoder::get_dst_path() const
{
    return _dstPath;
}
Encoding __INTERNAL__Encoder::get_encoding() const {
    return _encoding;
}
Result __INTERNAL__Encoder::get_encoder_progress (EncoderProgress &progress) const
{
    progress.dstFilePath    = _dstPath;
    progress.status         = _status.load();
    switch (progress.status) {
        case ENCODER_ACTIVE:
            if (_tracker.total == 0)
                return {IRIS_FAILURE,"Tracker returned 0 total tiles in slide"};
            progress.progress       = static_cast<float>(_tracker.completed.load())
                                    / static_cast<float>(_tracker.total);
            progress.dstFilePath    = _tracker.dst_path;
            progress.errorMsg       = _tracker.error_msg;
            return IRIS_SUCCESS;
            
        case ENCODER_ERROR: {
            MutexLock __ (const_cast<Mutex&>(_tracker.error_msg_mutex));
            progress.errorMsg       = _tracker.error_msg;
        }
        case ENCODER_INACTIVE:
        case ENCODER_SHUTDOWN:
            return IRIS_SUCCESS;
    }   return IRIS_FAILURE;
}
// MARK: Setters
void __INTERNAL__Encoder::set_src_path(const std::string &source)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            throw std::runtime_error("Encoder is currently active; cannot change source path");
    }
    _srcPath = source;
}
void __INTERNAL__Encoder::set_dst_path(const std::string &destination)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            throw std::runtime_error("Encoder is currently active; cannot change destination path");
    }
    _dstPath = destination;
}
void __INTERNAL__Encoder::set_encoding(Encoding desired_encoding)
{
    switch (_status) {
        case ENCODER_INACTIVE:break;
        default:
            std::cerr << "Encoder is currently active; cannot change encoding\n";
            return;
    }
    _encoding = desired_encoding;
}
Result __INTERNAL__Encoder::reset_encoder()
{
    switch (_status) {
        case ENCODER_INACTIVE:
        case ENCODER_ACTIVE:
            _status.store(ENCODER_ERROR);
        case ENCODER_ERROR:
            _status.notify_all();
            for (auto& thread : _threads)
                if (thread.joinable()) thread.join();
            break;
            
        case ENCODER_SHUTDOWN:  return {
            IRIS_FAILURE,
            "Cannot reset an encoder in SHUTDOWN"
        };
    }
    
    MutexLock __ (_tracker.error_msg_mutex);
    _tracker.dst_path.clear();
    _tracker.error_msg.clear();
    _tracker.completed = 0;
    _status.store(ENCODER_INACTIVE);
    
    return IRIS_SUCCESS;
}

// MARK: - OPENSLIDE METHODS
#if IRIS_INCLUDE_OPENSLIDE
#include <openslide/openslide.h>
inline Extent READ_EXTENT_OPENSLIDE (openslide_t* openslide)
{
    Extent          extent;
    
    int64_t L0_width, L0_height;
    openslide_get_level0_dimensions(openslide,&L0_width,&L0_height);

    auto n_levels = openslide_get_level_count(openslide);
    int64_t width, height;
    openslide_get_level_dimensions(openslide, n_levels-1, &width, &height);
    extent.width        = U32_CAST(width);
    extent.height       = U32_CAST(height);
    extent.layers       = LayerExtents(n_levels);
    auto extent_IT     = extent.layers.begin();
    auto f_level        = U32_CAST(0);
    auto r_level        = U32_CAST(extent.layers.size() - 1);
    for (; extent_IT  != extent.layers.end(); f_level++, r_level--, extent_IT++) {
        auto& _SL_      = *extent_IT;
        openslide_get_level_dimensions(openslide, r_level, &width, &height);
        _SL_.xTiles      = U32_CAST(std::ceil(F32_CAST(width)/F32_CAST(TILE_PIX_LENGTH)));
        _SL_.yTiles      = U32_CAST(std::ceil(F32_CAST(height)/F32_CAST(TILE_PIX_LENGTH)));
        _SL_.scale       =  width > height ?
        F32_CAST(width)/F32_CAST(extent.width) :
        F32_CAST(height)/F32_CAST(extent.height);
    }
    for (extent_IT = extent.layers.begin(); extent_IT != extent.layers.end(); extent_IT++)
        extent_IT->downsample = extent.layers.back().scale / extent_IT->scale;
    
    return extent;
}
inline Buffer READ_OPENSLIDE_TILE (const EncoderSource src, LayerIndex __LI, TileIndex __TI)
{
    const auto os       = src.openslide;
    if (os == NULL)                                     return NULL;
    const auto extent   = src.extent;
    if (__LI    >= extent.layers.size())                return NULL;
    auto& __LE   = extent.layers[__LI];
    if (__TI    >= __LE.xTiles * __LE.yTiles)           return NULL;
    auto buffer         = Create_strong_buffer  (TILE_PIX_BYTES_RGBA);
    auto& level_extent  = extent.layers[__LI];
    auto openSlideLevel = static_cast<uint32_t> ((extent.layers.size()-1)-__LI);
    auto x_tile_index   = static_cast<float>    (__TI % level_extent.xTiles);
    auto y_tile_index   = static_cast<float>    (__TI / level_extent.xTiles);
    openslide_read_region(os, static_cast<uint32_t*>(buffer->append(TILE_PIX_BYTES_RGBA)),
                          static_cast<int64_t>  (std::round(x_tile_index * TILE_PIX_LENGTH * level_extent.downsample)),
                          static_cast<int64_t>  (std::round(y_tile_index * TILE_PIX_LENGTH * level_extent.downsample)),
                          openSlideLevel,
                          TILE_PIX_LENGTH, TILE_PIX_LENGTH);
    return buffer;
}
enum OpenSlideProperties {
    NOT_OPENSLIDE,
    UNUSED,
    MPP,
    OBJECTIVE_POWER,
    
};
inline OpenSlideProperties PARSE_OPENSLIDE_PROPERTY (const char* const key_chars) {
    if (strstr(key_chars, "openslide") == NULL) return NOT_OPENSLIDE;
    if (strcmp(key_chars, OPENSLIDE_PROPERTY_NAME_MPP_X) == 0) return MPP;
    if (strcmp(key_chars, OPENSLIDE_PROPERTY_NAME_OBJECTIVE_POWER) == 0) return OBJECTIVE_POWER;
    return UNUSED;
}
inline Metadata READ_OPENSLIDE_METADATA (const EncoderSource src) {
    openslide_t* os = src.openslide;
    Metadata metadata;
    
    // Insert the attributes
    metadata.attributes.type = METADATA_FREE_TEXT;
    const char* const* attributes = openslide_get_property_names(os);
    for (int index = 0; attributes[index] != NULL; ++index) {
        switch (PARSE_OPENSLIDE_PROPERTY(attributes[index])) {
            // Unused openslie parameter, probably duplicated elsewhere
            case UNUSED: continue;
            
            // Break and encode as is. It's likely a vendor feature
            case NOT_OPENSLIDE: break;
                
            // MpPN (Normalized MPP to layer scale)
            // We multiply because this value is currently norm
            // relative to the highest resolution layer (ex 1/64x)
            // This will allow for direct comp with on-screen pixels
            // Because viewers work in relative zoom.
            // We want this 1:1 lowest res layer (layer scale = 1)
            case MPP: {
                metadata.micronsPerPixel = round(atof
                (openslide_get_property_value(os, attributes[index])) *
                openslide_get_level_downsample
                (os,openslide_get_level_count(os)-1) * 1000.f)/1000.f;
                // You will note it's rounded to the 1000ths
                continue;
            }
                
            // Normalize the magnification coefficient ratio
            // to the layer scale. This allows for multiplying
            // with relative zoom for direct comp with on-screen pixels
            case OBJECTIVE_POWER:
                metadata.magnification = round(atof
                (openslide_get_property_value(os, attributes[index])) /
                openslide_get_level_downsample
                (os,openslide_get_level_count(os)-1) * 1000.f)/1000.f;
                // You will note it's rounded to the 1000ths
                continue;
        }
        if (auto attribute = openslide_get_property_value(os, attributes[index])) {
            auto __key = std::string(attributes[index]);
            auto __str = std::string(attribute);
            metadata.attributes[__key] = std::u8string(__str.begin(),__str.end());
        }
    }
    
    // Insert the associated image labels
    const char* const* image_labels = openslide_get_associated_image_names(os);
    for (int label = 0; image_labels[label] != NULL; ++label)
        metadata.associatedImages.insert(std::string(image_labels[label]));
    
    // Insert the ICC profile (if present)
    int64_t icc_bytes = openslide_get_icc_profile_size(os);
    if (icc_bytes > 0) {
        metadata.ICC_profile.resize(icc_bytes);
        openslide_read_icc_profile(os, metadata.ICC_profile.data());
    }
    
    return metadata;
}
inline AssociatedImageInfo READ_OPENSLIDE_ASSOCIATED_IMAGE_INFO (openslide_t* os, const std::string& label)
{
    int64_t width, height;
    openslide_get_associated_image_dimensions(os, label.c_str(), &width, &height);
    if (width > UINT32_MAX) throw std::runtime_error
        ("openslide associated image width is greater than 32-bit max value");
    if (height > UINT32_MAX) throw std::runtime_error
        ("openslide associated image height is greater than 32-bit max value");
    return AssociatedImageInfo {
        .imageLabel     = label,
        .width          = static_cast<uint32_t>(width),
        .height         = static_cast<uint32_t>(height),
        .encoding       = IMAGE_ENCODING_DEFAULT,
        .sourceFormat   = Iris::FORMAT_B8G8R8A8, // Openslide is alway ARGB (big-endian)
        .orientation    = ORIENTATION_0
    };
}
inline Buffer READ_OPENSLIDE_ASSOCIATED_IMAGE (openslide_t* os, const AssociatedImageInfo& info)
{
    size_t image_size = info.width * info.height * sizeof(uint32_t);
    Buffer dst = Create_strong_buffer(image_size);
    openslide_read_associated_image(os, info.imageLabel.c_str(), static_cast<uint32_t*>(dst->data()));
    dst->set_size(image_size);
    return dst;
}
#endif
// MARK: - APERIO SPECIFIC METHODS

// MARK: - FILE ENCODING METHODS
inline EncoderSource OPEN_SOURCE (std::string& path, const Context context = NULL)
{
    if (!std::filesystem::exists(path)) throw std::runtime_error
        ("File system failed to identify source file " + path);
    
    if (is_iris_codec_file(path)) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_IRISSLIDE;
        source.irisSlide    = open_slide(SlideOpenInfo {
            .filePath       = path,
            .context        = context,
            .writeAccess    = false,
        });
        if (!source.irisSlide) throw std::runtime_error
            ("No valid Iris slide returned from IrisCodec::open_slide");
        
        source.extent       = source.irisSlide->get_slide_info().extent;
        source.format       = source.irisSlide->get_slide_info().format;
            
        return source;
    }
    #if IRIS_INCLUDE_OPENSLIDE
    if (openslide_detect_vendor(path.c_str())) {
        EncoderSource source;
        source.sourceType   = EncoderSource::ENCODER_SRC_OPENSLIDE;
        source.openslide    = openslide_open(path.c_str());
        
        if (!source.openslide) throw std::runtime_error
            ("No valid openslide handle returned from openslide_open");

        source.extent       = READ_EXTENT_OPENSLIDE(source.openslide);
        source.format       = FORMAT_B8G8R8A8; // OpenSlide always reads ARGB
        
        return source;
    }
    throw std::runtime_error("Provided source file path was not recognized by any available decoders.");
    #else
    throw std::runtime_error("Provided source file path was not recognized by any available decoders. You may need an encoder built with OpenSlide enabled.");
    #endif
   
}
inline BYTE* FILE_CHECK_EXPAND (const File& file, size_t required_size)
{
    if (required_size > file->size) {
        auto result     = resize_file(file, {
            .size       = required_size,
            .pageAlign  = false,
        });
        if (result != IRIS_SUCCESS)
            throw std::runtime_error
            ("Failed to resize slide file "+file->path+": " + result.message);
    } return file->ptr;
}
inline Buffer READ_SOURCE_TILE (const EncoderSource& src, LayerIndex layer, TileIndex tile)
{
    switch (src.sourceType) {
            
        case EncoderSource::ENCODER_SRC_UNDEFINED: throw std::runtime_error("Cannot read source tile; undefined source");
        case EncoderSource::ENCODER_SRC_IRISSLIDE:
            return IrisCodec::read_slide_tile (SlideTileReadInfo{
                .slide          = src.irisSlide,
                .layerIndex     = layer,
                .tileIndex      = tile,
                .desiredFormat  = src.format
        });
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            #if IRIS_INCLUDE_OPENSLIDE
            return READ_OPENSLIDE_TILE (src, layer, tile);
            #else
            throw std::runtime_error("Openslide linkage was NOT compiled into this binary. Request a new version of Iris Codec with OpenSlide support if you would like to decode slide scanning vendor slide files only accessable to OpenSlide.");
            #endif
            
        case EncoderSource::ENCODER_SRC_APERIO:
            throw std::runtime_error("APERIO TIFF reads not yet built; Use openslide for the moment");
    } return Buffer();
}
inline static void ENCODE_SOURCE_PYRAMID (const Context ctx,
                                          const EncoderSource& src,
                                          const File file,
                                          EncoderTracker* _tracker,
                                          Abstraction::TileTable* _table,
                                          atomic_uint64* _offset,
                                          AtomicEncoderStatus* _status)
{
    auto& extent    = src.extent;
    auto& tracker   = *_tracker;
    auto& table     = *_table;
    auto& offset    = *_offset;
    auto& status    = *_status;
    
    // Allocate a layer and tile index counter.
    uint32_t __LI           = 0;
    uint32_t __TI           = 0;
    uint32_t __LAYERS       = U32_CAST(extent.layers.size());
    
    try { for (__LI = 0; __LI < __LAYERS; ++__LI) {
        auto& layer_tracker = tracker.layers[__LI];
        auto& layer_table   = table.layers[__LI];
        for (__TI = 0; __TI < layer_tracker.size(); ++__TI) {
            // System check step: Only continue if the encoder is active
            if (status != ENCODER_ACTIVE) return;
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  CAPTURE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto& tile  = layer_tracker[__TI];
            auto STATUS = TILE_FREE;
            if (tile.status.compare_exchange_strong(STATUS, TILE_READING)==false)
                continue;
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  READ TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto pixel_array = READ_SOURCE_TILE(src, __LI, __TI);
            if (!pixel_array) throw std::runtime_error("Failed to read slide image data");
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  COMPRESS PIXEL ARRAY STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto bytes      = ctx->compress_tile({
                .pixelArray = pixel_array,
                .format     = src.format,
                .encoding   = table.encoding
            });
            if (!bytes) throw std::runtime_error("Failed to compress slide image data");
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  WRITE TO FILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            auto& entry     = layer_table[__TI];
            entry.size      = U32_CAST(bytes->size());
            entry.offset    = offset.fetch_add(entry.size);
            ReadLock shared_write_lock (file->resize);
            tile.status     = TILE_ENCODING;
            if (entry.offset + entry.size > file->size) {
                shared_write_lock.unlock();
                WriteLock resize_lock (file->resize);
                // Expand the file by 500 MB per expansion
                // We will shrink it back down to size at the end.
                auto result = resize_file(file, FileResizeInfo {
                    .size = file->size + (size_t)5E8,
                });
                if (result != IRIS_SUCCESS)
                    throw std::runtime_error("Failed to resize growing tile blocks");
                resize_lock.unlock();
                shared_write_lock.lock();
            }
            auto dst = file->ptr + entry.offset;
            memcpy(dst, bytes->data(), entry.size);
            shared_write_lock.unlock();
            // TODO: Should we file synchronize (POSIX msyc) here? Seems like unecessary overhead
            
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            //  RELEASE TILE STEP
            //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
            tile.status.store(TILE_COMPLETE);
            tracker.completed++;
        }
    }
    } catch (std::runtime_error&e) {
        status.store(ENCODER_ERROR);
        MutexLock __ (tracker.error_msg_mutex);
        tracker.error_msg += std::string("Slide tile encoding failed: ") +
                             e.what() + "\n";
        status.notify_all();
        return;
    }
    
}
inline static void ENCODE_DERIVE_PYRAMID (const Context ctx,
                                          const EncoderSource& src,
                                          const File& file,
                                          EncoderTracker* _tracker,
                                          AtomicEncoderStatus* _status,
                                          const std::function <void(uint32_t layer_index,
                                                                    uint32_t y_index,
                                                                    uint32_t x_index)>
                                          & ENQUEUE_TILE)
{
    const auto& src_extent   = src.extent;
    const auto& layer_extent = src_extent.layers.back();
    auto& layer_tracker      = _tracker->layers.back();
    try {
        uint32_t src_l = U32_CAST(src_extent.layers.size()-1);
        uint32_t dst_l = U32_CAST(_tracker->layers.size()-1);
        for (uint32_t y = 0; y < layer_extent.yTiles; ++y) {
            for (uint32_t x = 0; x < layer_extent.xTiles; ++x) {
                if (_status->load() != ENCODER_ACTIVE) return;
                
                uint32_t __TI = y * layer_extent.xTiles + x;
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  CAPTURE TILE STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                auto& tile  = layer_tracker[__TI];
                auto STATUS = TILE_FREE;
                if (tile.status.compare_exchange_strong(STATUS, TILE_READING)==false)
                    continue;
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  READ TILE STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                tile.pixels = READ_SOURCE_TILE(src, src_l, __TI);
                if (!tile.pixels) throw std::runtime_error("Failed to read slide image data");
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                //  PROPOGATE TILE ENCODING STEP
                //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
                tile.status.store(TILE_PENDING);
                ENQUEUE_TILE (dst_l,y,x);
            }
        }
    } catch (std::runtime_error&e) {
        _status->store(ENCODER_ERROR);
        MutexLock __ (_tracker->error_msg_mutex);
        _tracker->error_msg += std::string("Slide tile encoding failed: ") +
                             e.what() + "\n";
        _status->notify_all();
        return;
    }
}
inline void VALIDATE_TILE_WRITES (const EncoderTracker& tracker, const Abstraction::TileTable& table)
{
    if (tracker.layers.size() != table.layers.size())
        throw std::runtime_error("Tile encoder tracker and tile ptr map mismatch. File corruption.");
    for (int layer_idx = U32_CAST(tracker.layers.size()-1); layer_idx>=0; --layer_idx) {
        auto&& tracker_layer = tracker.layers[layer_idx];
        auto&& table_layer   = table.layers[layer_idx];
        for (auto tile_idx = 0; tile_idx < tracker_layer.size(); ++tile_idx) {
            if (tracker_layer[tile_idx].status != TILE_COMPLETE) {
                std::cout << "[" << layer_idx << ","
                << tile_idx/table.extent.layers[layer_idx].xTiles << ","
                << tile_idx%table.extent.layers[layer_idx].xTiles << "] "
                << tile_idx << " ("
                << tracker_layer[tile_idx].subtile<< ")\n";
                continue;
            }
            
        }
    }
    for (auto layer_idx = 0; layer_idx < tracker.layers.size(); ++layer_idx) {
        auto&& tracker_layer = tracker.layers[layer_idx];
        auto&& table_layer   = table.layers[layer_idx];
        if (tracker_layer.size() != table_layer.size())
            throw std::runtime_error
            ("Tile encoder tracker and tile ptr map mismatch for layer " +
             std::to_string(layer_idx) + ". File corruption.\n" +
             "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
        for (auto tile_idx = 0; tile_idx < tracker_layer.size(); ++tile_idx) {
            if (tracker_layer[tile_idx].status != TILE_COMPLETE)
                throw std::runtime_error
                ("Layer" + std::to_string(layer_idx) + ", tile " + std::to_string(tile_idx) +
                 "was marked as incompletely encoded. Encoding incomplete.\n" +
                 "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
            auto&& tile = table_layer[tile_idx];
            if (tile.offset == NULL_OFFSET || tile.size == 0)
                throw std::runtime_error
                ("Layer" + std::to_string(layer_idx) + ", tile " + std::to_string(tile_idx) +
                 "contained invalid size or mapped file ptr identified in the table. File corruption.\n" +
                 "Place a breakpoint in"+__FILE__+"at line " +std::to_string(__LINE__));
        }
    }
}
inline Offset STORE_TILE_TABLE (const File& file, const Abstraction::TileTable& table, atomic_uint64& offset)
{
    // Perform checks here
    if (table.layers.size() != table.extent.layers.size())
        throw std::runtime_error("Failure in tile ptr encoding; table does not match slide extent.");
    
    uint32_t    n_layers    = U32_CAST(table.layers.size());
    auto        __base      = file->ptr;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // SLIDE TILE ARRAY SERIALIZATION
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto    tiles_size      = Serialization::SIZE_TILE_OFFSETS(table.layers);
    Offset  tiles_offset    = offset.fetch_add(tiles_size);
    __base                  = FILE_CHECK_EXPAND(file, offset); // Always check bounds before writing
    Serialization::STORE_TILE_OFFSETS (__base, tiles_offset, table.layers);
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // SLIDE TILE EXTENT SERIALIZATION
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Write Iris::Extent to file
    // This must be written backwards as an array of layer extents
    auto   l_extents_size   = Serialization::SIZE_EXTENTS(table.extent.layers);
    Offset l_extents_offset = offset.fetch_add(l_extents_size);
    __base                  = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_EXTENTS (__base, l_extents_offset, table.extent.layers);
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // WRITE THE TILE TABLE HEADER
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //
    auto    ttable_size     = Serialization::TILE_TABLE::HEADER_SIZE;
    Offset  ttable_offset   = offset.fetch_add(ttable_size);
    __base                  = FILE_CHECK_EXPAND(file, offset);
    Serialization::TileTableCreateInfo tile_table_create_info {
        .tileTableOffset    = ttable_offset,
        .encoding           = table.encoding,
        .format             = table.format,
        .tilesOffset        = tiles_offset,
        .layerExtentsOffset = l_extents_offset,
        .layers             = n_layers,
        .widthPixels        = table.extent.width,
        .heightPixels       = table.extent.height,
    };
    Serialization::STORE_TILE_TABLE (__base, tile_table_create_info);
    
    // Return the Tile Table Offset
    return ttable_offset;
}
inline Offset RESERVE_METADATA (const File& file, atomic_uint64& offset)
{
    auto    metadata_size   = Serialization::METADATA::HEADER_SIZE;
    Offset  metadata_offset = offset.fetch_add(metadata_size);
    FILE_CHECK_EXPAND(file, offset);
    return  metadata_offset;
}
inline Metadata READ_METADATA (const EncoderSource& source) {
    switch (source.sourceType) {
        case EncoderSource::ENCODER_SRC_UNDEFINED:
            throw std::runtime_error
            ("READ_METADATA failed due to ENCODER_SRC_UNDEFINED source type");
        case EncoderSource::ENCODER_SRC_IRISSLIDE:
            return source.irisSlide->get_slide_info().metadata;
        case EncoderSource::ENCODER_SRC_OPENSLIDE:
            return READ_OPENSLIDE_METADATA(source);
        case EncoderSource::ENCODER_SRC_APERIO:
            //TODO: APERIO READ METADATA
            throw std::runtime_error
            ("READ_METADATA failed as APERIO TIFF reads not yet built; Use openslide for the moment");
    } throw std::runtime_error
    ("READ_METADATA due to invalid source type value ("+std::to_string(source.sourceType)+")");
}
inline Offset STORE_ICC (const File& file,
                         const Metadata& metadata,
                         atomic_uint64& offset)
{
    // If there is no embedded ICC profile, return NULL_OFFSET
    // to indicate this optional block will not be used.
    if (metadata.ICC_profile.size() == 0) return NULL_OFFSET;
    
    // Get bytes size and write ICC profile to byte stream
    Size profile_size       = Serialization::SIZE_ICC_COLOR_PROFILE(metadata.ICC_profile);
    Offset profile_offset   = offset.fetch_add(profile_size);
    auto   __base           = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_ICC_COLOR_PROFILE(__base, profile_offset, metadata.ICC_profile);
    
    // Return the byte location of the ICC profile
    return profile_offset;
}
inline Offset STORE_ASSOCIATED_IMAGES (const Context& ctx,
                                       const File& file,
                                       const EncoderSource& source,
                                       const Metadata& metadata,
                                       atomic_uint64& offset)
{
    // If there are no associated images, return NULL_OFFSET
    // to indicate this optional block will not be used.
    if (metadata.associatedImages.size() == 0) return NULL_OFFSET;
    
    // Otherwise begin encoding images
    Serialization::AssociatedImageCreateInfo associated_images;
    
    // Encode each of the associated image variable byte blocks
    // This corresponds with IFE Specification Section 2.4.7
    for (auto& label : metadata.associatedImages) {
        AssociatedImageInfo info;
        Buffer bytes;
        try {
            // Get the compressed image stream (bytes) and image info (info)
            // routine based upon source type
            switch (source.sourceType) {
                case EncoderSource::ENCODER_SRC_UNDEFINED:
                    throw std::runtime_error("STORE_METADATA failed due to undefined source type");
                case EncoderSource::ENCODER_SRC_IRISSLIDE:
                    info    = source.irisSlide->get_assoc_image_info(label);
                    bytes   = source.irisSlide->get_assoc_image(label);
                    break;
                case EncoderSource::ENCODER_SRC_OPENSLIDE:
                    info    = READ_OPENSLIDE_ASSOCIATED_IMAGE_INFO(source.openslide, label);
                    bytes   = ctx->compress_image(CompressImageInfo{
                        .pixelArray = READ_OPENSLIDE_ASSOCIATED_IMAGE(source.openslide, info),
                        .width      = info.width,
                        .height     = info.height,
                        .format     = info.sourceFormat,
                        .encoding   = info.encoding,
                        .quality    = QUALITY_DEFAULT
                    });
                    break;
                case EncoderSource::ENCODER_SRC_APERIO:
                    //TODO: APERIO READ METADATA
                    throw std::runtime_error("READ_METADATA failed as APERIO TIFF reads not yet built; Use openslide for the moment");
            }
            if (bytes->size() == 0) throw std::runtime_error
                ("no bytes given for image buffer byte size");
            
            // Store the data-block 1) get size 2) write to the stream 3) record location
            Size block_size         = Serialization::SIZE_IMAGES_BYTES({
                .title              = label,
                .dataBytes          = bytes->size()
            });
            Offset block_offset     = offset.fetch_add(block_size);
            auto   __base           = FILE_CHECK_EXPAND(file, offset);
            Serialization::STORE_IMAGES_BYTES(__base, {
                .offset             = block_offset,
                .title              = label,
                .data               = (BYTE*)bytes->data(),
                .dataBytes          = bytes->size()
            });
            associated_images.images.push_back({
                .offset             = block_offset,
                .info               = info
            });
            
        } catch (std::runtime_error &error) {
            std::cout   << "Failed to store associated image labeled \""
            << label << "\": " << error.what() << "\n";
            continue;
        }
    }
    
    // Now record of all associated images to the byte stream
    Size   images_size              = Serialization::SIZE_IMAGES_ARRAY(associated_images);
    Offset images_offset            = offset.fetch_add(images_size);
    associated_images.offset        = images_offset;
    auto   __base                   = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_IMAGES_ARRAY(__base, associated_images);
    
    // Return the images array offset
    return images_offset;
}
inline Offset STORE_ATTRIBUTES (const File& file,
                                const Metadata& metadata,
                                atomic_uint64& offset)
{
    // If there are no attributes, return NULL_OFFSET
    // to indicate this optional block will not be used.
    const auto& attributes = metadata.attributes;
    if (attributes.size() == 0) return NULL_OFFSET;
    
    // Attributes validation
    switch (attributes.type) {
        case METADATA_UNDEFINED:throw std::runtime_error
            ("Metadata attributes have an undefined type. These will NOT be written to the file stream.");
            
        case METADATA_I2S:
            if (!attributes.version) {
                // Freetext metadata
                break;
            }
            // TODO: Add I2S here. Add I2S Validation here
            break;
        case METADATA_DICOM:
            
            // TODO: Add libDICOM validation here.
            break;
    }
    
    // Attributes are stored in 3 data-blocks
    // 1) Sizes
    // 2) Bytes
    // 3) Attributes header
    
    // Store the attributes sizes (how to slice up the char byte blob)
    // Sections 2.2.4
    Size sizes_size     = Serialization::SIZE_ATTRIBUTES_SIZES(attributes);
    Offset sizes_offset = offset.fetch_add(sizes_size);
    auto  __base        = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_ATTRIBUTES_SIZES(__base, sizes_offset, attributes);
    
    // Store the raw attributes characters byte-blob
    // Section 2.2.5
    Size bytes_size     = Serialization::SIZE_ATTRIBUTES_BYTES(attributes);
    Offset bytes_offset = offset.fetch_add(bytes_size);
    __base              = FILE_CHECK_EXPAND(file, offset);
    Serialization::STORE_ATTRIBUTES_BYTES(__base, bytes_offset, attributes);
    
    // Store the annotation header
    Offset attr_offset  = offset.fetch_add(Serialization::ATTRIBUTES::HEADER_SIZE);
    Serialization::STORE_ATTRIBUTES(__base, {
        .attributesOffset   = attr_offset,
        .type               = attributes.type,
        .version            = attributes.version,
        .sizes              = sizes_offset,
        .bytes              = bytes_offset
    });
    
    return attr_offset;
}
inline void STORE_METADATA (const File& file,
                            const Offset metadata_offset,
                            const Metadata& metadata,
                            const Offset ICC_offset,
                            const Offset images_offset,
                            const Offset attributes_offset,
                            const Offset annotations_offset)
{
    Serialization::STORE_METADATA(file->ptr, {
        .metadataOffset     = metadata_offset,
        .codecVersion       = get_codec_version(),
        .attributes         = attributes_offset,
        .images             = images_offset,
        .ICC_profile        = ICC_offset,
        .annotations        = annotations_offset,
        .micronsPerPixel    = metadata.micronsPerPixel,
        .magnification      = metadata.magnification
    });
}
inline void STORE_FILE_HEADER (const File& file,
                               const Size file_size,
                               const uint32_t revision,
                               const Offset tile_table_offset,
                               const Offset metadata_offset)
{
    if (file->size < file_size) throw std::runtime_error
        ("[ERROR] File failed size check. Attempting to write header for truncated file.");
    Serialization::STORE_FILE_HEADER(file->ptr, {
        .fileSize           = file_size,
        .revision           = revision,
        .tileTableOffset    = tile_table_offset,
        .metadataOffset     = metadata_offset
    });
    resize_file(file, FileResizeInfo {
        .size               = file_size,
        .pageAlign          = false
    });
}
inline void RESET_TRACKER (EncoderTracker &_tracker, const File &file, const Extent &extent) {
    _tracker.dst_path   = file->get_path();
    _tracker.completed  = 0;
    _tracker.total      = 0;
    _tracker.layers     = EncoderTracker::Layers(extent.layers.size());
    for (auto __li = 0; __li < _tracker.layers.size(); ++__li) {
        auto& __le              = extent.layers[__li];
        auto  n_tiles           = __le.xTiles*__le.yTiles;
        _tracker.layers[__li]   = EncoderTracker::Layer(n_tiles);
        _tracker.total         += n_tiles;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~ TILE DERIVATION ~~~~~~~~~~~~~~~~~~~~~~~~ //
Iris::Extent GENERATE_DERIVED_EXTENT (const EncoderDerivation &_derivation,
                                      const EncoderSource &source);
void ENCODE_DERIVED_TILE (const DerivationInfo& info,
                          AtomicEncoderStatus* _status,
                          uint32_t l, uint32_t y, uint32_t x);

// ~~~~~~~~~~~~~~~~~~~~~~~ END TILE DERIVATION ~~~~~~~~~~~~~~~~~~~~~~ //

Result __INTERNAL__Encoder::dispatch_encoder()
{
    ENCODING_START:
    auto STATUS = ENCODER_INACTIVE;
    if (_status.compare_exchange_strong(STATUS, ENCODER_ACTIVE) == false)
    switch (STATUS) {
    case ENCODER_INACTIVE: goto ENCODING_START;
    case ENCODER_ACTIVE: throw std::runtime_error
            ("[ERROR] The encoder is currently active. An encoder instance must complete before reuse. Exiting");
    case ENCODER_ERROR: throw std::runtime_error
            ("[ERROR] The encoder encountered an error in previous encoding. Please reset.");
    case ENCODER_SHUTDOWN: throw std::runtime_error
            ("[ERROR] Encoder is being shutdown. Cannot start encoding.");
    }
    
    // Attempt to open the source slide file
    auto source = OPEN_SOURCE (_srcPath);
    
    // Validate encoding
    switch (_encoding) {
        case TILE_ENCODING_JPEG: break;
        case TILE_ENCODING_AVIF: break;
        case TILE_ENCODING_IRIS: throw std::runtime_error
            ("[ERROR] Encoding using the Iris Codec is not available for community use.");
        default: throw std::runtime_error
            ("[ERROR] Encoder does not have a valid Iris::Encoding format set");
    }
    if (source.format == FORMAT_UNDEFINED)
        source.format = FORMAT_R8G8B8A8;
    
    // Get the source file's name
    std::filesystem::path source_file_path = _srcPath;
    auto source_name = source_file_path.stem();
    auto source_dir  = source_file_path.parent_path();
    
    // Format the output file path
    if (_dstPath.length() == 0)
        _dstPath = source_dir.make_preferred().string();
    else _dstPath = std::filesystem::path(_dstPath).make_preferred().string();
    if (std::filesystem::is_directory(_dstPath) == false) throw std::runtime_error
        ("[ERROR] Invalid encoder destination directory path "+_dstPath);
    if (_dstPath.back() != std::filesystem::path::preferred_separator)
        _dstPath += std::filesystem::path::preferred_separator;
    std::filesystem::path dst_file_path = _dstPath + source_name.string() + ".iris";
    
    // If the output file already exists, inform that it will be overwritten
    if (std::filesystem::exists(dst_file_path))
        std::cout       << "[WARNING] Destination file " << dst_file_path
                        << " already exists. Overwriting...\n";
    
    // Generate a temporary cache file.
    // We do not write directly to the output file path. It's better
    // practice to open a temp file within the temp_dir and write to that
    // (in case it fails we don't keep an artifiact).
    // We then rename it to the output file path once encoding is successful.
    auto file = create_cache_file({
        .unlink     = false,    // Maintain OS link to file so it can be renamed
        .context    = _context, // Provide own Codec context
    }); if (file == nullptr) throw std::runtime_error
        ("[ERROR] Could not create a temporary slide file for encoding");
    
    // This is the extent of the output slide file
    Iris::Extent extent;
    if (_derive /* If we are deriving all lower-res layers */)
        extent  = GENERATE_DERIVED_EXTENT (_derivation, source);
    // Otherwise just copy the source extent
    else extent = source.extent;
    
    // Reset the tracker
    RESET_TRACKER (_tracker, file, extent);
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // BEGIN OUR ASYNCHRONOUS STEPS; This thread will return immediately
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Begin to dispatch threads
    // First create an asynchronous callback pool for derived layer
    // encoding methods. These execute stochastically when tiles are ready.
    //
    // Then we begin dispatching our core encoding threads:
    // __INTERNAL__Encoder::dispatch_encoder() is an ASYNCHRONOUS method
    // there will be a separate main thread that waits upon the encoding
    // threads. This is threads[0]. We will move the remainder of the
    // method to this separate thread...
    _threads    = Threads(std::thread::hardware_concurrency()+1);
    _threads[0] = std::thread {[this, file, source, extent, dst_file_path](){
        
        // ~~~ We are now on the separate asynchronous main thread ~~~
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // CREATE TILE TABLE STEP
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Create the tracker to synchronize threads and monitor progress
        using TileTable     = Abstraction::TileTable;
        
        TileTable tile_table;
        tile_table.encoding = _encoding;
        tile_table.format   = source.format;
        tile_table.layers   = TileTable::Layers(extent.layers.size());
        tile_table.extent   = extent;
        for (auto __li = 0; __li < extent.layers.size(); ++__li) {
            auto& __le      = extent.layers[__li];
            auto  n_tiles   = __le.xTiles*__le.yTiles;
            tile_table.layers[__li] = TileTable::Layer(n_tiles);
        }
        
        // Create the file byte offset tracker and reserve space for the footer
        atomic_uint64 offset = Serialization::FILE_HEADER::HEADER_SIZE;
        
        // Create the downsample information struct
        // WARNING: THIS MUST PERSIST UNTIL ALL ASYNC THREADS ARE COMPLETE
        const DerivationQueue queue = Iris::Async::createThreadPool();
        const DerivationInfo downsample_info {
            .context    = _context,
            .queue      = queue,
            .strategy   = _derivation,
            .file       = file,
            .tracker    = _tracker,
            .table      = tile_table,
            .offset     = offset
        };
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // DISPATCH THE TILE ENCODING THREADS AND WAIT UPON THEIR COMPLETION
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            if (!_derive) /* Just copy source */ _threads[thread_idx] =
                std::thread {&ENCODE_SOURCE_PYRAMID,
                    _context, source, file,         // Compressor, source and dst
                    &_tracker, &tile_table, &offset,// File structure trackers
                    &_status                        // Encoder status
                };
            else /* Spool up async tile derivation */ _threads[thread_idx] =
                std::thread {&ENCODE_DERIVE_PYRAMID,
                    _context, source, file,         // Compressor, source and dst
                    &_tracker, &_status,            // Tile and Encoder statuses
                    
                    // This lambda function starts the propagation of encoding
                    // the slide pyramid by enqueueing downsampling / writing
                    [this,downsample_info](uint32_t l, uint32_t y, uint32_t x){
                        downsample_info.queue->issue_task
                        (std::bind(ENCODE_DERIVED_TILE ,downsample_info,
                                   &_status, l, y, x));
                    }
                };
        for (auto thread_idx = 1; thread_idx < _threads.size(); ++thread_idx)
            if (_threads[thread_idx].joinable()) _threads[thread_idx].join();
        // Await asynchronous thread pool if encoding tasks were delegated
        downsample_info.queue->wait_until_complete();
        // It is NOW safe to destroy the downsample_info struct
        // ~~~~~~~~~~~~~~~~~~~~~ END TILE ENCODING ~~~~~~~~~~~~~~~~~~~~~~~~~
        
        // If any thread has inactivated the encoder, exit
        if (_status!= ENCODER_ACTIVE) { _status.notify_all(); goto ENCODING_FAILED;}
        // This is our exit routine. Delete the created file
        if (false) { ENCODING_FAILED: IrisCodec::delete_file(file); return;}
        
        // ~~~~~~~~~~~~~~~~~~~~~ BEGIN VALIDATION  ~~~~~~~~~~~~~~~~~~~~~~~~~
        Offset tile_table_offset = NULL_OFFSET;
        try {
            // Check the tiles to ensure they were properly written to file
            VALIDATE_TILE_WRITES (_tracker, tile_table);
            
            // Write the tile table and return the offset
            tile_table_offset = STORE_TILE_TABLE (file, tile_table, offset);
            
        } catch (std::runtime_error &error) {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += std::string("Tile table validation failed: ") +
                                  error.what() + "\n";
            _status.notify_all();
            return;
        }
        
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // METADATA FORMATTING BLOCK
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // We prefer the following structure based upon anticipated frequency of updates
        // SOF | TILES | TILE TABLE | METADATA HEADER | IMAGES | ATTRIBUTES | ANNOTATIONS
        
        try {
            // Read the source metadata
            Metadata metadata           = READ_METADATA (source);
            
            // Reserve space for the Metadata block. I like to put it earlier
            // as it has no signficant risk of growing in size with file modification
            Offset metadata_offset      = RESERVE_METADATA(file, offset);
            
            // Write the metadata and return the metadata block offset
            Offset ICC_offset           = STORE_ICC (file, metadata, offset);
            
            // Write all of the associated images to disk
            Offset images_offset        = STORE_ASSOCIATED_IMAGES (_context, file, source, metadata, offset);
            
            // Write all of the attributes to disk
            Offset attributes_offset    = STORE_ATTRIBUTES (file, metadata, offset);
            
            // TODO: write annotations to disk
            Offset annoations_offset    = NULL_OFFSET; // Will tackle this next.
            
            // Store the metadata
            STORE_METADATA      (file, metadata_offset, metadata,
                                 ICC_offset,
                                 images_offset,
                                 attributes_offset,
                                 annoations_offset);
            
            // Store the file header
            STORE_FILE_HEADER   (file,
                                 offset.load(), 0,
                                 tile_table_offset,
                                 metadata_offset);
            
        } catch (std::runtime_error& e) {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += std::string("Metadata encoding failed: ") +
                                  e.what() + "\n";
            _status.notify_all();
            goto ENCODING_FAILED;
        }
        
        auto rename = IrisCodec::rename_file(file, dst_file_path.string());
        if (rename & IRIS_FAILURE) {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += rename.message;
            _status.notify_all();
            goto ENCODING_FAILED;
        }
        
        // Encoding is complete. Notify any waiting threads
        auto STATUS = ENCODER_ACTIVE;
        if (_status.compare_exchange_strong(STATUS, ENCODER_INACTIVE) == false) {
            std::cerr   << "Codec Error -- Encoder exited with status "
                        << TO_STRING(_status) << "\n";
        } _status.notify_all();
    }};
    
    // We have successfully dispatched the encoding method and may return.
    // All other steps will continue on the _threads[0] thread.
    return IRIS_SUCCESS;
}
Result __INTERNAL__Encoder::interrupt_encoder()
{
    switch (_status) {
        case ENCODER_ACTIVE: {
            _status.store(ENCODER_ERROR);
            MutexLock __ (_tracker.error_msg_mutex);
            _tracker.error_msg += "Encoder manually interrupted\n";
            _status.notify_all();
        } return IRIS_SUCCESS;
            
        case ENCODER_ERROR:
            _status.notify_all();
            return {
                IRIS_FAILURE,
                "Encoder already possesses the ENCODER_ERROR status."
            };
            
        case ENCODER_INACTIVE:
        case ENCODER_SHUTDOWN:
            return IRIS_SUCCESS;
    }   return IRIS_FAILURE;
}
void __INTERNAL__Encoder::encode_derived_tile (uint32_t layer, uint32_t y, uint32_t x)
{
    
}
} // END IRIS CODEC NAMESPACE
