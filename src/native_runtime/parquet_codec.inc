// Low-level building blocks for std.dataframe's Parquet support: the Thrift compact protocol (file
// metadata and page headers), page decompression (Snappy, GZIP, LZ4_RAW, and ZSTD when built with
// libzstd), and the Parquet value encodings (PLAIN, RLE/bit-packed hybrid, dictionary indices,
// DELTA_BINARY_PACKED, DELTA_LENGTH_BYTE_ARRAY, DELTA_BYTE_ARRAY, BYTE_STREAM_SPLIT).
//
// Everything here is self-contained (no Arrow, no Thrift library) so the runtime keeps building
// with nothing but a C++17 compiler. Readers never trust sizes from the file: every length is
// checked against the bytes that are actually there.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#if defined(CSEC_HAVE_ZSTD)
#include <zstd.h>
#endif

// <windows.h> defines min/max macros; keep them out of std::min/std::max here and in
// dataframe_runtime.inc, which restores them at its end.
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

namespace csec_parquet {

// ---- Thrift compact protocol -------------------------------------------------------------------

enum CompactType : int {
    kStop = 0,
    kBoolTrue = 1,
    kBoolFalse = 2,
    kByte = 3,
    kI16 = 4,
    kI32 = 5,
    kI64 = 6,
    kDouble = 7,
    kBinary = 8,
    kList = 9,
    kSet = 10,
    kMap = 11,
    kStruct = 12,
};

// A decoded Thrift value. Structs keep their fields as (id, value) pairs; lists, sets, and maps
// (keys and values alternating) keep their elements in `list`.
struct TValue {
    int type = kStop;
    long long i = 0;
    double d = 0.0;
    std::string bin;
    std::vector<TValue> list;
    std::vector<std::pair<int, TValue>> fields;

    const TValue* field(int id) const {
        for (const auto& f : fields) {
            if (f.first == id) return &f.second;
        }
        return nullptr;
    }
    long long intOr(int id, long long fallback) const {
        const TValue* f = field(id);
        return f ? f->i : fallback;
    }
    bool has(int id) const { return field(id) != nullptr; }
    std::string stringOr(int id, const std::string& fallback = std::string()) const {
        const TValue* f = field(id);
        return f ? f->bin : fallback;
    }
};

class ThriftReader {
public:
    ThriftReader(const uint8_t* data, size_t size) : start_(data), p_(data), end_(data + size) {}

    bool ok() const { return ok_; }
    size_t consumed() const { return static_cast<size_t>(p_ - start_); }

    bool readStruct(TValue& out, int depth = 0) {
        if (depth > kMaxDepth) return fail();
        out.type = kStruct;
        int lastId = 0;
        for (;;) {
            const uint8_t header = byte();
            if (!ok_) return false;
            if (header == 0) return true;
            const int type = header & 0x0f;
            const int delta = header >> 4;
            const int id = delta != 0 ? lastId + delta : static_cast<int>(zigzag());
            lastId = id;
            TValue value;
            if (type == kBoolTrue || type == kBoolFalse) {
                value.type = kBoolTrue;
                value.i = type == kBoolTrue ? 1 : 0;
            } else if (!readValue(type, value, depth + 1)) {
                return false;
            }
            out.fields.emplace_back(id, std::move(value));
        }
    }

private:
    static constexpr int kMaxDepth = 64;

    bool fail() {
        ok_ = false;
        return false;
    }
    size_t remaining() const { return static_cast<size_t>(end_ - p_); }
    uint8_t byte() {
        if (p_ >= end_) {
            ok_ = false;
            return 0;
        }
        return *p_++;
    }
    uint64_t varint() {
        uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            const uint8_t b = byte();
            if (!ok_) return 0;
            result |= static_cast<uint64_t>(b & 0x7f) << shift;
            if ((b & 0x80) == 0) return result;
        }
        ok_ = false;
        return 0;
    }
    int64_t zigzag() {
        const uint64_t v = varint();
        return static_cast<int64_t>(v >> 1) ^ -static_cast<int64_t>(v & 1);
    }

