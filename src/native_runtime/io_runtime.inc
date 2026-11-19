// Chunked binary file input (std.io).
//
// Files are opened in binary mode and read in caller-sized chunks returned as NUL-terminated byte
// strings. The number of bytes actually read is reported separately (csec_io_last_count), so data
// containing NUL bytes is handled exactly; index the chunk with `chunk[i]` for 0 <= i < count. Each
// chunk is a fresh allocation: release it with csec_string_release once it is no longer needed.

#include <cstdio>
#include <memory>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace csec_io {

struct InputFile {
    FILE* file = nullptr;
    long long lastCount = 0;
    ~InputFile() {
        if (file) std::fclose(file);
    }
};

csec_threads::HandleTable<InputFile>& inputFiles() {
    static csec_threads::HandleTable<InputFile> table;
    return table;
}

// A read-only memory mapping of a whole file. Indexing the view (`view[i]`, 0 <= i < size) reads the
// file through the OS page cache with no copy, so several threads can scan one mapping in parallel.
struct MappedFile {
    const char* data = nullptr;
    long long size = 0;
    bool copied = false;
#ifdef _WIN32
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
#endif
    void unmap() {
#ifdef _WIN32
        if (data && size > 0) UnmapViewOfFile(data);
#else
        if (data && size > 0) munmap(const_cast<char*>(data), static_cast<size_t>(size));
#endif
        data = nullptr;
    }
    ~MappedFile() {
        if (copied) csecFree(const_cast<char*>(data));
        else unmap();
#ifdef _WIN32
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
#endif
    }
    bool terminate() {
#ifdef _WIN32
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        const long long page = static_cast<long long>(info.dwPageSize);
#else
        const long long page = static_cast<long long>(sysconf(_SC_PAGESIZE));
#endif
        if (size == 0 || page <= 0 || size % page != 0) return true;
        char* copy = static_cast<char*>(csecAlloc(static_cast<size_t>(size) + 1));
        if (!copy) return false;
        std::memcpy(copy, data, static_cast<size_t>(size));
        copy[size] = 0;
        unmap();
        data = copy;
        copied = true;
        return true;
    }
};

csec_threads::HandleTable<MappedFile>& mappedFiles() {
    static csec_threads::HandleTable<MappedFile> table;
    return table;
}

} // namespace csec_io

extern "C" {

// Size of the file in bytes, or -1 if it cannot be read.
long long csec_io_file_size(const char* path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(std::filesystem::u8path(path ? path : ""), ec);
    return ec ? -1 : static_cast<long long>(size);
}

// Open a file for binary reading; 0 on failure.
long long csec_io_open_read(const char* path) {
    if (!path) return 0;
#ifdef _WIN32
    FILE* file = nullptr;
    std::wstring wide = std::filesystem::u8path(path).wstring();
    if (_wfopen_s(&file, wide.c_str(), L"rb") != 0) file = nullptr;
#else
    FILE* file = std::fopen(path, "rb");
#endif
    if (!file) return 0;
    auto input = std::make_shared<csec_io::InputFile>();
    input->file = file;
    return csec_io::inputFiles().add(input);
}

// Move to byte `offset` from the start; 1 on success.
int csec_io_seek(long long handle, long long offset) {
    auto input = csec_io::inputFiles().get(handle);
    if (!input || offset < 0) return 0;
#ifdef _WIN32
    return _fseeki64(input->file, offset, SEEK_SET) == 0 ? 1 : 0;
#else
    return fseeko(input->file, static_cast<off_t>(offset), SEEK_SET) == 0 ? 1 : 0;
#endif
}

// Read up to maxBytes; the chunk is NUL-terminated and its byte count is csec_io_last_count.
char* csec_io_read(long long handle, int maxBytes) {
    auto input = csec_io::inputFiles().get(handle);
    const size_t want = maxBytes > 0 ? static_cast<size_t>(maxBytes) : 0;
    char* buffer = static_cast<char*>(csecAlloc(want + 1));
    if (!buffer) return nullptr;
    size_t read = 0;
    if (input && want > 0) read = std::fread(buffer, 1, want, input->file);
    buffer[read] = '\0';
    if (input) input->lastCount = static_cast<long long>(read);
    return buffer;
}

// Bytes delivered by the most recent csec_io_read on this handle (0 at end of file).
long long csec_io_last_count(long long handle) {
    auto input = csec_io::inputFiles().get(handle);
    return input ? input->lastCount : 0;
}

void csec_io_close(long long handle) {
    csec_io::inputFiles().take(handle);
}

// Decimal text to a number (leading/trailing spaces allowed); `fallback` if it is not a number.
long long csec_io_parse_long(const char* text, long long fallback) {
    if (!text) return fallback;
    char* end = nullptr;
    errno = 0;
    const long long value = std::strtoll(text, &end, 10);
    if (end == text || errno == ERANGE) return fallback;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    return *end == '\0' ? value : fallback;
}

// Map a whole file read-only; 0 on failure. An empty file maps to an empty view.
long long csec_io_map_open(const char* path) {
    if (!path) return 0;
    auto mapped = std::make_shared<csec_io::MappedFile>();
    static const char emptyView[1] = { 0 };
#ifdef _WIN32
    std::wstring wide = std::filesystem::u8path(path).wstring();
    mapped->file = CreateFileW(wide.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (mapped->file == INVALID_HANDLE_VALUE) return 0;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(mapped->file, &size)) return 0;
    mapped->size = size.QuadPart;
    if (mapped->size == 0) {
        mapped->data = emptyView;
    } else {
        mapped->mapping = CreateFileMappingW(mapped->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapped->mapping) return 0;
        mapped->data = static_cast<const char*>(MapViewOfFile(mapped->mapping, FILE_MAP_READ, 0, 0, 0));
        if (!mapped->data) return 0;
    }
#else
    const int fd = ::open(path, O_RDONLY);
    if (fd < 0) return 0;
    struct stat st{};
    if (fstat(fd, &st) != 0) { ::close(fd); return 0; }
    mapped->size = static_cast<long long>(st.st_size);
    if (mapped->size == 0) {
        mapped->data = emptyView;
    } else {
        void* view = mmap(nullptr, static_cast<size_t>(mapped->size), PROT_READ, MAP_PRIVATE, fd, 0);
        if (view == MAP_FAILED) { ::close(fd); return 0; }
        madvise(view, static_cast<size_t>(mapped->size), MADV_SEQUENTIAL);
        mapped->data = static_cast<const char*>(view);
    }
    ::close(fd);
#endif
    if (!mapped->terminate()) return 0;
    return csec_io::mappedFiles().add(mapped);
}

// The mapped bytes (valid until csec_io_map_close), followed by a NUL; index below csec_io_map_size.
char* csec_io_map_data(long long handle) {
    auto mapped = csec_io::mappedFiles().get(handle);
    return mapped ? const_cast<char*>(mapped->data) : nullptr;
}

long long csec_io_map_size(long long handle) {
    auto mapped = csec_io::mappedFiles().get(handle);
    return mapped ? mapped->size : 0;
}

void csec_io_map_close(long long handle) {
    csec_io::mappedFiles().take(handle);
}

// Free a string the runtime allocated (e.g. a chunk from csec_io_read). Never pass a string literal.
void csec_string_release(char* text) {
    if (csecRcTracked(text)) return;
    csecFree(text);
}

} // extern "C"
