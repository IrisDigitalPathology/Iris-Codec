/**
 * @file EncoderMain.cpp
 * @author Ryan Landvater
 * @brief 
 * @version 2025.1.0
 * @date 2025-01-23
 * 
 * Iris Codec Encoder tool for use in encoding slide files in the
 * Iris Codec File Extension format.
 *
 * The Iris Codec Encoder is licensed under the MIT software license
 *
 * @copyright Copyright (c) Ryan Landvater, 2025
 * 
 */
#if _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include <thread>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <set>
#include <iomanip>   // for std::setfill, std::setw

#include "IrisCodecCore.hpp"
constexpr char help_statement[] = 
"Iris Codec Encoder allows for the encoding of whole slide image \
(WSI) files into the Iris Codec file extension format (.iris)\n \
The Iris Codec Encoder tool is Copyright (c) 2025 Ryan Landvater  \n \
Arugments:\n \
-h --help: Print this help text \n \
-s --source: File path to the source WSI file (must be a compatible WSI format understood by OpenSlide) \n \
-o --outdir: File path to the output file directory. The encoder will name the file XXX.iris where XXX \
represents the previous file name (ex /path/to/slide.svs will be named /outdir/slide.iris) \n \
-d --derive: Generate the lower resolution layers. Options are 2x, 4x, or use-source (default)\
-sm --strip_metadata: Strip patient identifiers from the encoded metadata within the slide file \
-e --encoding: JPEG or AVIF (default JPEG)\
-c --concurrency: How many threads should this run on (defaults to all cores for fastest encoding)\
\n";
const std::u8string complt_char = u8"█";
enum ArgumentFlag : uint32_t {
    ARG_HELP    = 0,
    ARG_SOURCE,
    ARG_OUTDIR,
    ARG_DERIVE,
    ARG_STRIP_METADATA,
    ARG_ENCODING,
    ARG_CONCURRENCY,
    ARG_INVALID = UINT32_MAX
};
inline ArgumentFlag PARSE_ARGUMENT (const char* arg_str) {
    if (!strcmp(arg_str,"-h") || !strcmp(arg_str, "--help"))
        return ARG_HELP;
    if (!strcmp(arg_str,"-s") || !strcmp(arg_str, "--source"))
        return ARG_SOURCE;
    if (!strcmp(arg_str,"-o") || !strcmp(arg_str, "--outdir"))
        return ARG_OUTDIR;
    if (!strcmp(arg_str,"-d") || !strcmp(arg_str, "--derive"))
        return ARG_DERIVE;
    if (!strcmp(arg_str,"-sm") || !strcmp(arg_str, "--strip_metadata"))
        return ARG_STRIP_METADATA;
    if (!strcmp(arg_str, "-e") || !strcmp(arg_str, "--encoding"))
        return ARG_ENCODING;
    if (!strcmp(arg_str, "-c") || !strcmp(arg_str, "--concurrency"))
        return ARG_CONCURRENCY;
    return ARG_INVALID;
}
inline IrisCodec::Encoding PARSE_ENCODING (std::string arg)
{
    for (auto& c : arg) toupper(c);
    if (!strcmp(arg.c_str(), "JPEG"))
        return IrisCodec::TILE_ENCODING_JPEG;
    if (!strcmp(arg.c_str(), "AVIF"))
        return IrisCodec::TILE_ENCODING_AVIF;
    return IrisCodec::TILE_ENCODING_UNDEFINED;
}
inline IrisCodec::EncoderDerivation::Layers PARSE_DERIVATION (std::string arg)
{
    for (auto& c : arg) tolower(c);
    if (!strcmp(arg.c_str(), "2x") || !strcmp(arg.c_str(), "2"))
        return IrisCodec::EncoderDerivation::ENCODER_DERIVE_2X_LAYERS;
    if (!strcmp(arg.c_str(), "4x") || !strcmp(arg.c_str(), "4"))
        return IrisCodec::EncoderDerivation::ENCODER_DERIVE_4X_LAYERS;
    return IrisCodec::EncoderDerivation::ENCODER_DERIVE_UNDEFINED;
}
int main(int argc, char const *argv[])
{
    std::locale::global(std::locale("en_US.UTF-8"));
    
    IrisCodec::EncodeSlideInfo info;
    IrisCodec::EncoderDerivation derivation;
    bool strip_metadata     = false;
    info.desiredEncoding    = IrisCodec::TILE_ENCODING_DEFAULT;
    if (argc < 2) {
        std::cerr << help_statement;
        return EXIT_FAILURE;
    } if (argc == 2) {
        if (PARSE_ARGUMENT(argv[1]) == ARG_HELP) {
            std::cerr << help_statement;
            return EXIT_SUCCESS;
        } info.srcFilePath = std::string(argv[1]);
    } else for (auto argi = 1; argi < argc; ++argi) {
        ARGUMENT_PARSING:
        switch (PARSE_ARGUMENT(argv[argi])) {
            case ARG_HELP:
                std::cout << help_statement;
                return EXIT_SUCCESS;
            case ARG_SOURCE:
                if (argi+1>=argc) {
                    std::cerr<<"Source argument requires file path\n";
                    return EXIT_FAILURE;
                } info.srcFilePath  = std::string(argv[++argi]);
                break;
            case ARG_OUTDIR:
                if (argi+1>=argc) {
                    std::cerr<<"output argument requires dir path\n";
                    return EXIT_FAILURE;
                } info.dstFilePath  = std::string(argv[++argi]);
                break;
            case ARG_DERIVE:
                if (argi+1>=argc) {
                    std::cerr<<"if deriving layers, you must define the layer scale (2x, 4x)\n";
                    return EXIT_FAILURE;
                } derivation.layers = PARSE_DERIVATION(argv[++argi]);
                if (derivation.layers == IrisCodec::EncoderDerivation::ENCODER_DERIVE_UNDEFINED){
                    std::cerr   << "Undefined derived layer amount given " << argv[argi] << ". "
                                << "Valid values include 2x (to generate each half-size layer like DZI files) "
                                << "or 4x (to generate one layer for each 4x downsampling like SVS files).\n";
                    return EXIT_FAILURE;
                } info.derivation = &derivation;
                break;
            case ARG_STRIP_METADATA:
                strip_metadata  = true;
                break;
            case ARG_ENCODING:
                if (argi+1>=argc) {
                    std::cerr<<"encoding argument requires valid encoding variable\n";
                    return EXIT_FAILURE;
                } info.desiredEncoding = PARSE_ENCODING(std::string(argv[++argi]));
                if (info.desiredEncoding == IrisCodec::TILE_ENCODING_UNDEFINED) {
                    std::cerr<<"Undefined encoding value given " << argv[argi] << "\n";
                    return EXIT_FAILURE;
                }
                break;
            case ARG_CONCURRENCY: {
                if (argi+1>=argc) {
                    std::cerr<<"concurrency argument requires a number of threads (should be less than cores)\n";
                    return EXIT_FAILURE;
                } auto arg = std::string(argv[++argi]);
                try {info.concurrency = std::stoi(arg);}
                catch (...) {
                    std::cerr   << "Failed to parse a thread number for concurrency from the argument \""
                    << arg << "\"\n";
                    return EXIT_FAILURE;
                } if (info.concurrency > std::thread::hardware_concurrency())
                    std::cout   << "[WARNING] The thread concurrency given "
                    << arg << "is greater than the number of cores"
                    << std::thread::hardware_concurrency() << ". This is a "
                    << "bad idea; the system works best when at the number of cores (which is the default)."
                    << "If you want the greatest speed, do not define -c/--concurrency, or use it to lower performance.";
            } break;
            case ARG_INVALID:
                std::cerr   << "Unknown argument \""
                            << argv[argi]
                            << "\"\n";
                return EXIT_FAILURE;
        }
    }
    if (info.srcFilePath.size() == 0) {
        std::cerr << "Encoder requies at least an input file path of source slide file";
        return EXIT_FAILURE;
    }
    if (info.dstFilePath.size() != 0)
        if (std::filesystem::is_directory(info.dstFilePath) == false)
            std::filesystem::create_directory(info.dstFilePath);
    #if _WIN32
    struct winsize {
        unsigned short ws_col = 80;
    } console_dim;
    #else
    winsize console_dim;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_dim);
    #endif

    auto encoder = IrisCodec::create_encoder(info);
    if (!encoder) {
        std::cerr   << "Failed to create a slide encoder. The system will exit.";
        return EXIT_FAILURE;
    }
    
    // Dispatch the encoder. This will return immediately after
    // initializing the encoding process on multiple asynchronous threads
    auto result = IrisCodec::dispatch_encoder(encoder);
    if (result != Iris::IRIS_SUCCESS) {
        std::cerr   << "Encoder reported failure to begin encoding: "
                    << result.message
                    << "\n";
        return EXIT_FAILURE;
    }
    
    // Check on the encoder progress during the encoding process
    namespace time = std::chrono;
    IrisCodec::EncoderProgress progress;
    result = IrisCodec::get_encoder_progress(encoder, progress);
    std::cout << "Encoding slide file: " << progress.dstFilePath << "\n";
    float cmplt = 0.001f;
    auto  start = std::chrono::system_clock::now();
    while (progress.status == IrisCodec::ENCODER_ACTIVE) {
        
        // Check on the current encoder progress
        result = IrisCodec::get_encoder_progress(encoder, progress);
        if (result != Iris::IRIS_SUCCESS) {
            std::cerr   << "\r Error during progress check: "
                        << result.message;
            return EXIT_FAILURE;
        }
        
        // Print the progress bar
        cmplt           = progress.progress > cmplt ? progress.progress : cmplt;
        auto DUR        = time::duration_cast<time::seconds>(time::system_clock::now() - start);
        auto ETA        = static_cast<int>(static_cast<float>(DUR.count())/cmplt) - DUR.count();
        int progress_bar_width = console_dim.ws_col>>1;
        std::stringstream log;
        log << "[";
        for (int block_i = 0; block_i < progress_bar_width*cmplt; ++block_i)
            log << std::string(reinterpret_cast<const char*>(complt_char.c_str()),complt_char.size());
        std::ostringstream eta_stream;
        eta_stream << std::setfill('0') << std::setw(2) << (ETA/60)
                   << ":" << std::setfill('0') << std::setw(2) << (ETA%60);
        std::ostringstream dur_stream;
        dur_stream << std::setfill('0') << std::setw(2) << (DUR.count()/60)
                   << ":" << std::setfill('0') << std::setw(2) << (DUR.count()%60);
        log << std::string(progress_bar_width*(1-cmplt),'.')
            << "] "
            << std::setprecision(3) << cmplt*100.f
            << "%  Time Remaining "
            << eta_stream.str() << " (" << dur_stream.str() << " elapsed)";
        std::cout << "\x1b[2K" << "\r" << log.str() << std::flush;
        
        // Sleep the thread for a second between checks / updates
        std::this_thread::sleep_for(time::seconds(1));
    }
    
    result = IrisCodec::get_encoder_progress(encoder, progress);
    if (result != Iris::IRIS_SUCCESS) {
        std::cerr   << "\n Error during progress check: "
                    << result.message;
        return EXIT_FAILURE;
    } else if (progress.status == IrisCodec::ENCODER_ERROR) {
        std::cerr   << "\n Error during slide encoding: "
                    << progress.errorMsg;
    } else {
        std::cout << "\nIris Encoder completed successfully\n";
        return EXIT_SUCCESS;
    }
    
}

