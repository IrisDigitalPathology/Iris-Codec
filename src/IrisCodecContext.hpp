//
//  IrisCodecContext.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//

#ifndef IrisCodecContext_hpp
#define IrisCodecContext_hpp
namespace IrisCodec {
using namespace Iris;
class __INTERNAL__Context {
    Device                              _device         = NULL;
    bool                                _gpuAV1Decode   = false;
    bool                                _gpuAV1Encode   = false;
public:
    explicit __INTERNAL__Context        (const ContextCreateInfo&);
    __INTERNAL__Context                 (const __INTERNAL__Context&) = delete;
    __INTERNAL__Context operator =      (const __INTERNAL__Context&) = delete;
   ~__INTERNAL__Context                 ();
    Quality     get_quality             () const;
    Subsampling get_subsampling         () const;
    void        set_quality             (Quality);
    void        set_subsampling         (Subsampling);
    
    Buffer compress_tile                (const CompressTileInfo&) const;
    Buffer decompress_tile              (const DecompressTileInfo&) const;
    Buffer compress_image               (const CompressImageInfo&) const;
    Buffer decompress_image             (const DecompressImageInfo&) const;
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecContext_hpp */
