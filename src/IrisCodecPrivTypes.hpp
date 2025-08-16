//
//  IrisCodecPrivTypes.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/10/24.
//

#ifndef IrisCodecPrivTypes_h
#define IrisCodecPrivTypes_h
extern "C" {
typedef struct _openslide       openslide_t;
typedef struct tiff             TIFF;
}
namespace IrisCodec {
using       Tile            = std::shared_ptr<class __INTERNAL__Tile>;
using       File            = std::shared_ptr<class __INTERNAL__File>;

/// Create a new file for writing
File    create_file         (const struct FileCreateInfo&);

/// Open a file for read or read-write access.
File    open_file           (const struct FileOpenInfo&);

/// Create a new file-system temporary file for temporary archiving of slide data to disk.
File    create_cache_file   (const struct CacheCreateInfo&);

/// Resize a file
Result  resize_file         (const File&, const struct FileResizeInfo&);

/// Rename a file
Result  rename_file         (const File&, const std::string&);

/// Delete the file
Result delete_file          (const File &file);

struct FileCreateInfo {
    std::string     filePath;
    size_t          initial_size = static_cast<size_t>(5E6);
};
struct FileOpenInfo {
    std::string     filePath;
    bool            writeAccess = false;
};
struct FileResizeInfo {
    size_t          size;
    bool            pageAlign           = false;
};
struct CompressTileInfo {
    Buffer          pixelArray          = NULL;
//    Buffer          destinationOptional = NULL;
    Format          format              = Iris::FORMAT_UNDEFINED;
    Encoding        encoding            = TILE_ENCODING_UNDEFINED;
    Quality         quality             = QUALITY_DEFAULT;
    Subsampling     subsampling         = SUBSAMPLE_DEFAULT;
};
struct DecompressTileInfo {
    Buffer          compressed          = NULL;
    Buffer          optionalDestination = NULL;
    Format          desiredFormat       = Iris::FORMAT_UNDEFINED;
    Encoding        encoding            = TILE_ENCODING_UNDEFINED;
};
struct CompressImageInfo {
    Buffer          pixelArray          = NULL;
//    Buffer          destinationOptional = NULL;
    uint32_t        width               = 0;
    uint32_t        height              = 0;
    Format          format              = Iris::FORMAT_UNDEFINED;
    ImageEncoding   encoding            = IMAGE_ENCODING_UNDEFINED;
    Quality         quality             = QUALITY_DEFAULT;
    Subsampling     subsampling         = SUBSAMPLE_DEFAULT;
};
struct DecompressImageInfo {
    Buffer          compressed          = NULL;
    Buffer          optionalDestination = NULL;
    uint32_t        width               = 0;
    uint32_t        height              = 0;
    Format          sourceFormat        = Iris::FORMAT_UNDEFINED;
    Format          desiredFormat       = Iris::FORMAT_UNDEFINED;
    ImageEncoding   encoding            = IMAGE_ENCODING_UNDEFINED;
};
// MARK: - FILE ACCESS DATA STRUCTURES
using FileLock = std::shared_ptr<class __INTERNAL__FileLock>;

// MARK: - ENCODER STRUCTURES
enum __tileStatus {
    TILE_FREE,
    TILE_INITIALIZING,
    TILE_READING,
    TILE_PENDING,
    TILE_ENCODING,
    TILE_COMPLETE,
};
using Subtile                   = uint16_t;
using SubtileTracker            = std::atomic<Subtile>;
#define SUBTILES_COMPLETE       UINT16_MAX
//#define SUBTILES_COMPLETE       UINT16_MAX;
static_assert(std::is_same<Subtile, uint16_t>::value,
"If you change/expand subtile flag, remember to update the \
SUBTILESCMPLT to the max value of the new type");
using DcmFile = std::shared_ptr<struct __INTERNAL__DcmFile>;
struct TileTracker {
    std::atomic<__tileStatus>   status;
    SubtileTracker              subtile;
    Iris::Buffer                pixels  = NULL;
    Iris::Buffer                stream  = NULL;
    TileTracker() :
    status  (TILE_FREE),
    subtile (0){}
};
struct EncoderSource {
    enum {
        ENCODER_SRC_UNDEFINED   = 0,
        ENCODER_SRC_IRISSLIDE,
        ENCODER_SRC_OPENSLIDE,
        ENCODER_SRC_DICOM,
        ENCODER_SRC_APERIO,
    }               sourceType  = ENCODER_SRC_UNDEFINED;
    Format          format      = Iris::FORMAT_UNDEFINED;
    Encoding        encoding    = TILE_ENCODING_UNDEFINED;
    Extent          extent;
    Slide           irisSlide   = NULL;
    DcmFile         dicomFile   = NULL;
    openslide_t*    openslide   = NULL;
    TIFF*           svs         = NULL;
};
struct EncoderTracker {
    using Layer                 = std::vector<TileTracker>;
    using Layers                = std::vector<Layer>;
    using Counter               = Iris::atomic_uint32;
    std::string     dst_path;
    Layers          layers;
    Counter         completed;
    uint32_t        total;
    Mutex           error_msg_mutex;
    std::string     error_msg;
    EncoderTracker  ():
    completed       (0),
    total           (0){}
};
struct DerivationInfo {
    using Queue                 = Async::ThreadPool;
    using Strategy              = EncoderDerivation;
    using Tracker               = EncoderTracker;
    using Table                 = IrisCodec::Abstraction::TileTable;
    using AtomicOffset          = atomic_uint64;
    const Context&  context;
    const Queue&    queue;
    const Strategy& strategy;
    const File&     file;
    Tracker&        tracker;
    Table&          table;
    AtomicOffset&   offset;
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecPrivTypes_h */
