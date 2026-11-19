// Inter-task communication: pipes, shared memory, and mail (std.pipe / std.shared / std.mail).
//
// These work between OS threads, between microthreads, and (for child-process pipes, named shared
// memory, and mail) between OS processes. Objects are 64-bit handles. Every blocking operation waits
// cooperatively (csecCooperativeWait): while another microthread of the calling OS thread can run it
// yields to it, otherwise it sleeps for a short interval and polls again, so a microthread blocked on a
// pipe never stalls the other microthreads of its thread.

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#ifndef _WIN32
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace csec_ipc {

using csec_threads::HandleTable;

char* ownedCopy(const std::string& text) {
    return csec_microthread::ownedText(text);
}

// Wait until `ready()` holds; `timeoutMs < 0` waits forever. Returns whether it became ready.
template <typename Ready>
bool csecCooperativeWait(Ready ready, long long timeoutMs) {
    const bool bounded = timeoutMs >= 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(bounded ? timeoutMs : 0);
    for (;;) {
        if (ready()) return true;
        if (bounded && std::chrono::steady_clock::now() >= deadline) return false;
        if (csec_microthread::hasRunnableOthers()) {
            csec_process_yield();
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

// ---- in-process pipes ----------------------------------------------------------------------------

struct Pipe {
    std::mutex mutex;
    std::string data;
    bool closed = false;
};

HandleTable<Pipe>& pipes() { return csec_threads::leakedTable<Pipe>(); }

// Take one line (without its terminator) from `buffer` when a full line or end of input is there.
bool takeLine(std::string& buffer, bool atEnd, std::string& line) {
    const size_t newline = buffer.find('\n');
    if (newline == std::string::npos) {
        if (!atEnd || buffer.empty()) return false;
        line = buffer;
        buffer.clear();
        return true;
    }
    line = buffer.substr(0, newline);
    buffer.erase(0, newline + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}

// ---- child processes with piped stdin/stdout -------------------------------------------------------

struct PipedProcess {
    std::mutex inputMutex;
    std::mutex outputMutex;
    std::string buffer;
    bool eof = false;
    bool inputOpen = true;
#ifdef _WIN32
    PROCESS_INFORMATION info{};
    HANDLE toChild = nullptr;
    HANDLE fromChild = nullptr;
#else
    pid_t pid = -1;
    int toChild = -1;
    int fromChild = -1;
#endif
};

HandleTable<PipedProcess>& pipedProcesses() { return csec_threads::leakedTable<PipedProcess>(); }

// Move whatever the child has written so far into the buffer without blocking; detects end of output.
void pumpChildOutput(PipedProcess& p) {
    if (p.eof) return;
#ifdef _WIN32
    if (!p.fromChild) {
#else
    if (p.fromChild < 0) {
#endif
        p.eof = true;
        return;
    }
    char chunk[4096];
#ifdef _WIN32
    for (;;) {
        DWORD available = 0;
        if (!PeekNamedPipe(p.fromChild, nullptr, 0, nullptr, &available, nullptr)) {
            p.eof = true;   // broken pipe: the child closed its stdout
            return;
        }
        if (available == 0) return;
        DWORD read = 0;
        const DWORD want = available < sizeof(chunk) ? available : static_cast<DWORD>(sizeof(chunk));
        if (!ReadFile(p.fromChild, chunk, want, &read, nullptr) || read == 0) {
            p.eof = true;
            return;
        }
        p.buffer.append(chunk, read);
    }
#else
    for (;;) {
        pollfd fd{ p.fromChild, POLLIN, 0 };
        if (poll(&fd, 1, 0) <= 0) return;
        const ssize_t read = ::read(p.fromChild, chunk, sizeof(chunk));
        if (read <= 0) {
            p.eof = true;
            return;
        }
        p.buffer.append(chunk, static_cast<size_t>(read));
    }
#endif
}

void closeChildInputLocked(PipedProcess& p) {
    if (!p.inputOpen) return;
    p.inputOpen = false;
#ifdef _WIN32
    CloseHandle(p.toChild);
    p.toChild = nullptr;
#else
    ::close(p.toChild);
    p.toChild = -1;
#endif
}

void closeChildInput(PipedProcess& p) {
    std::lock_guard<std::mutex> lock(p.inputMutex);
    closeChildInputLocked(p);
}

// ---- shared memory -------------------------------------------------------------------------------

constexpr unsigned long long kSharedMagic = 0x6373656353484D31ULL; // "csecSHM1"
constexpr size_t kSharedHeader = 64;

struct SharedHeader {
    std::atomic<unsigned long long> magic;
    std::atomic<unsigned long long> size;   // usable bytes after the header
    std::atomic<long long> lock;            // spin lock for sharedLock/sharedUnlock
    long long reserved[5];
};
static_assert(sizeof(SharedHeader) == kSharedHeader, "shared header layout");

struct SharedRegion {
    unsigned char* base = nullptr;
    size_t mapped = 0;
    size_t usable = 0;
    bool named = false;
#ifdef _WIN32
    HANDLE mapping = nullptr;
#endif

    SharedHeader* header() const { return reinterpret_cast<SharedHeader*>(base); }
    unsigned char* data() const { return base + kSharedHeader; }
    size_t size() const { return base ? usable : 0; }

    ~SharedRegion() {
        if (!base) return;
        if (!named) {
            std::free(base);
            return;
        }
#ifdef _WIN32
        UnmapViewOfFile(base);
        if (mapping) CloseHandle(mapping);
#else
        munmap(base, mapped);
#endif
    }
};

HandleTable<SharedRegion>& sharedRegions() { return csec_threads::leakedTable<SharedRegion>(); }

std::string sanitizedName(const char* name) {
    std::string result = "csec_";
    for (const char* p = name ? name : ""; *p; ++p) {
        const char c = *p;
        const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        result.push_back(safe ? c : '_');
    }
    return result;
}

#ifndef _WIN32
// POSIX shared-memory object name for a region name. macOS limits these names to 31 bytes
// (PSHMNAMLEN), so a longer name is replaced by a fixed-length hash of it.
std::string posixSharedName(const std::string& name) {
    std::string object = "/" + name;
    if (object.size() <= 31) return object;
    unsigned long long hash = 1469598103934665603ULL;   // FNV-1a
    for (unsigned char c : name) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    char digest[24];
    std::snprintf(digest, sizeof(digest), "/csec_%016llx", hash);
    return digest;
}
#endif

size_t roundUp8(long long bytes) {
    if (bytes < 8) bytes = 8;
    return static_cast<size_t>((bytes + 7) / 8 * 8);
}

std::shared_ptr<SharedRegion> openNamedRegion(const std::string& name, size_t dataBytes) {
    auto region = std::make_shared<SharedRegion>();
    region->named = true;
    const size_t total = kSharedHeader + dataBytes;
    bool created = false;
#ifdef _WIN32
    const std::string objectName = "Local\\" + name;
    region->mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
        static_cast<DWORD>(static_cast<unsigned long long>(total) >> 32), static_cast<DWORD>(total & 0xFFFFFFFFu), objectName.c_str());
    if (!region->mapping) return nullptr;
    created = GetLastError() != ERROR_ALREADY_EXISTS;
    region->base = static_cast<unsigned char*>(MapViewOfFile(region->mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (!region->base) return nullptr;
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(region->base, &info, sizeof(info)) == 0) return nullptr;
    region->mapped = info.RegionSize;
#else
    const std::string objectName = posixSharedName(name);
    int fd = shm_open(objectName.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd >= 0) {
        created = true;
        if (ftruncate(fd, static_cast<off_t>(total)) != 0) { ::close(fd); return nullptr; }
        region->mapped = total;
    } else {
        fd = shm_open(objectName.c_str(), O_RDWR, 0600);
        if (fd < 0) return nullptr;
        struct stat st{};
        if (fstat(fd, &st) != 0 || st.st_size < static_cast<off_t>(kSharedHeader)) { ::close(fd); return nullptr; }
        region->mapped = static_cast<size_t>(st.st_size);
    }
    void* mapped = mmap(nullptr, region->mapped, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close(fd);
    if (mapped == MAP_FAILED) return nullptr;
    region->base = static_cast<unsigned char*>(mapped);
#endif
    if (region->mapped < kSharedHeader) return nullptr;
    SharedHeader* header = region->header();
    if (created) {
        header->size.store(dataBytes);
        header->magic.store(kSharedMagic);
        region->usable = (std::min)(dataBytes, region->mapped - kSharedHeader);
    } else {
        // The creator may still be initialising the header.
        csecCooperativeWait([&] { return header->magic.load() == kSharedMagic; }, 2000);
        if (header->magic.load() != kSharedMagic) return nullptr;
        region->usable = static_cast<size_t>((std::min<unsigned long long>)(header->size.load(), region->mapped - kSharedHeader));
    }
    return region;
}

std::shared_ptr<SharedRegion> regionFor(long long handle) { return sharedRegions().get(handle); }

template <typename T>
T* slotAt(SharedRegion& region, long long offset) {
    if (offset < 0 || static_cast<unsigned long long>(offset) > region.size() || sizeof(T) > region.size() - static_cast<size_t>(offset)) return nullptr;
    return reinterpret_cast<T*>(region.data() + offset);
}

void lockRegion(SharedRegion& region) {
    auto& word = region.header()->lock;
    csecCooperativeWait([&] {
        long long expected = 0;
        return word.compare_exchange_strong(expected, 1);
    }, -1);
}

void unlockRegion(SharedRegion& region) {
    region.header()->lock.store(0);
}

// ---- mail: named mailboxes in shared memory --------------------------------------------------------
//
// A mailbox is a named shared region holding a byte ring of variable-length messages, so senders and
// receivers may live in any thread, microthread, or OS process. Record layout:
//   u32 bodyLength, u32 senderLength, i64 tag, sender bytes, body bytes.

constexpr size_t kMailCapacity = 1u << 20; // ring bytes per mailbox

struct MailState {
    unsigned long long head;   // total bytes consumed
    unsigned long long tail;   // total bytes produced
    long long count;           // messages queued
    long long reserved[5];
};
static_assert(sizeof(MailState) == 64, "mail state layout");

struct MailEndpoint {
    std::shared_ptr<SharedRegion> region;
    std::string sender;
    std::mutex lastMutex;
    std::string lastBody;
    std::string lastSender;
    long long lastTag = 0;

    MailState* state() const { return reinterpret_cast<MailState*>(region->data()); }
    unsigned char* ring() const { return region->data() + sizeof(MailState); }
    size_t capacity() const { return region->size() - sizeof(MailState); }
};

HandleTable<MailEndpoint>& mailEndpoints() { return csec_threads::leakedTable<MailEndpoint>(); }

void ringWrite(MailEndpoint& box, unsigned long long position, const void* source, size_t length) {
    const size_t cap = box.capacity();
    const auto* bytes = static_cast<const unsigned char*>(source);
    for (size_t i = 0; i < length; ++i) box.ring()[(position + i) % cap] = bytes[i];
}

void ringRead(MailEndpoint& box, unsigned long long position, void* target, size_t length) {
    const size_t cap = box.capacity();
    auto* bytes = static_cast<unsigned char*>(target);
    for (size_t i = 0; i < length; ++i) bytes[i] = box.ring()[(position + i) % cap];
}

// Pop one message into the endpoint's "last" fields; false when the mailbox is empty.
bool popMail(MailEndpoint& box) {
    lockRegion(*box.region);
    MailState* state = box.state();
    const unsigned long long head = state->head;
    const unsigned long long used = state->tail - head;
    std::uint32_t lengths[2] = { 0, 0 };
    long long tag = 0;
    if (state->count <= 0 || used > box.capacity() || used < sizeof(lengths) + sizeof(tag)) {
        unlockRegion(*box.region);
        return false;
    }
    ringRead(box, head, lengths, sizeof(lengths));
    ringRead(box, head + sizeof(lengths), &tag, sizeof(tag));
    const unsigned long long cursor = head + sizeof(lengths) + sizeof(tag);
    const unsigned long long payload = used - sizeof(lengths) - sizeof(tag);
    if (lengths[1] > payload || lengths[0] > payload - lengths[1]) {
        unlockRegion(*box.region);
        return false;
    }
    std::string sender;
    std::string body;
    try {
        sender.resize(lengths[1]);
        body.resize(lengths[0]);
    } catch (...) {
        unlockRegion(*box.region);
        return false;
    }
    ringRead(box, cursor, sender.data(), sender.size());
    ringRead(box, cursor + sender.size(), body.data(), body.size());
    state->head = cursor + sender.size() + body.size();
    state->count -= 1;
    unlockRegion(*box.region);
    std::lock_guard<std::mutex> lock(box.lastMutex);
    box.lastTag = tag;
    box.lastSender = std::move(sender);
    box.lastBody = std::move(body);
    return true;
}

} // namespace csec_ipc

extern "C" {

// ---- in-process pipes ----------------------------------------------------------------------------

long long csec_pipe_new(void) {
    return csec_ipc::pipes().add(std::make_shared<csec_ipc::Pipe>());
}

// Append text; returns the number of bytes written (0 once the pipe is closed).
int csec_pipe_write(long long handle, const char* text) {
    auto pipe = csec_ipc::pipes().get(handle);
    if (!pipe || !text) return 0;
    std::lock_guard<std::mutex> lock(pipe->mutex);
    if (pipe->closed) return 0;
    pipe->data.append(text);
    return static_cast<int>(std::strlen(text));
}

// Read up to maxBytes (all buffered bytes if maxBytes <= 0), waiting until some data or end of input.
char* csec_pipe_read(long long handle, int maxBytes) {
    using namespace csec_ipc;
    auto pipe = pipes().get(handle);
    if (!pipe) return ownedCopy("");
    csecCooperativeWait([&] {
        std::lock_guard<std::mutex> lock(pipe->mutex);
        return !pipe->data.empty() || pipe->closed;
    }, -1);
    std::lock_guard<std::mutex> lock(pipe->mutex);
    const size_t count = maxBytes <= 0 || static_cast<size_t>(maxBytes) > pipe->data.size()
        ? pipe->data.size() : static_cast<size_t>(maxBytes);
    std::string chunk = pipe->data.substr(0, count);
    pipe->data.erase(0, count);
    return ownedCopy(chunk);
}

// Read one line without its terminator, waiting for it; "" at end of input (check csec_pipe_eof).
char* csec_pipe_read_line(long long handle) {
    using namespace csec_ipc;
    auto pipe = pipes().get(handle);
    if (!pipe) return ownedCopy("");
    std::string line;
    csecCooperativeWait([&] {
        std::lock_guard<std::mutex> lock(pipe->mutex);
        return takeLine(pipe->data, pipe->closed, line) || (pipe->closed && pipe->data.empty());
    }, -1);
    return ownedCopy(line);
}

// Close the write side: readers drain what is buffered, then see end of input.
void csec_pipe_close(long long handle) {
    auto pipe = csec_ipc::pipes().get(handle);
    if (!pipe) return;
    std::lock_guard<std::mutex> lock(pipe->mutex);
    pipe->closed = true;
}

int csec_pipe_eof(long long handle) {
    auto pipe = csec_ipc::pipes().get(handle);
    if (!pipe) return 1;
    std::lock_guard<std::mutex> lock(pipe->mutex);
    return pipe->closed && pipe->data.empty() ? 1 : 0;
}

int csec_pipe_available(long long handle) {
    auto pipe = csec_ipc::pipes().get(handle);
    if (!pipe) return 0;
    std::lock_guard<std::mutex> lock(pipe->mutex);
    return static_cast<int>(pipe->data.size());
}

void csec_pipe_free(long long handle) {
    auto pipe = csec_ipc::pipes().take(handle);
    if (!pipe) return;
    std::lock_guard<std::mutex> lock(pipe->mutex);
    pipe->closed = true;
}

// ---- child processes with piped stdin/stdout -------------------------------------------------------

// Start `command` through the shell with its stdin and stdout connected to the caller (stderr is
// inherited). Returns a handle, or 0 on failure.
long long csec_piped_process_open(const char* command) {
    using namespace csec_ipc;
    if (!command) return 0;
    auto process = std::make_shared<PipedProcess>();
#ifdef _WIN32
    SECURITY_ATTRIBUTES inherit{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE childOutRead = nullptr, childOutWrite = nullptr, childInRead = nullptr, childInWrite = nullptr;
    if (!CreatePipe(&childOutRead, &childOutWrite, &inherit, 0)) return 0;
    if (!CreatePipe(&childInRead, &childInWrite, &inherit, 0)) {
        CloseHandle(childOutRead);
        CloseHandle(childOutWrite);
        return 0;
    }
    SetHandleInformation(childOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(childInWrite, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childInRead;
    startup.hStdOutput = childOutWrite;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    std::string line = std::string("cmd.exe /c ") + command;
    std::vector<char> mutableLine(line.begin(), line.end());
    mutableLine.push_back('\0');
    const BOOL started = CreateProcessA(nullptr, mutableLine.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process->info);
    CloseHandle(childOutWrite);
    CloseHandle(childInRead);
    if (!started) {
        CloseHandle(childOutRead);
        CloseHandle(childInWrite);
        return 0;
    }
    process->toChild = childInWrite;
    process->fromChild = childOutRead;
#else
    int toChild[2], fromChild[2];
    if (pipe(toChild) != 0) return 0;
    if (pipe(fromChild) != 0) { ::close(toChild[0]); ::close(toChild[1]); return 0; }
    for (int fd : { toChild[0], toChild[1], fromChild[0], fromChild[1] }) fcntl(fd, F_SETFD, FD_CLOEXEC);
    std::fflush(stdout);
    const pid_t pid = fork();
    if (pid < 0) {
        ::close(toChild[0]); ::close(toChild[1]); ::close(fromChild[0]); ::close(fromChild[1]);
        return 0;
    }
    if (pid == 0) {
        dup2(toChild[0], 0);
        dup2(fromChild[1], 1);
        ::close(toChild[0]); ::close(toChild[1]); ::close(fromChild[0]); ::close(fromChild[1]);
        execl("/bin/sh", "sh", "-c", command, static_cast<char*>(nullptr));
        _exit(127);
    }
    ::close(toChild[0]);
    ::close(fromChild[1]);
    signal(SIGPIPE, SIG_IGN);
    process->pid = pid;
    process->toChild = toChild[1];
    process->fromChild = fromChild[0];
#endif
    return pipedProcesses().add(process);
}

int csec_piped_process_write(long long handle, const char* text) {
    auto process = csec_ipc::pipedProcesses().get(handle);
    if (!process || !text) return 0;
    std::lock_guard<std::mutex> lock(process->inputMutex);
    if (!process->inputOpen) return 0;
    const size_t length = std::strlen(text);
    size_t written = 0;
    while (written < length) {
#ifdef _WIN32
        DWORD chunk = 0;
        if (!WriteFile(process->toChild, text + written, static_cast<DWORD>(length - written), &chunk, nullptr) || chunk == 0) break;
#else
        const ssize_t chunk = ::write(process->toChild, text + written, length - written);
        if (chunk <= 0) break;
#endif
        written += static_cast<size_t>(chunk);
    }
    return static_cast<int>(written);
}

// Signal end of input to the child (e.g. so `sort` can produce its output).
void csec_piped_process_close_input(long long handle) {
    if (auto process = csec_ipc::pipedProcesses().get(handle)) csec_ipc::closeChildInput(*process);
}

char* csec_piped_process_read_line(long long handle) {
    using namespace csec_ipc;
    auto process = pipedProcesses().get(handle);
    if (!process) return ownedCopy("");
    std::string line;
    csecCooperativeWait([&] {
        std::lock_guard<std::mutex> lock(process->outputMutex);
        pumpChildOutput(*process);
        return takeLine(process->buffer, process->eof, line) || (process->eof && process->buffer.empty());
    }, -1);
    return ownedCopy(line);
}

// Everything the child writes until it closes its output.
char* csec_piped_process_read_all(long long handle) {
    using namespace csec_ipc;
    auto process = pipedProcesses().get(handle);
    if (!process) return ownedCopy("");
    std::string all;
    csecCooperativeWait([&] {
        std::lock_guard<std::mutex> lock(process->outputMutex);
        pumpChildOutput(*process);
        if (!process->eof) return false;
        all.swap(process->buffer);
        return true;
    }, -1);
    std::string normalised;
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i] == '\r' && i + 1 < all.size() && all[i + 1] == '\n') continue;
        normalised.push_back(all[i]);
    }
    return ownedCopy(normalised);
}

int csec_piped_process_eof(long long handle) {
    auto process = csec_ipc::pipedProcesses().get(handle);
    if (!process) return 1;
    std::lock_guard<std::mutex> lock(process->outputMutex);
    csec_ipc::pumpChildOutput(*process);
    return process->eof && process->buffer.empty() ? 1 : 0;
}

// Close input, wait for the child to exit, release the handle; returns the exit code.
int csec_piped_process_wait(long long handle) {
    using namespace csec_ipc;
    auto process = pipedProcesses().take(handle);
    if (!process) return -1;
    auto pump = [&] {
        std::lock_guard<std::mutex> lock(process->outputMutex);
        pumpChildOutput(*process);
    };
    csecCooperativeWait([&] {
        pump();
        std::unique_lock<std::mutex> input(process->inputMutex, std::try_to_lock);
        if (!input.owns_lock()) return false;
        closeChildInputLocked(*process);
        return true;
    }, -1);
#ifdef _WIN32
    csecCooperativeWait([&] {
        pump(); // keep draining so a chatty child cannot block on a full pipe
        return WaitForSingleObject(process->info.hProcess, 0) == WAIT_OBJECT_0;
    }, -1);
    DWORD code = 0;
    GetExitCodeProcess(process->info.hProcess, &code);
    CloseHandle(process->info.hThread);
    CloseHandle(process->info.hProcess);
    std::lock_guard<std::mutex> lock(process->outputMutex);
    CloseHandle(process->fromChild);
    process->fromChild = nullptr;
    process->eof = true;
    return static_cast<int>(code);
#else
    int status = 0;
    csecCooperativeWait([&] {
        pump();
        return waitpid(process->pid, &status, WNOHANG) == process->pid;
    }, -1);
    std::lock_guard<std::mutex> lock(process->outputMutex);
    ::close(process->fromChild);
    process->fromChild = -1;
    process->eof = true;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

// ---- shared memory -------------------------------------------------------------------------------

// Private shared memory for the threads and microthreads of this process (zero-initialised).
long long csec_shm_new(long long bytes) {
    using namespace csec_ipc;
    auto region = std::make_shared<SharedRegion>();
    const size_t dataBytes = roundUp8(bytes);
    region->base = static_cast<unsigned char*>(std::calloc(1, kSharedHeader + dataBytes));
    if (!region->base) return 0;
    region->mapped = kSharedHeader + dataBytes;
    region->usable = dataBytes;
    region->header()->size.store(dataBytes);
    region->header()->magic.store(kSharedMagic);
    return sharedRegions().add(region);
}

// Named shared memory visible to every process of the user session that opens the same name. The
// first opener creates it (zero-initialised) with `bytes`; later openers get the existing size.
long long csec_shm_open(const char* name, long long bytes) {
    using namespace csec_ipc;
    auto region = openNamedRegion(sanitizedName(name) + "_shm", roundUp8(bytes));
    return region ? sharedRegions().add(region) : 0;
}

long long csec_shm_size(long long handle) {
    auto region = csec_ipc::regionFor(handle);
    return region ? static_cast<long long>(region->size()) : 0;
}

long long csec_shm_get_long(long long handle, long long index) {
    auto region = csec_ipc::regionFor(handle);
    auto* slot = region ? csec_ipc::slotAt<std::atomic<long long>>(*region, index * 8) : nullptr;
    return slot ? slot->load() : 0;
}

void csec_shm_set_long(long long handle, long long index, long long value) {
    auto region = csec_ipc::regionFor(handle);
    if (auto* slot = region ? csec_ipc::slotAt<std::atomic<long long>>(*region, index * 8) : nullptr) slot->store(value);
}

// Atomically add `delta` to the Long slot `index`; returns the new value.
long long csec_shm_add_long(long long handle, long long index, long long delta) {
    auto region = csec_ipc::regionFor(handle);
    auto* slot = region ? csec_ipc::slotAt<std::atomic<long long>>(*region, index * 8) : nullptr;
    return slot ? slot->fetch_add(delta) + delta : 0;
}

int csec_shm_compare_exchange_long(long long handle, long long index, long long expected, long long desired) {
    auto region = csec_ipc::regionFor(handle);
    auto* slot = region ? csec_ipc::slotAt<std::atomic<long long>>(*region, index * 8) : nullptr;
    return slot && slot->compare_exchange_strong(expected, desired) ? 1 : 0;
}

double csec_shm_get_double(long long handle, long long index) {
    auto region = csec_ipc::regionFor(handle);
    auto* slot = region ? csec_ipc::slotAt<double>(*region, index * 8) : nullptr;
    return slot ? *slot : 0.0;
}

void csec_shm_set_double(long long handle, long long index, double value) {
    auto region = csec_ipc::regionFor(handle);
    if (auto* slot = region ? csec_ipc::slotAt<double>(*region, index * 8) : nullptr) *slot = value;
}

int csec_shm_get_byte(long long handle, long long offset) {
    auto region = csec_ipc::regionFor(handle);
    auto* slot = region ? csec_ipc::slotAt<unsigned char>(*region, offset) : nullptr;
    return slot ? *slot : 0;
}

void csec_shm_set_byte(long long handle, long long offset, int value) {
    auto region = csec_ipc::regionFor(handle);
    if (auto* slot = region ? csec_ipc::slotAt<unsigned char>(*region, offset) : nullptr) *slot = static_cast<unsigned char>(value);
}

// Store a NUL-terminated string at byte `offset`; returns the bytes stored (0 if it does not fit).
int csec_shm_write_string(long long handle, long long offset, const char* text) {
    auto region = csec_ipc::regionFor(handle);
    if (!region || !text || offset < 0) return 0;
    const size_t length = std::strlen(text);
    if (static_cast<unsigned long long>(offset) + length + 1 > region->size()) return 0;
    std::memcpy(region->data() + offset, text, length + 1);
    return static_cast<int>(length);
}

char* csec_shm_read_string(long long handle, long long offset) {
    auto region = csec_ipc::regionFor(handle);
    if (!region || offset < 0 || static_cast<unsigned long long>(offset) >= region->size()) return csec_ipc::ownedCopy("");
    const char* start = reinterpret_cast<const char*>(region->data() + offset);
    size_t length = 0;
    const size_t limit = region->size() - static_cast<size_t>(offset);
    while (length < limit && start[length] != '\0') ++length;
    return csec_ipc::ownedCopy(std::string(start, length));
}

// Region-wide lock usable across threads, microthreads, and processes.
void csec_shm_lock(long long handle) {
    if (auto region = csec_ipc::regionFor(handle)) csec_ipc::lockRegion(*region);
}

void csec_shm_unlock(long long handle) {
    if (auto region = csec_ipc::regionFor(handle)) csec_ipc::unlockRegion(*region);
}

void csec_shm_close(long long handle) {
    csec_ipc::sharedRegions().take(handle);
}

// POSIX keeps named shared memory until it is removed; on Windows it disappears with its last user.
void csec_shm_remove(const char* name) {
#ifndef _WIN32
    shm_unlink(csec_ipc::posixSharedName(csec_ipc::sanitizedName(name) + "_shm").c_str());
    shm_unlink(csec_ipc::posixSharedName(csec_ipc::sanitizedName(name) + "_mail").c_str());
#else
    (void)name;
#endif
}

// ---- mail ----------------------------------------------------------------------------------------

// Open the named mailbox (creating it if needed). `sender` names the messages this endpoint sends.
long long csec_mail_open(const char* mailbox, const char* sender) {
    using namespace csec_ipc;
    auto region = openNamedRegion(sanitizedName(mailbox) + "_mail", sizeof(MailState) + kMailCapacity);
    if (!region || region->size() < sizeof(MailState) + 64) return 0;
    auto endpoint = std::make_shared<MailEndpoint>();
    endpoint->region = region;
    endpoint->sender = sender ? sender : "";
    return mailEndpoints().add(endpoint);
}

// Queue a message; 1 on success, 0 if the mailbox is full or the message is too large.
int csec_mail_send(long long endpoint, long long tag, const char* body) {
    using namespace csec_ipc;
    auto box = mailEndpoints().get(endpoint);
    if (!box) return 0;
    const std::string text = body ? body : "";
    const std::uint32_t lengths[2] = { static_cast<std::uint32_t>(text.size()), static_cast<std::uint32_t>(box->sender.size()) };
    const size_t record = sizeof(lengths) + sizeof(tag) + box->sender.size() + text.size();
    lockRegion(*box->region);
    MailState* state = box->state();
    const unsigned long long used = state->tail - state->head;
    if (used > box->capacity() || record > box->capacity() - used) {
        unlockRegion(*box->region);
        return 0;
    }
    unsigned long long cursor = state->tail;
    ringWrite(*box, cursor, lengths, sizeof(lengths));
    cursor += sizeof(lengths);
    ringWrite(*box, cursor, &tag, sizeof(tag));
    cursor += sizeof(tag);
    ringWrite(*box, cursor, box->sender.data(), box->sender.size());
    cursor += box->sender.size();
    ringWrite(*box, cursor, text.data(), text.size());
    state->tail = cursor + text.size();
    state->count += 1;
    unlockRegion(*box->region);
    return 1;
}

// Wait up to timeoutMs (forever if < 0) for a message; on success its body, sender, and tag become
// available through csec_mail_last_*. Returns 1 if a message was received.
int csec_mail_receive(long long endpoint, long long timeoutMs) {
    using namespace csec_ipc;
    auto box = mailEndpoints().get(endpoint);
    if (!box) return 0;
    return csecCooperativeWait([&] { return popMail(*box); }, timeoutMs) ? 1 : 0;
}

char* csec_mail_last_body(long long endpoint) {
    auto box = csec_ipc::mailEndpoints().get(endpoint);
    if (!box) return csec_ipc::ownedCopy("");
    std::lock_guard<std::mutex> lock(box->lastMutex);
    return csec_ipc::ownedCopy(box->lastBody);
}

char* csec_mail_last_sender(long long endpoint) {
    auto box = csec_ipc::mailEndpoints().get(endpoint);
    if (!box) return csec_ipc::ownedCopy("");
    std::lock_guard<std::mutex> lock(box->lastMutex);
    return csec_ipc::ownedCopy(box->lastSender);
}

long long csec_mail_last_tag(long long endpoint) {
    auto box = csec_ipc::mailEndpoints().get(endpoint);
    if (!box) return 0;
    std::lock_guard<std::mutex> lock(box->lastMutex);
    return box->lastTag;
}

int csec_mail_count(long long endpoint) {
    auto box = csec_ipc::mailEndpoints().get(endpoint);
    if (!box) return 0;
    return static_cast<int>(box->state()->count);
}

void csec_mail_close(long long endpoint) {
    csec_ipc::mailEndpoints().take(endpoint);
}

} // extern "C"
