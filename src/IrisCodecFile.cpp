//
//  IrisCodecFile.cpp
//  Iris
//
//  Created by Ryan Landvater on 1/10/24.
//
#include <string>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include <system_error>
#include "IrisCodecPriv.hpp"
namespace IrisCodec {
inline void         GENERATE_TEMP_FILE          (const File& file, bool unlink);
inline size_t       GET_FILE_SIZE               (const File& file);
inline void         PERFORM_FILE_MAPPING        (const File& file);
inline size_t       RESIZE_FILE                 (const File& file, size_t bytes);
inline void         RENAME_FILE                 (const File& file, const std::string& path);
inline void         DELETE_FILE                 (const File& file);
inline bool         LOCK_FILE                   (const File& file, bool exclusive, bool wait);
inline void         UNLOCK_FILE                 (const File& file);

// MARK: - WINDOWS FILE IO Implementations
#if _WIN32
#include <io.h>
size_t get_page_size() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
};
const size_t page_size = get_page_size();
inline void GENERATE_TEMP_FILE (File& file, bool ulink) {
    auto& file_path = file->path;
    
    TCHAR TEMP_DIR_PATH [MAX_PATH+1];
    TCHAR TEMP_FILE_NAME[MAX_PATH+1];
    
    auto  dir_len = GetTempPath2 (MAX_PATH+1, TEMP_DIR_PATH);
    if (!dir_len) throw std::system_error(errno, std::generic_category(),
        "GENERATE_TEMP_FILE failed to get temp file dir");
    
    auto name_len = GetTempFileName (TEMP_DIR_PATH,
                                     TEXT("IrisCodecCache"),
                                     0,TEMP_FILE_NAME);
    if (!name_len) throw std::system_error(errno, std::generic_category(),
        "GENERATE_TEMP_FILE failed to get unique cache file name");

    file_path = std::string(TEMP_FILE_NAME, TEMP_FILE_NAME + name_len);
    

    // Replace the unsafe fopen with the safer fopen_s
    if (fopen_s(&file->handle, file->path.c_str(), "rbR+") != 0) {
        throw std::runtime_error("Failed to open file with fopen_s");
    }

    // Unlink the file from the file system so that when the program
    // closes, the file will be immediately released.
    if (ulink) {
        if (_unlink(file_path.c_str()) == -1)
            assert(false && "failed to unlink the file from the filesystem");
        file->linked = false;
    }

}
inline size_t GET_FILE_SIZE (File& file) {
    auto& handle = file->handle;
    int result = -1;

    // If no handle provided, return
    if (handle == NULL)
        throw std::system_error(errno, std::generic_category(),
            "Cannot determine file size. Invalid file handle");

    // Seek the end of the file.
    result = fseek(handle, 0L, SEEK_END);
    if (result == -1)
        throw std::system_error(errno, std::generic_category(),
            "Cannot determine file size. Failed to seek file end");

    // Get the file size
    size_t size = ftell(handle);

    result = fseek(handle, 0L, SEEK_SET);
    if (result == -1)
        throw std::system_error(errno, std::generic_category(),
            "Failed to return to file start during file size determination");

    file->size = size;
    return size;
}
inline void PERFORM_FILE_MAPPING (File& file) {
    // Of note, we will NEVER EVER map with execetion privilages for safety. EVER.

    // Get the WIN32 file handle
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file->handle));
    if (handle == NULL)
        throw std::system_error(errno, std::generic_category(),
            "failed to get a WIN32 file handle to map.");

    file->map = CreateFileMapping(
        handle,         // Windows file handle
        NULL,           // Default security access descriptor
        file->writeAccess ? PAGE_READWRITE : PAGE_READONLY,
        0,              // High 32 bits of 64 bit file size (zero for full file)
        0,              // Low 32 bits of 64 bit file size (zero for full file)
        NULL);
    if (file->map == NULL)
        throw std::system_error(errno, std::generic_category(),
            "failed to create file mapping object.");

    // Generate a view of the file
    file->ptr = (BYTE*) MapViewOfFile(
        file->map,      // File Mapping object handle
        file->writeAccess ? FILE_MAP_WRITE : FILE_MAP_READ,
        0, 0,           // We will always map from the beginning of the file
        0);             // And map the entire file (zero for full extent)

    // If no pointer was returned, the
    if (file->ptr == NULL)
        throw std::system_error(errno, std::generic_category(),
            "failed create mapped file view.");
}
inline static void UNMAP_FILE(HANDLE& map, BYTE*& ptr) {
    if (ptr == nullptr) return;
    if (FlushViewOfFile(ptr, 0) == false)
        throw std::system_error(errno, std::generic_category(),
            "Failed to fush mapped file data contents");
    if (UnmapViewOfFile(ptr) == false) {
        throw std::system_error(errno, std::generic_category(),
            "Failed to unmap file");
        ptr = nullptr;
    }
    if (map == INVALID_HANDLE_VALUE) return;
    if (CloseHandle(map) == false) {
        throw std::system_error(errno, std::generic_category(),
            "Failed to close file mapping object");
        map = INVALID_HANDLE_VALUE;
    }
}
inline size_t RESIZE_FILE (const File& file, size_t bytes) {
    auto& map = file->map;
    auto& ptr = file->ptr;
    auto& size = file->size;

    // Return if no change needed
    if (size == bytes) return size;

    // Get the WIN32 file handle
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file->handle));
    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(errno, std::generic_category(),
            "failed to get a WIN32 file handle to map.");

    // Reinterpret the size_t variable as a Large_Integer (ugh Windows)
    LARGE_INTEGER& LI_bytes = reinterpret_cast<LARGE_INTEGER&>(bytes);

    UNMAP_FILE(map, ptr);

    // Set the new file size by moving the pointer to the new size
    // and then resizing the file to the location of the pointer
    if (SetFilePointerEx(
        handle,     // Move the file pointer for the file handle
        LI_bytes,   // A long-integer version of the new size in bytes
        NULL,       // From the beginning of the file (FILE_BEGIN)
        FILE_BEGIN ) == 0)
        throw std::system_error(errno, std::generic_category(),
            "Failed to move file pointer to new location in file");
    if (SetEndOfFile(handle) == false)
        throw std::system_error(errno, std::generic_category(),
            "Failed to ftruncate-resize file");

    file->map = CreateFileMapping(
        handle,         // Windows file handle
        NULL,           // Default security access descriptor
        file->writeAccess,   // Use above access flags
        0,              // High 32 bits of 64 bit file size (zero for full file)
        0,              // Low 32 bits of 64 bit file size (zero for full file)
        NULL);
    if (file->map == INVALID_HANDLE_VALUE)
        throw std::system_error(errno, std::generic_category(),
            "failed to create file mapping object.");

    // Generate a view of the file
    file->ptr = (BYTE*)MapViewOfFile(
        file->map,      // File Mapping object handle
        file->writeAccess ? FILE_MAP_WRITE : FILE_MAP_READ,
        0, 0,           // We will always map from the beginning of the file
        0);             // And map the entire file (zero for full extent)

    // If no pointer was returned, the
    if (file->ptr == NULL)
        throw std::system_error(errno, std::generic_category(),
            "failed create mapped file view.");

    size = bytes;
    return bytes;
}
inline void RENAME_FILE (const File& file, const std::string& path)
{
    if (rename(file->path.c_str(), path.c_str()) == -1)
        throw std::system_error(errno, std::generic_category(),
                                "failed to rename the file");
}
inline void DELETE_FILE (const File& file)
{
    _unlink(file->path.c_str());
    file->linked = false;
}
inline bool LOCK_FILE(File& file, bool exclusive, bool wait)
{
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file->handle));

    int flags = 0;
    flags |= exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    flags |= wait ? 0 : LOCKFILE_FAIL_IMMEDIATELY;

    // We will need to fix this in the future whenever we want to call async file locks...
    assert(wait == false && "Async callback has not be established for windows. OVERLAPPED hEvent cannot be 0 for assync");

    OVERLAPPED overlapped {
        .hEvent = 0
    };

    return LockFileEx(
        handle,
        flags,
        0,
        file->size & UINT32_MAX,
        file->size >> 32,
        &overlapped);

}
inline void UNLOCK_FILE(File& file)
{
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file->handle));

    OVERLAPPED overlapped{
        .hEvent = 0
    };

    if (UnlockFileEx(
        handle,
        0,
        file->size & UINT32_MAX,
        file->size >> 32,
        &overlapped) == false)
        throw std::system_error( errno, std::generic_category(),
            "Failed to unlock a locked file.");
}
#else
// MARK: - POSIX COMPLIENT IMPLEMENTATIONS
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>
const size_t page_size = getpagesize();
inline void GENERATE_TEMP_FILE (const File& file, bool ulink)
{
    // Create the file path template (the 'X' values will be modified).Co
    auto& file_path     = file->path;
    file_path           = std::filesystem::temp_directory_path().string() +
                          std::string("IrisCodecCache_XXXXXX");
    
    // Ask the file system to create a unique temporary file
    int file_descriptor = mkstemp(const_cast<char*>(file_path.c_str()));
    if (file_descriptor == -1)
        throw std::system_error(errno,std::generic_category(),
                                "Failed to create an cache file.");
    
    // Open stream access to the file
    file->handle       = fdopen(file_descriptor, "wb+");
    
    // Unlink the file from the file system so that when the program
    // closes, the file will be immediately released.
    if (ulink) {
        if (unlink(file_path.c_str()) == -1)
            assert(false && "failed to unlink the file from the filesystem");
        file->linked = false;
    }
}
inline size_t GET_FILE_SIZE (const File& file)
{
    auto& handle = file->handle;
    int result   = -1;
    
    // If no handle provided, return
    if (handle == NULL)
        throw std::system_error(errno,std::generic_category(),
                                "Cannot determine file size. Invalid file handle");
    
    // Seek the end of the file.
    result = fseek(handle, 0L, SEEK_END);
    if (result == -1)
        throw std::system_error(errno,std::generic_category(),
                                "Cannot determine file size. Failed to seek file end");
    
    // Get the file size
    size_t size = ftell(handle);
    
    result = fseek(handle, 0L, SEEK_SET);
    if (result == -1)
        throw std::system_error(errno,std::generic_category(),
                                "Failed to return to file start during file size determination");
    
    file->size = size;
    return size;
}
inline size_t RESIZE_FILE (const File& file, size_t bytes)
{
    auto& handle    = file->handle;
    auto& ptr       = file->ptr;
    auto& size      = file->size;
    int result      = -1;
    
    // Return if no change needed
    if (size == bytes) return size;
    
    // Set the new file size
    result = ftruncate(fileno(handle), bytes);
    if (result == -1)
        throw std::system_error(errno,std::generic_category(),
                                "Failed to ftruncate-resize file");
    
    // If we are expanding the file, write a byte to trigger
    // the OS to recognize the new file size.
    if (bytes > size) {
        fseek(handle, 0L, SEEK_END);
        fwrite("", sizeof(char), 1, handle);
        fseek(handle, 0L, SEEK_SET);
    }
    
    // If the file is already mapped, we must update the mapping.
    if (ptr) {
        // Of note, we will NEVER EVER PROT_EXEC for safety. EVER.
        int access_flags    = file->writeAccess ? PROT_READ | PROT_WRITE : PROT_READ;
        
        // Get the file descriptor
        int posix_file      = fileno(file->handle);
        if (posix_file == -1)
            throw std::system_error(errno,std::generic_category(),
                                    "failed to get a posix file descriptor to map.");
        
        // Remap the new file size
        munmap                      (file->ptr, file->size);
        auto new_ptr        = mmap  (ptr, bytes,     // No initial ptr, but map all bytes
                                     access_flags,   // Access map based on write vs read/write
                                     MAP_SHARED,     // Allow other processes to see updates
                                     posix_file,0);  // Get the file descriptor, no offset
        if (new_ptr == NULL)
            throw std::system_error(errno,std::generic_category(),
                                    "Failed to remap resized file");
        
        // Assign the new ptr if it is new
        if (new_ptr != ptr) file->ptr = static_cast<BYTE*>(new_ptr);
    }
    
    size = bytes;
    return bytes;
}
inline void PERFORM_FILE_MAPPING (const File& file)
{
    // Of note, we will NEVER EVER PROT_EXEC for safety. EVER.
    int access_flags = file->writeAccess ? PROT_READ | PROT_WRITE : PROT_READ;
    
    // Get the file descriptor
    int posix_file   = fileno(file->handle);
    if (posix_file == -1)
        throw std::system_error(errno,std::generic_category(),
                                "failed to get a posix file descriptor to map.");
    
    // Map the file into memory using MMAP
    file->ptr = static_cast<BYTE*>(mmap(NULL, file->size,  // No initial ptr, but map all bytes
                                        access_flags,      // Access map based on write vs read/write
                                        MAP_SHARED,        // Allow other processes to see updates
                                        posix_file,0));    // Get the file descriptor, no offset
    
    // If no pointer was returned, the
    if (file->ptr == MAP_FAILED)
        throw std::system_error(errno,std::generic_category(),
                                "failed to map the file.");

}
inline bool LOCK_FILE (const File& file, bool exclusive, bool wait)
{
    int posix_file  = fileno(file->handle);
    int flags       = 0;
    flags          |= exclusive ? LOCK_EX : LOCK_SH;
    flags          |= wait ? 0 : LOCK_NB; // If no wait, no thread block (NB)
    
    // If Flock fails, return false. Maybe add info later?
    int lock_sucscess = flock(posix_file, flags);
    if (lock_sucscess == -1)
        return false;
    
    // Lock achieved.
    return true;
}
inline void RENAME_FILE (const File& file, const std::string& path)
{
    if (rename(file->path.c_str(), path.c_str()) == -1)
        throw std::system_error(errno, std::generic_category(),
                                "failed to rename the file");
}
inline void DELETE_FILE (const File& file)
{
    unlink(file->path.c_str());
    file->linked = false;
}
inline void UNLOCK_FILE (const File& file)
{
    int posix_file      = fileno(file->handle);
    int unlock_success  = flock(posix_file, LOCK_UN);
    if (unlock_success == -1)
        throw std::system_error(errno,std::generic_category(),
                                "failed to unlock the file");
}
inline void UNMAP_FILE (BYTE*& ptr, size_t bytes)
{
    // If there is a ptr, unmap it
    if (ptr) munmap(ptr, bytes);
    
    // Then nullify the stale ptr
    ptr = NULL;
}
#endif // END POSIX COMPLIENT
// MARK: - Iris Codec File API Calls
File create_file (const struct FileCreateInfo &create_info)
{
    try {
        File file = std::make_shared<__INTERNAL__File>(create_info);
        
        if (create_info.initial_size == 0)
            throw std::runtime_error("There must be an initial file size to map");
        
        // Create a file for reading and writing in binary format.
        #if _WIN32
        fopen_s(&file->handle, file->path.data(), "wb+");
        #else
        file->handle = fopen(file->path.data(), "wb+");
        #endif
        if (!file->handle)
            throw std::runtime_error("Failed to create the file");
        
        // Size the initial file in memory
        RESIZE_FILE (file, create_info.initial_size);
        
        // Map the file into memory
        PERFORM_FILE_MAPPING (file);
        
        // Return the newly mapped file
        return file;
        
    } catch (std::system_error &e) {
        std::cerr   << "Failed to create file"
                    << create_info.filePath << ": "
                    << e.what() << " - "
                    << e.code().message() << "\n";
        return NULL;
    } catch (std::runtime_error &e) {
        std::cerr   << "Failed to create file"
                    << create_info.filePath << ": "
                    << e.what() << "\n";
        return NULL;
    }   return NULL;
}

