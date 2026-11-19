namespace csec_leak {

struct Tracker {
    std::mutex mutex;
    std::unordered_map<void*, size_t> live;
    bool enabled = false;
    Tracker() {
        const char* flag = std::getenv("CSEC_LEAK_CHECK");
        enabled = flag && *flag && *flag != '0';
    }
};

Tracker& tracker() {
    static Tracker* instance = new Tracker();
    return *instance;
}

struct Reporter {
    ~Reporter() {
        Tracker& t = tracker();
        if (!t.enabled) return;
        std::lock_guard<std::mutex> lock(t.mutex);
        size_t bytes = 0;
        for (const auto& entry : t.live) bytes += entry.second;
        std::fprintf(stderr, "csec-leak-check: %zu block(s), %zu byte(s) not freed\n", t.live.size(), bytes);
        std::fflush(stderr);
    }
};

Reporter reporter;

}

static void* csecAlloc(size_t size) {
    void* block = std::malloc(size ? size : 1);
    csec_leak::Tracker& t = csec_leak::tracker();
    if (block && t.enabled) {
        std::lock_guard<std::mutex> lock(t.mutex);
        t.live[block] = size;
    }
    return block;
}

static void csecFree(void* block) {
    if (!block) return;
    csec_leak::Tracker& t = csec_leak::tracker();
    if (t.enabled) {
        std::lock_guard<std::mutex> lock(t.mutex);
        t.live.erase(block);
    }
    std::free(block);
}

extern "C" CSEC_NATIVE_API void* csec_alloc(long long size) {
    return csecAlloc(size > 0 ? static_cast<size_t>(size) : 0);
}

extern "C" CSEC_NATIVE_API void csec_free(void* block) {
    csecFree(block);
}