    bool readValue(int type, TValue& out, int depth) {
        if (depth > kMaxDepth) return fail();
        out.type = type;
        switch (type) {
        case kBoolTrue:
        case kBoolFalse:
            // Inside a collection a bool is a whole byte (1 = true).
            out.type = kBoolTrue;
            out.i = byte() == kBoolTrue ? 1 : 0;
            return ok_;
        case kByte:
            out.i = static_cast<int8_t>(byte());
            return ok_;
        case kI16:
        case kI32:
        case kI64:
            out.i = zigzag();
            return ok_;
        case kDouble: {
            if (remaining() < 8) return fail();
            uint64_t bits = 0;
            for (int k = 0; k < 8; ++k) bits |= static_cast<uint64_t>(p_[k]) << (8 * k);
            p_ += 8;
            std::memcpy(&out.d, &bits, sizeof bits);
            return true;
        }
        case kBinary: {
            const uint64_t length = varint();
            if (!ok_ || length > remaining()) return fail();
            out.bin.assign(reinterpret_cast<const char*>(p_), static_cast<size_t>(length));
            p_ += length;
            return true;
        }
        case kList:
        case kSet: {
            const uint8_t header = byte();
            if (!ok_) return false;
            uint64_t count = header >> 4;
            const int elementType = header & 0x0f;
            if (count == 15) count = varint();
            if (!ok_ || count > remaining()) return fail();  // every element takes at least one byte
            out.list.resize(static_cast<size_t>(count));
            for (auto& element : out.list) {
                if (!readValue(elementType, element, depth + 1)) return false;
            }
            return true;
        }
        case kMap: {
            const uint64_t count = varint();
            if (!ok_) return false;
            if (count == 0) return true;
            const uint8_t types = byte();
            if (!ok_ || count > remaining()) return fail();
            out.list.resize(static_cast<size_t>(count) * 2);
            for (size_t k = 0; k < out.list.size(); ++k) {
                if (!readValue(k % 2 == 0 ? types >> 4 : types & 0x0f, out.list[k], depth + 1)) return false;
            }
            return true;
        }
        case kStruct:
            return readStruct(out, depth);
        default:
            return fail();
        }
    }

    const uint8_t* start_;
    const uint8_t* p_;
    const uint8_t* end_;
    bool ok_ = true;
};

class ThriftWriter {
public:
    std::string out;

    void i32(int id, long long value) {
        fieldHeader(id, kI32);
        zigzag(value);
    }
    void i64(int id, long long value) {
        fieldHeader(id, kI64);
        zigzag(value);
    }
    void string(int id, const std::string& value) {
        fieldHeader(id, kBinary);
        varint(value.size());
        out += value;
    }
    void beginStruct(int id) {
        fieldHeader(id, kStruct);
        pushStruct();
    }
    void endStruct() {
        out.push_back(0);
        lastId_ = stack_.back();
        stack_.pop_back();
    }
    void beginList(int id, int elementType, size_t count) {
        fieldHeader(id, kList);
        if (count < 15) {
            out.push_back(static_cast<char>((count << 4) | elementType));
        } else {
            out.push_back(static_cast<char>(0xf0 | elementType));
            varint(count);
        }
    }
    // A struct element of a list: fields follow, then endStruct().
    void beginListStruct() { pushStruct(); }
    void listI32(long long value) { zigzag(value); }
    void listString(const std::string& value) {
        varint(value.size());
        out += value;
    }
    // End of the outermost (unnamed) struct.
    void stop() { out.push_back(0); }

private:
    void pushStruct() {
        stack_.push_back(lastId_);
        lastId_ = 0;
    }
    void fieldHeader(int id, int type) {
        const int delta = id - lastId_;
        if (delta > 0 && delta <= 15) {
            out.push_back(static_cast<char>((delta << 4) | type));
        } else {
            out.push_back(static_cast<char>(type));
            zigzag(id);
        }
        lastId_ = id;
    }
    void varint(uint64_t value) {
        while (value >= 0x80) {
            out.push_back(static_cast<char>((value & 0x7f) | 0x80));
            value >>= 7;
        }
        out.push_back(static_cast<char>(value));
    }
    void zigzag(long long value) {
        varint((static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63));
    }

    int lastId_ = 0;
    std::vector<int> stack_;
};

// ---- Decompression -----------------------------------------------------------------------------

