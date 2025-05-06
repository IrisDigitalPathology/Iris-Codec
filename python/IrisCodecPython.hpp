/**
 * @file IrisCodecPython.hpp
 * @author Ryan Landvater
 * @brief  Iris Codec API Python Bindings.
 * @version 2025.1.0
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 * Created by Ryan Landvater on 1/14/2025
 *
 */
#include "IrisCodecPriv.hpp"
//#include "IrisCodecExtensionPython.hpp"
namespace IrisCodec {
Version _get_codec_version ()
{
    return Version {
        CODEC_MAJOR_VERSION,
        CODEC_MINOR_VERSION,
        CODEC_BUILD_NUMBER
    };
}
inline Result _validate_slide  (const std::string& file_path)
{
    return validate_slide ( SlideOpenInfo {
        .filePath       = file_path,
        .context        = nullptr,
        .writeAccess    = false
    });
}
inline Slide _open_slide (const std::string& file_path, const Context& context = nullptr, const bool write_access = false)
{
    Slide slide = open_slide ( SlideOpenInfo {
        .filePath       = file_path,
        .context        = context,
        .writeAccess    = write_access
    });
    return slide;
}
inline std::tuple<Result, SlideInfo> _get_info (const Slide& slide)
{
    SlideInfo info;
    return std::tuple<Result,SlideInfo>
        (get_slide_info(slide, info),info);
}
struct SlideTile {
    Buffer          data;
};
inline py::array_t<uint8_t> _read_slide_tile (const Slide& __sl, const unsigned __li, const unsigned __ti)
{
    auto shape  = std::vector<size_t>{TILE_PIX_LENGTH,TILE_PIX_LENGTH,4};
    auto array  = py::array_t<uint8_t>(shape);
    auto buffer = Iris::Wrap_weak_buffer_fom_data(array.data(0), array.size());
    auto pixels = read_slide_tile( SlideTileReadInfo {
        .slide                  = __sl,
        .layerIndex             = __li,
        .tileIndex              = __ti,
        .optionalDestination    = buffer,
        .desiredFormat          = Iris::FORMAT_R8G8B8A8
    });
    if (buffer != pixels) {
        printf("Failed to read slide pixel values into destination buffer; buffer was insuffiently sized");
        return py::array_t<uint8_t>();
    }
    return array;
}
inline py::array_t<uint8_t> _read_slide_tile_coords (const Slide& __sl, const unsigned __li, const unsigned __xi, const unsigned __yi)
{
    auto shape  = std::vector<size_t>{TILE_PIX_LENGTH,TILE_PIX_LENGTH,4};
    auto array  = py::array_t<uint8_t>(shape);
    auto buffer = Iris::Wrap_weak_buffer_fom_data(array.data(0), array.size());
    auto extent = __sl->get_slide_info().extent;
    if (__li >= extent.layers.size()) { printf
        ("read_slide_tile error: layer index (%u) out of bounds", __li);
        return array;
    }
    auto& layer = extent.layers[__li];
    if (__xi >= layer.xTiles || __yi >= layer.yTiles) {printf
        ("read_slide_tile error: tile index (%u, %u) out of layer bounds (%u,%u)",
         __xi, __yi, layer.xTiles, layer.yTiles);
        return array;
    }
    
    auto __ti = __yi * layer.xTiles + __xi;
    auto pixels = read_slide_tile( SlideTileReadInfo {
        .slide                  = __sl,
        .layerIndex             = __li,
        .tileIndex              = __ti,
        .optionalDestination    = buffer,
        .desiredFormat          = Iris::FORMAT_R8G8B8A8
    });
    if (buffer != pixels) {
        printf("Failed to read slide pixel values into destination buffer; buffer was insuffiently sized");
        return py::array_t<uint8_t>();
    }
    return array;
}
inline py::array_t<uint8_t> _read_slide_tile_channels (const Slide& __sl, const unsigned __li, const unsigned __ti)
{
    auto buffer = read_slide_tile( SlideTileReadInfo {
        .slide          = __sl,
        .layerIndex     = __li,
        .tileIndex      = __ti,
        .desiredFormat  = FORMAT_R8G8B8A8
    });
    if (!buffer) return py::array_t<uint8_t>();
    std::vector<size_t> array_shape {3,TILE_PIX_LENGTH,TILE_PIX_LENGTH};
    auto array = py::array_t<uint8_t>(array_shape);
    auto ptr_0 = const_cast<BYTE*>(array.data(0));
    auto ptr_1 = const_cast<BYTE*>(array.data(1));
    auto ptr_2 = const_cast<BYTE*>(array.data(2));
    auto src   = static_cast<uint32_t*>(buffer->data());
    // TODO: SIMD INSTRUCTIONS
    int compiler_warning_reminder_add_simd = 0;
    for (int p_i = 0; p_i < TILE_PIX_AREA; ++p_i) {
        uint32_t val    = src[p_i];
        ptr_0[p_i]      = (val>> 0) & 0xFF;
        ptr_1[p_i]      = (val>> 8) & 0xFF;
        ptr_2[p_i]      = (val>>16) & 0xFF;
    }
    return array;
}
inline std::tuple<Result,AssociatedImageInfo> _get_associated_image_info (const Slide& __sl, const std::string& label)
{
    AssociatedImageInfo info {
        .imageLabel = label
    };
    return std::tuple<Result,AssociatedImageInfo>
    (get_associated_image_info(__sl, info),info);
}
inline py::array_t<uint8_t> _read_associated_image (const Slide& __sl, const std::string& label)
{
    // Get the info for shaping
    AssociatedImageInfo info {
        .imageLabel = label
    };
    auto result = get_associated_image_info(__sl, info);
    if (result & IRIS_FAILURE)
        return py::array_t<uint8_t>();
    
    auto shape  = std::vector<size_t>{info.height,info.width,4};
    auto array  = py::array_t<uint8_t>(shape);
    auto buffer = Iris::Wrap_weak_buffer_fom_data(array.data(0), array.size());
    
    // Read the actual image bytes
    auto pixels = read_associated_image({
        .slide                  = __sl,
        .imageLabel             = label,
        .optionalDestination    = buffer,
        .desiredFormat          = Iris::FORMAT_R8G8B8A8
    });
    if (!buffer) return py::array_t<uint8_t>();
    if (buffer != pixels) {
        printf("Failed to read slide pixel values into destination buffer; buffer was insuffiently sized");
        return py::array_t<uint8_t>();
    }
    return array;
}
inline std::string PRINT_ENCODING (Encoding encoding)
{
    std::stringstream descr;
    descr << "Slide Tile Encoding: ";
    switch (encoding) {
        case TILE_ENCODING_UNDEFINED:
            descr << "Undefined encoding";
            break;
        case TILE_ENCODING_IRIS:
            descr << "Iris Codec Encoded";
            break;
        case TILE_ENCODING_JPEG:
            descr << "Jpeg Encoded";
            break;
        case TILE_ENCODING_AVIF:
            descr << "Avif Encoded";
            break;
    }
    descr << "\n";
    return descr.str();
}
inline std::string _read_attribute (const Attributes& attributes, const std::string& key)
{
    auto value = attributes.find(key);
    if (value == attributes.cend())
        return "No value returned for attribute " + key + "\n";
    return std::string(value->second.begin(),value->second.end());
}
inline std::string PRINT_ATTRIBUTE_LIST (const Attributes& attributes)
{
    std::stringstream descr;
    descr   << "Slide attributes:\n";
    for (auto&& attr : attributes)
        descr   << "\t" << attr.first << ": "
                << std::string(attr.second.begin(),attr.second.end())
                << "\n";
    return descr.str();
}
inline std::string PRINT_ASSOCIATED_IMAGE_LIST (const Metadata::ImageLabels& labels)
{
    std::stringstream descr;
    for (auto&& label : labels)
        descr << label << "\n";
    return descr.str();
}
inline std::string PRINT_METADATA (const Metadata metadata)
{
    std::stringstream descr;
    descr   << "File Metadata Description:\n"
            << "\tEncoded with Iris Codec Version: "
                << metadata.codec.major << "."
                << metadata.codec.minor << "."
                << metadata.codec.build  << "\n";
    
    descr   << "\tSlide attributes:\n";
    for (auto&& attr : metadata.attributes)
        descr   << "\t\t" << attr.first << ": "
                << std::string(attr.second.begin(),attr.second.end())
                << "\n";
    
    descr   << "\tAssociated Images:\n";
    for (auto&& label : metadata.associatedImages)
        descr   << "\t\t" << label << "\n";
    
    descr   << "\tMagnification Coefficient: "
            << metadata.magnification << "/scale\n";
    
    descr   << "\tNormalized microns per pixel: "
            << metadata.micronsPerPixel << "um/pix/scale\n";
    
    return descr.str();
}
inline std::string PRINT_SLIDE_INFO (const SlideInfo& info)
{
    return
    PRINT_ENCODING(info.encoding) +
    PRINT_EXTENT(info.extent) +
    PRINT_METADATA(info.metadata);
}
} // END IRISCODEC NAMESPACE


