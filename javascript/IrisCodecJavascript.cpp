/**
 * @file IrisCodecJavascript.cpp
 * @author Ryan Landvater (ryanlandvater [at] gmail [dot] com)
 * @brief Iris-codec implementation for JavaScript bindings that
 * allows remote HTTP range request streaming of Iris File Extension
 * encoded whole slide image files. This provides required logic to
 * abstract the file and the Emscripten Bindings to allow JS access.
 * @version 2025.2.0
 * @date 2025-08-09
 * 
 * @copyright Copyright Iris Developers (c) 2025
 * 
 */
#include "IrisFileExtension.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/fetch.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <set>


// Convenience Serialization functions
// MARK: - SERIALIZATION HELPER FUNCTIONS
namespace Iris {
inline std::string SERIALIZE_FORMAT (const Iris::Format &format)
{
    switch (format) {
        case FORMAT_UNDEFINED:  return "\"FORMAT_UNDEFINED\"";
        case FORMAT_B8G8R8:     return "\"FORMAT_B8G8R8\"";
        case FORMAT_R8G8B8:     return "\"FORMAT_R8G8B8\"";
        case FORMAT_B8G8R8A8:   return "\"FORMAT_B8G8R8A8\"";
        case FORMAT_R8G8B8A8:   return "\"FORMAT_R8G8B8A8\"";
    }
    return "\"UNDEFINED FORMAT\"";
}
inline std::string SERIALIZE_ENCODING (const IrisCodec::Encoding &encoding)
{
    switch (encoding) {
        case IrisCodec::TILE_ENCODING_UNDEFINED:    return "\"ENCODING_UNDEFINED\"";
        case IrisCodec::TILE_ENCODING_IRIS:         return "\"image/iris\"";
        case IrisCodec::TILE_ENCODING_JPEG:         return "\"image/jpeg\"";
        case IrisCodec::TILE_ENCODING_AVIF:         return "\"image/avif\"";
    }
    return "\"UNDEFINED ENCODING\"";
}
inline void SERIALIZE_LAYER_EXTENT (const LayerExtents &extent, std::stringstream& stream)
{
    stream << "[";
    for (auto&& layer : extent) {
        stream  << "{"
                << "\"x_tiles\": " << layer.xTiles << ","
                << "\"y_tiles\": " << layer.yTiles << ","
                << "\"scale\": " << layer.scale
                << "},";
    }
    stream.seekp(-1,stream.cur) << "]";
}
inline void SERALIZE_SLIDE_EXTENT (const Extent &extent, std::stringstream& stream)
{
    stream  << "{"
            << "\"width\": " << extent.width << ","
            << "\"height\": " << extent.height << ","
            << "\"layers\": ";
    
    SERIALIZE_LAYER_EXTENT(extent.layers, stream);
    stream << "},";
}
} // END IRIS namespace

// MARK: - EMSCRIPTEN FETCH HELPER FUNCTIONS
using namespace emscripten;
/** Fetch is a C++ wrapper to allow access to std::function
 *  which is significantly more robust in terms of variable/state capture
 *  than the C-functon-pointer counterpart. It's annoying I have to
 *  do this but that's the cost of some C libraries.
 */
struct Fetch {
    std::function<void(emscripten_fetch_t*)> _onSuccess = NULL;
    std::function<void(emscripten_fetch_t*)> _onFailure = NULL;
    Fetch               (void){}
    Fetch               (const Fetch&) = delete;
    Fetch operator =    (const Fetch&) = delete;
};
inline void ON_SUCCESS (emscripten_fetch_t*f) {
    auto fetch = static_cast<Fetch*>(f->userData);
    if (fetch->_onSuccess) fetch->_onSuccess(f);
    emscripten_fetch_close(f);
    delete fetch;
}
inline void ON_FAILURE (emscripten_fetch_t*f) {
    auto fetch = static_cast<Fetch*>(f->userData);
    if (fetch->_onFailure) fetch->_onFailure(f);
    emscripten_fetch_close(f);
    delete fetch;
}
inline std::vector<std::string> PARSE_RESPONSE_HEADERS (emscripten_fetch_t*f)
{
    std::vector<std::string> response_headers;
    std::vector<char> _headers (emscripten_fetch_get_response_headers_length(f)+1);
    emscripten_fetch_get_response_headers(f, _headers.data(), _headers.size());
    char** entries = emscripten_fetch_unpack_response_headers(_headers.data());
    if (!entries) throw std::runtime_error
        ("HTTP HEAD response for file size did not contain headers");
    
    for (char** current_entry = entries; *current_entry != nullptr; ++current_entry) {
        response_headers.push_back([current_entry]() -> std::string {
            std::string entry (*current_entry);
            std::transform(entry.begin(), entry.end(), entry.begin(), []
                           (unsigned char c){return std::tolower(c);});
            return entry;
        }());
    } emscripten_fetch_free_unpacked_response_headers(entries);
    return response_headers;
}
inline size_t PARSE_REMOTE_FILE_SIZE (emscripten_fetch_t *f)
{
    auto response_headers = PARSE_RESPONSE_HEADERS(f);
    
    size_t file_size = 0;
    for (auto _entry = response_headers.begin();
         _entry != response_headers.end(); ++_entry)
        if (_entry->compare("content-length") == 0 &&
            _entry+1 != response_headers.end()) {
            file_size = std::stoull(*++_entry);
            break;
        }
    
    if (!file_size) throw std::runtime_error
        ("Content-Length header not found");
    return file_size;
}