bool snappyDecompress(const uint8_t* src, size_t size, std::string& out) {
    size_t pos = 0;
    uint64_t length = 0;
    for (int shift = 0;; shift += 7) {
        if (pos >= size || shift > 35) return false;
        const uint8_t b = src[pos++];
        length |= static_cast<uint64_t>(b & 0x7f) << shift;
        if ((b & 0x80) == 0) break;
    }
    out.clear();
    out.reserve(static_cast<size_t>(std::min<uint64_t>(length, static_cast<uint64_t>(size) * 32)));
    while (pos < size) {
        const uint8_t tag = src[pos++];
        const int kind = tag & 3;
        if (kind == 0) {
            size_t literal = tag >> 2;
            if (literal >= 60) {
                const size_t bytes = literal - 59;
                if (pos + bytes > size) return false;
                literal = 0;
                for (size_t k = 0; k < bytes; ++k) literal |= static_cast<size_t>(src[pos + k]) << (8 * k);
                pos += bytes;
            }
            literal += 1;
            if (literal > size - pos) return false;
            out.append(reinterpret_cast<const char*>(src + pos), literal);
            pos += literal;
            continue;
        }
        size_t copy = 0;
        size_t offset = 0;
        if (kind == 1) {
            if (pos >= size) return false;
            copy = ((tag >> 2) & 7) + 4;
            offset = (static_cast<size_t>(tag >> 5) << 8) | src[pos++];
        } else if (kind == 2) {
            if (pos + 2 > size) return false;
            copy = (tag >> 2) + 1;
            offset = src[pos] | (static_cast<size_t>(src[pos + 1]) << 8);
            pos += 2;
        } else {
            if (pos + 4 > size) return false;
            copy = (tag >> 2) + 1;
            offset = src[pos] | (static_cast<size_t>(src[pos + 1]) << 8) | (static_cast<size_t>(src[pos + 2]) << 16) |
                     (static_cast<size_t>(src[pos + 3]) << 24);
            pos += 4;
        }
        if (offset == 0 || offset > out.size() || out.size() + copy > length) return false;
        const size_t from = out.size() - offset;
        for (size_t k = 0; k < copy; ++k) out.push_back(out[from + k]);
    }
    return out.size() == length;
}

// Raw DEFLATE (RFC 1951), after zlib's "puff" reference decoder.
class Inflater {
public:
    Inflater(const uint8_t* data, size_t size, std::string& out) : in_(data), size_(size), out_(out) {}

    // Decode one DEFLATE stream; `pos()` is then just past it.
    bool run() {
        int last = 0;
        do {
            last = bits(1);
            const int type = bits(2);
            if (error_) return false;
            bool ok = false;
            if (type == 0) ok = stored();
            else if (type == 1) ok = fixed();
            else if (type == 2) ok = dynamic();
            if (!ok || error_) return false;
        } while (!last);
        return true;
    }
    size_t pos() const { return pos_; }

private:
    struct Huffman {
        short count[16];
        short symbol[288];
    };

    int bits(int need) {
        uint32_t value = bitBuffer_;
        while (bitCount_ < need) {
            if (pos_ >= size_) {
                error_ = true;
                return 0;
            }
            value |= static_cast<uint32_t>(in_[pos_++]) << bitCount_;
            bitCount_ += 8;
        }
        bitBuffer_ = value >> need;
        bitCount_ -= need;
        return static_cast<int>(value & ((1u << need) - 1));
    }

    bool stored() {
        bitBuffer_ = 0;
        bitCount_ = 0;
        if (pos_ + 4 > size_) return false;
        const unsigned length = in_[pos_] | (in_[pos_ + 1] << 8);
        const unsigned complement = in_[pos_ + 2] | (in_[pos_ + 3] << 8);
        pos_ += 4;
        if (length != (~complement & 0xffff) || length > size_ - pos_) return false;
        out_.append(reinterpret_cast<const char*>(in_ + pos_), length);
        pos_ += length;
        return true;
    }

    static void construct(Huffman& h, const short* lengths, int n) {
        for (int len = 0; len < 16; ++len) h.count[len] = 0;
        for (int symbol = 0; symbol < n; ++symbol) h.count[lengths[symbol]]++;
        short offsets[16];
        offsets[1] = 0;
        for (int len = 1; len < 15; ++len) offsets[len + 1] = static_cast<short>(offsets[len] + h.count[len]);
        for (int symbol = 0; symbol < n; ++symbol) {
            if (lengths[symbol] != 0) h.symbol[offsets[lengths[symbol]]++] = static_cast<short>(symbol);
        }
    }