static inline void DEFINE_IRIS_CODEC_SUBMODULE (pybind11::module_& base)
{
    using namespace Iris;
    using namespace IrisCodec;
    using namespace pybind11::literals;
    namespace Codec = IrisCodec;
    
    auto m = base.def_submodule("Codec", "This is the Codec submodule that allows access to Iris Encoded Slide Files");
    
    // CLASS DEFINITIONS
    py::enum_<Encoding>                                     (m, "Encoding")
        .value("TILE_ENCODING_UNDEFINED",   TILE_ENCODING_UNDEFINED)
        .value("TILE_ENCODING_JPEG",        TILE_ENCODING_JPEG)
        .value("TILE_ENCODING_AVIF",        TILE_ENCODING_AVIF)
        .value("TILE_ENCODING_IRIS",        TILE_ENCODING_IRIS)
        .value("TILE_ENCODING_DEFAULT",     TILE_ENCODING_DEFAULT)
        .def("__repr__",                    &PRINT_ENCODING);
    
    py::class_<Attributes>                                  (m, "Attributes")
        .def("__getitem__",                 &_read_attribute,
             py::arg("key"), "Return the slide attribute value associated with the provided key/token")
        .def("__repr__",                    &PRINT_ATTRIBUTE_LIST)
        .def("__len__", [](const Attributes &attr){
            return attr.size();
        })
        .def("__iter__", [](const Attributes &attr){
            return py::make_key_iterator(attr.begin(), attr.end());
        }, py::keep_alive<0, 1>())
        .def("__contains__", [](const Attributes &attr,
                                const std::string &key){
            return attr.find(key)==attr.cend()?false:true;
        });
    
    py::class_<Metadata>                                    (m, "Metadata")
        .def_readonly("codec_version",      &Metadata::codec)
        .def_readonly("attributes",         &Metadata::attributes)
        .def_readonly("associated_images",  &Metadata::associatedImages)
        .def_readonly("annotations",        &Metadata::annotations)
        .def_readonly("annotation_groups",  &Metadata::annotationGroups)
        .def_readonly("microns_per_pixel",  &Metadata::micronsPerPixel)
        .def_readonly("magnification",      &Metadata::magnification)
        .def("__repr__",                    &PRINT_METADATA)
        .doc()="Slide metadata containing information about the Iris File Extension slide file";
    
    py::class_<SlideInfo>                                   (m, "SlideInfo")
        .def_readonly("extent",             &SlideInfo::extent)
        .def_readonly("encoding",           &SlideInfo::encoding)
        .def_readonly("metadata",           &SlideInfo::metadata)
        .def("__repr__",                    &PRINT_SLIDE_INFO)
        .doc() = "Basic slide information that includes the version of Iris Codec used to encode the slide file, the slide extent, both in lowest resolution pixels and layers comprising number of 256 pixel tiles";
    
    py::enum_<ImageEncoding>                                (m, "AssociatedImageEncoding")
        .value("IMAGE_ENCODING_UNDEFINED",  IMAGE_ENCODING_UNDEFINED)
        .value("IMAGE_ENCODING_PNG",        IMAGE_ENCODING_PNG)
        .value("IMAGE_ENCODING_JPEG",       IMAGE_ENCODING_JPEG)
        .value("IMAGE_ENCODING_AVIF",       IMAGE_ENCODING_AVIF)
        .value("IMAGE_ENCODING_DEFAULT",    IMAGE_ENCODING_DEFAULT);
    
    py::class_<AssociatedImageInfo>                         (m, "AssociatedImageInfo")
        .def_readonly("image_label",        &AssociatedImageInfo::imageLabel)
        .def_readonly("width",              &AssociatedImageInfo::width)
        .def_readonly("height",             &AssociatedImageInfo::height)
        .def_readonly("encoding",           &AssociatedImageInfo::encoding)
        .def_readonly("source_format",      &AssociatedImageInfo::sourceFormat);
//        .def_readonly("orientation",        &AssociatedImageInfo::orientation);
    
    py::class_<Codec::__INTERNAL__Context,  Codec::Context> (m, "Context")
        .doc() = "Iris Codec Context helper class used for access to GPU encoding and decoding methods. Use of a context is most beneficial when encoding or decoding multiple files as it can optimize and orchistrate an encoding queue.";
    
    py::class_<Codec::__INTERNAL__Slide, Codec::Slide>      (m, "Slide")
        .def("get_info", &_get_info)
        .def("read_slide_tile",             &_read_slide_tile,
             py::arg("layer_index") = 0,
             py::arg("tile_index")  = 0)
        .def("read_slide_tile",             &_read_slide_tile_coords,
             py::arg("layer_index") = 0,
             py::arg("x_tile_index")= 0,
             py::arg("y_tile_index")= 0)
        .def("read_slide_tile_channels",    &_read_slide_tile_channels,
             "Returns slide tile pixel data in the form of a [4,256,256] numpy array with each major",
             py::arg("layer_index") = 0,
             py::arg("tile_index")  = 0)
        .def("get_associated_image_info",   &_get_associated_image_info,
             "Return image information about an associated image such as label or thumbnail",
             py::arg("image_label"))
        .def("read_associated_image",       &_read_associated_image,
             "Return a pixel array containing an associated image such as a label or thubnail in R8G8B8A8 format [width,height,4] numpy array",
             py::arg("image_label"))
        .doc() = "Iris Codec Slide (distinct from Iris Core Slide, loader class) is a light-weight wrapper around a whole slide image file used to access slide information and raw image data";
    
    // FUNCTION DEFINITIONS
    m.def("get_codec_version", &Codec::_get_codec_version,
          "Get version of the Iris Codec module specifically, which is distinct from the overall Iris version (Iris.Core.get_version())");
    
    m.def("validate_slide_path", &Codec::_validate_slide,
          "Method to validate a whole slide image file to assess if the file path can produce a valid Iris Codec Slide. Returns a Result with possible erorr message on failure",
          py::arg("file_path"));
    
    m.def("open_slide",         &_open_slide,
          "Method to open a whole slide image file at the given file path with optional Iris Codec Context for efficiency and GPU optimizations as well as a write_access boolean flag request mutable access. Returns a NULL object on failure. For more information about failure, use validate_slide_path and examine the result.message",
          py::arg("file_path"),
          py::arg("codec_context")  = Codec::Context(),
          py::arg("write_access")   = false);
    
}