// MARK: - IRIS CODEC JavaScript SLIDE WRAPPER
namespace IrisCodec {
class __INTERNAL__Slide {
    const std::string               _url;
    const Abstraction::File         _abstraction;
public:
    explicit __INTERNAL__Slide      (const std::string& url,
                                    const Abstraction::File& __a) :
    _url                            (url),
    _abstraction                    (__a) {}
    __INTERNAL__Slide               (const __INTERNAL__Slide&) = delete;
    __INTERNAL__Slide operator =    (const __INTERNAL__Slide&) = delete;
   ~__INTERNAL__Slide               (){}
    // Return codec version used to encode the slide
    Version get_slide_codec_version () const {
        return _abstraction.metadata.codec;
    }
    SlideInfo get_slide_info () const {
        return SlideInfo {
            .format         = _abstraction.tileTable.format,
            .encoding       = _abstraction.tileTable.encoding,
            .extent         = _abstraction.tileTable.extent,
            .metadata       = _abstraction.metadata,
        };
    }
    std::string get_metadata_JSON () const
    {
        using namespace Iris;
        SlideInfo info {
            .format         = _abstraction.tileTable.format,
            .encoding       = _abstraction.tileTable.encoding,
            .extent         = _abstraction.tileTable.extent,
            .metadata       = _abstraction.metadata,
        };
        std::stringstream stream;
        stream << "{";
        stream << "\"type\": \"slide_metadata\",";
        if (info.format)
            stream  <<"\"format\": " << SERIALIZE_FORMAT (info.format) << ",";
        if (info.encoding)
            stream  <<"\"encoding\": " << SERIALIZE_ENCODING(info.encoding) << ",";
        
        stream <<"\"extent\": ";
        SERALIZE_SLIDE_EXTENT(info.extent,stream);
        
        stream.seekp(-1,stream.cur) << "}";
        return stream.str();
    }
    void get_slide_tile (uint32_t layer, uint32_t tile_indx, emscripten::val onDone) {
        // Pull the extent and check that the layer in within info
        auto& layers = _abstraction.tileTable.layers;
        if (layer >= layers.size()) throw std::runtime_error
            ("Requested layer("+ std::to_string(layer) +") is out of bounds");
        
        // Pull the layer extent and check that the tile is within info
        auto& tiles = layers[layer];
        if (tile_indx >= tiles.size()) throw std::runtime_error
            ("Requested tile("+ std::to_string(tile_indx) +") is out of layer "+std::to_string(layer)+" bounds");
        
        // Get the entry information (offset and size)
        auto& entry     = tiles[tile_indx];
                
        auto fetch = new Fetch ();
        fetch->_onSuccess = [this,onDone](emscripten_fetch_t* f){
            if (f && (f->status == 206 || f->status == 200));
            else throw std::runtime_error
                (std::string("Fetch failed with code (HTTP ") +
                 std::to_string(f->status) +
                 "): "+ f->statusText);
            // Create a Uint8Array view of the fetched data
            val uint8_array_view = emscripten::val(emscripten::typed_memory_view(f->numBytes, f->data));
            
            // Create a JavaScript Array containing the Uint8Array
            val js_array = val::array();
            js_array.call<void>("push", uint8_array_view);
            
            // Create the Blob object with type 'image/jpeg'
            val options = val::object();
            options.set("type", SERIALIZE_ENCODING(_abstraction.tileTable.encoding));
            val blob = val::global("Blob").new_(js_array, options);
            
            // Pass the Blob object to the JavaScript onDone callback
            onDone(blob);
        };
        fetch->_onFailure = [layer, tile_indx, onDone](emscripten_fetch_t* f){
            std::cerr   << "[ERROR] Failed to fetch tile data for layer "
                        << layer << ", tile " << tile_indx << "\n";
            onDone(val::undefined());
        };
    
        std::string range_header = "bytes="
            + std::to_string(entry.offset) + "-"
            + std::to_string(entry.offset+entry.size-1);
        std::vector<const char*> requestHeaders = {
            "Range", range_header.c_str(),
            nullptr
        };
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.requestHeaders = requestHeaders.data();
        attr.userData = fetch;
        // Set the callbacks
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess = ON_SUCCESS;
        attr.onerror = ON_FAILURE;
        emscripten_fetch(&attr, this->_url.c_str());
        // Return immediately
    }
};
void _validateFileStructure(const std::string& url, emscripten::val onDone) {
    auto fetch = new Fetch ();
    fetch->_onSuccess = [url, onDone](emscripten_fetch_t*f)
    {
        
        try {
            if (!f || f->status != 200) throw std::runtime_error
                (f?"Failed to fetch file size (HTTP "+std::to_string(f->status)+"):"+f->statusText:
                 "Failed to fetch file size: invalid response structure (emscripten-fetch error)");

            size_t file_size = PARSE_REMOTE_FILE_SIZE (f);
            
            // Validate the remote file structure
            auto result = validate_file_structure(url, file_size);
            // Pass the result back to JavaScript
            onDone(val(result));
        } catch (std::runtime_error& error) {
            onDone(val(Iris::Result(Iris::IRIS_FAILURE,error.what())));
        }
    };
    fetch->_onFailure = [onDone](emscripten_fetch_t* f){
        std::cerr   << "[ERROR] Failed to validate file structure: "
                    << "Failed to perform HEAD request.\n";
        onDone(val(Iris::Result(Iris::IRIS_FAILURE,"Failed to perform HEAD request")));
    };
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "HEAD");
    attr.userData = fetch;
    // Set the callbacks
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = ON_SUCCESS;
    attr.onerror = ON_FAILURE;
    emscripten_fetch(&attr, url.c_str());
    // Return immediately
}
void _openSlide(const std::string& url, emscripten::val onDone) {
    auto fetch = new Fetch ();
    fetch->_onSuccess = [url, onDone](emscripten_fetch_t*f)
    {
        try {
            if (!f || f->status != 200) throw std::runtime_error
                (f?"Failed to fetch file size (HTTP "+std::to_string(f->status)+"):"+f->statusText:
                 "Failed to fetch file size: invalid response structure (emscripten-fetch error)");
            
            size_t file_size = PARSE_REMOTE_FILE_SIZE (f);
            
            // Validate the remote file structure
            auto result = validate_file_structure(url, file_size);
            if (result != IRIS_SUCCESS) throw std::runtime_error
                ("Failed to validate Iris Slide: " + result.message);
            auto file = abstract_file_structure(url, file_size);
            
            // Pass the resulting slide back to JavaScript
            onDone(val(std::make_shared<__INTERNAL__Slide>(url,file)));
            
        } catch (std::runtime_error& error) {
            onDone(val(val::undefined()));
        }
        
    };
    fetch->_onFailure = [onDone](emscripten_fetch_t* f){
        std::cerr   << "[ERROR] Failed to validate file structure: "
                    << "Failed to perform HEAD request.\n";
        onDone(val(val::undefined()));
    };
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "HEAD");
    attr.userData = fetch;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = ON_SUCCESS;
    attr.onerror = ON_FAILURE;
    emscripten_fetch(&attr, url.c_str());
    // Return immediately
}
} // END IRIS_CODEC


