/**
 * @file IrisPython.cpp
 * @author Ryan Landvater
 * @brief  Iris API Python Bindings.
 * @version 2025.1.0
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 * Created by Ryan Landvater on 1/14/2025
 *
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py    = pybind11;

#include "IrisTypesPython.hpp"
#include "IrisCodecPython.hpp"
PYBIND11_MODULE(iris_core, m)
{
    // Creae the Iris Core Submodule
    DEFINE_IRIS_TYPES  (m);
    
    // Create the Iris Codec Submodule
    DEFINE_IRIS_CODEC_SUBMODULE (m);
}
