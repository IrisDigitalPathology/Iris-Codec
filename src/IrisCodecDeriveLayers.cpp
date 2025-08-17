//
//  IrisCodecDeriveLayers.cpp
//  Iris
//
//  Created by Ryan Landvater on 7/7/25.
//
#include <cmath>
#include "IrisCodecPriv.hpp"
#include "IrisSIMD.hpp"
namespace IrisCodec {
Iris::Extent GENERATE_DERIVED_EXTENT (const EncoderDerivation &_derivation, const EncoderSource &source) {
    Iris::Extent extent;
    int8_t __bs = -1; // Bit-shift
    int8_t __bm = 0; // Bit-mask
    int8_t __li = -1; // Layer index
    switch (_derivation.layers) {
        case EncoderDerivation::ENCODER_DERIVE_UNDEFINED:
        case EncoderDerivation::ENCODER_DERIVE_USE_SOURCE:
            throw std::runtime_error
            ("GENERATE_DERIVED_EXTENT _derivation.layers is ENCODER_DERIVE_UNDEFINED/SOURCE");
        case EncoderDerivation::ENCODER_DERIVE_2X_LAYERS:
            __bs            = 1; // 2x downsample
            __bm            = 0x01; // Odd bitmask
            __li            = 8; // Generate 8 layers
            // 256 pix orig -> 128, 64, 32, 16, 8, 4, 2, 1
            extent.layers   = LayerExtents(9);
            break;
        case EncoderDerivation::ENCODER_DERIVE_4X_LAYERS:
            __bs            = 2; // 4x downsample
            __bm            = 0x03; // 4x bitmask
            __li            = 4; // Generate 4 layers
            // 256 pix orig -> 64, 16, 4, 1
            extent.layers   = LayerExtents(5);
            break;
    }
    if (__bs == -1 || __li == -1) throw std::runtime_error
        ("[ERROR] Undefined EncoderDerivation layers. Define in EncoderCreateInfo.");
    
    // Derive the number low resolution layer (MIPS)
    auto& base_extent       = source.extent.layers.back();
    auto& layers            = extent.layers;
    auto downsample         = 1;
    auto xTiles             = base_extent.xTiles;
    auto yTiles             = base_extent.yTiles;
    for (; __li >= 0 && xTiles > 0 && yTiles > 0; --__li) {
        // Assign the layer extent
        layers[__li].xTiles     = xTiles;
        layers[__li].yTiles     = yTiles;
        layers[__li].downsample = (float)downsample;
        
        // Downsample by whatever level and add 1 tile for any partial tiles
        xTiles              = (xTiles >> __bs) + (xTiles&__bm?1:0);
        yTiles              = (yTiles >> __bs) + (yTiles&__bm?1:0);
        downsample        <<= __bs;
    }
    
    // If there are blank layers (ie no tiles), delete them
    if (__li > -1) for (;__li >= 0; --__li)
        layers.erase(layers.begin());
    
    // Assign the scale of higher res layers
    // relative to layer[0]. This is not always the reciprocal
    // of downsample (i.e. if there were blank layers).
    auto scale = 1;
    for (__li = 0, scale = 1; __li < layers.size(); ++__li, scale <<= __bs)
        layers[__li].scale = (float)scale;
    
    // Recalculate the viewable area
    // NOTE: Scale is relative to layer 0 of an image, which may be different
    // from a source. Downsample is independent and only relative to highest res
    extent.width    = U32_CAST(round(static_cast<float>(source.extent.width) /
                                     extent.layers.front().downsample *
                                     source.extent.layers.front().downsample));
    extent.height   = U32_CAST(round(static_cast<float>(source.extent.height)/
                                     extent.layers.front().downsample *
                                     source.extent.layers.front().downsample));
    
    return extent;
}
inline Subtile DOWNSAMPLE_2x_AVG (const Buffer& src, const Buffer& dst, uint32_t y, uint32_t x, uint8_t channels)
{
    const Subtile  s_y  = y&0x01;
    const Subtile  s_x  = x&0x01;
    Iris::SIMD::Downsample_into_tile_2x_avg(src, dst, s_y, s_x, channels);
    return 1<<((s_y<<1)|s_x);
}
inline Subtile DOWNSAMPLE_4x_AVG (const Buffer& src, const Buffer& dst, uint32_t y, uint32_t x, uint8_t channels)
{
    const Subtile s_y = y&0x03, s_x = x&0x03;
    Iris::SIMD::Downsample_into_tile_4x_avg(src, dst, s_y, s_x, channels);
    return 1<<((s_y<<2)|s_x);
}
inline void GENERATE_TILE_BUFFER (Buffer& pixels, const Abstraction::TileTable& table)
{
    assert(pixels == NULL && "Tile buffers are already allocated");
    
    int bpp = 0;
    switch (table.format) {
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            bpp = 3;
            break;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            bpp = 4;
            break;
        case Iris::FORMAT_UNDEFINED:
        default: throw std::runtime_error
            ("GENERATE_TILE_BUFFER undefined pixel byte format");
    }
    // Create the canvas
    pixels = Iris::Create_strong_buffer(TILE_PIX_AREA * bpp);
    // Blank / white pixel buffer canvas
    memset(pixels->data(), 0xFF, TILE_PIX_AREA * bpp);
}
inline void SET_SUBTILE_TRACKER (SubtileTracker& subtile,
                                 const DerivationInfo& info,
                                 uint32_t l, uint32_t y, uint32_t x)
{
    auto& extent = info.table.extent.layers[l];
    uint32_t yTiles = extent.yTiles, xTiles = extent.xTiles;
    unsigned y_extent, x_extent;
    switch (info.strategy.layers) {
        case EncoderDerivation::ENCODER_DERIVE_UNDEFINED:
        case EncoderDerivation::ENCODER_DERIVE_USE_SOURCE:
            throw std::runtime_error
            ("SET_SUBTILE_TRACKER info.strategy.layers is ENCODER_DERIVE_UNDEFINED/SOURCE");
        case EncoderDerivation::ENCODER_DERIVE_2X_LAYERS:
            
            // This checks if the subtile is in a partially filled derived tile
            // 0xFE is the reciprocal of the residual bitmask 0x01 in 2x downsample
            // This will return true if x and y do not extend into a partially filled
            // within the subsequent (layer-1) layer.
            if (y < (yTiles & UINT32_MAX-1) && x < (xTiles & UINT32_MAX-1))
                subtile = SUBTILES_COMPLETE ^ 0x0F;
            else {
                // How this works is
                subtile = SUBTILES_COMPLETE;
                y_extent = y<(yTiles&UINT32_MAX-1)?2:1;
                x_extent = x<(xTiles&UINT32_MAX-1)?2:1;
                for (auto sub_y = 0; sub_y < y_extent; ++sub_y)
                    for (auto sub_x = 0; sub_x < x_extent; ++sub_x)
                        subtile ^= 1<<(sub_y * 2 + sub_x);
                return;
            }
            return;
            
        case EncoderDerivation::ENCODER_DERIVE_4X_LAYERS:
            
            // This checks if the tile is in a partially filled subtile
            // 0xFC is the reciprocal of the residual bitmask 0x03 in 4x downsample
            // This will return true if x and y do not extend into a partially filled
            // within the subsequent (layer-1) layer.
            if (y < (yTiles & UINT32_MAX-3) && x < (xTiles & UINT32_MAX-3))
                subtile = SUBTILES_COMPLETE ^ UINT16_MAX;
            else {
                subtile = SUBTILES_COMPLETE;
                y_extent = y<(yTiles&UINT32_MAX-3)?4:yTiles&0x03;
                x_extent = x<(xTiles&UINT32_MAX-3)?4:xTiles&0x03;
                for (auto sub_y = 0; sub_y < y_extent; ++sub_y)
                    for (auto sub_x = 0; sub_x < x_extent; ++sub_x)
                        subtile ^= 1<<(sub_y * 4 + sub_x);
            }
            return;
    }
    

}
inline void DOWNSAMPLE_TILE (const DerivationInfo& info,
                             const Buffer& src,
                             uint32_t l, uint32_t y, uint32_t x,
                             const std::function
                             <void(uint32_t n_l, uint32_t n_y, uint32_t n_x)>
                             & ENQUEUE_NEXT_TILE)
{
    auto& extent    = info.table.extent;
    auto& tracker   = info.tracker;
    auto n_l = l-1;
    uint32_t n_y,n_x;
    std::function<uint16_t(const Buffer&, const Buffer&,uint32_t,uint32_t,uint8_t)> downsample;
    switch (info.strategy.layers) {
        case EncoderDerivation::ENCODER_DERIVE_UNDEFINED:
        case EncoderDerivation::ENCODER_DERIVE_USE_SOURCE:
            throw std::runtime_error
            ("DOWNSAMPLE_TILE info.strategy.layers is ENCODER_DERIVE_UNDEFINED/SOURCE");
        case EncoderDerivation::ENCODER_DERIVE_2X_LAYERS:
            downsample  = DOWNSAMPLE_2x_AVG;
            n_y     = y>>1;
            n_x     = x>>1;
            break;
        case EncoderDerivation::ENCODER_DERIVE_4X_LAYERS:
            downsample  = DOWNSAMPLE_4x_AVG;
            n_y         = y>>2;
            n_x         = x>>2;
            break;
    }
    uint8_t channels = 0;
    switch (info.table.format) {
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            channels = 3;
        case Iris::FORMAT_UNDEFINED:
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            channels = 4;
    }

    assert(l < extent.layers.size() &&
           "ENCODE_DERIVED_TILE layer index out of extent bounds");
    assert(l < tracker.layers.size() &&
           "ENCODE_DERIVED_TILE layer index out of tracker bounds");
    assert((y * extent.layers[l].xTiles + x) < tracker.layers[l].size() &&
           "ENCODE_DERIVED_TILE tile index out of tracker bounds");
    assert((channels > 3 && channels < 5) &&
           "ENCODE_DERIVED_TILE contains an invalid number of color channels");
    
    auto& tile = tracker.layers[n_l][n_y*extent.layers[n_l].xTiles+n_x];
    //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //  INITIALIZE TILE STEP
    //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //  Lazy instantiation of tile buffers means that only one thread
    //  can create the buffer canvas. Others must wait on it before
    //  reading into the buffer (which they can do concurrently)
    auto STATUS     = TILE_FREE;
    if (tile.status.compare_exchange_strong(STATUS, TILE_INITIALIZING))
        goto ALLOCATE_TILE;
    else switch (STATUS) {
            
        // First thread here; allocate the tile pixel array (bytes)
        case TILE_FREE: tile.status.store(TILE_INITIALIZING);
        ALLOCATE_TILE:
            GENERATE_TILE_BUFFER(tile.pixels, info.table);
            SET_SUBTILE_TRACKER(tile.subtile, info, l, y, x);
            tile.status.store(TILE_READING);
            tile.status.notify_all();
            break;
            
        // Sunsequent threads wait until the buffer is allocated
        // Before reading (ie downsampling) into that tile
        case TILE_INITIALIZING:
            tile.status.wait(TILE_INITIALIZING);
        case TILE_READING:
            break;
            
        // These flags should NEVER fire here
        case TILE_PENDING:
        case TILE_ENCODING:
        case TILE_COMPLETE:
            std::cerr   << "ENCODE_DOWNSAMPLE_TILE synchronization error. "
                        << "Set a breakpoint in " << __FILE__
                        << " line " << __LINE__ << "to debug\n";
            return;
    }
    //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    //  DOWNSAMPLE TILE STEP
    //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    auto subtile = downsample (src,tile.pixels,y,x,channels);

    // Atomic bit-OR on the completed tile subregion bit
    // This informs other threads that
    auto completed = tile.subtile.load();
    assert((completed|subtile) != completed);
    while (!tile.subtile.compare_exchange_weak(completed, completed|subtile));
    
    assert(completed != SUBTILES_COMPLETE && "Tile Write SYNCHRONIZATION ERROR");
    
    // If the completion flag is completely filled in, enqueue that tile for encoding
    if ((completed|subtile) == SUBTILES_COMPLETE) {
        if (!tile.status.compare_exchange_strong(STATUS = TILE_READING, TILE_PENDING))
            throw std::runtime_error("ENCODE_SYNCHRONIZATION ERROR");
        ENQUEUE_NEXT_TILE(n_l,n_y,n_x);
    }
}
void ENCODE_DERIVED_TILE (const DerivationInfo& info,
                          AtomicEncoderStatus* _status,
                          uint32_t l, uint32_t y, uint32_t x) {
    
    auto& table     = info.table;
    auto& extent    = table.extent;
    auto& tracker   = info.tracker;
    try {
        if (_status->load() != ENCODER_ACTIVE) return;
        assert(l < extent.layers.size() &&
               "ENCODE_DERIVED_TILE layer index out of extent bounds");
        assert(l < tracker.layers.size() &&
               "ENCODE_DERIVED_TILE layer index out of tracker bounds");
        assert((y * extent.layers[l].xTiles + x) < tracker.layers[l].size() &&
               "ENCODE_DERIVED_TILE tile index out of tracker bounds");
        assert(l < table.layers.size() &&
               "ENCODE_DERIVED_TILE layer index out of file tile table bounds");
        assert((y * extent.layers[l].xTiles + x) < table.layers[l].size() &&
               "ENCODE_DERIVED_TILE layer index out of file tile table bounds");
        
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  CAPTURE TILE STEP
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  Recapture the tile on this asynchronous thread
        auto  t      = y * extent.layers[l].xTiles + x;
        auto& tile   = tracker.layers[l][t];
        auto  STATUS = TILE_PENDING;
        if (!tile.status.compare_exchange_strong(STATUS, TILE_ENCODING))return;
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  DOWNSAMPLE STEP
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  If not last layer, downsample the pixels and write
        //  into a part of that lower resolution tile
        if (l > 0) DOWNSAMPLE_TILE (info, tile.pixels, l, y, x, [info,_status]
                                    (uint32_t _l, uint32_t _y, uint32_t _x){
            // Pseudo recursion; reissue this for the next layer's tile
            info.queue->issue_task
            (std::bind(ENCODE_DERIVED_TILE,info, _status, _l, _y, _x));
        });
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  COMPRESS PIXEL ARRAY STEP
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        auto& stream = tile.stream; // Compressed byte stream
        if (!stream) {
            stream = info.context->compress_tile({
                .pixelArray = tile.pixels,
                .format     = table.format,
                .encoding   = table.encoding
            });
        } if (!stream) throw std::runtime_error
            ("Failed to compress slide image data");
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  WRITE TO FILE STEP
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        auto& file      = info.file;
        auto& entry     = table.layers[l][t];
        entry.size      = U32_CAST(stream->size());
        entry.offset    = info.offset.fetch_add(entry.size);
        ReadLock shared_write_lock (file->resize);
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
        memcpy(dst, stream->data(), entry.size);
        shared_write_lock.unlock();
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        //  RELEASE TILE STEP
        //  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        tile.status.store(TILE_COMPLETE);
        tile.pixels = NULL;
        tile.stream = NULL;
        tracker.completed++;
    } catch (std::runtime_error &error) {
        _status->store(ENCODER_ERROR);
        MutexLock __ (tracker.error_msg_mutex);
        tracker.error_msg += std::string("Derived tile encoding failed: ") +
                             error.what() + "\n";
        _status->notify_all();
        return;
    }
}
}
