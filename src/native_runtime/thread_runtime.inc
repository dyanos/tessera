// OS threads, synchronisation primitives, and OS processes (std.thread / std.process).
//
// Everything is addressed through 64-bit handles (never 0 for a live object) so the library modules
// can be written with plain `external def` declarations. Each OS thread gets its own microthread
// scheduler on first use (process_runtime.inc keeps the scheduler thread_local), so a thread may
// spawn Erlang-style processes of its own.

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>

namespace csec_threads {

template <typename T>
class HandleTable {
public:
    long long add(std::shared_ptr<T> object) {
        std::lock_guard<std::mutex> lock(mutex_);
        const long long handle = next_++;
        objects_[handle] = std::move(object);
        return handle;
    }
    std::shared_ptr<T> get(long long handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = objects_.find(handle);
        return it == objects_.end() ? nullptr : it->second;
    }
    std::shared_ptr<T> take(long long handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = objects_.find(handle);
        if (it == objects_.end()) return nullptr;
        auto object = std::move(it->second);
        objects_.erase(it);
        return object;
    }

private:
    std::mutex mutex_;
    std::map<long long, std::shared_ptr<T>> objects_;
    long long next_ = 1;
};

struct ThreadRecord {
    std::thread thread;
    std::atomic<int> result{0};
    std::atomic<bool> finished{false};
    void* env = nullptr;
    // A thread that is never joined is abandoned (as in C, Go, or Rust): the program may exit while it
    // runs. The last reference can also be dropped on the thread itself, so ~thread() must not
    // terminate the process.
    ~ThreadRecord() {
        if (thread.joinable()) thread.detach();
    }
};

// Wait on `cv` until `ready()` holds (`lock` is held on entry and on return); `timeoutMs < 0` waits
// forever. Returns whether `ready()` became true. A microthread must not block its OS thread while
// another microthread of that thread could run (the one it waits for may share the scheduler), so
// while others are runnable it releases the lock and yields; otherwise it sleeps on `cv` in short
// slices so that microthreads woken by timers are noticed.
template <typename Ready>
bool waitCooperatively(std::unique_lock<std::mutex>& lock, std::condition_variable& cv, Ready ready, long long timeoutMs) {
    const bool bounded = timeoutMs >= 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(bounded ? timeoutMs : 0);
    while (!ready()) {
        const auto now = std::chrono::steady_clock::now();
        if (bounded && now >= deadline) return false;
        if (csec_microthread::hasRunnableOthers()) {
            lock.unlock();
            csec_process_yield();
            lock.lock();
            continue;
        }
        auto slice = now + std::chrono::milliseconds(csec_microthread::scheduler.initialized ? 1 : 50);
        if (bounded && deadline < slice) slice = deadline;
        cv.wait_until(lock, slice);
    }
    return true;
}

// A non-recursive lock that microthreads can contend for: waiting yields to the other microthreads of
// the OS thread instead of deadlocking it, and unlocking a mutex that is not locked is a reported
// no-op rather than undefined behaviour.
struct Mutex {
    std::mutex guard;
    std::condition_variable released;
    bool locked = false;
};

struct Channel {
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<long long> items;
    bool closed = false;
};

// The tables are intentionally never destroyed: abandoned threads may still use them while the
// program's static destructors run at exit.
template <typename T>
HandleTable<T>& leakedTable() {
    static auto* table = new HandleTable<T>();
    return *table;
}

HandleTable<ThreadRecord>& threads() { return leakedTable<ThreadRecord>(); }
HandleTable<Mutex>& mutexes() { return leakedTable<Mutex>(); }
HandleTable<std::atomic<long long>>& atomics() { return leakedTable<std::atomic<long long>>(); }
HandleTable<Channel>& channels() { return leakedTable<Channel>(); }

#ifdef _WIN32
struct OsProcess {
    PROCESS_INFORMATION info{};
};
#else
struct OsProcess {
    pid_t pid = -1;
    int status = -1;
    bool reaped = false;
};
#endif
HandleTable<OsProcess>& osProcesses() { return leakedTable<OsProcess>(); }

} // namespace csec_threads

#ifndef _WIN32
#include <csignal>
#include <sys/wait.h>
#endif