// MARK: - EMSCRIPTEN BINDINGS
// BEGIN BINDINGS
EMSCRIPTEN_BINDINGS(iris_codec) {
    class_<std::set<std::string>>("StringSet")
        .function("has", +[](std::set<std::string>& m, std::string key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("keys", +[](std::set<std::string>& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p);
            return arr;
        });
    class_<std::set<uint32_t>>("Uint32Set")
        .function("has", +[](std::set<uint32_t>& m, uint32_t key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("keys", +[](std::set<uint32_t>& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p);
            return arr;
        });

    
    using namespace IrisCodec;
    emscripten::value_object<Iris::Version>("Version")
        .field("major", &Iris::Version::major)
        .field("minor", &Iris::Version::minor)
        .field("build", &Iris::Version::build);

    enum_<Iris::ResultFlag>("ResultFlag")
        .value("IRIS_SUCCESS", Iris::ResultFlag::IRIS_SUCCESS)
        .value("IRIS_FAILURE", Iris::ResultFlag::IRIS_FAILURE)
        .value("IRIS_UNINITIALIZED", Iris::ResultFlag::IRIS_UNINITIALIZED)
        .value("IRIS_VALIDATION_FAILURE", Iris::ResultFlag::IRIS_VALIDATION_FAILURE)
        .value("IRIS_WARNING", Iris::ResultFlag::IRIS_WARNING)
        .value("IRIS_WARNING_VALIDATION", Iris::ResultFlag::IRIS_WARNING_VALIDATION)
        .value("RESULT_MAX_ENUM", Iris::ResultFlag::RESULT_MAX_ENUM);

    value_object<Iris::Result>("Result")
        .field("flag", &Iris::Result::flag)
        .field("message", &Iris::Result::message);

    enum_<Iris::Format>("Format")
        .value("FORMAT_UNDEFINED", Iris::FORMAT_UNDEFINED)
        .value("FORMAT_B8G8R8", Iris::FORMAT_B8G8R8)
        .value("FORMAT_R8G8B8", Iris::FORMAT_R8G8B8)
        .value("FORMAT_B8G8R8A8", Iris::FORMAT_B8G8R8A8)
        .value("FORMAT_R8G8B8A8", Iris::FORMAT_R8G8B8A8);
    enum_<IrisCodec::Encoding>("Encoding")
        .value("TILE_ENCODING_UNDEFINED", IrisCodec::TILE_ENCODING_UNDEFINED)
        .value("TILE_ENCODING_IRIS", IrisCodec::TILE_ENCODING_IRIS)
        .value("TILE_ENCODING_JPEG", IrisCodec::TILE_ENCODING_JPEG)
        .value("TILE_ENCODING_AVIF", IrisCodec::TILE_ENCODING_AVIF);

    value_object<LayerExtent>("LayerExtent")
        .field("xTiles", &LayerExtent::xTiles)
        .field("yTiles", &LayerExtent::yTiles)
        .field("scale", &LayerExtent::scale)
        .field("downsample", &LayerExtent::downsample);
    register_vector<LayerExtent>("LayerExtents");

    value_object<Iris::Extent>("Extent")
        .field("width", &Extent::width)
        .field("height", &Extent::height)
        .field("layers", &Extent::layers);
    
    enum_<MetadataType>("MetadataType")
        .value("METADATA_UNDEFINED", METADATA_UNDEFINED)
        .value("METADATA_I2S", METADATA_I2S)
        .value("METADATA_DICOM", METADATA_DICOM)
        .value("METADATA_FREE_TEXT", METADATA_FREE_TEXT);

    class_<Attributes>("Attributes")
        .function("has", +[](Attributes& m, std::string key){
            return m.find(key) == m.end() ? false : true;
        })
        .function("get",+[](Attributes& m, std::string key) -> std::string {
            auto it = m.find(key);
            return it == m.end() ? std::string{} :
            std::string(reinterpret_cast<const char* const>
                (it->second.c_str()), it->second.size());
        })
        .function("keys", +[](Attributes& m) {
            val arr = val::array();
            unsigned idx = 0;
            for (auto& p : m) arr.set(idx++, p.first);
            return arr;
        })
        .property("type", &Attributes::type)
        .property("version", &Attributes::version);
        
    value_object<Metadata>("Metadata")
        .field("codec", &Metadata::codec)
        .field("attributes", &Metadata::attributes)
        .field("associatedImages", &Metadata::associatedImages)
        .field("ICC_profile", &Metadata::ICC_profile)
        .field("annotations", &Metadata::annotations)
        .field("annotationGroups", &Metadata::annotationGroups)
        .field("micronsPerPixel", &Metadata::micronsPerPixel)
        .field("magnification", &Metadata::magnification);

    value_object<SlideInfo>("SlideInfo")
        .field("format", &SlideInfo::format)
        .field("encoding", &SlideInfo::encoding)
        .field("extent", &SlideInfo::extent)
        .field("metadata", &SlideInfo::metadata);
        
    class_<IrisCodec::__INTERNAL__Slide>("Slide")
        .smart_ptr<IrisCodec::Slide>("Slide")
        .function("getSlideCodecVersion", &IrisCodec::__INTERNAL__Slide::get_slide_codec_version)
        .function("getSlideInfo",  &IrisCodec::__INTERNAL__Slide::get_slide_info)
        .function("getMetadataJSON",  &IrisCodec::__INTERNAL__Slide::get_metadata_JSON)
        .function("getSlideTile", &IrisCodec::__INTERNAL__Slide::get_slide_tile);
    
    function("validateFileStructure", &_validateFileStructure);
    function("openIrisSlide", &_openSlide);

}
