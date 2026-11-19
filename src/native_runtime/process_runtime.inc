// Erlang-style microthreads ("processes").
//
// A process is a lightweight user-mode thread with its own stack and a private mailbox of Int
// messages. Processes share nothing: a spawned lambda's captured environment is copied at spawn
// time, and processes communicate only by `send`/`receive`. Scheduling is cooperative and M:1 —
// every process of a scheduler runs on the OS thread that created it, and a context switch happens
// only at `receive` (empty mailbox), `yield`, `joinProcesses`, or when a process finishes. Because a
// process never runs concurrently with another process of the same scheduler, mailbox operations
// need no locks.
//
// Context switching uses Win32 fibers on Windows and, on POSIX (Linux, macOS), a small assembly
// stack switch for x86-64/AArch64 with a ucontext fallback elsewhere (context_switch.inc). The thread that first
// touches the process API becomes the root process (pid 1, normally `main`).

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

#include "context_switch.inc"

namespace csec_microthread {

constexpr size_t kProcessStackSize = 512 * 1024;

enum class ProcState { Runnable, Running, Waiting, Joining, Done };

struct Process {
    int pid = 0;
    ProcState state = ProcState::Runnable;
    bool queued = false;
    void* code = nullptr;
    void* env = nullptr;
    bool ownsEnv = false;
    std::deque<long long> mailbox;
    std::deque<std::string> textMailbox;
    bool hasDeadline = false;
    bool timedOut = false;
    std::chrono::steady_clock::time_point deadline;
    std::vector<std::pair<int, long long>> monitors; // (watcher pid, message to deliver on exit)
#ifdef _WIN32
    void* fiber = nullptr;
#else
#if CSEC_FAST_CONTEXT
    void* stackPointer = nullptr;   // saved by csec_ctx_switch while the process is not running
#else
    ucontext_t context;
#endif
    void* stack = nullptr;          // mapped stack (with guard page); null for the root process
    size_t stackBytes = 0;
#endif
};

struct Scheduler {
    bool initialized = false;
    int nextPid = 2;
    std::unordered_map<int, std::unique_ptr<Process>> live;
    std::deque<Process*> runQueue;
    std::vector<Process*> joiners;
    std::vector<std::unique_ptr<Process>> zombies;
    Process* current = nullptr;
    Process* root = nullptr;
    int timedWaiters = 0;
};

thread_local Scheduler scheduler;

Scheduler& ensureScheduler() {
    Scheduler& s = scheduler;
    if (s.initialized) return s;
    s.initialized = true;
    auto root = std::make_unique<Process>();
    root->pid = 1;
    root->state = ProcState::Running;
#ifdef _WIN32
    if (IsThreadAFiber()) {
        root->fiber = GetCurrentFiber();
    } else {
        root->fiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
    }
    if (!root->fiber) {
        std::fprintf(stderr, "csec: failed to initialise the process scheduler\n");
        std::exit(70);
    }
#endif
    s.root = root.get();
    s.current = root.get();
    s.live.emplace(1, std::move(root));
    return s;
}

Process* findLive(int pid) {
    auto& s = scheduler;
    auto it = s.live.find(pid);
    return it == s.live.end() ? nullptr : it->second.get();
}

// Release the stacks of processes that finished. Only ever called on a different stack from the
// ones being freed (i.e. right after a context switch lands somewhere).
void reapZombies() {
    auto& s = scheduler;
    for (auto& zombie : s.zombies) {
#ifdef _WIN32
        if (zombie->fiber) DeleteFiber(zombie->fiber);
#else
        csecFreeStack(zombie->stack, zombie->stackBytes);
#endif
    }
    s.zombies.clear();
}

void enqueue(Process* p) {
    p->state = ProcState::Runnable;
    if (!p->queued) {
        p->queued = true;
        scheduler.runQueue.push_back(p);
    }
}

void clearDeadline(Process* p) {
    if (p->hasDeadline) {
        p->hasDeadline = false;
        --scheduler.timedWaiters;
    }
}

void wakeOnTimeout(Process* p) {
    clearDeadline(p);
    p->timedOut = true;
    enqueue(p);
}

// Wake every timed waiter whose deadline has passed. Returns the earliest pending deadline holder.
Process* expireTimers() {
    auto& s = scheduler;
    if (s.timedWaiters == 0) return nullptr;
    const auto now = std::chrono::steady_clock::now();
    Process* earliest = nullptr;
    for (auto& entry : s.live) {
        Process* p = entry.second.get();
        if (p->state != ProcState::Waiting || !p->hasDeadline) continue;
        if (p->deadline <= now) {
            wakeOnTimeout(p);
        } else if (!earliest || p->deadline < earliest->deadline) {
            earliest = p;
        }
    }
    return earliest;
}

Process* pickNext() {
    auto& s = scheduler;
    for (;;) {
        Process* earliest = expireTimers();
        while (!s.runQueue.empty()) {
            Process* p = s.runQueue.front();
            s.runQueue.pop_front();
            p->queued = false;
            if (p->state == ProcState::Runnable) return p;
        }
        if (earliest) {
            std::this_thread::sleep_until(earliest->deadline);
            wakeOnTimeout(earliest);
            continue;
        }
        if (!s.joiners.empty()) {
            Process* joiner = s.joiners.back();
            s.joiners.pop_back();
            return joiner;
        }
        std::fprintf(stderr, "csec: deadlock - every process is blocked in receive() or await\n");
        std::fflush(stderr);
        std::exit(70);
    }
}

void switchTo(Process* next) {
    auto& s = scheduler;
    Process* prev = s.current;
    next->state = ProcState::Running;
    if (next == prev) return;
    s.current = next;
#ifdef _WIN32
    SwitchToFiber(next->fiber);
#elif CSEC_FAST_CONTEXT
    csec_ctx_switch(&prev->stackPointer, next->stackPointer);
#else
    swapcontext(&prev->context, &next->context);
#endif
    // Execution resumes here when some other process switches back to `prev`.
    reapZombies();
}

// Make a waiting process re-check the condition it waits on (a message, a future, ...). Its timeout,
// if any, stays armed: a wake-up for a different kind of message must not cancel it.
void wake(Process* target) {
    if (target->state == ProcState::Waiting) enqueue(target);
}

void deliver(Process* target, long long message) {
    target->mailbox.push_back(message);
    wake(target);
}

[[noreturn]] void finishCurrent() {
    auto& s = scheduler;
    Process* self = s.current;
    self->state = ProcState::Done;
    clearDeadline(self);
    for (const auto& monitor : self->monitors) {
        if (Process* watcher = findLive(monitor.first)) deliver(watcher, monitor.second);
    }
    self->monitors.clear();
    if (self->ownsEnv) csecFree(self->env);
    self->env = nullptr;
    auto it = s.live.find(self->pid);
    s.zombies.push_back(std::move(it->second));
    s.live.erase(it);
    switchTo(pickNext());
    std::abort(); // a finished process is never resumed
}

void runCurrent() {
    reapZombies();
    Process* self = scheduler.current;
    using Entry = void (*)(void*);
    reinterpret_cast<Entry>(self->code)(self->env);
    finishCurrent();
}

#ifdef _WIN32
VOID CALLBACK fiberEntry(LPVOID) {
    runCurrent();
}
#elif !CSEC_FAST_CONTEXT
void contextEntry() {
    runCurrent();
}
#endif

} // namespace csec_microthread

