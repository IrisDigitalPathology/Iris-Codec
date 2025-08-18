//
//  IrisCodecPriv.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//

#ifndef IrisCodecPriv_h
#define IrisCodecPriv_h
#ifndef CODEC_MAJOR_VERSION
#define CODEC_MAJOR_VERSION 2025
#endif
#ifndef CODEC_MINOR_VERSION
#define CODEC_MINOR_VERSION 3
#endif
#ifndef CODEC_BUILD_NUMBER
#define CODEC_BUILD_NUMBER  0
#endif
#ifndef FLT16_MIN
#define _Float16            float // use 32-bit if no 16
#endif
#ifndef U16_CAST
#define U16_CAST(X)         static_cast<uint16_t>(X)
#endif
#ifndef U32_CAST
#define U32_CAST(X)         static_cast<uint32_t>(X)
#endif
#ifndef F16_CAST
#define F16_CAST(X)         static_cast<_Float16>(X)
#endif
#ifndef F32_CAST
#define F32_CAST(X)         static_cast<float>(X)
#endif
#ifndef F64_CAST
#define F64_CAST(X)         static_cast<double>(X)
#endif
#ifndef IRIS_INCLUDE_OPENSLIDE
#define IRIS_INCLUDE_OPENSLIDE 1
#endif
#include <iostream>
#include <assert.h>
#include "IrisCore.hpp"
#include "IrisCodecCore.hpp"
#include "IrisBuffer.hpp"
#include "IrisQueue.hpp"
#include "IrisAsync.hpp"
#include "IrisCodecExtension.hpp"
#include "IrisCodecPrivTypes.hpp"
#include "IrisCodecFile.hpp"
#include "IrisCodecContext.hpp"
#include "IrisCodecSlide.hpp"
#include "IrisCodecCache.hpp"
#include "IrisCodecEncoder.hpp"

#endif /* IrisCodecPriv_h */