    int decode(const Huffman& h) {
        int code = 0;
        int first = 0;
        int index = 0;
        for (int len = 1; len < 16; ++len) {
            code |= bits(1);
            if (error_) return -1;
            const int count = h.count[len];
            if (code - count < first) return h.symbol[index + (code - first)];
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }

    bool codes(const Huffman& lengthCode, const Huffman& distanceCode) {
        static const short lengthBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                             31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
        static const short lengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                              2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
        static const int distanceBase[30] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                                             193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
        static const short distanceExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
                                                6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
        for (;;) {
            int symbol = decode(lengthCode);
            if (symbol < 0) return false;
            if (symbol < 256) {
                out_.push_back(static_cast<char>(symbol));
                continue;
            }
            if (symbol == 256) return true;
            symbol -= 257;
            if (symbol >= 29) return false;
            const size_t length = lengthBase[symbol] + bits(lengthExtra[symbol]);
            symbol = decode(distanceCode);
            if (symbol < 0 || symbol >= 30) return false;
            const size_t distance = distanceBase[symbol] + bits(distanceExtra[symbol]);
            if (error_ || distance > out_.size()) return false;
            const size_t from = out_.size() - distance;
            for (size_t k = 0; k < length; ++k) out_.push_back(out_[from + k]);
        }
    }

    bool fixed() {
        static Huffman lengthCode;
        static Huffman distanceCode;
        static const bool built = [] {
            short lengths[288];
            int symbol = 0;
            for (; symbol < 144; ++symbol) lengths[symbol] = 8;
            for (; symbol < 256; ++symbol) lengths[symbol] = 9;
            for (; symbol < 280; ++symbol) lengths[symbol] = 7;
            for (; symbol < 288; ++symbol) lengths[symbol] = 8;
            construct(lengthCode, lengths, 288);
            for (symbol = 0; symbol < 30; ++symbol) lengths[symbol] = 5;
            construct(distanceCode, lengths, 30);
            return true;
        }();
        (void)built;
        return codes(lengthCode, distanceCode);
    }

    bool dynamic() {
        static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        const int lengthCount = bits(5) + 257;
        const int distanceCount = bits(5) + 1;
        const int codeCount = bits(4) + 4;
        if (error_ || lengthCount > 286 || distanceCount > 30) return false;
        short lengths[320] = {0};
        for (int k = 0; k < codeCount; ++k) lengths[order[k]] = static_cast<short>(bits(3));
        Huffman lengthCode;
        Huffman distanceCode;
        construct(lengthCode, lengths, 19);
        int index = 0;
        while (index < lengthCount + distanceCount) {
            int symbol = decode(lengthCode);
            if (symbol < 0) return false;
            if (symbol < 16) {
                lengths[index++] = static_cast<short>(symbol);
                continue;
            }
            short value = 0;
            int repeat = 0;
            if (symbol == 16) {
                if (index == 0) return false;
                value = lengths[index - 1];
                repeat = 3 + bits(2);
            } else if (symbol == 17) {
                repeat = 3 + bits(3);
            } else {
                repeat = 11 + bits(7);
            }
            if (error_ || index + repeat > lengthCount + distanceCount) return false;
            while (repeat--) lengths[index++] = value;
        }
        if (lengths[256] == 0) return false;
        construct(lengthCode, lengths, lengthCount);
        construct(distanceCode, lengths + lengthCount, distanceCount);
        return codes(lengthCode, distanceCode);
    }

    const uint8_t* in_;
    size_t size_;
    size_t pos_ = 0;
    uint32_t bitBuffer_ = 0;
    int bitCount_ = 0;
    bool error_ = false;
    std::string& out_;
};

// GZIP (RFC 1952), possibly several concatenated members; a bare zlib stream is accepted too.
bool gzipDecompress(const uint8_t* src, size_t size, std::string& out) {
    out.clear();
    size_t pos = 0;
    if (size >= 2 && (src[0] & 0x0f) == 8 && ((src[0] << 8) | src[1]) % 31 == 0) {
        Inflater inflater(src + 2, size - 2, out);  // zlib wrapper
        return inflater.run();
    }
    while (pos < size) {
        if (size - pos < 18 || src[pos] != 0x1f || src[pos + 1] != 0x8b || src[pos + 2] != 8) return pos > 0;
        const uint8_t flags = src[pos + 3];
        pos += 10;
        if (flags & 4) {
            if (pos + 2 > size) return false;
            pos += 2 + (src[pos] | (src[pos + 1] << 8));
        }
        if (flags & 8) {
            while (pos < size && src[pos] != 0) ++pos;
            ++pos;
        }
        if (flags & 16) {
            while (pos < size && src[pos] != 0) ++pos;
            ++pos;
        }
        if (flags & 2) pos += 2;
        if (pos >= size) return false;
        Inflater inflater(src + pos, size - pos, out);
        if (!inflater.run()) return false;
        pos += inflater.pos() + 8;  // CRC32 and ISIZE
    }
    return true;
}

// LZ4 block format (Parquet's LZ4_RAW codec).
bool lz4Decompress(const uint8_t* src, size_t size, size_t expected, std::string& out) {
    out.clear();
    out.reserve(std::min<uint64_t>(expected, static_cast<uint64_t>(size) * 256));
    size_t pos = 0;
    while (pos < size) {
        const uint8_t token = src[pos++];
        size_t literal = token >> 4;
        if (literal == 15) {
            uint8_t b = 0;
            do {
                if (pos >= size) return false;
                b = src[pos++];
                literal += b;
            } while (b == 255);
        }
        if (literal > size - pos) return false;
        out.append(reinterpret_cast<const char*>(src + pos), literal);
        pos += literal;
        if (pos >= size) break;  // the last sequence has literals only
        if (pos + 2 > size) return false;
        const size_t offset = src[pos] | (static_cast<size_t>(src[pos + 1]) << 8);
        pos += 2;
        size_t match = token & 15;
        if (match == 15) {
            uint8_t b = 0;
            do {
                if (pos >= size) return false;
                b = src[pos++];
                match += b;
            } while (b == 255);
        }
        match += 4;
        if (offset == 0 || offset > out.size() || out.size() + match > expected) return false;
        const size_t from = out.size() - offset;
        for (size_t k = 0; k < match; ++k) out.push_back(out[from + k]);
    }
    return out.size() == expected;
}

enum Codec : int { kUncompressed = 0, kSnappy = 1, kGzip = 2, kLzo = 3, kBrotli = 4, kLz4Hadoop = 5, kZstd = 6, kLz4Raw = 7 };

const char* codecName(int codec) {
    switch (codec) {
    case kUncompressed: return "UNCOMPRESSED";
    case kSnappy: return "SNAPPY";
    case kGzip: return "GZIP";
    case kLzo: return "LZO";
    case kBrotli: return "BROTLI";
    case kLz4Hadoop: return "LZ4";
    case kZstd: return "ZSTD";
    case kLz4Raw: return "LZ4_RAW";
    default: return "unknown";
    }
}

bool zstdAvailable() {
#if defined(CSEC_HAVE_ZSTD)
    return true;
#else
    return false;
#endif
}

// Decompress one page body into `out`. `error` explains a failure.
bool decompress(int codec, const uint8_t* src, size_t size, size_t expected, std::string& out, std::string& error) {
    bool ok = false;
    switch (codec) {
    case kUncompressed:
        out.assign(reinterpret_cast<const char*>(src), size);
        ok = true;
        break;
    case kSnappy:
        ok = snappyDecompress(src, size, out);
        break;
    case kGzip:
        ok = gzipDecompress(src, size, out);
        break;
    case kLz4Raw:
        ok = lz4Decompress(src, size, expected, out);
        break;
    case kZstd:
#if defined(CSEC_HAVE_ZSTD)
        {
            const unsigned long long frameSize = ZSTD_getFrameContentSize(src, size);
            if (frameSize == ZSTD_CONTENTSIZE_ERROR || (frameSize != ZSTD_CONTENTSIZE_UNKNOWN && frameSize != expected)) break;
        }
        out.resize(expected);
        {
            const size_t written = ZSTD_decompress(out.empty() ? nullptr : &out[0], expected, src, size);
            ok = !ZSTD_isError(written);
            if (ok) out.resize(written);
        }
        break;
#else
        error = "ZSTD-compressed Parquet needs a runtime built with libzstd (CSEC_WITH_ZSTD)";
        return false;
#endif
    default:
        error = std::string("unsupported Parquet compression codec ") + codecName(codec);
        return false;
    }
    if (!ok) error = std::string("corrupt ") + codecName(codec) + " page";
    else if (out.size() != expected) {
        error = std::string(codecName(codec)) + " page decompressed to an unexpected size";
        ok = false;
    }
    return ok;
}

// ---- Encodings ---------------------------------------------------------------------------------

enum Encoding : int {
    kPlain = 0,
    kPlainDictionary = 2,
    kRle = 3,
    kBitPacked = 4,
    kDeltaBinaryPacked = 5,
    kDeltaLengthByteArray = 6,
    kDeltaByteArray = 7,
    kRleDictionary = 8,
    kByteStreamSplit = 9,
};

enum PhysicalType : int { kBoolean = 0, kInt32 = 1, kInt64 = 2, kInt96 = 3, kFloat = 4, kDoubleType = 5, kByteArray = 6, kFixedLenByteArray = 7 };

// Decoded values of one page (or a dictionary): integers, floating point, or byte strings,
// depending on the physical type.
struct Values {
    std::vector<long long> ints;
    std::vector<double> doubles;
    std::vector<std::string> bytes;