/// Open a file for read or read-write access.
File    open_file (const struct FileOpenInfo &open_info)
{
    try {
        File file = std::make_shared<__INTERNAL__File>(open_info);

        // Open the file for reading or reading and writing depending on write access
        #if _WIN32
        if (fopen_s(&file->handle, file->path.c_str(), open_info.writeAccess ? "rbR+" : "rbR") != 0) {
            throw std::runtime_error("Failed to open file");
        }
        #else
        file->handle = fopen(file->path.data(), open_info.writeAccess ? "rb+" : "rb");
        if (!file->handle) throw std::runtime_error
            ("Failed to open the file");
        #endif

        // Get the file size.
        GET_FILE_SIZE(file);
        
        // Map the file into memory
        PERFORM_FILE_MAPPING(file);
        
        // Return the newly mapped file
        return file;
        
    } catch (std::system_error &e) {
        std::cerr   << "Failed to open file"
                    << open_info.filePath << ": "
                    << e.what() << " - "
                    << e.code().message() << "\n";
        
        // Return a nullptr
        return NULL;
    } catch (std::runtime_error &e) {
        std::cerr   << "Failed to open file"
                    << open_info.filePath << ": "
                    << e.what() << "\n";
        return NULL;
    }   return NULL;
}

/// Create a new file-system temporary file for temporary archiving of slide data to disk.
File create_cache_file (const struct CacheCreateInfo &create_info)
{
    try {
        File file = std::make_shared<__INTERNAL__File>(create_info);
        
        // Generate the unlinked and unique temporary cache file.
        GENERATE_TEMP_FILE(file, create_info.unlink);
        
        // Set the initial cache file to about 500 MB at the page break.
        RESIZE_FILE(file, ((size_t)5E8&~(page_size-1))+page_size);

        // Map the file into memory
        PERFORM_FILE_MAPPING(file);
        
        // Return the new cache file
        return file;
        
    } catch (std::system_error &e) {
        std::cerr   << "Failed to create an cache file: "
                    << e.what() << " - "
                    << e.code().message() << "\n";
        
        // Return a nullptr
        return NULL;
    } catch (std::runtime_error &e) {
        std::cerr   << "Failed to create file: "
                    << e.what() << "\n";
        return NULL;
    }   return NULL;
}