#if !defined(_WIN32) && CSEC_FAST_CONTEXT
extern "C" void csec_ctx_entry(void) {
    csec_microthread::runCurrent();
}
#endif

extern "C" {

int csec_process_spawn(void* code, void* env, long long envSize) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    auto p = std::make_unique<Process>();
    p->pid = s.nextPid++;
    p->code = code;
    // Share nothing: snapshot the closure environment so the child owns its own copy, independent of
    // the spawning frame's lifetime.
    if (env && envSize > 0) {
        p->env = csecAlloc(static_cast<size_t>(envSize));
        if (!p->env) return 0;
        std::memcpy(p->env, env, static_cast<size_t>(envSize));
        p->ownsEnv = true;
    } else {
        p->env = env;
    }
#ifdef _WIN32
    p->fiber = CreateFiberEx(0, kProcessStackSize, FIBER_FLAG_FLOAT_SWITCH, fiberEntry, nullptr);
    if (!p->fiber) {
        if (p->ownsEnv) csecFree(p->env);
        return 0;
    }
#else
    p->stack = csecAllocateStack(kProcessStackSize, p->stackBytes);
    if (!p->stack) {
        if (p->ownsEnv) csecFree(p->env);
        return 0;
    }
#if CSEC_FAST_CONTEXT
    p->stackPointer = csecInitialStackPointer(p->stack, p->stackBytes);
#else
    if (getcontext(&p->context) != 0) {
        csecFreeStack(p->stack, p->stackBytes);
        if (p->ownsEnv) csecFree(p->env);
        return 0;
    }
    p->context.uc_stack.ss_sp = p->stack;
    p->context.uc_stack.ss_size = p->stackBytes;
    p->context.uc_link = nullptr;
    makecontext(&p->context, contextEntry, 0);
#endif
#endif
    Process* raw = p.get();
    const int pid = raw->pid;
    s.live.emplace(pid, std::move(p));
    enqueue(raw);
    return pid;
}