extern "C" {

// ---- threads ------------------------------------------------------------------------------------

// Start an OS thread running the closure `code(env)` (an `() => Int` lambda or function). As with
// spawn, `envSize` bytes of the environment are copied so the thread owns its captures.
long long csec_thread_start(void* code, void* env, long long envSize) {
    using namespace csec_threads;
    auto record = std::make_shared<ThreadRecord>();
    if (env && envSize > 0) {
        record->env = std::malloc(static_cast<size_t>(envSize));
        if (!record->env) return 0;
        std::memcpy(record->env, env, static_cast<size_t>(envSize));
    } else {
        record->env = env;
    }
    const bool ownsEnv = env && envSize > 0;
    ThreadRecord* raw = record.get();
    const long long handle = threads().add(record);
    try {
        // The thread keeps its own reference, so the record outlives a handle that is never joined.
        raw->thread = std::thread([record, code, ownsEnv]() {
            using Entry = int (*)(void*);
            record->result = reinterpret_cast<Entry>(code)(record->env);
            if (ownsEnv) std::free(record->env);
            record->env = nullptr;
            record->finished = true;
        });
    } catch (...) {
        threads().take(handle);
        if (ownsEnv) std::free(raw->env);
        return 0;
    }
    return handle;
}

// Wait for the thread and return the Int its body returned (-1 for an unknown handle).
int csec_thread_join(long long handle) {
    auto current = csec_threads::threads().get(handle);
    if (!current || current->thread.get_id() == std::this_thread::get_id()) return -1;
    auto record = csec_threads::threads().take(handle);
    if (!record) return -1;
    // Let the other microthreads of this OS thread run while the joined thread finishes.
    while (!record->finished && csec_microthread::hasRunnableOthers()) csec_process_yield();
    try {
        if (record->thread.joinable()) record->thread.join();
    } catch (...) {
        return -1;
    }
    return record->result;
}

int csec_thread_is_finished(long long handle) {
    auto record = csec_threads::threads().get(handle);
    return !record || record->finished ? 1 : 0;
}

long long csec_thread_current_id(void) {
    return static_cast<long long>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0x7fffffffffffLL);
}

void csec_thread_sleep(int milliseconds) {
    if (milliseconds > 0) std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void csec_thread_yield(void) {
    std::this_thread::yield();
}

int csec_thread_hardware_concurrency(void) {
    const unsigned count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : static_cast<int>(count);
}

// ---- mutex --------------------------------------------------------------------------------------

long long csec_mutex_new(void) {
    return csec_threads::mutexes().add(std::make_shared<csec_threads::Mutex>());
}

int csec_mutex_lock(long long handle) {
    auto m = csec_threads::mutexes().get(handle);
    if (!m) return 0;
    std::unique_lock<std::mutex> lock(m->guard);
    csec_threads::waitCooperatively(lock, m->released, [&] { return !m->locked; }, -1);
    m->locked = true;
    return 1;
}

int csec_mutex_try_lock(long long handle) {
    auto m = csec_threads::mutexes().get(handle);
    if (!m) return 0;
    std::lock_guard<std::mutex> lock(m->guard);
    if (m->locked) return 0;
    m->locked = true;
    return 1;
}

// Returns 0 (and does nothing) for an unknown handle or a mutex that is not locked.
int csec_mutex_unlock(long long handle) {
    auto m = csec_threads::mutexes().get(handle);
    if (!m) return 0;
    {
        std::lock_guard<std::mutex> lock(m->guard);
        if (!m->locked) return 0;
        m->locked = false;
    }
    m->released.notify_one();
    return 1;
}

void csec_mutex_free(long long handle) {
    csec_threads::mutexes().take(handle);
}

// ---- atomics ------------------------------------------------------------------------------------

long long csec_atomic_new(long long initial) {
    return csec_threads::atomics().add(std::make_shared<std::atomic<long long>>(initial));
}

long long csec_atomic_get(long long handle) {
    auto a = csec_threads::atomics().get(handle);
    return a ? a->load() : 0;
}

void csec_atomic_set(long long handle, long long value) {
    if (auto a = csec_threads::atomics().get(handle)) a->store(value);
}

long long csec_atomic_add(long long handle, long long delta) {
    auto a = csec_threads::atomics().get(handle);
    return a ? a->fetch_add(delta) + delta : 0;
}

int csec_atomic_compare_exchange(long long handle, long long expected, long long desired) {
    auto a = csec_threads::atomics().get(handle);
    return a && a->compare_exchange_strong(expected, desired) ? 1 : 0;
}

void csec_atomic_free(long long handle) {
    csec_threads::atomics().take(handle);
}

// ---- channels (blocking, multi-producer multi-consumer, between OS threads) -----------------------

long long csec_channel_new(void) {
    return csec_threads::channels().add(std::make_shared<csec_threads::Channel>());
}

int csec_channel_send(long long handle, long long value) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return 0;
    {
        std::lock_guard<std::mutex> lock(c->mutex);
        if (c->closed) return 0;
        c->items.push_back(value);
    }
    c->ready.notify_one();
    return 1;
}

// Waits until a value arrives, at most `milliseconds` (< 0 waits forever); returns `fallback` on
// timeout, once the channel is closed and drained, or for an unknown handle. A microthread waiting
// here lets the other microthreads of its OS thread run.
long long csec_channel_receive_timeout(long long handle, long long milliseconds, long long fallback) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return fallback;
    std::unique_lock<std::mutex> lock(c->mutex);
    csec_threads::waitCooperatively(lock, c->ready, [&] { return !c->items.empty() || c->closed; }, milliseconds);
    if (c->items.empty()) return fallback;
    long long value = c->items.front();
    c->items.pop_front();
    return value;
}

