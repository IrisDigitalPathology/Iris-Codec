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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <set>
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
-sm --strip_metadata: Strip patient identifiers from the encoded metadata within the slide file \
-e --encoding: JPEG or AVIF (default JPEG)\
\n";
const std::u8string complt_char = u8"█";
enum ArgumentFlag : uint32_t {
    ARG_HELP    = 0,
    ARG_SOURCE,
    ARG_OUTDIR,
    ARG_STRIP_METADATA,
    ARG_ENCODING,
    ARG_INVALID = UINT32_MAX
};
inline ArgumentFlag PARSE_ARGUMENT (const char* arg_str) {
    if (strstr(arg_str,"-h") || strstr(arg_str, "--help"))
        return ARG_HELP;
    if (strstr(arg_str,"-s") || strstr(arg_str, "--source"))
        return ARG_SOURCE;
    if (strstr(arg_str,"-o") || strstr(arg_str, "--outdir"))
        return ARG_OUTDIR;
    if (strstr(arg_str,"-sm") || strstr(arg_str, "--strip_metadata"))
        return ARG_STRIP_METADATA;
    if (strstr(arg_str, "-e") || strstr(arg_str, "--encoding"))
        return ARG_ENCODING;
    return ARG_INVALID;
}
inline IrisCodec::Encoding PARSE_ENCODING (std::string encoding_string)
{
    for (auto& c : encoding_string) c = toupper(c);
    if (strcmp(encoding_string.c_str(), "JPEG") == 0)
        return IrisCodec::TILE_ENCODING_JPEG;
    if (strcmp(encoding_string.c_str(), "AVIF") == 0)
        return IrisCodec::TILE_ENCODING_AVIF;
    return IrisCodec::TILE_ENCODING_UNDEFINED;
}
int main(int argc, char const *argv[])
{
    std::string source_path;
    std::string out_path;
    std::locale::global(std::locale("en_US.UTF-8"));
    
    bool strip_metadata = false;
    auto encoding       = IrisCodec::TILE_ENCODING_DEFAULT;
    if (argc < 2) {
        std::cerr << help_statement;
        return EXIT_FAILURE;
    } if (argc == 2) {
        if (PARSE_ARGUMENT(argv[1]) == ARG_HELP) {
            std::cerr << help_statement;
            return EXIT_SUCCESS;
        } source_path = std::string(argv[1]);
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
                } source_path   = std::string(argv[++argi]);
                break;
            case ARG_OUTDIR:
                if (argi+1>=argc) {
                    std::cerr<<"output argument requires dir path\n";
                    return EXIT_FAILURE;
                } out_path      = std::string(argv[++argi]);
                break;
            case ARG_STRIP_METADATA:
                strip_metadata  = true;
                break;
            case ARG_ENCODING:
                if (argi+1>=argc) {
                    std::cerr<<"encoding argument requires requires valid encoding variable\n";
                    return EXIT_FAILURE;
                } encoding = PARSE_ENCODING(std::string(argv[++argi]));
                if (encoding == IrisCodec::TILE_ENCODING_UNDEFINED) {
                    std::cerr<<"Undefined encoding value given " << argv[argi] << "\n";
                    return EXIT_FAILURE;
                }
                break;
            case ARG_INVALID:
                std::cerr   << "Unknown argument \""
                            << argv[argi]
                            << "\"\n";
                return EXIT_FAILURE;
        }
    }
    if (source_path.size() == 0) {
        std::cerr << "Encoder requies at least an input file path of source slide file";
        return EXIT_FAILURE;
    }
    if (out_path.size() != 0)
        if (std::filesystem::is_directory(out_path) == false)
            std::filesystem::create_directory(out_path);
            
    #if _WIN32
    struct winsize {
        int ws_col; 
    } console_dim;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    console_dim.ws_col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
    winsize console_dim;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_dim);
    #endif
    
    // Create the encoder object for this particular file
    IrisCodec::EncodeSlideInfo slide_encode_info {
        .srcFilePath        = source_path,
        .dstFilePath        = out_path,
        .desiredEncoding    = encoding,
    };
    auto encoder = IrisCodec::create_encoder(slide_encode_info);
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
        auto duration   = time::duration_cast<time::seconds>(time::system_clock::now() - start);
        auto ETA        = static_cast<int>(static_cast<float>(duration.count())/cmplt) - duration.count();
        int progress_bar_width = console_dim.ws_col>>1;
        std::stringstream log;
        log << "[";
        for (int block_i = 0; block_i < progress_bar_width*cmplt; ++block_i)
            log << std::string(reinterpret_cast<const char*>(complt_char.c_str()),complt_char.size());
        log << std::string(progress_bar_width*(1-cmplt),'.')
            << "] "
            << std::setprecision(3) << cmplt*100.f
            << "%  Time Remaining "
            << ETA/60/10 << ETA/60%10 << ":" << ETA%60/10 << ETA%60%10;
        std::cout << "\x1b[2K" << "\r" << log.str() << std::flush;;
        
        // Sleep the thread for a second between checks / updates
        std::this_thread::sleep_for(time::seconds(1));
    }
    
    result = IrisCodec::get_encoder_progress(encoder, progress);
    if (result != Iris::IRIS_SUCCESS) {
        std::cerr   << "\n Error during progress check: "
                    << result.message;
        return EXIT_FAILURE;
    } else {
        std::cout << "\nIris Encoder completed successfully\n";
        return EXIT_SUCCESS;
    }
    
}

