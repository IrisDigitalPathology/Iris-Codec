//
//  IrisCodecSlide.cpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//
#include <assert.h>
#include "IrisCodecPriv.hpp"

namespace IrisCodec {
inline File OPEN_FILE (const SlideOpenInfo& info)
{
    FileOpenInfo open_info {
        .filePath       = info.filePath,
        .writeAccess    = info.writeAccess,
    };
    auto file = open_file(open_info);
    if (file == nullptr) throw std::runtime_error("no valid file opened.");
    
    return file;
}
Iris::Result is_iris_codec_file(const std::string &file_path) noexcept
{
    try {
        FileOpenInfo file_info {
            .filePath       = file_path,
            .writeAccess    = false,
        };
        auto file = open_file(file_info);
        if (file == nullptr) throw std::runtime_error
            ("file path is not a valid file\n");
        
        if (is_Iris_Codec_file(file->ptr, file->size) == false)
            throw std::runtime_error
            ("file does not contain an Iris Codec Extension header.\n");
        
        return IRIS_SUCCESS;
    } catch (std::runtime_error&e) {
        return Iris::Result (
            IRIS_FAILURE, 
            "Iris File Extension test failed ("+
            file_path + "): " +
            e.what() + "\n"
        );
    }
}
Iris::Result validate_slide (const struct SlideOpenInfo &info) noexcept
{
    try {
        FileOpenInfo file_info {
            .filePath       = info.filePath,
            .writeAccess    = false,
        };
        auto file = open_file(file_info);
        if (file == nullptr) throw std::runtime_error("file path is not a valid file\n");
        
        ReadLock read_lock (file->resize);
        return validate_file_structure(file->ptr, file->size);
        
    } catch (std::runtime_error &e) {
        #if IRIS_DEBUG
        std::cerr   << log.str();
        #endif
        return Iris::Result (
            IRIS_FAILURE, 
            "Iris File Extension slide (" + 
            info.filePath + ") failed validation: " +
            e.what () + "\n"
        );
    }
}
Slide open_slide (const struct SlideOpenInfo &info) noexcept
{
    try {
        // Create a context if not provided
        Context context = info.context;
        if (context == nullptr) {
            ContextCreateInfo context_info {};
            context = std::make_shared<__INTERNAL__Context>(context_info);
        }
        if (context == nullptr) 
            throw std::runtime_error("No valid context");
        
        // Open the file
        FileOpenInfo file_info {
            .filePath       = info.filePath,
            .writeAccess    = false,
        };
        auto file = open_file(file_info);
        if (file == nullptr)
            throw std::runtime_error("no valid file opened.");
        
        // Create the slide object
        ReadLock read_lock (file->resize);
        Slide slide = std::make_shared<__INTERNAL__Slide>(context,file);
        if (slide == nullptr) throw std::runtime_error ("Failed to create slide object");
        
        // Return the slide
        return slide;
        
    } catch (std::runtime_error &e) {
        std::cerr   << "Failed to open the slide "
                    << info.filePath << ": "
                    << e.what() << "\n";
        return nullptr;
    }   return NULL;
}
Iris::Result get_slide_info(const Slide &slide, SlideInfo& info) noexcept
{
    try {
        if (!slide)
            throw std::runtime_error("no valid slide object");
        
        info = slide->get_slide_info();
        
        return IRIS_SUCCESS;
    } catch (std::runtime_error& e) {
        return  {
            IRIS_FAILURE,
            std::string("Failed to read slide info: ") + e.what()
        };
    }   return IRIS_FAILURE;
}
Buffer read_slide_tile(const SlideTileReadInfo &info) noexcept
{
    try {
        // Ensure the slide object is valid
        if (info.slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        // Read the slide tile
        auto result = info.slide->read_slide_tile(info);
        return result;
        
    } catch (std::runtime_error& e) {
        std::cerr << "Failed to read the slide tile"
                    << "[layer " << info.layerIndex
                    << ", tile " << info.tileIndex
                    << "]: " << e.what() << "\n";
        return NULL;
    }   return NULL;
}
Result get_associated_image_info(const Slide &slide, AssociatedImageInfo &info) noexcept
{
    try {
        if (!slide) throw std::runtime_error
            ("No valid codec slide object");
        if (!info.imageLabel.size()) throw std::runtime_error
            ("No image label provided within AssociatedImageInfo struct");
        
        info = slide->get_assoc_image_info(info.imageLabel);
        return IRIS_SUCCESS;
        
    } catch (std::runtime_error& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to get associated image info: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
Buffer read_associated_image(const AssociatedImageReadInfo &info) noexcept
{
    try {
        if (!info.slide) throw std::runtime_error
            ("No valid slide object");
        if (!info.imageLabel.size()) throw std::runtime_error
            ("No image label provided within AssociatedImageReadInfo struct");
        
        return info.slide->read_assc_image(info);
        
    } catch (std::runtime_error &error) {
        std::cerr   << "Failed to read the associated image labeled \""
                    << info.imageLabel <<  "\": " << error.what();
        return NULL;
    }   return NULL;
}
Iris::Result annotate_slide(const Annotation &annotation) noexcept
{
    try {
        if (annotation.slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        assert(false && "IrisCodec::annotate_slide() in irisCodecSlide.cpp not yet fully written");
//        return annotation.slide->write_slide_annotation(annotation);
    } catch (std::runtime_error& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to annotate the codec slide: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
Iris::Result get_slide_annotations(const Slide &slide, Annotations &annotations) noexcept
{
    try {
        if (slide == NULL)
            throw std::runtime_error("No valid codec slide object");
        
        assert(false && "IrisCodec::get_slide_annotations() in irisCodecSlide.cpp not yet fully written");
        annotations.clear();
        
        return IRIS_SUCCESS;
    } catch (std::runtime_error& e) {
        return {
            IRIS_FAILURE,
            std::string("Failed to retrieve slide annotation objects: ") +
            e.what()
        };
    }   return IRIS_FAILURE;
}
__INTERNAL__Slide::__INTERNAL__Slide    (const Context& cxt, const File& file) :
_context                                (cxt),
_file                                   (file),
_abstraction                            (abstract_file_structure(file->ptr, file->size))
{
    
}
__INTERNAL__Slide::~__INTERNAL__Slide   ()
{
    
}

Version __INTERNAL__Slide::get_slide_codec_version() const
{
    return _abstraction.metadata.codec;
}
SlideInfo __INTERNAL__Slide::get_slide_info() const
{
    return SlideInfo {
        .format         = _abstraction.tileTable.format,
        .encoding       = _abstraction.tileTable.encoding,
        .extent         = _abstraction.tileTable.extent,
        .metadata       = _abstraction.metadata,
    };
}
Buffer __INTERNAL__Slide::get_slide_tile_entry(uint32_t layer, uint32_t tile_indx) const
{
    ReadLock lock (_file->resize);
    
    // Pull the extent and check that the layer in within info
    auto& ttable = _abstraction.tileTable;
    auto& layers = ttable.layers;
    if (layer >= layers.size())
        throw std::runtime_error("layer in SlideTileReadInfo is out of bounds");
    
    // Pull the layer extent and check that the tile is within info
    auto& tiles = layers[layer];
    if (tile_indx >= tiles.size())
        throw std::runtime_error("tile in SLideTileReadInfo is out of layer bounds");
    
    // Get the offset and size of the tile entry
    auto& entry     = tiles[tile_indx];
    return Iris::Copy_strong_buffer_from_data(_file->ptr + entry.offset, entry.size);
}
Buffer __INTERNAL__Slide::read_slide_tile(const SlideTileReadInfo &info) const
{
    ReadLock lock (_file->resize);
    
    // Pull the extent and check that the layer in within info
    auto& ttable = _abstraction.tileTable;
    auto& layers = ttable.layers;
    if (info.layerIndex >= layers.size())
        throw std::runtime_error("layer in SlideTileReadInfo is out of bounds");
    
    // Pull the layer extent and check that the tile is within info
    auto& tiles = layers[info.layerIndex];
    if (info.tileIndex >= tiles.size())
        throw std::runtime_error("tile in SLideTileReadInfo is out of layer bounds");
    
    // Get the offset and size of the tile entry
    auto& entry     = tiles[info.tileIndex];
    Buffer src      = Iris::Wrap_weak_buffer_fom_data (_file->ptr + entry.offset, entry.size);
    
    
    // Initialize the write destination
    Buffer dst_buffer   = nullptr;
    size_t dst_size     = 0;
    switch (info.desiredFormat) {
        case FORMAT_UNDEFINED: throw std::runtime_error
            ("desired format in SLideTileReadInfo is undefined");
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            dst_size = TILE_PIX_AREA * 3;
            break;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            dst_size = TILE_PIX_AREA * 4;
            break;
    } if (!dst_size) throw std::runtime_error
        ("invalid desired slide format in SlideTileReadInfo");
    
    // Check to see if there is a destination provided to write into, and if that
    // destination buffer is sufficiently large to hold the unpacked data.
    if (info.optionalDestination && info.optionalDestination->capacity() >= dst_size)
            dst_buffer = info.optionalDestination;
    else    dst_buffer = Iris::Create_strong_buffer(dst_size);
    
    // Return the decompressed file structure
    dst_buffer = _context->decompress_tile({
        .compressed             = src,
        .optionalDestination    = dst_buffer,
        .desiredFormat          = info.desiredFormat,
        .encoding               = ttable.encoding,
    });
    if (!dst_buffer) throw std::runtime_error
        ("Failed to decompress slide tile");
    
    return dst_buffer;
}
AssociatedImageInfo __INTERNAL__Slide::get_assoc_image_info (const std::string &image_label) const
{
    ReadLock lock (_file->resize);
    
    const auto image_itr = _abstraction.images.find(image_label);
    if (image_itr == _abstraction.images.cend())
        throw std::runtime_error("get_assoc_image_info failed as there is no image with label \""+
                                 image_label + "\" within the slide file.");
    
    return image_itr->second.info;
}
Buffer __INTERNAL__Slide::get_assoc_image (const std::string &image_label) const
{
    ReadLock lock (_file->resize);
    
    const auto image_itr = _abstraction.images.find(image_label);
    if (image_itr == _abstraction.images.cend())
        throw std::runtime_error("get_assoc_image failed as there is no image with label \""+
                                 image_label + "\" within the slide file.");
    
    const auto& entry = image_itr->second;
    return Copy_strong_buffer_from_data(_file->ptr + entry.offset, entry.byteSize);
}
Buffer __INTERNAL__Slide::read_assc_image (const AssociatedImageReadInfo &info) const
{
    ReadLock lock (_file->resize);
    
    const auto image_itr = _abstraction.images.find(info.imageLabel);
    if (image_itr == _abstraction.images.cend())
        throw std::runtime_error("get_assoc_image failed as there is no image with label \""+
                                 info.imageLabel + "\" within the slide file.");
    
    const auto& entry   = image_itr->second;
    Buffer src          = Iris::Wrap_weak_buffer_fom_data (_file->ptr + entry.offset, entry.byteSize);
    
    // Initialize the write destination
    Buffer dst_buffer   = nullptr;
    size_t image_pixels = entry.info.width * entry.info.height;
    size_t dst_size     = 0;
    switch (info.desiredFormat) {
        case FORMAT_UNDEFINED: throw std::runtime_error
            ("desired format in SLideTileReadInfo is undefined");
        case Iris::FORMAT_B8G8R8:
        case Iris::FORMAT_R8G8B8:
            dst_size = image_pixels * 3; // 3 bytes per pixel format
            break;
        case Iris::FORMAT_B8G8R8A8:
        case Iris::FORMAT_R8G8B8A8:
            dst_size = image_pixels * 4; // 4 bytes per pixel format
            break;
    } if (!dst_size) throw std::runtime_error
        ("invalid desired slide format in SlideTileReadInfo");
    
    // Check to see if there is a destination provided to write into, and if that
    // destination buffer is sufficiently large to hold the unpacked data.
    if (info.optionalDestination && info.optionalDestination->capacity() >= dst_size)
            dst_buffer = info.optionalDestination;
    else    dst_buffer = Iris::Create_strong_buffer(dst_size);
    
    // Return the decompressed file structure
    dst_buffer = _context->decompress_image(DecompressImageInfo {
        .compressed             = src,
        .optionalDestination    = dst_buffer,
        .width                  = entry.info.width,
        .height                 = entry.info.height,
        .sourceFormat           = entry.info.sourceFormat,
        .desiredFormat          = info.desiredFormat,
        .encoding               = entry.info.encoding,
    });
    if (!dst_buffer) throw std::runtime_error
        ("Failed to decompress slide tile");
    
    return dst_buffer;
}
Iris::Result __INTERNAL__Slide::write_slide_annotation (const IrisCodec::Annotation &annotation)
{
    return IRIS_FAILURE;
}
} // END IRIS CODEC NAMESPACE
