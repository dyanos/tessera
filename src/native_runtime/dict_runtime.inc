// Dictionaries: the runtime behind the `{key: value, ...}` literal and the `Dict<K, V>` type.
//
// A dictionary is an opaque heap object handed to compiled code as a pointer. Keys and values cross
// the boundary as 64-bit payloads: integers are sign-extended, Bool/Char are zero-extended,
// Float/Double travel as the bit pattern of a double, and Strings, nested dictionaries, arrays and
// object references travel as their pointer. The compiler tells the dictionary the kind of its keys
// and values when it is created, so lookups can hash String keys by content and toString() can
// print the entries. Entries keep insertion order, like Python's dict.
//
// A set (`{1, 2, 3}`, `Set<T>`) is the same object with `isSet` set: only the keys are used, and it
// prints as `{1, 2, 3}` (or `set()` when empty).

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace csec_dict {

// Kinds shared with the compiler (src/dict_support.cpp).
enum Kind : int {
    KindInt = 0,     // signed integer (Byte/Short/Int/Long)
    KindDouble = 1,  // Float or Double, as the bits of a double
    KindBool = 2,
    KindString = 3,
    KindChar = 4,
    KindDict = 5,    // a nested dictionary
    KindArray = 6,   // an array pointer; see Dict::elementKind/elementBytes
    KindOpaque = 7,  // any other reference (class instance, tensor, closure, ...)
};

struct Entry {
    long long key = 0;
    long long value = 0;
    long long length = -1;  // element count for an array value, when known
    bool live = true;
};

struct Dict {
    int keyKind = KindInt;
    int valueKind = KindInt;
    int elementKind = KindInt;  // for KindArray values
    int elementBytes = 8;
    bool isSet = false;
    std::vector<Entry> entries;
    size_t dead = 0;
    std::unordered_map<long long, size_t> intIndex;
    std::unordered_map<std::string, size_t> textIndex;

    bool textKeys() const { return keyKind == KindString; }

    static std::string text(long long key) {
        const char* p = reinterpret_cast<const char*>(static_cast<intptr_t>(key));
        return p ? std::string(p) : std::string();
    }

    long long find(long long key) const {
        if (textKeys()) {
            auto it = textIndex.find(text(key));
            return it == textIndex.end() ? -1 : static_cast<long long>(it->second);
        }
        auto it = intIndex.find(key);
        return it == intIndex.end() ? -1 : static_cast<long long>(it->second);
    }

    void compact() {
        if (dead == 0) return;
        std::vector<Entry> kept;
        kept.reserve(entries.size() - dead);
        for (const auto& entry : entries) {
            if (entry.live) kept.push_back(entry);
        }
        entries.swap(kept);
        dead = 0;
        intIndex.clear();
        textIndex.clear();
        for (size_t i = 0; i < entries.size(); ++i) {
            if (textKeys()) textIndex[text(entries[i].key)] = i;
            else intIndex[entries[i].key] = i;
        }
    }

    void set(long long key, long long value, long long length) {
        long long at = find(key);
        if (at >= 0) {
            entries[static_cast<size_t>(at)].value = value;
            entries[static_cast<size_t>(at)].length = length;
            return;
        }
        Entry entry;
        entry.key = key;
        entry.value = value;
        entry.length = length;
        if (textKeys()) {
            // Keep a private copy of the key text so a key built at runtime stays valid.
            const std::string copy = text(key);
            char* owned = static_cast<char*>(std::malloc(copy.size() + 1));
            if (owned) {
                std::memcpy(owned, copy.c_str(), copy.size() + 1);
                entry.key = static_cast<long long>(reinterpret_cast<intptr_t>(owned));
            }
            textIndex[copy] = entries.size();
        }
        else {
            intIndex[key] = entries.size();
        }
        entries.push_back(entry);
    }

    bool remove(long long key) {
        long long at = find(key);
        if (at < 0) return false;
        Entry& entry = entries[static_cast<size_t>(at)];
        if (textKeys()) {
            textIndex.erase(text(entry.key));
        }
        else {
            intIndex.erase(entry.key);
        }
        entry.live = false;
        ++dead;
        if (dead > 16 && dead * 2 > entries.size()) compact();
        return true;
    }

    size_t size() const { return entries.size() - dead; }

    const Entry* at(long long index) {
        compact();
        if (index < 0 || static_cast<size_t>(index) >= entries.size()) return nullptr;
        return &entries[static_cast<size_t>(index)];
    }
};

Dict* asDict(void* handle) { return static_cast<Dict*>(handle); }

double bitsToDouble(long long bits) {
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void appendQuoted(std::string& out, const char* text) {
    out += '\'';
    for (const char* p = text ? text : ""; *p; ++p) {
        if (*p == '\'' || *p == '\\') out += '\\';
        if (*p == '\n') { out += "\\n"; continue; }
        if (*p == '\t') { out += "\\t"; continue; }
        out += *p;
    }
    out += '\'';
}

void appendDouble(std::string& out, double value) {
    // The shortest text that reads back as the same double (what std::to_chars produces; written
    // with snprintf because Apple's libc++ only has floating-point to_chars from macOS 13.3).
    char buffer[64];
    for (int precision = 1; precision <= 17; ++precision) {
        std::snprintf(buffer, sizeof(buffer), "%.*g", precision, value);
        if (std::strtod(buffer, nullptr) == value) break;
    }
    std::string text(buffer);
    // Python prints whole floats with a trailing ".0".
    if (text.find_first_of(".eEni") == std::string::npos) text += ".0";
    out += text;
}

void appendDict(std::string& out, Dict* dict, int depth);

void appendScalar(std::string& out, int kind, long long bits, int depth) {
    switch (kind) {
        case KindInt: out += std::to_string(bits); break;
        case KindDouble: appendDouble(out, bitsToDouble(bits)); break;
        case KindBool: out += (bits & 1) ? "True" : "False"; break;
        case KindString: appendQuoted(out, reinterpret_cast<const char*>(static_cast<intptr_t>(bits))); break;
        case KindChar: {
            char text[2] = { static_cast<char>(bits & 0xFF), 0 };
            appendQuoted(out, text);
            break;
        }
        case KindDict: appendDict(out, asDict(reinterpret_cast<void*>(static_cast<intptr_t>(bits))), depth + 1); break;
        default: {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "<object 0x%llx>", static_cast<unsigned long long>(bits));
            out += buffer;
            break;
        }
    }
}

long long loadElement(const unsigned char* base, long long index, int kind, int bytes) {
    const unsigned char* p = base + index * bytes;
    switch (kind) {
        case KindDouble:
            if (bytes == 4) {
                float f = 0.0f;
                std::memcpy(&f, p, sizeof(f));
                double d = f;
                long long bits = 0;
                std::memcpy(&bits, &d, sizeof(bits));
                return bits;
            }
            else {
                long long bits = 0;
                std::memcpy(&bits, p, sizeof(bits));
                return bits;
            }
        case KindInt:
            if (bytes == 1) return static_cast<signed char>(*p);
            if (bytes == 2) { int16_t v; std::memcpy(&v, p, 2); return v; }
            if (bytes == 4) { int32_t v; std::memcpy(&v, p, 4); return v; }
            { long long v; std::memcpy(&v, p, 8); return v; }
        case KindBool:
        case KindChar:
            return *p;
        default: {
            intptr_t v = 0;
            std::memcpy(&v, p, sizeof(v));
            return static_cast<long long>(v);
        }
    }
}

void appendValue(std::string& out, Dict* dict, const Entry& entry, int depth) {
    if (dict->valueKind != KindArray) {
        appendScalar(out, dict->valueKind, entry.value, depth);
        return;
    }
    const unsigned char* base = reinterpret_cast<const unsigned char*>(static_cast<intptr_t>(entry.value));
    if (!base || entry.length < 0 || dict->elementKind == KindArray) {
        out += "[...]";
        return;
    }
    out += '[';
    for (long long i = 0; i < entry.length; ++i) {
        if (i > 0) out += ", ";
        appendScalar(out, dict->elementKind, loadElement(base, i, dict->elementKind, dict->elementBytes), depth);
    }
    out += ']';
}

void appendDict(std::string& out, Dict* dict, int depth) {
    if (!dict) { out += "None"; return; }
    if (depth > 64) { out += "{...}"; return; }
    dict->compact();
    if (dict->isSet) {
        if (dict->entries.empty()) { out += "set()"; return; }
        out += '{';
        bool firstElement = true;
        for (const auto& entry : dict->entries) {
            if (!firstElement) out += ", ";
            firstElement = false;
            appendScalar(out, dict->keyKind, entry.key, depth);
        }
        out += '}';
        return;
    }
    out += '{';
    bool first = true;
    for (const auto& entry : dict->entries) {
        if (!first) out += ", ";
        first = false;
        appendScalar(out, dict->keyKind, entry.key, depth);
        out += ": ";
        appendValue(out, dict, entry, depth);
    }
    out += '}';
}

[[noreturn]] void keyError(Dict* dict, long long key) {
    std::string text;
    appendScalar(text, dict ? dict->keyKind : KindInt, key, 0);
    std::fflush(stdout);
    std::fprintf(stderr, "KeyError: %s\n", text.c_str());
    std::fflush(stderr);
    std::exit(1);
}

} // namespace csec_dict