int csec_process_self(void) {
    return csec_microthread::ensureScheduler().current->pid;
}

int csec_process_send(int pid, long long message) {
    using namespace csec_microthread;
    ensureScheduler();
    Process* target = findLive(pid);
    if (!target) return 0; // like Erlang, sending to a dead process is not an error
    deliver(target, message);
    return 1;
}

long long csec_process_receive(void) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    Process* self = s.current;
    while (self->mailbox.empty()) {
        self->state = ProcState::Waiting;
        switchTo(pickNext());
    }
    long long message = self->mailbox.front();
    self->mailbox.pop_front();
    return message;
}

long long csec_process_receive_timeout(long long milliseconds, long long fallback) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    Process* self = s.current;
    if (self->mailbox.empty() && milliseconds > 0) {
        self->timedOut = false;
        self->deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        self->hasDeadline = true;
        ++s.timedWaiters;
        while (self->mailbox.empty() && !self->timedOut) {
            self->state = ProcState::Waiting;
            switchTo(pickNext());
        }
        clearDeadline(self);
    }
    if (self->mailbox.empty()) return fallback;
    long long message = self->mailbox.front();
    self->mailbox.pop_front();
    return message;
}

void csec_process_yield(void) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    expireTimers();
    if (s.runQueue.empty()) return;
    enqueue(s.current);
    switchTo(pickNext());
}

int csec_process_alive(int pid) {
    csec_microthread::ensureScheduler();
    return csec_microthread::findLive(pid) ? 1 : 0;
}

int csec_process_monitor(int pid, long long message) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    Process* target = findLive(pid);
    if (!target) {
        deliver(s.current, message); // already gone: notify immediately, as Erlang's monitor does
        return 0;
    }
    target->monitors.emplace_back(s.current->pid, message);
    return 1;
}

int csec_process_mailbox_size(void) {
    return static_cast<int>(csec_microthread::ensureScheduler().current->mailbox.size());
}

int csec_process_count(void) {
    return static_cast<int>(csec_microthread::ensureScheduler().live.size());
}

// Run every other process until none can make progress (all finished or blocked forever in
// receive). Returns the number of other processes still alive (i.e. blocked) afterwards.
int csec_process_join(void) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    if (!s.runQueue.empty() || s.timedWaiters > 0) {
        Process* self = s.current;
        self->state = ProcState::Joining;
        s.joiners.push_back(self);
        switchTo(pickNext());
    }
    return static_cast<int>(s.live.size()) - 1;
}

} // extern "C"

// ---- Futures (`async call` / `await`) -------------------------------------------------------------
//
// A future is a one-shot result slot filled by the microthread that runs an `async` call. Values are
// carried as 64-bit payloads (integers sign-extended, Double bit-cast, pointers as addresses); the
// compiler converts back to the awaited type. Awaiting blocks only the awaiting process.
namespace csec_microthread {
struct Future {
    std::mutex mutex;
    bool done = false;
    long long value = 0;
    Scheduler* owner = nullptr;
    std::vector<int> waiters; // pids of `owner` blocked in await
};
}