Result resize_file (const File &file, const struct FileResizeInfo &info)
{
    // Check if page alignment was requested
    auto size = info.pageAlign ?
    // if page align, drop the size to the closest page break
    // and then add one page tobe at least the request size
    (info.size & ~(page_size-1)) + page_size :
    // Else, just do the info.size
    info.size;
    
    try {
        RESIZE_FILE (file, size);
        return IRIS_SUCCESS;
    } catch (std::system_error&e) {
        return {
            IRIS_FAILURE,
            e.what()
        };
    } return IRIS_FAILURE;
}
Result rename_file(const File &file, const std::string &new_path)
{
    if (file->get_path() == new_path) return IRIS_SUCCESS;
    if (file->linked == false)
        return Iris::Result (
            IRIS_FAILURE,"File is an unlinked temporary file. These do not contain a valid OS linkage and thus cannot be renamed."
        );
    try {
        RENAME_FILE(file, new_path);
        file->rename_file(new_path);
        return IRIS_SUCCESS;
    } catch (std::system_error &e) {
        return Iris::Result
        (IRIS_FAILURE, std::string("Failed to rename iris codec file: ")+e.what());
    }   return IRIS_FAILURE;
}
Result delete_file(const File &file)
{
    if (std::filesystem::exists(file->path) == false)
        return Iris::Result
        (IRIS_FAILURE, "failed to delete file due to invalid file path");
    if (file->linked == false) return IRIS_SUCCESS;
    try {
        DELETE_FILE(file);
        return IRIS_SUCCESS;
    } catch (std::system_error &e) {
        return Iris::Result
        (IRIS_FAILURE, std::string("Failed to delete file: ")+e.what());
    }   return IRIS_FAILURE;
    
    
}
__INTERNAL__File::__INTERNAL__File  (const FileOpenInfo& info) :
    path                            (info.filePath),
    writeAccess                     (info.writeAccess),
    linked                          (true)
{
    
}
__INTERNAL__File::__INTERNAL__File  (const FileCreateInfo& info) :
    path                            (info.filePath),
    writeAccess                     (true),
    linked                          (true)
{

}
__INTERNAL__File::__INTERNAL__File  (const CacheCreateInfo& info) :
    writeAccess                     (true),
    linked                          (true)
{

}
__INTERNAL__File::~__INTERNAL__File ()
{
    // If the file is mapped, unmap
    #if _WIN32
    UNMAP_FILE  (map, ptr);
    #else
    UNMAP_FILE  (ptr, size);
    #endif
    
    // Close the file
    if (handle) fclose (handle);
}
std::string __INTERNAL__File::get_path() const
{
    return path;
}
BYTE* __INTERNAL__File::get_ptr() const
{
    return ptr;
}
void __INTERNAL__File::rename_file (const std::string& new_path)
{
    path = new_path;
}
} // END IRIS CODEC NAMESPACE
