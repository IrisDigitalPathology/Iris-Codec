//
//  IrisCodecEncoder.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/14/24.
//

#ifndef IrisCodecEncoder_hpp
#define IrisCodecEncoder_hpp
namespace IrisCodec {
struct EncoderTracker;
using AtomicEncoderStatus           = std::atomic<EncoderStatus>;
using DerivationQueue               = Iris::Async::ThreadPool;
class __INTERNAL__Encoder {
    enum SourceType {
        ENCODER_SOURCE_UNDEFINED    = 0,
        ENCODER_SOURCE_FILE,
        ENCODER_SOURCE_CACHE
    }                               _srcType        = ENCODER_SOURCE_UNDEFINED;
    unsigned                        _concurrency    = std::thread::hardware_concurrency();
    bool                            _derive;
    const Context                   _context;
    std::string                     _srcPath;
    std::string                     _dstPath;
    bool                            _anonymize;
    Encoding                        _encoding;
    EncoderDerivation               _derivation;
    Threads                         _threads;
    EncoderTracker                  _tracker;
    AtomicEncoderStatus             _status;
public:
    explicit __INTERNAL__Encoder    (const IrisCodec::EncodeSlideInfo& info);
    __INTERNAL__Encoder             (const __INTERNAL__Encoder&) = delete;
    __INTERNAL__Encoder& operator = (const __INTERNAL__Encoder&) = delete;
   ~__INTERNAL__Encoder             ();
    
    EncoderStatus get_status        () const;
    Context     get_context         () const;
    std::string get_src_path        () const;
    std::string get_dst_path        () const;
    Encoding    get_encoding        () const;
    Result  get_encoder_progress    (EncoderProgress&) const;
    
    void    set_src_path            (const std::string& source);
    void    set_src_cache           (const Cache& source);
    void    set_dst_path            (const std::string& destination);
    void    set_encoding            (Encoding desired_encoding);
    Result  reset_encoder           ();
    Result  dispatch_encoder        ();
    Result  interrupt_encoder       ();
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecEncoder_hpp */