extern "C" {

void* csec_future_new(void) {
    auto* future = new csec_microthread::Future();
    future->owner = &csec_microthread::scheduler;
    return future;
}

void csec_future_complete(void* handle, long long value) {
    using namespace csec_microthread;
    auto* future = static_cast<Future*>(handle);
    if (!future) return;
    std::vector<int> waiters;
    {
        std::lock_guard<std::mutex> lock(future->mutex);
        if (future->done) return;
        future->done = true;
        future->value = value;
        if (future->owner == &scheduler) waiters.swap(future->waiters);
    }
    for (int pid : waiters) {
        Process* waiter = findLive(pid);
        if (waiter && waiter->state == ProcState::Waiting) {
            clearDeadline(waiter);
            enqueue(waiter);
        }
    }
}

int csec_future_ready(void* handle) {
    auto* future = static_cast<csec_microthread::Future*>(handle);
    if (!future) return 0;
    std::lock_guard<std::mutex> lock(future->mutex);
    return future->done ? 1 : 0;
}

long long csec_future_await(void* handle) {
    using namespace csec_microthread;
    auto* future = static_cast<Future*>(handle);
    if (!future) return 0;
    auto& s = ensureScheduler();
    for (;;) {
        Process* self = s.current;
        const bool local = future->owner == &s;
        {
            std::lock_guard<std::mutex> lock(future->mutex);
            if (future->done) return future->value;
            if (local) future->waiters.push_back(self->pid);
        }
        if (!local) {
            self->timedOut = false;
            self->deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
            self->hasDeadline = true;
            ++s.timedWaiters;
        }
        self->state = ProcState::Waiting;
        switchTo(pickNext());
        clearDeadline(self);
    }
}

// Start `code(env)` as a microthread (env copied, as for spawn) whose job is to fill `future`.
// Used by `async f(args)`; the generated thunk calls csec_future_complete itself.
int csec_async_start(void* code, void* env, long long envSize) {
    return csec_process_spawn(code, env, envSize);
}

} // extern "C"

// ---- Text messages and cooperative waiting ---------------------------------------------------------
//
// Besides Int messages, every process has a second FIFO mailbox of strings (sendText/receiveText), so
// processes can exchange structured data without sharing memory (the text is copied).
//
// Blocking IPC primitives (pipes, mail, shared-memory locks; ipc_runtime.inc) wait cooperatively:
// while another process of this OS thread can run they yield to it, otherwise they sleep briefly.
namespace csec_microthread {
bool hasRunnableOthers() {
    auto& s = scheduler;
    if (!s.initialized) return false;
    expireTimers();
    return !s.runQueue.empty();
}

char* ownedText(const std::string& text) {
    char* result = static_cast<char*>(csecAlloc(text.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, text.data(), text.size());
    result[text.size()] = '\0';
    return result;
}
}

extern "C" {

int csec_process_send_text(int pid, const char* text) {
    using namespace csec_microthread;
    ensureScheduler();
    Process* target = findLive(pid);
    if (!target) return 0;
    target->textMailbox.emplace_back(text ? text : "");
    wake(target);
    return 1;
}

char* csec_process_receive_text(void) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    Process* self = s.current;
    while (self->textMailbox.empty()) {
        self->state = ProcState::Waiting;
        switchTo(pickNext());
    }
    std::string text = std::move(self->textMailbox.front());
    self->textMailbox.pop_front();
    return ownedText(text);
}

char* csec_process_receive_text_timeout(long long milliseconds, const char* fallback) {
    using namespace csec_microthread;
    auto& s = ensureScheduler();
    Process* self = s.current;
    if (self->textMailbox.empty() && milliseconds > 0) {
        self->timedOut = false;
        self->deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        self->hasDeadline = true;
        ++s.timedWaiters;
        while (self->textMailbox.empty() && !self->timedOut) {
            self->state = ProcState::Waiting;
            switchTo(pickNext());
        }
        clearDeadline(self);
    }
    if (self->textMailbox.empty()) return ownedText(fallback ? fallback : "");
    std::string text = std::move(self->textMailbox.front());
    self->textMailbox.pop_front();
    return ownedText(text);
}

int csec_process_text_count(void) {
    return static_cast<int>(csec_microthread::ensureScheduler().current->textMailbox.size());
}

} // extern "C"
