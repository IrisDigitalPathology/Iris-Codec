/**
 * @file IrisCodecDcmBridge.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-08-16
 *
 * This file provides a bridge for working with DICOM files
 * in the Iris Codec Encoder. It includes utilities for reading
 * DICOM metadata, extracting image frames, and handling
 * DICOM-specific attributes. The code is designed to simplify
 * DICOM parsing and ensure compatibility with the Iris Codec's requirements.
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "IrisCodecPriv.hpp"
#include <filesystem>
#include <sstream>

#ifdef __cplusplus
extern "C" {
#endif
#include <dicom/dicom.h>
#ifdef __cplusplus
}
#endif

// For anyone reading this...
// ... I really hate trying to work with DICOM
// In a complicated world, just try to make things simple

namespace IrisCodec {
struct DicomLevelDimension {
    std::filesystem::path   filePath;
    DcmFilehandle* handle   = NULL;
    uint64_t width          = 0; // tile width
    uint64_t height         = 0; // tile height
    uint64_t frames         = 0; // Total number of frames
    uint64_t fullColumns    = 0; // TotalPixelMatrixColumns
    uint64_t fullRows       = 0; // TotalPixelMatrixRows
};

// Get StudyInstanceUID from a DICOM file
inline std::string GET_STUDY_INSTANCE_UID (DcmFilehandle* fh) {
    DcmError* error = nullptr;
    const DcmDataSet* dataset = dcm_filehandle_get_metadata_subset(&error, fh);
    if (!dataset) throw std::runtime_error
        ("Failed to retrieve study DICOM metadata");
    
    DcmElement* element = dcm_dataset_get
        (&error, dataset, dcm_dict_tag_from_keyword("StudyInstanceUID"));
    
    if (!element) throw std::runtime_error
        ("StudyInstanceUID not found in DICOM file");

    const char* value = nullptr;
    if (!dcm_element_get_value_string(&error, element, 0, &value))
        throw std::runtime_error("Failed to get StudyInstanceUID value");
    
    return value ? std::string(value) : "";
}

// Get level dimensions (width, height) from a DICOM file
inline DicomLevelDimension GET_LEVEL_DIMENSION
 (DcmFilehandle* fh, const std::filesystem::path& filePath) {
    DicomLevelDimension dim;
    DcmError* error = nullptr;
    
    const DcmDataSet* dataset = dcm_filehandle_get_metadata_subset(&error, fh);
    if (!dataset) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        (std::string("Failed to get metadata for level. ") +
         dcm_error_get_message(error));
    }
    
    DcmElement* element = NULL;
    const char* string  = NULL;
    int64_t cols = 0, rows = 0, frames = 0, fullCols = 0, fullRows = 0;
    element = dcm_dataset_get(&error, dataset, 0x00280011);
    if (!element || !dcm_element_get_value_integer(&error, element, 0, &cols)) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        (std::string("Failed to get Columns value for level ") + msg);
    }
    element = dcm_dataset_get(&error, dataset, 0x00280010);
    if (!element || !dcm_element_get_value_integer(&error, element, 0, &rows)) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        ("Failed to get Rows value for level "+ msg);
    }
    element = dcm_dataset_get(&error, dataset, 0x00280008);
    if (!element || !dcm_element_get_value_string(&error, element, 0, &string)) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        ("Failed to get total number of frames for level " + msg);
    } frames = std::stoll(string);
    element = dcm_dataset_get(&error, dataset, 0x00480006);
    if (!element || !dcm_element_get_value_integer(&error, element, 0, &fullCols)) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        ("Failed to get TotalPixelMatrixColumns value for level " + msg);
    }
    element = dcm_dataset_get(&error, dataset, 0x00480007);
    if (!element || !dcm_element_get_value_integer(&error, element, 0, &fullRows)) {
        const auto c_err = dcm_error_get_message(error);
        std::string msg = c_err?c_err:"";
        dcm_error_clear(&error);
        throw std::runtime_error
        ("Failed to get TotalPixelMatrixRows value for level " + msg);
    }
    
    return DicomLevelDimension {
        .filePath       = filePath,
        .handle         = fh,
        .width          = static_cast<uint64_t>(cols),
        .height         = static_cast<uint64_t>(rows),
        .frames         = static_cast<uint64_t>(frames),
        .fullColumns    = static_cast<uint64_t>(fullCols),
        .fullRows       = static_cast<uint64_t>(fullRows),
    };
}

// Find all DICOM files in the directory with the same StudyInstanceUID
inline std::vector<std::filesystem::path> FIND_DICOM_FILE_WITH_STUDY_UID
 (const std::filesystem::path& directory, const std::string& studyUID) {
    namespace fs = std::filesystem;
    if (!fs::exists(directory) || !fs::is_directory(directory))
        throw std::runtime_error
        ("The provided DICOM path directory "+directory.string()+
         " is an invalid directory path.");
    std::vector<fs::path> result;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dcm") {
            auto fh = dcm_filehandle_create_from_file(NULL, entry.path().string().c_str());
            if (fh) {
                std::string uid = GET_STUDY_INSTANCE_UID(fh);
                if (studyUID.compare(uid) == 0)
                    result.push_back(entry.path());
                dcm_filehandle_destroy(fh);
            }
        }
    }
    return result;
}

constexpr char DICOM_JPEG_8 []  = "1.2.840.10008.1.2.4.50";
constexpr char DICOM_JPEG_12 [] = "1.2.840.10008.1.2.4.51";
constexpr char DICOM_JPEG_Pr [] = "1.2.840.10008.1.2.4.70";
inline Encoding PARSE_DICOM_ENCODING (DcmFilehandle* handle)
{
    const char* TS_UID = dcm_filehandle_get_transfer_syntax_uid(handle);
    if (!TS_UID) return TILE_ENCODING_UNDEFINED;

    else if (!strcmp(TS_UID, DICOM_JPEG_8)) return TILE_ENCODING_JPEG;
    else if (!strcmp(TS_UID, DICOM_JPEG_12)) return TILE_ENCODING_JPEG;
    else if (!strcmp(TS_UID, DICOM_JPEG_Pr)) return TILE_ENCODING_JPEG;

    return TILE_ENCODING_UNDEFINED;
}

// I know this is recursive and therefore potentially unsafe but it's DICOM and
// I just don't have the energy to fix DICOM parsing...
// The compiler should inline all of it so the recusion should go away...
inline DcmElement* PARSE_NESTED_ELEMENT (const DcmDataSet* ds, std::vector<uint32_t> tags)
{
    DcmError* error = NULL;
    uint32_t tag = tags.front();
    tags.erase(tags.begin());
    if (tags.size()) {
        auto element = dcm_dataset_get(&error, ds, tag);
        if (!element || dcm_dict_vr_class(dcm_element_get_vr(element)) != DCM_VR_CLASS_SEQUENCE) {
            std::cout   << "[WARNING] Could not retrieve DICOM tag ("
                        << tag << ") from nested sequence: "
                        << dcm_error_get_message(error) << "\n";
            dcm_error_clear (&error);
            return NULL;
        }
        
        DcmSequence* sequence = nullptr;
        if (!dcm_element_get_value_sequence(&error, element, &sequence)) {
            std::cout   << "[WARNING] Could not retrieve DICOM sequence ("
                        << tag << "): "
                        << dcm_error_get_message(error) << "\n";
            dcm_error_clear (&error);
            return NULL;
        }
        
        ds = dcm_sequence_get(&error, sequence, 0);
        if (!ds) {
            std::cout   << "[WARNING] No items in Shared Functional Groups Sequence ("
                        << tag << "): "
                        << dcm_error_get_message(error) << "\n";
            dcm_error_clear (&error);
            return NULL;
        }
        
        return PARSE_NESTED_ELEMENT(ds, tags);
    } else {
        auto element = dcm_dataset_get(&error, ds, tag);
        if (!element) {
            std::cout   << "[WARNING] Could not retrieve DICOM tag ("
                        << tag << ") from nested sequence: "
                        << dcm_error_get_message(error) << "\n";
            dcm_error_clear (&error);
            return NULL;
        } return element;
    }
}
inline float PARSE_MPP (const DcmDataSet* ds)
{
    std::vector<uint32_t> tags {
        dcm_dict_tag_from_keyword("SharedFunctionalGroupsSequence"),
        dcm_dict_tag_from_keyword("PixelMeasuresSequence"),
        dcm_dict_tag_from_keyword("PixelSpacing")
    };
    const char* string = NULL;
    DcmElement* element = PARSE_NESTED_ELEMENT(ds, tags);
    if (!element) goto FAILED_PARSE;
    
    string = dcm_element_value_to_string(element);
    if (!string) goto FAILED_PARSE;
    try{
        return std::stof(std::string((char*)string).substr(1, ',')) * 1000.f;
    } catch (...) {
        goto FAILED_PARSE;
    }
    
FAILED_PARSE:
    std::cout   << "[WARNING] Failed to get DICOM SharedFunctionalGroupsSequence/"
    << "PixelMeasuresSequence/PixelSpacing nested sequence entry. "
    << "The resulting slide will not be able to provide on-slide measurments.\n";
    return 0;
}
inline float PARSE_MAGNIFICATION (const DcmDataSet* ds)
{
    std::vector<uint32_t> tags {
        dcm_dict_tag_from_keyword("OpticalPathSequence"),
        dcm_dict_tag_from_keyword("ObjectiveLensPower")
    };
    const char* string = NULL;
    DcmElement* element = PARSE_NESTED_ELEMENT(ds, tags);
    if (!element) goto FAILED_PARSE;
    
    string = dcm_element_value_to_string (element);
    if (!string) goto FAILED_PARSE;
    try{
        return std::stof(string);
    } catch (...) {
        goto FAILED_PARSE;
    }
    
FAILED_PARSE:
    std::cout   << "[WARNING] Failed to get DICOM OpticalPathSequence/"
    << "ObjectiveLensPower/ nested sequence entry. The resulting slide will "
    << "not be able to provide a microscope objective equivalent.\n";
    return 0;
}

const std::string quote = "\"";
static bool COLLECT_SEQ (const DcmDataSet* set, uint32_t index, void* stream_ptr);
static bool COLLECT_SEQ_TAG (const DcmElement* element, void* stream_ptr) {
    auto& __stream = *reinterpret_cast
    <std::stringstream*>(stream_ptr);
    
    auto tag = dcm_element_get_tag(element);
    // Excluded tags switch
    switch (tag) {
        case 0x00282000: return true; // ICC Profile
        default: break;
    }
    
    std::string val;
    if (dcm_dict_vr_class(dcm_element_get_vr(element))
        == DCM_VR_CLASS_SEQUENCE) {
        std::stringstream stream;
        stream << "[";
        DcmSequence* sequence = nullptr;
        if (!dcm_element_get_value_sequence(NULL, element, &sequence))
            return true;
        
        dcm_sequence_foreach(sequence, COLLECT_SEQ, &stream);
        stream.seekp(-1, stream.cur) << "]";
        val = stream.str();
        if (val.length() == 1) val = std::string();
    } else val = quote + dcm_element_value_to_string(element) + quote;
    
    if (tag && !val.empty())
        __stream <<"\"" << std::to_string(tag) << "\":" << val << ",";
    
    return true;
}
static bool COLLECT_SEQ (const DcmDataSet* set, uint32_t index, void* stream_ptr) {
    dcm_dataset_foreach(set, COLLECT_SEQ_TAG, stream_ptr);
    return true;
}
static bool COLLECT_TAG (const DcmElement* element, void* attr_ptr)
{
    auto& attributes = *reinterpret_cast
    <Attributes*>(attr_ptr);
    
    auto tag = dcm_element_get_tag(element);
    switch (tag) {
        default: break;
    }
    std::string val;
    if (dcm_dict_vr_class(dcm_element_get_vr(element))
        == DCM_VR_CLASS_SEQUENCE) {
        std::stringstream stream;
        stream << "[";
        DcmSequence* sequence = nullptr;
        if (!dcm_element_get_value_sequence(NULL, element, &sequence))
            return true;
        
        dcm_sequence_foreach(sequence, COLLECT_SEQ, &stream);
        stream.seekp(-1, stream.cur) << "]";
        val = stream.str();
        if (val.length() == 1) val = std::string();
    } else val = dcm_element_value_to_string(element);
    
    if (tag && !val.empty())
        attributes[std::to_string(tag)] = std::u8string((char8_t*)val.data());
    
    return true;  // Continue iteration
}
const std::chrono::time_point NOW {std::chrono::system_clock::now()};
const std::chrono::year_month_day YMD {std::chrono::floor<std::chrono::days>(NOW)};
inline Metadata PARSE_DICOM_ATTRIBUTES (const DcmDataSet* ds, Metadata& metadata, bool anonymize) {
    
    auto& attrib    = metadata.attributes;
    attrib.type     = METADATA_DICOM;
    // DICOM does not exactly use versions.
    // As a compromise, Iris fills this in with the year to give some general idea of version.
    auto element = dcm_dataset_get(NULL, ds, 0x00080020);
    if (!element) {
        std::cout   << "[WARNING] Could not retrieve DICM study date. "
                    << "Assigning current year as DICOM file version instead.\n";
        attrib.version  = static_cast<int>(YMD.year());
    } else {
        try {
            attrib.version = std::stoi
            (std::string(dcm_element_value_to_string(element)).substr(0,4));
        } catch (...) {
            attrib.version  = static_cast<int>(YMD.year());
        }
    }
    
    if (!anonymize) {
        dcm_dataset_foreach(ds, COLLECT_TAG, &attrib);
    }
    
    metadata.micronsPerPixel = PARSE_MPP(ds);
    metadata.magnification   = PARSE_MAGNIFICATION(ds);
    return metadata;
}
inline void PARSE_ICC_PROFILE (const DcmDataSet* ds, Metadata& metadata)
{
    std::vector<uint32_t> tags {
        dcm_dict_tag_from_keyword("OpticalPathSequence"),
        dcm_dict_tag_from_keyword("ICCProfile")
    };
    const void* data = NULL;
    DcmElement* element = PARSE_NESTED_ELEMENT(ds, tags);
    DcmError* error = NULL;
    if (!element ||
        dcm_dict_vr_class(dcm_element_get_vr(element))!=DCM_VR_CLASS_BINARY ||
        dcm_element_get_value_binary(&error, element, &data) == false) {
        std::string msg = dcm_error_get_message(error)?dcm_error_get_message(error):"";
        dcm_error_clear(&error);
        std::cout   << "[WARNING] Failed to get DICOM OpticalPathSequence/"
        << "ObjectiveLensPower/ nested sequence entry: "
        << msg << ". The resulting slide will "
        << "not be able to provide a microscope objective equivalent.\n";
    }
    metadata.ICC_profile = std::string
    ((const char*)data, dcm_element_get_length(element));
}
struct __INTERNAL__DcmFile {
    using Levels = std::vector<DicomLevelDimension>;
    const std::string   _study;
    Levels              _levels;
    Encoding            _encoding;
    __INTERNAL__DcmFile (const std::string& __s) : _study(__s){}
    __INTERNAL__DcmFile (const __INTERNAL__DcmFile&) = delete;
    __INTERNAL__DcmFile operator = (const __INTERNAL__DcmFile&) = delete;
    ~__INTERNAL__DcmFile ()
    {
        for (auto& level : _levels) {
            dcm_filehandle_destroy(level.handle);
        }
    }
};

// Open a DICOM file and return a DcmFile containing all levels for the study, ordered from lowest to highest resolution
DcmFile open_dicom_file(const std::filesystem::path& filePath) {
    DcmError* error = nullptr;
    DcmFilehandle* fh = dcm_filehandle_create_from_file(&error, filePath.string().c_str());
    if (!fh) {
        std::string msg = "DICOM open error: ";
        if (error) {
            msg += dcm_error_get_message(error);
            dcm_error_clear(&error);
        } else {
            msg += "Unknown error";
        }
        throw std::runtime_error(msg);
    }
    auto UID = GET_STUDY_INSTANCE_UID(fh);
    auto file = std::make_shared<__INTERNAL__DcmFile>(UID);
    dcm_filehandle_destroy(fh);
    
    // Find all files in the directory with the same StudyInstanceUID
    auto  dir    = filePath.parent_path();
    auto  files  = FIND_DICOM_FILE_WITH_STUDY_UID(dir, UID);
    auto& levels = file->_levels;
    for (auto&& f : files) {
        fh = dcm_filehandle_create_from_file(&error, f.string().c_str());
        if (!fh) {
            std::string msg = "DICOM open error: ";
            if (error) {
                msg += dcm_error_get_message(error);
                dcm_error_clear(&error);
            } else {
                msg += "Unknown error";
            }
            throw std::runtime_error(msg);
        }
        levels.push_back(DicomLevelDimension());
        levels.back().handle = fh; // In case error thrown
        levels.back() = GET_LEVEL_DIMENSION(fh, f);
        
        // If this is a thumbnail or label image, remove it
        if (levels.back().width == levels.back().fullColumns ||
            levels.back().height == levels.back().fullRows)
            levels.pop_back();
    }
    // Sort levels from lowest to highest resolution (by width ascending, then height ascending)
    std::sort(levels.begin(), levels.end(), [](const DicomLevelDimension& a, const DicomLevelDimension& b) {
        return a.fullRows*a.fullColumns < b.fullRows*b.fullColumns;
    });
    
    // Make sure the base level complies
    if (levels.back().width != TILE_PIX_LENGTH ||
        levels.back().height != TILE_PIX_LENGTH) {
        throw std::runtime_error("DICOM base level tile dimensions ("+
                                 std::to_string(levels.back().width) +
                                 "px by "+ std::to_string(levels.back().height) +
                                 "px) are not compatible with Iris (256 px)");
    }
    file->_encoding = PARSE_DICOM_ENCODING(levels.back().handle);
    if (file->_encoding == TILE_ENCODING_UNDEFINED)
        throw std::runtime_error("DICOM tile encoding incompatible with Iris.");
    
    
    // STRIP NON-COMPLIANT LEVELS
    for (auto l_it = levels.begin(); l_it != levels.cend();) {
        if (l_it->width != TILE_PIX_LENGTH || l_it->height != TILE_PIX_LENGTH) {
            std::cout   << "[WARNING] DICOM level " << l_it->filePath
                        << "level tile dimensions ("
                        << l_it->width << "px by " << l_it->height
                        << ") are not compatible with Iris (256 px). Removing level.\n";
            levels.erase(l_it);
            continue;
        }
        if (file->_encoding != PARSE_DICOM_ENCODING(l_it->handle)) {
            std::cout   << "[WARNING] DICOM level " << l_it->filePath
                        << "level encoding ("
                        << dcm_filehandle_get_transfer_syntax_uid(l_it->handle)
                        << ") is inconsistent with the base level. Removing deviant.\n";
            levels.erase(l_it);
            continue;
        }
        // Iterate
        ++l_it;
    }
    
    for (auto l_it = levels.begin(); l_it != levels.cend();) {
        if (!dcm_filehandle_prepare_read_frame(&error, l_it->handle)) {
            std::cout   << "[WARNING] DICOM level " << l_it->filePath
                        << "level unable to generate frame offset table ("
                        << error
                        << "). Removing level.\n";
            levels.erase(l_it);
            dcm_error_clear(&error);
            continue;
        } ++l_it;
    }
    
    if (!levels.size()) throw std::runtime_error
        ("Following parsing, there are no viable layers within this DICOM file.");
    
    if (error) dcm_error_clear (&error);
    return file;
}
uint32_t get_dicom_number_of_levels (DcmFile dicom)
{
    return U32_CAST(dicom->_levels.size());
}
uint32_t get_dicom_number_of_frames  (DcmFile dicom, unsigned level)
{
    if (level < dicom->_levels.size())
        return U32_CAST(dicom->_levels[level].frames);
    throw std::runtime_error("Level "+std::to_string(level)+
                             "is out of DICOM file range.");
}
uint32_t get_dicom_layer_tile_width (DcmFile dicom, unsigned level)
{
    if (level < dicom->_levels.size())
        return U32_CAST(dicom->_levels[level].width);
    throw std::runtime_error("Level "+std::to_string(level)+
                             "is out of DICOM file range.");
}
uint32_t get_dicom_layer_tile_height (DcmFile dicom, unsigned level)
{
    if (level < dicom->_levels.size())
        return U32_CAST(dicom->_levels[level].height);
    throw std::runtime_error("Level "+std::to_string(level)+
                             "is out of DICOM file range.");
}
uint32_t get_dicom_layer_width (DcmFile dicom, unsigned level)
{
    if (level < dicom->_levels.size())
        return U32_CAST(dicom->_levels[level].fullColumns);
    throw std::runtime_error("Level "+std::to_string(level)+
                             "is out of DICOM file range.");
}
uint32_t get_dicom_layer_height (DcmFile dicom, unsigned level)
{
    if (level < dicom->_levels.size())
        return U32_CAST(dicom->_levels[level].fullRows);
    throw std::runtime_error("Level "+std::to_string(level)+
                             "is out of DICOM file range.");
}
Encoding get_dicom_encoding(DcmFile dicom)
{
    return dicom->_encoding;
}
// Read a frame from a DICOM image at the specified level and frame index, returning an Iris::Buffer
Buffer get_dicom_frame_buffer(DcmFile dicom, unsigned levelIndex, unsigned frameIndex) {
    if (levelIndex >= dicom->_levels.size()) throw std::runtime_error
        ("Level " + std::to_string(levelIndex) +
        " is out of DICOM file range.");
    
    auto& level = dicom->_levels[levelIndex];
    DcmError* error = nullptr;
    if (frameIndex > level.frames) throw std::runtime_error
        ("Frame " + std::to_string(frameIndex) +
        " is out of DICOM layer (" +level.filePath.string()+ ")range.");
    
    DcmFilehandle* handle = level.handle;
    DcmFrame* frame = dcm_filehandle_read_frame(&error, handle, frameIndex);
    if (!frame) {
        std::cout   << "[ERROR] Failed to read DICOM tile "
                    << "(level " << levelIndex
                    << ", frame " << frameIndex << "): "
                    << dcm_error_get_message(error) << "\n";
        dcm_error_clear (&error);
        return NULL;
    }
    auto buffer = Copy_strong_buffer_from_data
    (dcm_frame_get_value(frame),dcm_frame_get_length(frame));
    dcm_frame_destroy(frame);
    return buffer;
}
Metadata get_dicom_metadata (DcmFile dicom, bool anonymize)
{
    DcmError* error = NULL;
    auto& fh = dicom->_levels.back().handle;
    const DcmDataSet* ds = dcm_filehandle_get_metadata_subset(&error, fh);
    if (!ds) throw std::runtime_error
        ("Failed to retrieve study DICOM metadata");
    
    Metadata metadata;
    auto s = metadata.attributes;
    
    PARSE_DICOM_ATTRIBUTES  (ds, metadata, anonymize);
    PARSE_ICC_PROFILE       (ds, metadata);
    
    return metadata;
}
} // END IRIS CODEC NAMESPACE

