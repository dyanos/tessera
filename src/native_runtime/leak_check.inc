#include <atomic>

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
        const char* detail = std::getenv("CSEC_LEAK_CHECK");
        if (detail && std::strcmp(detail, "2") == 0) {
            for (const auto& entry : t.live) {
                std::fprintf(stderr, "  %zu bytes at %p\n", entry.second, entry.first);
            }
        }
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

struct Entry {
    const void* key;
    long long count;
    void (*destructor)(void*);
};

const void* const tombstone = reinterpret_cast<const void*>(uintptr_t(1));

struct SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() {
        for (int spins = 0; flag.test_and_set(std::memory_order_acquire); ++spins) {
            if (spins >= 64) std::this_thread::yield();
        }
    }
    void unlock() { flag.clear(std::memory_order_release); }
};

struct Shard {
    SpinLock mutex;
    Entry* slots = nullptr;
    size_t capacity = 0;
    size_t used = 0;
    size_t live = 0;

    static size_t hash(const void* key, size_t mask) {
        uintptr_t k = reinterpret_cast<uintptr_t>(key) >> 4;
        k ^= k >> 17;
        k *= 0x9E3779B97F4A7C15ull;
        return static_cast<size_t>(k >> 20) & mask;
    }

    Entry* find(const void* key) {
        if (!capacity) return nullptr;
        const size_t mask = capacity - 1;
        for (size_t i = hash(key, mask);; i = (i + 1) & mask) {
            Entry& e = slots[i];
            if (e.key == key) return &e;
            if (!e.key) return nullptr;
        }
    }

    void rehash(size_t newCapacity) {
        Entry* old = slots;
        const size_t oldCapacity = capacity;
        slots = static_cast<Entry*>(std::calloc(newCapacity, sizeof(Entry)));
        capacity = newCapacity;
        used = live;
        const size_t mask = capacity - 1;
        for (size_t j = 0; j < oldCapacity; ++j) {
            const Entry& e = old[j];
            if (!e.key || e.key == tombstone) continue;
            size_t i = hash(e.key, mask);
            while (slots[i].key) i = (i + 1) & mask;
            slots[i] = e;
        }
        std::free(old);
    }

    Entry& insert(const void* key) {
        if ((used + 1) * 4 >= capacity * 3) rehash(capacity ? (live * 2 + 2 > capacity ? capacity * 2 : capacity) : 64);
        const size_t mask = capacity - 1;
        size_t i = hash(key, mask);
        size_t reuse = SIZE_MAX;
        for (;; i = (i + 1) & mask) {
            if (slots[i].key == key) return slots[i];
            if (slots[i].key == tombstone && reuse == SIZE_MAX) reuse = i;
            if (!slots[i].key) break;
        }
        if (reuse != SIZE_MAX) i = reuse;
        else ++used;
        ++live;
        slots[i] = Entry{ key, 0, nullptr };
        return slots[i];
    }

    void erase(Entry* e) {
        e->key = tombstone;
        e->destructor = nullptr;
        --live;
    }
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
    std::lock_guard<csec_rc::SpinLock> lock(shard.mutex);
    return shard.find(block) != nullptr;
}

extern "C" CSEC_NATIVE_API void csec_rc_adopt(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<csec_rc::SpinLock> lock(shard.mutex);
    csec_rc::Entry& e = shard.insert(block);
    e.count = 1;
    e.destructor = nullptr;
}

extern "C" CSEC_NATIVE_API void csec_rc_adopt_with(const void* block, void* destructor) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<csec_rc::SpinLock> lock(shard.mutex);
    csec_rc::Entry& e = shard.insert(block);
    e.count = 1;
    e.destructor = reinterpret_cast<void (*)(void*)>(destructor);
}

extern "C" CSEC_NATIVE_API void csec_rc_retain(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    std::lock_guard<csec_rc::SpinLock> lock(shard.mutex);
    if (csec_rc::Entry* e = shard.find(block)) ++e->count;
}

extern "C" CSEC_NATIVE_API void csec_rc_release(const void* block) {
    if (!block) return;
    csec_rc::Shard& shard = csec_rc::shardFor(block);
    void (*destructor)(void*) = nullptr;
    {
        std::lock_guard<csec_rc::SpinLock> lock(shard.mutex);
        csec_rc::Entry* e = shard.find(block);
        if (!e || --e->count > 0) return;
        destructor = e->destructor;
        shard.erase(e);
    }
    if (destructor) destructor(const_cast<void*>(block));
    csecFree(const_cast<void*>(block));
}

namespace csec_array {
struct Table {
    std::mutex mutex;
    std::unordered_map<const void*, long long> counts;
};
Table& table() {
    static Table* instance = new Table();
    return *instance;
}
}

extern "C" CSEC_NATIVE_API void csec_array_track(const void* block, long long count) {
    if (!block) return;
    csec_array::Table& t = csec_array::table();
    std::lock_guard<std::mutex> lock(t.mutex);
    if (count > 0) t.counts[block] = count;
    else t.counts.erase(block);
}

extern "C" CSEC_NATIVE_API long long csec_array_untrack(const void* block) {
    if (!block) return 0;
    csec_array::Table& t = csec_array::table();
    std::lock_guard<std::mutex> lock(t.mutex);
    auto found = t.counts.find(block);
    if (found == t.counts.end()) return 0;
    long long count = found->second;
    t.counts.erase(found);
    return count;
}
