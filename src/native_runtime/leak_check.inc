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

namespace csec_rc {

struct Shard {
    std::mutex mutex;
    std::unordered_map<const void*, long long> counts;
    std::unordered_map<const void*, void (*)(void*)> destructors;
};

Shard& shardFor(const void* block) {
    static Shard* shards = new Shard[64];
    const auto key = reinterpret_cast<uintptr_t>(block);
    return shards[(key >> 4) & 63];
}

}

static bool csecRcTracked(const void* block) {
    if (!block) return false;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<std::mutex> lock(shard.mutex);
    return shard.counts.count(block) != 0;
}

extern "C" CSEC_NATIVE_API void csec_rc_adopt(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<std::mutex> lock(shard.mutex);
    shard.counts[block] = 1;
}

extern "C" CSEC_NATIVE_API void csec_rc_adopt_with(const void* block, void* destructor) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<std::mutex> lock(shard.mutex);
    shard.counts[block] = 1;
    if (destructor) shard.destructors[block] = reinterpret_cast<void (*)(void*)>(destructor);
}

extern "C" CSEC_NATIVE_API void csec_rc_retain(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<std::mutex> lock(shard.mutex);
    auto found = shard.counts.find(block);
    if (found != shard.counts.end()) ++found->second;
}

extern "C" CSEC_NATIVE_API void csec_rc_release(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    bool last = false;
    void (*destructor)(void*) = nullptr;
    {
        std::lock_guard<std::mutex> lock(shard.mutex);
        auto found = shard.counts.find(block);
        if (found == shard.counts.end()) return;
        last = --found->second <= 0;
        if (last) {
            shard.counts.erase(found);
            auto dtor = shard.destructors.find(block);
            if (dtor != shard.destructors.end()) {
                destructor = dtor->second;
                shard.destructors.erase(dtor);
            }
        }
    }
    if (!last) return;
    if (destructor) destructor(const_cast<void*>(block));
    csecFree(const_cast<void*>(block));
}