    size_t size(int physical) const {
        switch (physical) {
        case kFloat:
        case kDoubleType: return doubles.size();
        case kByteArray:
        case kFixedLenByteArray: return bytes.size();
        default: return ints.size();
        }
    }
};

inline uint32_t le32(const uint8_t* p) {
    return p[0] | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
inline uint64_t le64(const uint8_t* p) {
    return le32(p) | (static_cast<uint64_t>(le32(p + 4)) << 32);
}

// Read `width` bits (<= 64) starting at bit `bit` of `p` (LSB first).
inline uint64_t unpackBits(const uint8_t* p, size_t bit, int width) {
    uint64_t value = 0;
    for (int k = 0; k < width;) {
        const size_t index = (bit + k) >> 3;
        const int shift = static_cast<int>((bit + k) & 7);
        const int take = std::min(8 - shift, width - k);
        value |= static_cast<uint64_t>((p[index] >> shift) & ((1u << take) - 1)) << k;
        k += take;
    }
    return value;
}

inline bool readUleb(const uint8_t* p, size_t size, size_t& pos, uint64_t& value) {
    value = 0;
    for (int shift = 0; shift < 64; shift += 7) {
        if (pos >= size) return false;
        const uint8_t b = p[pos++];
        value |= static_cast<uint64_t>(b & 0x7f) << shift;
        if ((b & 0x80) == 0) return true;
    }
    return false;
}

// RLE / bit-packed hybrid (levels, dictionary indices, RLE booleans). Decodes `count` values.
bool decodeHybrid(const uint8_t* p, size_t size, int width, size_t count, std::vector<uint32_t>& out) {
    out.clear();
    if (width == 0) {
        out.assign(count, 0);
        return true;
    }
    if (width > 32) return false;
    out.reserve(count);
    const size_t byteWidth = (static_cast<size_t>(width) + 7) / 8;
    size_t pos = 0;
    while (out.size() < count) {
        uint64_t header = 0;
        if (!readUleb(p, size, pos, header)) return false;
        if (header & 1) {
            const size_t groups = static_cast<size_t>(header >> 1);
            const size_t bytes = groups * static_cast<size_t>(width);
            const size_t available = std::min(bytes, size - pos);
            const size_t values = std::min(groups * 8, available * 8 / static_cast<size_t>(width));
            for (size_t k = 0; k < values && out.size() < count; ++k) {
                out.push_back(static_cast<uint32_t>(unpackBits(p + pos, k * width, width)));
            }
            pos += available;
            if (available < bytes && out.size() < count) return false;
        } else {
            const size_t run = static_cast<size_t>(header >> 1);
            if (pos + byteWidth > size) return false;
            uint32_t value = 0;
            for (size_t k = 0; k < byteWidth; ++k) value |= static_cast<uint32_t>(p[pos + k]) << (8 * k);
            pos += byteWidth;
            for (size_t k = 0; k < run && out.size() < count; ++k) out.push_back(value);
            if (run == 0) return false;
        }
    }
    return true;
}

// DELTA_BINARY_PACKED: decodes all values in the block stream and reports the bytes consumed.
bool decodeDeltaBinary(const uint8_t* p, size_t size, std::vector<long long>& out, size_t& consumed, size_t maxValues) {
    size_t pos = 0;
    uint64_t blockSize = 0, miniblocks = 0, total = 0, firstRaw = 0;
    if (!readUleb(p, size, pos, blockSize) || !readUleb(p, size, pos, miniblocks) || !readUleb(p, size, pos, total) ||
        !readUleb(p, size, pos, firstRaw)) {
        return false;
    }
    if (blockSize == 0 || blockSize % 128 != 0 || blockSize > (uint64_t(1) << 20) || miniblocks == 0 ||
        blockSize % miniblocks != 0 || (blockSize / miniblocks) % 32 != 0 || total > maxValues) {
        return false;
    }
    const size_t perMiniblock = static_cast<size_t>(blockSize / miniblocks);
    out.clear();
    out.reserve(static_cast<size_t>(total));
    uint64_t value = (firstRaw >> 1) ^ (~(firstRaw & 1) + 1);
    if (total > 0) out.push_back(static_cast<long long>(value));
    while (out.size() < total) {
        uint64_t minRaw = 0;
        if (!readUleb(p, size, pos, minRaw)) return false;
        const uint64_t minDelta = (minRaw >> 1) ^ (~(minRaw & 1) + 1);
        if (miniblocks > size - pos) return false;
        const uint8_t* widths = p + pos;
        pos += static_cast<size_t>(miniblocks);
        for (size_t m = 0; m < miniblocks && out.size() < total; ++m) {
            const int width = widths[m];
            if (width > 64) return false;
            const size_t bytes = perMiniblock * width / 8;
            if (bytes > size - pos) {
                // The final miniblock may be truncated to the values that remain.
                const size_t remaining = std::min<size_t>(static_cast<size_t>(total) - out.size(), perMiniblock);
                const size_t need = (remaining * width + 7) / 8;
                if (need > size - pos) return false;
            }
            for (size_t k = 0; k < perMiniblock && out.size() < total; ++k) {
                const uint64_t delta = width == 0 ? 0 : unpackBits(p + pos, k * width, width);
                value += minDelta + delta;
                out.push_back(static_cast<long long>(value));
            }
            pos += std::min(bytes, size - pos);
        }
    }
    consumed = pos;
    return true;
}

bool decodeDeltaLengthByteArray(const uint8_t* p, size_t size, std::vector<std::string>& out, size_t& consumed, size_t maxValues) {
    std::vector<long long> lengths;
    size_t pos = 0;
    if (!decodeDeltaBinary(p, size, lengths, pos, maxValues)) return false;
    out.clear();
    out.reserve(lengths.size());
    for (long long length : lengths) {
        if (length < 0 || static_cast<size_t>(length) > size - pos) return false;
        out.emplace_back(reinterpret_cast<const char*>(p + pos), static_cast<size_t>(length));
        pos += static_cast<size_t>(length);
    }
    consumed = pos;
    return true;
}

bool decodeDeltaByteArray(const uint8_t* p, size_t size, std::vector<std::string>& out, size_t maxValues) {
    std::vector<long long> prefixes;
    size_t pos = 0;
    if (!decodeDeltaBinary(p, size, prefixes, pos, maxValues)) return false;
    std::vector<std::string> suffixes;
    size_t used = 0;
    if (!decodeDeltaLengthByteArray(p + pos, size - pos, suffixes, used, maxValues)) return false;
    if (suffixes.size() != prefixes.size()) return false;
    out.clear();
    out.reserve(prefixes.size());
    std::string previous;
    for (size_t k = 0; k < prefixes.size(); ++k) {
        if (prefixes[k] < 0 || static_cast<size_t>(prefixes[k]) > previous.size()) return false;
        std::string value = previous.substr(0, static_cast<size_t>(prefixes[k])) + suffixes[k];
        out.push_back(value);
        previous = std::move(value);
    }
    return true;
}

// Seconds-since-epoch scale of an INT96 timestamp: nanoseconds of the day plus a Julian day.
inline long long int96ToNanos(const uint8_t* p) {
    const long long nanos = static_cast<long long>(le64(p));
    const long long julianDay = static_cast<long long>(le32(p + 8));
    return (julianDay - 2440588LL) * 86400LL * 1000000000LL + nanos;
}

// PLAIN-decode `count` values of `physical` type.
bool decodePlain(const uint8_t* p, size_t size, size_t count, int physical, int typeLength, Values& out, std::string& error) {
    auto truncated = [&] {
        error = "truncated PLAIN-encoded page";
        return false;
    };
    switch (physical) {
    case kBoolean:
        if ((count + 7) / 8 > size) return truncated();
        out.ints.reserve(out.ints.size() + count);
        for (size_t k = 0; k < count; ++k) out.ints.push_back((p[k >> 3] >> (k & 7)) & 1);
        return true;
    case kInt32:
        if (count > size / 4) return truncated();
        out.ints.reserve(out.ints.size() + count);
        for (size_t k = 0; k < count; ++k) out.ints.push_back(static_cast<int32_t>(le32(p + 4 * k)));
        return true;
    case kInt64:
        if (count > size / 8) return truncated();
        out.ints.reserve(out.ints.size() + count);
        for (size_t k = 0; k < count; ++k) out.ints.push_back(static_cast<long long>(le64(p + 8 * k)));
        return true;
    case kInt96:
        if (count > size / 12) return truncated();
        out.ints.reserve(out.ints.size() + count);
        for (size_t k = 0; k < count; ++k) out.ints.push_back(int96ToNanos(p + 12 * k));
        return true;
    case kFloat:
        if (count > size / 4) return truncated();
        out.doubles.reserve(out.doubles.size() + count);
        for (size_t k = 0; k < count; ++k) {
            const uint32_t bits = le32(p + 4 * k);
            float f;
            std::memcpy(&f, &bits, sizeof f);
            out.doubles.push_back(f);
        }
        return true;
    case kDoubleType:
        if (count > size / 8) return truncated();
        out.doubles.reserve(out.doubles.size() + count);
        for (size_t k = 0; k < count; ++k) {
            const uint64_t bits = le64(p + 8 * k);
            double d;
            std::memcpy(&d, &bits, sizeof d);
            out.doubles.push_back(d);
        }
        return true;
    case kByteArray: {
        size_t pos = 0;
        out.bytes.reserve(out.bytes.size() + count);
        for (size_t k = 0; k < count; ++k) {
            if (pos + 4 > size) return truncated();
            const size_t length = le32(p + pos);
            pos += 4;
            if (length > size - pos) return truncated();
            out.bytes.emplace_back(reinterpret_cast<const char*>(p + pos), length);
            pos += length;
        }
        return true;
    }
    case kFixedLenByteArray: {
        if (typeLength <= 0) {
            error = "FIXED_LEN_BYTE_ARRAY column without a type length";
            return false;
        }
        const size_t width = static_cast<size_t>(typeLength);
        if (count > size / width) return truncated();
        out.bytes.reserve(out.bytes.size() + count);
        for (size_t k = 0; k < count; ++k) out.bytes.emplace_back(reinterpret_cast<const char*>(p + k * width), width);
        return true;
    }
    default:
        error = "unknown Parquet physical type";
        return false;
    }
}

// BYTE_STREAM_SPLIT: byte k of value i is at p[k * count + i].
bool decodeByteStreamSplit(const uint8_t* p, size_t size, size_t count, int physical, int typeLength, Values& out, std::string& error) {
    size_t width = 0;
    switch (physical) {
    case kInt32:
    case kFloat: width = 4; break;
    case kInt64:
    case kDoubleType: width = 8; break;
    case kFixedLenByteArray: width = typeLength > 0 ? static_cast<size_t>(typeLength) : 0; break;
    default: break;
    }
    if (width == 0) {
        error = "BYTE_STREAM_SPLIT on an unsupported type";
        return false;
    }
    if (count > size / width) {
        error = "truncated BYTE_STREAM_SPLIT page";
        return false;
    }
    std::string joined(count * width, '\0');
    for (size_t i = 0; i < count; ++i) {
        for (size_t k = 0; k < width; ++k) joined[i * width + k] = static_cast<char>(p[k * count + i]);
    }
    return decodePlain(reinterpret_cast<const uint8_t*>(joined.data()), joined.size(), count, physical, typeLength, out, error);
}

inline int bitWidth(uint64_t maxValue) {
    int width = 0;
    while (maxValue > 0) {
        ++width;
        maxValue >>= 1;
    }
    return width;
}

// RLE-encode levels (all runs; valid RLE/bit-packed hybrid) for the writer.
std::string encodeLevelsRle(const std::vector<uint8_t>& levels) {
    std::string out;
    size_t k = 0;
    while (k < levels.size()) {
        size_t run = 1;
        while (k + run < levels.size() && levels[k + run] == levels[k]) ++run;
        uint64_t header = static_cast<uint64_t>(run) << 1;
        while (header >= 0x80) {
            out.push_back(static_cast<char>((header & 0x7f) | 0x80));
            header >>= 7;
        }
        out.push_back(static_cast<char>(header));
        out.push_back(static_cast<char>(levels[k]));
        k += run;
    }
    return out;
}

} // namespace csec_parquet
