//
//  IrisCodecSlide.hpp
//  Iris
//
//  Created by Ryan Landvater on 1/9/24.
//

#ifndef IrisCodecSlide_hpp
#define IrisCodecSlide_hpp
namespace IrisCodec {
class __INTERNAL__Slide {
    const Context                               _context;
    const File                                  _file;
    const Abstraction::File                     _abstraction;
public:
    explicit __INTERNAL__Slide                  (const Context&, const File&);
    __INTERNAL__Slide                           (const __INTERNAL__Slide&) = delete;
    __INTERNAL__Slide operator =                (const __INTERNAL__Slide&) = delete;
   ~__INTERNAL__Slide                           ();
    
    // Return codec version used to encode the slide
    Version             get_slide_codec_version () const;
    // Return the slide information
    SlideInfo           get_slide_info          () const;
    // Get the compressed slide tile entry
    Buffer              get_slide_tile_entry    (uint32_t layer, uint32_t tile_indx) const;
    // Read the slide tile entry to return a decompressed tile
    Buffer              read_slide_tile         (const SlideTileReadInfo&) const;
    // Get information about an associated image
    AssociatedImageInfo get_assoc_image_info    (const std::string& image_label) const;
    // Get the compressed associated image stream
    Buffer              get_assoc_image         (const std::string& image_label) const;
    // Read the associated image to return a decompressed image
    Buffer              read_assc_image         (const AssociatedImageReadInfo&) const;
    
    // Write a slide annotation to the file stream.
    Result              write_slide_annotation  (const IrisCodec::Annotation&);
};
} // END IRIS CODEC NAMESPACE
#endif /* IrisCodecSlide_hpp */