long long csec_channel_receive(long long handle, long long fallback) {
    return csec_channel_receive_timeout(handle, -1, fallback);
}

int csec_channel_is_closed(long long handle) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return 1;
    std::lock_guard<std::mutex> lock(c->mutex);
    return c->closed ? 1 : 0;
}

long long csec_channel_try_receive(long long handle, long long fallback) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return fallback;
    std::lock_guard<std::mutex> lock(c->mutex);
    if (c->items.empty()) return fallback;
    long long value = c->items.front();
    c->items.pop_front();
    return value;
}

int csec_channel_size(long long handle) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return 0;
    std::lock_guard<std::mutex> lock(c->mutex);
    return static_cast<int>(c->items.size());
}

void csec_channel_close(long long handle) {
    auto c = csec_threads::channels().get(handle);
    if (!c) return;
    {
        std::lock_guard<std::mutex> lock(c->mutex);
        c->closed = true;
    }
    c->ready.notify_all();
}

// Close the channel (waking its receivers) and release the handle.
void csec_channel_free(long long handle) {
    csec_channel_close(handle);
    csec_threads::channels().take(handle);
}

// ---- OS processes -------------------------------------------------------------------------------

// Run a shell command and wait for it; returns its exit code (-1 if it could not be started).
int csec_os_process_run(const char* command) {
    if (!command) return -1;
    std::fflush(stdout);
    const int status = std::system(command);
#ifdef _WIN32
    return status;
#else
    if (status == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

// Run a shell command and capture its standard output.
char* csec_os_process_output(const char* command) {
    std::string output;
    if (command) {
        std::fflush(stdout);
#ifdef _WIN32
        FILE* pipe = _popen(command, "r");
#else
        FILE* pipe = popen(command, "r");
#endif
        if (pipe) {
            char buffer[4096];
            size_t read = 0;
            while ((read = std::fread(buffer, 1, sizeof(buffer), pipe)) > 0) output.append(buffer, read);
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif
        }
    }
    // Normalise CRLF so callers see the same text on every platform.
    std::string normalised;
    normalised.reserve(output.size());
    for (size_t i = 0; i < output.size(); ++i) {
        if (output[i] == '\r' && i + 1 < output.size() && output[i + 1] == '\n') continue;
        normalised.push_back(output[i]);
    }
    char* result = static_cast<char*>(std::malloc(normalised.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, normalised.data(), normalised.size());
    result[normalised.size()] = '\0';
    return result;
}

// Start a shell command without waiting; returns a handle for wait/kill (0 on failure).
long long csec_os_process_start(const char* command) {
    if (!command) return 0;
    auto process = std::make_shared<csec_threads::OsProcess>();
#ifdef _WIN32
    std::string line = std::string("cmd.exe /c ") + command;
    std::vector<char> mutableLine(line.begin(), line.end());
    mutableLine.push_back('\0');
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    if (!CreateProcessA(nullptr, mutableLine.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process->info)) {
        return 0;
    }
#else
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command, static_cast<char*>(nullptr));
        _exit(127);
    }
    process->pid = pid;
#endif
    return csec_threads::osProcesses().add(process);
}

// Wait for a started process; returns its exit code and releases the handle.
int csec_os_process_wait(long long handle) {
    auto process = csec_threads::osProcesses().take(handle);
    if (!process) return -1;
#ifdef _WIN32
    WaitForSingleObject(process->info.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process->info.hProcess, &code);
    CloseHandle(process->info.hThread);
    CloseHandle(process->info.hProcess);
    return static_cast<int>(code);
#else
    int status = 0;
    if (!process->reaped && waitpid(process->pid, &status, 0) < 0) return -1;
    if (process->reaped) status = process->status;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

int csec_os_process_is_running(long long handle) {
    auto process = csec_threads::osProcesses().get(handle);
    if (!process) return 0;
#ifdef _WIN32
    return WaitForSingleObject(process->info.hProcess, 0) == WAIT_TIMEOUT ? 1 : 0;
#else
    if (process->reaped) return 0;
    int status = 0;
    pid_t result = waitpid(process->pid, &status, WNOHANG);
    if (result == 0) return 1;
    process->reaped = true;
    process->status = status;
    return 0;
#endif
}

int csec_os_process_kill(long long handle) {
    auto process = csec_threads::osProcesses().get(handle);
    if (!process) return 0;
#ifdef _WIN32
    return TerminateProcess(process->info.hProcess, 1) ? 1 : 0;
#else
    return kill(process->pid, SIGTERM) == 0 ? 1 : 0;
#endif
}

int csec_os_process_id(void) {
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

void csec_os_process_exit(int code) {
    std::fflush(stdout);
    std::fflush(stderr);
    std::exit(code);
}

} // extern "C"