extern "C" {

void* csec_set_new(int elementKind);

void* csec_dict_new(int keyKind, int valueKind, int elementKind, int elementBytes) {
    auto* dict = new csec_dict::Dict();
    dict->keyKind = keyKind;
    dict->valueKind = valueKind;
    dict->elementKind = elementKind;
    dict->elementBytes = elementBytes > 0 ? elementBytes : 8;
    return dict;
}

void csec_dict_free(void* handle) {
    auto* dict = csec_dict::asDict(handle);
    if (!dict) return;
    if (dict->textKeys()) {
        for (const auto& entry : dict->entries) {
            std::free(reinterpret_cast<void*>(static_cast<intptr_t>(entry.key)));
        }
    }
    delete dict;
}

void csec_dict_set(void* handle, long long key, long long value) {
    if (auto* dict = csec_dict::asDict(handle)) dict->set(key, value, -1);
}

void csec_dict_set_array(void* handle, long long key, long long value, long long length) {
    if (auto* dict = csec_dict::asDict(handle)) dict->set(key, value, length);
}

long long csec_dict_get(void* handle, long long key) {
    auto* dict = csec_dict::asDict(handle);
    long long at = dict ? dict->find(key) : -1;
    if (at < 0) csec_dict::keyError(dict, key);
    return dict->entries[static_cast<size_t>(at)].value;
}

long long csec_dict_get_or(void* handle, long long key, long long fallback) {
    auto* dict = csec_dict::asDict(handle);
    long long at = dict ? dict->find(key) : -1;
    return at < 0 ? fallback : dict->entries[static_cast<size_t>(at)].value;
}

int csec_dict_contains(void* handle, long long key) {
    auto* dict = csec_dict::asDict(handle);
    return dict && dict->find(key) >= 0 ? 1 : 0;
}

int csec_dict_remove(void* handle, long long key) {
    auto* dict = csec_dict::asDict(handle);
    return dict && dict->remove(key) ? 1 : 0;
}

long long csec_dict_size(void* handle) {
    auto* dict = csec_dict::asDict(handle);
    return dict ? static_cast<long long>(dict->size()) : 0;
}

long long csec_dict_key_at(void* handle, long long index) {
    auto* dict = csec_dict::asDict(handle);
    const csec_dict::Entry* entry = dict ? dict->at(index) : nullptr;
    if (!entry) {
        std::fflush(stdout);
        std::fprintf(stderr, "IndexError: dictionary index %lld out of range\n", index);
        std::exit(1);
    }
    return entry->key;
}

long long csec_dict_value_at(void* handle, long long index) {
    auto* dict = csec_dict::asDict(handle);
    const csec_dict::Entry* entry = dict ? dict->at(index) : nullptr;
    if (!entry) {
        std::fflush(stdout);
        std::fprintf(stderr, "IndexError: dictionary index %lld out of range\n", index);
        std::exit(1);
    }
    return entry->value;
}

void csec_dict_clear(void* handle) {
    auto* dict = csec_dict::asDict(handle);
    if (!dict) return;
    if (dict->textKeys()) {
        for (const auto& entry : dict->entries) {
            std::free(reinterpret_cast<void*>(static_cast<intptr_t>(entry.key)));
        }
    }
    dict->entries.clear();
    dict->dead = 0;
    dict->intIndex.clear();
    dict->textIndex.clear();
}

void* csec_dict_copy(void* handle) {
    auto* dict = csec_dict::asDict(handle);
    auto* copy = static_cast<csec_dict::Dict*>(csec_dict_new(
        dict ? dict->keyKind : 0, dict ? dict->valueKind : 0,
        dict ? dict->elementKind : 0, dict ? dict->elementBytes : 8));
    if (!dict) return copy;
    dict->compact();
    copy->isSet = dict->isSet;
    for (const auto& entry : dict->entries) copy->set(entry.key, entry.value, entry.length);
    return copy;
}

// The keys of a dictionary, as a new set.
void* csec_dict_keys(void* handle) {
    auto* dict = csec_dict::asDict(handle);
    auto* keys = static_cast<csec_dict::Dict*>(csec_set_new(dict ? dict->keyKind : 0));
    if (!dict) return keys;
    dict->compact();
    for (const auto& entry : dict->entries) keys->set(entry.key, 0, -1);
    return keys;
}

// ---- sets ----

void* csec_set_new(int elementKind) {
    auto* set = static_cast<csec_dict::Dict*>(csec_dict_new(elementKind, csec_dict::KindInt, 0, 8));
    set->isSet = true;
    return set;
}

int csec_set_add(void* handle, long long element) {
    auto* set = csec_dict::asDict(handle);
    if (!set || set->find(element) >= 0) return 0;
    set->set(element, 0, -1);
    return 1;
}

static csec_dict::Dict* csec_set_combine(void* left, void* right, bool keepLeftOnly, bool keepBoth, bool keepRightOnly) {
    auto* a = csec_dict::asDict(left);
    auto* b = csec_dict::asDict(right);
    auto* result = static_cast<csec_dict::Dict*>(csec_set_new(a ? a->keyKind : (b ? b->keyKind : 0)));
    if (a) {
        a->compact();
        for (const auto& entry : a->entries) {
            const bool inB = b && b->find(entry.key) >= 0;
            if ((inB && keepBoth) || (!inB && keepLeftOnly)) result->set(entry.key, 0, -1);
        }
    }
    if (b && keepRightOnly) {
        b->compact();
        for (const auto& entry : b->entries) {
            if (!(a && a->find(entry.key) >= 0)) result->set(entry.key, 0, -1);
        }
    }
    return result;
}

void* csec_set_union(void* left, void* right) { return csec_set_combine(left, right, true, true, true); }
void* csec_set_intersection(void* left, void* right) { return csec_set_combine(left, right, false, true, false); }
void* csec_set_difference(void* left, void* right) { return csec_set_combine(left, right, true, false, false); }
void* csec_set_symmetric_difference(void* left, void* right) { return csec_set_combine(left, right, true, false, true); }

// 1 when every element of `left` is in `right`.
int csec_set_is_subset(void* left, void* right) {
    auto* a = csec_dict::asDict(left);
    auto* b = csec_dict::asDict(right);
    if (!a) return 1;
    a->compact();
    for (const auto& entry : a->entries) {
        if (!b || b->find(entry.key) < 0) return 0;
    }
    return 1;
}

int csec_set_equals(void* left, void* right) {
    auto* a = csec_dict::asDict(left);
    auto* b = csec_dict::asDict(right);
    const size_t sizeA = a ? a->size() : 0;
    const size_t sizeB = b ? b->size() : 0;
    return sizeA == sizeB && csec_set_is_subset(left, right) ? 1 : 0;
}

// 1 when the sets share no element.
int csec_set_is_disjoint(void* left, void* right) {
    auto* a = csec_dict::asDict(left);
    auto* b = csec_dict::asDict(right);
    if (!a || !b) return 1;
    a->compact();
    for (const auto& entry : a->entries) {
        if (b->find(entry.key) >= 0) return 0;
    }
    return 1;
}

char* csec_dict_to_string(void* handle) {
    std::string out;
    csec_dict::appendDict(out, csec_dict::asDict(handle), 0);
    char* result = static_cast<char*>(std::malloc(out.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, out.c_str(), out.size() + 1);
    return result;
}

} // extern "C"
