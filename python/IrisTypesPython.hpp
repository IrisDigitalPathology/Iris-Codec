/**
 * @file IrisCorePython.hpp
 * @author Ryan Landvater
 * @brief  Iris Core API Python Bindings.
 * @version 2025.1.0
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 * Created by Ryan Landvater on 1/14/2025
 *
 */
#include "IrisTypes.hpp"
#include "IrisBuffer.hpp"
namespace Iris {
bool _success (const Result& result)
{
    return result == IRIS_SUCCESS;
}
py::array_t<uint8_t> _read_buffer (const Buffer &buffer)
{
    std::vector<size_t> shape {buffer->size()};
    py::array_t<uint8_t> array (shape);
    auto array_info = array.request();
    memcpy(array_info.ptr, buffer->data(), array_info.size);
    return array;
}
std::string PRINT_EXTENT (const Extent &extent)
{
    std::stringstream descr;
    descr   << "Slide Extent:\n"
            << "\tLayer 0 width: " << extent.width << "px\n"
            << "\tLayer 0 height: " << extent.height << "px\n";
    for (int l = 0; l < extent.layers.size(); ++l) {
        auto& layer = extent.layers[l];
        descr   << "\tLayer ["<<l<<"] extent:\n"
                << "\t\t x_tiles: " << layer.xTiles << "\n"
                << "\t\t y_tiles: " << layer.yTiles << "\n"
                << "\t\t scale: "   << round(layer.scale*10.f)/10.f<< "\n";
    }
    return descr.str();
}
}
static inline void DEFINE_IRIS_TYPES (pybind11::module_& m)
{
    using namespace Iris;
    using namespace pybind11::literals;
    
//    auto m = base.def_submodule
//    ("Core", "This is the Core Iris rendering submodule that allows access to the Iris Rendering System");
    
    py::class_<Iris::Version>                               (m, "Version")
        .def_readonly("major",      &Iris::Version::major)
        .def_readonly("minor",      &Iris::Version::minor)
        .def_readonly("build",      &Iris::Version::build)
        .def("__repr__",[](const Iris::Version &v) {
            std::stringstream version_str;
            version_str<<v.major<<"."<<v.minor<<"."<<v.build;
            return version_str.str();
        })
        .doc() = "Iris version (major, minor, and build number)";
    py::enum_<Iris::ResultFlag>                             (m, "ResultFlag")
        .value("Success",           IRIS_SUCCESS)
        .value("Failure",           IRIS_FAILURE)
        .value("Uninitialized",     IRIS_UNINITIALIZED)
        .value("ValidationFailure", IRIS_VALIDATION_FAILURE)
        .value("UNDEFINED_ERROR",   RESULT_MAX_ENUM);
    
    py::class_<Iris::Result>                                (m, "Result")
        .def         ("success",    &_success)
        .def_readonly("value",      &Result::flag)
        .def_readonly("message",    &Result::message)
        .def("__repr__",            [](const Iris::Result &result) {
            if (result & IRIS_FAILURE) {
                return "Failure: " + result.message + "\n";
            }
            else if (result & IRIS_WARNING) {
                return "Warning: " + result.message + "\n";
            }
            else {
                return std::string("Success\n");
            }
        });
    py::enum_<Iris::Format>                                 (m, "Format")
        .value("FORMAT_B8G8R8",     Iris::FORMAT_B8G8R8)
        .value("FORMAT_R8G8B8",     Iris::FORMAT_R8G8B8)
        .value("FORMAT_B8G8R8A8",   Iris::FORMAT_B8G8R8A8)
        .value("FORMAT_R8G8B8A8",   Iris::FORMAT_R8G8B8A8);
    py::class_<Iris::LayerExtent>                           (m, "LayerExtent")
        .def_readonly("x_tiles",    &LayerExtent::xTiles)
        .def_readonly("y_tiles",    &LayerExtent::yTiles)
        .def_readonly("scale",      &LayerExtent::scale)
        .def_readonly("downsample", &LayerExtent::downsample)
        .def("__repr__",            [](const Iris::LayerExtent &layer) {
            std::stringstream descr;
            descr   << "x_tiles: "  << layer.xTiles << "\n"
                    << "y_tiles: "  << layer.yTiles << "\n"
                    << "scale: "    << round(layer.scale*10.f)/10.f<< "\n"
                    << "downsample: " << round(layer.downsample*10.f)/10.f << "\n";
            return descr.str();
        });
    py::class_<Iris::Extent>                                (m, "Extent")
        .def_readonly("width",      &Extent::width)
        .def_readonly("height",     &Extent::height)
        .def_readonly("layers",     &Extent::layers)
        .def("__repr__",            &PRINT_EXTENT);
    py::enum_<Iris::BufferReferenceStrength>                (m, "BufferStrength")
        .value("REFERENCE_WEAK",    REFERENCE_WEAK)
        .value("REFERENCE_STRONG",  REFERENCE_STRONG);
    
    py::class_<Iris::__INTERNAL__Buffer, Iris::Buffer>      (m, "Buffer")
        .def(py::init<BufferReferenceStrength>())
        .def(py::init<BufferReferenceStrength,size_t>())
        .def("get_strength",        &__INTERNAL__Buffer::get_strength)
        .def("data",                &_read_buffer);

}
