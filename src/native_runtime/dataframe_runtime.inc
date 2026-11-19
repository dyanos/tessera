// std.dataframe: columnar data frames, Parquet and CSV input/output, and the usual frame operations
// (select, filter, sort, group-by aggregation, join, computed columns, concat).
//
// A frame is an ordered set of named, typed columns of equal length. Column types are Long, Double,
// and String; every value may also be null. Frames are addressed through 64-bit handles, like the
// other runtime objects. Operations never modify their input: each one returns a new frame (0 on
// failure, with the reason in csec_df_last_error). Frames under construction (csec_df_add_column /
// csec_df_append_*) must not be modified from two threads at once; finished frames may be read from
// any number of threads.
//
// Parquet: flat schemas (nested and repeated columns are reported, and can be skipped by selecting
// columns), data page v1 and v2, the PLAIN / dictionary / RLE / DELTA_* / BYTE_STREAM_SPLIT encodings,
// and UNCOMPRESSED, SNAPPY, GZIP, LZ4_RAW, and (when built with libzstd) ZSTD pages. Columns are
// decoded in parallel on several OS threads. INT32/INT64/BOOLEAN/INT96 become Long (INT96 as
// nanoseconds since the epoch, DATE as days, TIMESTAMP in the file's unit), FLOAT/DOUBLE/DECIMAL/
// FLOAT16 become Double, BYTE_ARRAY/FIXED_LEN_BYTE_ARRAY become String. The writer produces
// uncompressed PLAIN files with OPTIONAL INT64/DOUBLE/BYTE_ARRAY(UTF8) columns.

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <exception>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace csec_df {

enum class ColType { Long, Double, String, Bool };

const char* typeName(ColType type) {
    switch (type) {
    case ColType::Long: return "long";
    case ColType::Bool: return "bool";
    case ColType::Double: return "double";
    default: return "string";
    }
}

struct Column {
    std::string name;
    ColType type = ColType::Long;
    std::vector<long long> longs;
    std::vector<double> doubles;
    std::vector<std::string> strings;
    std::vector<unsigned char> valid;  // 1 = a value is present

    Column() = default;
    Column(std::string columnName, ColType columnType) : name(std::move(columnName)), type(columnType) {}

    size_t size() const { return valid.size(); }
    // Bool cells are stored as 0/1 in `longs`, like Long cells.
    bool usesLongs() const { return type == ColType::Long || type == ColType::Bool; }
    bool isNull(size_t row) const { return row >= valid.size() || !valid[row]; }

    void reserve(size_t n) {
        valid.reserve(n);
        if (usesLongs()) longs.reserve(n);
        else if (type == ColType::Double) doubles.reserve(n);
        else strings.reserve(n);
    }
    void pushNull() {
        valid.push_back(0);
        if (usesLongs()) longs.push_back(0);
        else if (type == ColType::Double) doubles.push_back(0.0);
        else strings.emplace_back();
    }
    void pushLong(long long value) {
        valid.push_back(1);
        longs.push_back(value);
    }
    void pushBool(bool value) {
        valid.push_back(1);
        longs.push_back(value ? 1 : 0);
    }
    void pushDouble(double value) {
        valid.push_back(1);
        doubles.push_back(value);
    }
    void pushString(std::string value) {
        valid.push_back(1);
        strings.push_back(std::move(value));
    }
    // Append row `row` of `source`, which has the same type.
    void appendFrom(const Column& source, size_t row) {
        if (source.isNull(row)) {
            pushNull();
            return;
        }
        if (usesLongs()) pushLong(source.longs[row]);
        else if (type == ColType::Double) pushDouble(source.doubles[row]);
        else pushString(source.strings[row]);
    }
    void appendAll(const Column& source) {
        valid.insert(valid.end(), source.valid.begin(), source.valid.end());
        if (usesLongs()) longs.insert(longs.end(), source.longs.begin(), source.longs.end());
        else if (type == ColType::Double) doubles.insert(doubles.end(), source.doubles.begin(), source.doubles.end());
        else strings.insert(strings.end(), source.strings.begin(), source.strings.end());
    }
    bool numeric() const { return type != ColType::String; }
    double number(size_t row) const { return usesLongs() ? static_cast<double>(longs[row]) : doubles[row]; }
    Column emptyLike() const { return Column(name, type); }
};

struct DataFrame {
    std::vector<Column> columns;

    size_t rows() const {
        size_t n = 0;
        for (const auto& c : columns) n = std::max(n, c.size());
        return n;
    }
    // Pad short columns (frames built by appending) with nulls.
    void normalize() {
        const size_t n = rows();
        for (auto& c : columns) {
            while (c.size() < n) c.pushNull();
        }
    }
    int find(const std::string& name) const {
        for (size_t k = 0; k < columns.size(); ++k) {
            if (columns[k].name == name) return static_cast<int>(k);
        }
        return -1;
    }
};

using FramePtr = std::shared_ptr<DataFrame>;

csec_threads::HandleTable<DataFrame>& frames() { return csec_threads::leakedTable<DataFrame>(); }

thread_local std::string lastError;

long long fail(const std::string& message) {
    lastError = message;
    return 0;
}

long long store(DataFrame&& frame) {
    frame.normalize();
    return frames().add(std::make_shared<DataFrame>(std::move(frame)));
}

FramePtr frameOf(long long handle) {
    auto frame = frames().get(handle);
    if (!frame) lastError = "unknown data frame handle";
    else frame->normalize();
    return frame;
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::string();
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> splitList(const char* spec) {
    std::vector<std::string> items;
    if (!spec) return items;
    std::string current;
    for (const char* p = spec;; ++p) {
        if (*p == ',' || *p == '\0') {
            std::string item = trim(current);
            if (!item.empty()) items.push_back(item);
            current.clear();
            if (*p == '\0') break;
        } else {
            current.push_back(*p);
        }
    }
    return items;
}

std::string formatDouble(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "inf" : "-inf";
    char buffer[40];
    std::snprintf(buffer, sizeof buffer, "%.15g", value);
    if (std::strtod(buffer, nullptr) != value) std::snprintf(buffer, sizeof buffer, "%.17g", value);
    return buffer;
}

std::string cellText(const Column& column, size_t row, const char* nullText) {
    if (column.isNull(row)) return nullText;
    if (column.type == ColType::Bool) return column.longs[row] ? "true" : "false";
    if (column.type == ColType::Long) return std::to_string(column.longs[row]);
    if (column.type == ColType::Double) return formatDouble(column.doubles[row]);
    return column.strings[row];
}

// Run `body(k)` for k in [0, count), on several threads when there is enough work (`work` is a rough
// cell count). Each k must touch only its own data.
template <typename Body>
void parallelFor(size_t count, size_t work, Body body) {
    const unsigned hardware = std::max(1u, std::thread::hardware_concurrency());
    const size_t workers = std::min<size_t>(count, std::min<unsigned>(hardware, 16));
    if (workers <= 1 || work < 200000) {
        for (size_t k = 0; k < count; ++k) body(k);
        return;
    }
    std::atomic<size_t> next{0};
    std::mutex failureMutex;
    std::exception_ptr failure;
    auto drain = [&] {
        try {
            for (size_t k = next++; k < count; k = next++) body(k);
        } catch (...) {
            std::lock_guard<std::mutex> lock(failureMutex);
            if (!failure) failure = std::current_exception();
            next = count;
        }
    };
    std::vector<std::thread> pool;
    try {
        pool.reserve(workers);
        for (size_t w = 1; w < workers; ++w) pool.emplace_back(drain);
    } catch (...) {
    }
    drain();
    for (auto& t : pool) t.join();
    if (failure) std::rethrow_exception(failure);
}

// Gather `rows` of one column (tight per-type loops instead of appendFrom per cell).
Column gatherColumn(const Column& column, const std::vector<size_t>& rows) {
    Column copy = column.emptyLike();
    const size_t n = rows.size();
    const size_t have = column.size();
    copy.valid.resize(n);
    for (size_t k = 0; k < n; ++k) copy.valid[k] = rows[k] < have ? column.valid[rows[k]] : 0;
    if (column.usesLongs()) {
        copy.longs.resize(n);
        for (size_t k = 0; k < n; ++k) copy.longs[k] = rows[k] < have ? column.longs[rows[k]] : 0;
    } else if (column.type == ColType::Double) {
        copy.doubles.resize(n);
        for (size_t k = 0; k < n; ++k) copy.doubles[k] = rows[k] < have ? column.doubles[rows[k]] : 0.0;
    } else {
        copy.strings.reserve(n);
        for (size_t k = 0; k < n; ++k) copy.strings.push_back(rows[k] < have ? column.strings[rows[k]] : std::string());
    }
    return copy;
}

// Rows `rows` of every column; columns are gathered in parallel for large frames.
DataFrame take(const DataFrame& frame, const std::vector<size_t>& rows) {
    DataFrame out;
    out.columns.resize(frame.columns.size());
    parallelFor(frame.columns.size(), rows.size() * frame.columns.size(), [&](size_t c) {
        out.columns[c] = gatherColumn(frame.columns[c], rows);
    });
    return out;
}

// ---- Parquet reading ---------------------------------------------------------------------------

using csec_parquet::TValue;

struct Leaf {
    std::string name;  // dotted path for nested leaves
    int physical = 0;
    int typeLength = 0;
    int maxDef = 0;
    int maxRep = 0;
    bool supported = true;
    bool decimal = false;
    bool float16 = false;
    bool unsignedInt = false;  // UINT_* / INTEGER(signed=false): statistics use unsigned order
    int scale = 0;
    ColType out = ColType::Long;
};

bool collectLeaves(const std::vector<TValue>& schema, size_t& index, const std::string& prefix, int def, int rep, bool nested,
                   std::vector<Leaf>& leaves, int children, int depth) {
    if (depth > 64) return false;
    for (int child = 0; child < children && index < schema.size(); ++child) {
        const TValue& element = schema[index++];
        const int repetition = static_cast<int>(element.intOr(3, 0));  // 0 required, 1 optional, 2 repeated
        const int elementDef = def + (repetition != 0 ? 1 : 0);
        const int elementRep = rep + (repetition == 2 ? 1 : 0);
        const std::string name = prefix.empty() ? element.stringOr(4) : prefix + "." + element.stringOr(4);
        const int grandchildren = static_cast<int>(element.intOr(5, 0));
        if (grandchildren > 0 || !element.has(1)) {
            if (!collectLeaves(schema, index, name, elementDef, elementRep, true, leaves, grandchildren, depth + 1)) return false;
            continue;
        }
        Leaf leaf;
        leaf.name = name;
        leaf.physical = static_cast<int>(element.intOr(1, 0));
        leaf.typeLength = static_cast<int>(element.intOr(2, 0));
        leaf.maxDef = elementDef;
        leaf.maxRep = elementRep;
        leaf.supported = !nested && repetition != 2;
        const TValue* logical = element.field(10);
        const long long converted = element.intOr(6, -1);
        if (converted == 5 || (logical && logical->has(5))) {
            leaf.decimal = true;
            if (element.has(7)) leaf.scale = static_cast<int>(element.intOr(7, 0));
            else if (logical && logical->has(5)) leaf.scale = static_cast<int>(logical->field(5)->intOr(1, 0));
        }
        leaf.float16 = logical && logical->has(15) && leaf.physical == csec_parquet::kFixedLenByteArray && leaf.typeLength == 2;
        leaf.unsignedInt = (converted >= 11 && converted <= 14) ||
                           (logical && logical->has(10) && logical->field(10)->intOr(2, 1) == 0);
        switch (leaf.physical) {
        case csec_parquet::kBoolean: leaf.out = ColType::Bool; break;
        case csec_parquet::kInt32:
        case csec_parquet::kInt64:
        case csec_parquet::kInt96: leaf.out = leaf.decimal ? ColType::Double : ColType::Long; break;
        case csec_parquet::kFloat:
        case csec_parquet::kDoubleType: leaf.out = ColType::Double; break;
        default: leaf.out = (leaf.decimal || leaf.float16) ? ColType::Double : ColType::String; break;
        }
        leaves.push_back(leaf);
    }
    return true;
}

double decimalFromBytes(const std::string& bytes) {
    // Big-endian two's complement. A negative value is -(~x + 1); summing the inverted bytes keeps
    // small magnitudes exact (subtracting 2^(8n) from the unsigned value would round them away).
    if (bytes.empty()) return 0.0;
    const bool negative = (static_cast<unsigned char>(bytes[0]) & 0x80) != 0;
    double magnitude = 0.0;
    for (unsigned char b : bytes) magnitude = magnitude * 256.0 + (negative ? 255 - b : b);
    return negative ? -(magnitude + 1.0) : magnitude;
}

double halfToDouble(const std::string& bytes) {
    if (bytes.size() < 2) return std::numeric_limits<double>::quiet_NaN();
    const unsigned bits = static_cast<unsigned char>(bytes[0]) | (static_cast<unsigned>(static_cast<unsigned char>(bytes[1])) << 8);
    const int sign = (bits >> 15) ? -1 : 1;
    const int exponent = (bits >> 10) & 0x1f;
    const int mantissa = bits & 0x3ff;
    if (exponent == 0) return sign * std::ldexp(mantissa, -24);
    if (exponent == 31) return mantissa ? std::numeric_limits<double>::quiet_NaN() : sign * std::numeric_limits<double>::infinity();
    return sign * std::ldexp(mantissa + 1024, exponent - 25);
}

void appendValue(Column& column, const Leaf& leaf, const csec_parquet::Values& values, size_t index) {
    using namespace csec_parquet;
    if (leaf.out == ColType::Long || leaf.out == ColType::Bool) {
        column.pushLong(values.ints[index]);
    } else if (leaf.out == ColType::String) {
        column.pushString(values.bytes[index]);
    } else if (leaf.physical == kFloat || leaf.physical == kDoubleType) {
        column.pushDouble(values.doubles[index]);
    } else if (leaf.float16) {
        column.pushDouble(halfToDouble(values.bytes[index]));
    } else {
        const double unscaled = (leaf.physical == kByteArray || leaf.physical == kFixedLenByteArray)
                                    ? decimalFromBytes(values.bytes[index])
                                    : static_cast<double>(values.ints[index]);
        column.pushDouble(unscaled / std::pow(10.0, leaf.scale));
    }
}

// Gather dictionary entries by index.
bool gather(const csec_parquet::Values& dictionary, int physical, const std::vector<uint32_t>& indices, csec_parquet::Values& out) {
    const size_t size = dictionary.size(physical);
    for (uint32_t index : indices) {
        if (index >= size) return false;
    }
    switch (physical) {
    case csec_parquet::kFloat:
    case csec_parquet::kDoubleType:
        out.doubles.reserve(indices.size());
        for (uint32_t index : indices) out.doubles.push_back(dictionary.doubles[index]);
        break;
    case csec_parquet::kByteArray:
    case csec_parquet::kFixedLenByteArray:
        out.bytes.reserve(indices.size());
        for (uint32_t index : indices) out.bytes.push_back(dictionary.bytes[index]);
        break;
    default:
        out.ints.reserve(indices.size());
        for (uint32_t index : indices) out.ints.push_back(dictionary.ints[index]);
        break;
    }
    return true;
}

bool decodeValues(int encoding, const uint8_t* p, size_t size, size_t count, const Leaf& leaf, bool haveDictionary,
                  const csec_parquet::Values& dictionary, csec_parquet::Values& values, std::string& error) {
    using namespace csec_parquet;
    switch (encoding) {
    case kPlain:
        return decodePlain(p, size, count, leaf.physical, leaf.typeLength, values, error);
    case kPlainDictionary:
    case kRleDictionary: {
        if (!haveDictionary) {
            error = "dictionary-encoded page without a dictionary page";
            return false;
        }
        if (count == 0) return true;
        if (size < 1) {
            error = "truncated dictionary-encoded page";
            return false;
        }
        std::vector<uint32_t> indices;
        if (!decodeHybrid(p + 1, size - 1, p[0], count, indices) || !gather(dictionary, leaf.physical, indices, values)) {
            error = "corrupt dictionary indices";
            return false;
        }
        return true;
    }
    case kRle: {
        if (leaf.physical != kBoolean || size < 4) {
            error = "RLE value encoding is only supported for BOOLEAN columns";
            return false;
        }
        const size_t length = le32(p);
        std::vector<uint32_t> bits;
        if (length > size - 4 || !decodeHybrid(p + 4, length, 1, count, bits)) {
            error = "corrupt RLE boolean page";
            return false;
        }
        values.ints.assign(bits.begin(), bits.end());
        return true;
    }
    case kDeltaBinaryPacked: {
        size_t used = 0;
        if ((leaf.physical != kInt32 && leaf.physical != kInt64) || !decodeDeltaBinary(p, size, values.ints, used, count)) {
            error = "corrupt DELTA_BINARY_PACKED page";
            return false;
        }
        if (leaf.physical == kInt32) {
            for (auto& v : values.ints) v = static_cast<int32_t>(v);
        }
        return true;
    }
    case kDeltaLengthByteArray: {
        size_t used = 0;
        if (!decodeDeltaLengthByteArray(p, size, values.bytes, used, count)) {
            error = "corrupt DELTA_LENGTH_BYTE_ARRAY page";
            return false;
        }
        return true;
    }
    case kDeltaByteArray:
        if (!decodeDeltaByteArray(p, size, values.bytes, count)) {
            error = "corrupt DELTA_BYTE_ARRAY page";
            return false;
        }
        return true;
    case kByteStreamSplit:
        return decodeByteStreamSplit(p, size, count, leaf.physical, leaf.typeLength, values, error);
    default:
        error = "unsupported Parquet encoding " + std::to_string(encoding);
        return false;
    }
}

// Decode one column chunk and append its rows to `column`.
bool readColumnChunk(const uint8_t* file, size_t fileSize, const TValue& chunk, const Leaf& leaf, Column& column, std::string& error) {
    using namespace csec_parquet;
    const TValue* meta = chunk.field(3);
    if (!meta) {
        error = "column chunk without metadata";
        return false;
    }
    if (chunk.has(1)) {
        error = "column chunks in external files are not supported";
        return false;
    }
    const int codec = static_cast<int>(meta->intOr(4, 0));
    const long long totalValues = meta->intOr(5, 0);
    long long start = meta->intOr(9, -1);
    const long long dictionaryOffset = meta->intOr(11, -1);
    if (dictionaryOffset > 0 && dictionaryOffset < start) start = dictionaryOffset;
    if (start < 4 || static_cast<size_t>(start) >= fileSize) {
        error = "column chunk offset outside the file";
        return false;
    }
    size_t pos = static_cast<size_t>(start);
    long long read = 0;
    Values dictionary;
    bool haveDictionary = false;
    std::string page;
    std::vector<uint32_t> definitions;
    const int defWidth = bitWidth(static_cast<uint64_t>(leaf.maxDef));
    while (read < totalValues) {
        if (pos >= fileSize) {
            error = "column chunk runs past the end of the file";
            return false;
        }
        ThriftReader reader(file + pos, fileSize - pos);
        TValue header;
        if (!reader.readStruct(header)) {
            error = "corrupt page header";
            return false;
        }
        pos += reader.consumed();
        const int type = static_cast<int>(header.intOr(1, -1));
        const long long uncompressedSize = header.intOr(2, 0);
        const long long compressedSize = header.intOr(3, 0);
        if (compressedSize < 0 || uncompressedSize < 0 || static_cast<size_t>(compressedSize) > fileSize - pos) {
            error = "page size outside the file";
            return false;
        }
        const uint8_t* body = file + pos;
        const size_t bodySize = static_cast<size_t>(compressedSize);
        pos += bodySize;

        if (type == 2) {  // dictionary page
            const TValue* dictionaryHeader = header.field(7);
            const size_t count = dictionaryHeader ? static_cast<size_t>(dictionaryHeader->intOr(1, 0)) : 0;
            if (!decompress(codec, body, bodySize, static_cast<size_t>(uncompressedSize), page, error)) return false;
            dictionary = Values();
            if (!decodePlain(reinterpret_cast<const uint8_t*>(page.data()), page.size(), count, leaf.physical, leaf.typeLength, dictionary, error)) {
                return false;
            }
            haveDictionary = true;
            continue;
        }
        if (type != 0 && type != 3) continue;  // index pages

        size_t count = 0;
        int encoding = 0;
        const uint8_t* valueData = nullptr;
        size_t valueSize = 0;
        definitions.clear();
        if (type == 0) {
            const TValue* dataHeader = header.field(5);
            if (!dataHeader) {
                error = "data page without a header";
                return false;
            }
            count = static_cast<size_t>(dataHeader->intOr(1, 0));
            encoding = static_cast<int>(dataHeader->intOr(2, 0));
            if (!decompress(codec, body, bodySize, static_cast<size_t>(uncompressedSize), page, error)) return false;
            const uint8_t* data = reinterpret_cast<const uint8_t*>(page.data());
            size_t offset = 0;
            if (leaf.maxDef > 0) {
                if (page.size() < 4) {
                    error = "truncated definition levels";
                    return false;
                }
                const size_t length = le32(data);
                if (length > page.size() - 4 || !decodeHybrid(data + 4, length, defWidth, count, definitions)) {
                    error = "corrupt definition levels";
                    return false;
                }
                offset = 4 + length;
            }
            valueData = data + offset;
            valueSize = page.size() - offset;
        } else {
            const TValue* dataHeader = header.field(8);
            if (!dataHeader) {
                error = "data page v2 without a header";
                return false;
            }
            count = static_cast<size_t>(dataHeader->intOr(1, 0));
            encoding = static_cast<int>(dataHeader->intOr(4, 0));
            const long long defLength = dataHeader->intOr(5, 0);
            const long long repLength = dataHeader->intOr(6, 0);
            const bool compressed = dataHeader->intOr(7, 1) != 0;
            if (defLength < 0 || repLength < 0 || static_cast<unsigned long long>(defLength) > bodySize ||
                static_cast<unsigned long long>(repLength) > bodySize - static_cast<size_t>(defLength)) {
                error = "corrupt data page v2 level sizes";
                return false;
            }
            if (leaf.maxDef > 0 &&
                !decodeHybrid(body + repLength, static_cast<size_t>(defLength), defWidth, count, definitions)) {
                error = "corrupt definition levels";
                return false;
            }
            const uint8_t* rest = body + repLength + defLength;
            const size_t restSize = bodySize - static_cast<size_t>(repLength + defLength);
            if (compressed && codec != kUncompressed) {
                const long long expected = uncompressedSize - repLength - defLength;
                if (expected < 0 || !decompress(codec, rest, restSize, static_cast<size_t>(expected), page, error)) {
                    if (error.empty()) error = "corrupt data page v2";
                    return false;
                }
                valueData = reinterpret_cast<const uint8_t*>(page.data());
                valueSize = page.size();
            } else {
                valueData = rest;
                valueSize = restSize;
            }
        }

        if (count > static_cast<unsigned long long>(totalValues - read)) {
            error = "page holds more values than its column chunk";
            return false;
        }
        size_t present = count;
        if (leaf.maxDef > 0) {
            present = 0;
            for (uint32_t level : definitions) present += level == static_cast<uint32_t>(leaf.maxDef) ? 1 : 0;
        }
        column.reserve(column.size() + count);
        if ((encoding == kPlainDictionary || encoding == kRleDictionary) && present > 0) {
            // Append straight from the dictionary by index (no intermediate copy of every value).
            if (!haveDictionary) {
                error = "dictionary-encoded page without a dictionary page";
                return false;
            }
            std::vector<uint32_t> indices;
            const size_t dictionarySize = dictionary.size(leaf.physical);
            if (valueSize < 1 || !decodeHybrid(valueData + 1, valueSize - 1, valueData[0], present, indices)) {
                error = "corrupt dictionary indices";
                return false;
            }
            size_t next = 0;
            for (size_t k = 0; k < count; ++k) {
                if (leaf.maxDef > 0 && definitions[k] != static_cast<uint32_t>(leaf.maxDef)) {
                    column.pushNull();
                    continue;
                }
                const uint32_t index = indices[next++];
                if (index >= dictionarySize) {
                    error = "corrupt dictionary indices";
                    return false;
                }
                appendValue(column, leaf, dictionary, index);
            }
            read += static_cast<long long>(count);
            continue;
        }
        Values values;
        if (!decodeValues(encoding, valueData, valueSize, present, leaf, haveDictionary, dictionary, values, error)) return false;
        if (values.size(leaf.physical) < present) {
            error = "page holds fewer values than its header says";
            return false;
        }
        size_t next = 0;
        for (size_t k = 0; k < count; ++k) {
            if (leaf.maxDef > 0 && definitions[k] != static_cast<uint32_t>(leaf.maxDef)) {
                column.pushNull();
            } else {
                appendValue(column, leaf, values, next++);
            }
        }
        read += static_cast<long long>(count);
    }
    return true;
}

bool readFile(const std::string& path, std::string& data) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 0) return false;
    in.seekg(0, std::ios::beg);
    data.resize(static_cast<size_t>(size));
    if (size > 0) in.read(&data[0], size);
    return static_cast<bool>(in) || in.eof();
}

// What the most recent Parquet scan on this thread read (for tests and LazyFrame.explain users).
struct ScanInfo {
    size_t rowGroupsTotal = 0;
    size_t rowGroupsRead = 0;
    size_t columnsTotal = 0;
    size_t columnsRead = 0;
};
thread_local ScanInfo lastScan;

// Min/max statistics of a column chunk as doubles (only for signed integer and floating columns).
struct ChunkStats {
    bool valid = false;
    double min = 0.0;
    double max = 0.0;
};

bool chunkStats(const Leaf& leaf, const TValue& chunk, ChunkStats& stats) {
    using namespace csec_parquet;
    stats = ChunkStats();
    if (leaf.decimal || leaf.float16 || leaf.unsignedInt) return false;
    const TValue* meta = chunk.field(3);
    const TValue* s = meta ? meta->field(12) : nullptr;
    if (!s) return false;
    // min_value/max_value (6/5) replace the deprecated min/max (2/1), whose sort order is unreliable
    // for anything but the numeric types handled here.
    const TValue* minV = s->field(6) ? s->field(6) : s->field(2);
    const TValue* maxV = s->field(5) ? s->field(5) : s->field(1);
    if (!minV || !maxV) return false;
    auto decode = [&](const std::string& bytes, double& out) {
        const auto* p = reinterpret_cast<const uint8_t*>(bytes.data());
        switch (leaf.physical) {
        case kInt32:
            if (bytes.size() != 4) return false;
            out = static_cast<double>(static_cast<int32_t>(le32(p)));
            return true;
        case kInt64:
            if (bytes.size() != 8) return false;
            out = static_cast<double>(static_cast<long long>(le64(p)));
            return true;
        case kFloat: {
            if (bytes.size() != 4) return false;
            const uint32_t bits = le32(p);
            float f;
            std::memcpy(&f, &bits, sizeof f);
            out = f;
            return !std::isnan(out);
        }
        case kDoubleType: {
            if (bytes.size() != 8) return false;
            const uint64_t bits = le64(p);
            std::memcpy(&out, &bits, sizeof out);
            return !std::isnan(out);
        }
        default:
            return false;
        }
    };
    stats.valid = decode(minV->bin, stats.min) && decode(maxV->bin, stats.max);
    return stats.valid;
}

// Footer metadata and flattened leaf columns of a Parquet file held in memory.
bool parseParquetMeta(const uint8_t* file, size_t size, const std::string& path, TValue& meta, std::vector<Leaf>& leaves, std::string& error) {
    using namespace csec_parquet;
    if (size < 12 || std::memcmp(file, "PAR1", 4) != 0 || std::memcmp(file + size - 4, "PAR1", 4) != 0) {
        error = path + " is not a Parquet file";
        return false;
    }
    const size_t footerSize = le32(file + size - 8);
    if (footerSize > size - 12) {
        error = "corrupt Parquet footer";
        return false;
    }
    ThriftReader reader(file + size - 8 - footerSize, footerSize);
    if (!reader.readStruct(meta)) {
        error = "corrupt Parquet file metadata";
        return false;
    }
    const TValue* schema = meta.field(2);
    if (!schema || schema->list.empty()) {
        error = "Parquet file without a schema";
        return false;
    }
    leaves.clear();
    size_t index = 1;
    if (!collectLeaves(schema->list, index, "", 0, 0, false, leaves, static_cast<int>(schema->list[0].intOr(5, 0)), 0)) {
        error = "Parquet schema is nested too deeply";
        return false;
    }
    return true;
}

// Column names of a Parquet file, read from its footer only.
bool parquetColumnNames(const std::string& path, std::vector<std::string>& names, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot read " + path;
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 12) {
        error = path + " is not a Parquet file";
        return false;
    }
    in.seekg(size - 8);
    uint8_t tail[8];
    in.read(reinterpret_cast<char*>(tail), 8);
    const size_t footerSize = csec_parquet::le32(tail);
    if (static_cast<std::streamoff>(footerSize) > size - 12) {
        error = "corrupt Parquet footer";
        return false;
    }
    // Rebuild a minimal "file": magic + footer + length + magic, which parseParquetMeta accepts.
    std::string buffer = "PAR1";
    buffer.resize(4 + footerSize);
    in.seekg(size - 8 - static_cast<std::streamoff>(footerSize));
    in.read(&buffer[4], static_cast<std::streamsize>(footerSize));
    buffer.append(reinterpret_cast<const char*>(tail), 8);
    TValue meta;
    std::vector<Leaf> leaves;
    if (!parseParquetMeta(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size(), path, meta, leaves, error)) return false;
    names.clear();
    for (const auto& leaf : leaves) {
        if (leaf.supported) names.push_back(leaf.name);
    }
    return true;
}

using RowGroupFilter = std::function<bool(const std::vector<Leaf>& leaves, const TValue& rowGroup)>;

// Read a Parquet file into `frame`. `columns` (null = every column) projects and orders the result;
// `keepGroup` (may be empty) can skip row groups, e.g. by their statistics; a non-negative `limit`
// stops after the row groups that hold the first `limit` rows (the caller trims the rest).
bool readParquetIntoUnchecked(const std::string& path, const std::vector<std::string>* columns, const RowGroupFilter& keepGroup,
                              long long limit, DataFrame& frame, std::string& error) {
    using namespace csec_parquet;
    std::string data;
    if (!readFile(path, data)) {
        error = "cannot read " + path;
        return false;
    }
    const auto* file = reinterpret_cast<const uint8_t*>(data.data());
    const size_t size = data.size();
    TValue meta;
    std::vector<Leaf> leaves;
    if (!parseParquetMeta(file, size, path, meta, leaves, error)) return false;

    // Which leaves to read, in the requested order.
    std::vector<size_t> selected;
    if (!columns) {
        for (size_t k = 0; k < leaves.size(); ++k) {
            if (!leaves[k].supported) {
                error = "column '" + leaves[k].name +
                        "' is nested or repeated, which std.dataframe does not support; select the flat columns explicitly";
                return false;
            }
            selected.push_back(k);
        }
    } else {
        for (const auto& name : *columns) {
            size_t found = leaves.size();
            for (size_t k = 0; k < leaves.size(); ++k) {
                if (leaves[k].name == name) found = k;
            }
            if (found == leaves.size()) {
                error = "no column '" + name + "' in " + path;
                return false;
            }
            if (!leaves[found].supported) {
                error = "column '" + name + "' is nested or repeated, which is not supported";
                return false;
            }
            selected.push_back(found);
        }
    }

    // Which row groups to read.
    const TValue* rowGroups = meta.field(4);
    std::vector<const TValue*> groups;
    long long expectedRows = 0;
    const size_t groupCount = rowGroups ? rowGroups->list.size() : 0;
    for (size_t g = 0; g < groupCount; ++g) {
        if (limit >= 0 && expectedRows >= limit) break;
        const TValue& group = rowGroups->list[g];
        if (keepGroup && !keepGroup(leaves, group)) continue;
        groups.push_back(&group);
        expectedRows += group.intOr(3, 0);
    }
    lastScan.rowGroupsTotal = groupCount;
    lastScan.rowGroupsRead = groups.size();
    lastScan.columnsTotal = leaves.size();
    lastScan.columnsRead = selected.size();

    frame.columns.assign(selected.size(), Column());
    std::vector<std::string> errors(selected.size());
    for (size_t k = 0; k < selected.size(); ++k) {
        frame.columns[k] = Column(leaves[selected[k]].name, leaves[selected[k]].out);
        frame.columns[k].reserve(static_cast<size_t>(std::max(0LL, expectedRows)));
    }

    // Decode the columns in parallel; each worker takes whole columns.
    auto decodeColumn = [&](size_t k) {
        const size_t leafIndex = selected[k];
        try {
            for (const TValue* group : groups) {
                const TValue* chunks = group->field(1);
                if (!chunks || leafIndex >= chunks->list.size()) {
                    errors[k] = "row group without column '" + leaves[leafIndex].name + "'";
                    return;
                }
                if (!readColumnChunk(file, size, chunks->list[leafIndex], leaves[leafIndex], frame.columns[k], errors[k])) {
                    errors[k] = "column '" + leaves[leafIndex].name + "': " + errors[k];
                    return;
                }
            }
        } catch (const std::exception& e) {
            errors[k] = "column '" + leaves[leafIndex].name + "': " + e.what();
        }
    };
    const unsigned hardware = std::max(1u, std::thread::hardware_concurrency());
    const size_t workers = std::min<size_t>(selected.size(), std::min<unsigned>(hardware, 16));
    if (workers <= 1 || expectedRows < 4096) {
        for (size_t k = 0; k < selected.size(); ++k) decodeColumn(k);
    } else {
        std::atomic<size_t> nextColumn{0};
        auto drain = [&] {
            for (size_t k = nextColumn++; k < selected.size(); k = nextColumn++) decodeColumn(k);
        };
        std::vector<std::thread> pool;
        try {
            pool.reserve(workers);
            for (size_t w = 1; w < workers; ++w) pool.emplace_back(drain);
        } catch (...) {
        }
        drain();
        for (auto& t : pool) t.join();
    }
    for (const auto& e : errors) {
        if (!e.empty()) {
            error = e;
            return false;
        }
    }
    for (const auto& column : frame.columns) {
        if (static_cast<long long>(column.size()) != expectedRows) {
            error = "column '" + column.name + "' has " + std::to_string(column.size()) + " values, expected " +
                    std::to_string(expectedRows);
            return false;
        }
    }
    return true;
}

bool readParquetInto(const std::string& path, const std::vector<std::string>* columns, const RowGroupFilter& keepGroup,
                     long long limit, DataFrame& frame, std::string& error) {
    try {
        return readParquetIntoUnchecked(path, columns, keepGroup, limit, frame, error);
    } catch (const std::exception& e) {
        error = path + ": " + e.what();
        return false;
    }
}

long long readParquet(const char* path, const char* columnSpec) {
    lastError.clear();
    if (!path) return fail("no path given");
    const std::vector<std::string> wanted = splitList(columnSpec);
    DataFrame frame;
    std::string error;
    if (!readParquetInto(path, wanted.empty() ? nullptr : &wanted, RowGroupFilter(), -1, frame, error)) return fail(error);
    return store(std::move(frame));
}

// ---- Parquet writing ---------------------------------------------------------------------------

void putLe32(std::string& out, uint32_t value) {
    for (int k = 0; k < 4; ++k) out.push_back(static_cast<char>((value >> (8 * k)) & 0xff));
}
void putLe64(std::string& out, uint64_t value) {
    for (int k = 0; k < 8; ++k) out.push_back(static_cast<char>((value >> (8 * k)) & 0xff));
}

bool writeParquet(const DataFrame& frame, const char* path) {
    using csec_parquet::ThriftWriter;
    lastError.clear();
    std::ofstream out(path ? path : "", std::ios::binary | std::ios::trunc);
    if (!out) {
        fail(std::string("cannot write ") + (path ? path : ""));
        return false;
    }
    const size_t rows = frame.rows();
    const size_t groupRows = size_t(1) << 20;
    std::string file = "PAR1";

    struct ChunkInfo {
        long long offset;
        long long size;
        long long values;
    };
    std::vector<std::vector<ChunkInfo>> groups;
    for (size_t first = 0; first < rows || (rows == 0 && groups.empty()); first += groupRows) {
        const size_t count = std::min(groupRows, rows - first);
        std::vector<ChunkInfo> chunks;
        for (const auto& column : frame.columns) {
            std::vector<uint8_t> levels(count);
            std::string values;
            size_t boolBits = 0;  // BOOLEAN values are bit-packed, LSB first
            for (size_t k = 0; k < count; ++k) {
                const size_t row = first + k;
                levels[k] = column.isNull(row) ? 0 : 1;
                if (!levels[k]) continue;
                if (column.type == ColType::Bool) {
                    if (boolBits % 8 == 0) values.push_back('\0');
                    if (column.longs[row]) values.back() = static_cast<char>(values.back() | (1 << (boolBits % 8)));
                    ++boolBits;
                } else if (column.type == ColType::Long) {
                    putLe64(values, static_cast<uint64_t>(column.longs[row]));
                } else if (column.type == ColType::Double) {
                    uint64_t bits;
                    std::memcpy(&bits, &column.doubles[row], sizeof bits);
                    putLe64(values, bits);
                } else {
                    putLe32(values, static_cast<uint32_t>(column.strings[row].size()));
                    values += column.strings[row];
                }
            }
            const std::string encodedLevels = csec_parquet::encodeLevelsRle(levels);
            std::string body;
            putLe32(body, static_cast<uint32_t>(encodedLevels.size()));
            body += encodedLevels;
            body += values;

            ThriftWriter header;
            header.i32(1, 0);  // DATA_PAGE
            header.i32(2, static_cast<long long>(body.size()));
            header.i32(3, static_cast<long long>(body.size()));
            header.beginStruct(5);
            header.i32(1, static_cast<long long>(count));
            header.i32(2, csec_parquet::kPlain);
            header.i32(3, csec_parquet::kRle);
            header.i32(4, csec_parquet::kRle);
            header.endStruct();
            header.stop();

            const long long offset = static_cast<long long>(file.size());
            file += header.out;
            file += body;
            chunks.push_back({offset, static_cast<long long>(header.out.size() + body.size()), static_cast<long long>(count)});
        }
        groups.push_back(std::move(chunks));
        if (rows == 0) break;
    }

    auto physicalOf = [](ColType type) {
        return type == ColType::Long ? csec_parquet::kInt64 : type == ColType::Bool ? csec_parquet::kBoolean
             : type == ColType::Double ? csec_parquet::kDoubleType : csec_parquet::kByteArray;
    };
    ThriftWriter meta;
    meta.i32(1, 1);
    meta.beginList(2, csec_parquet::kStruct, frame.columns.size() + 1);
    meta.beginListStruct();
    meta.string(4, "schema");
    meta.i32(5, static_cast<long long>(frame.columns.size()));
    meta.endStruct();
    for (const auto& column : frame.columns) {
        meta.beginListStruct();
        meta.i32(1, physicalOf(column.type));
        meta.i32(3, 1);  // OPTIONAL
        meta.string(4, column.name);
        if (column.type == ColType::String) {
            meta.i32(6, 0);  // UTF8
            meta.beginStruct(10);
            meta.beginStruct(1);  // LogicalType.STRING
            meta.endStruct();
            meta.endStruct();
        }
        meta.endStruct();
    }
    meta.i64(3, static_cast<long long>(rows));
    meta.beginList(4, csec_parquet::kStruct, groups.size());
    for (size_t g = 0; g < groups.size(); ++g) {
        const size_t groupStart = g * groupRows;
        const long long count = static_cast<long long>(std::min(groupRows, rows - std::min(rows, groupStart)));
        long long totalSize = 0;
        meta.beginListStruct();
        meta.beginList(1, csec_parquet::kStruct, frame.columns.size());
        for (size_t c = 0; c < frame.columns.size(); ++c) {
            const ChunkInfo& info = groups[g][c];
            totalSize += info.size;
            meta.beginListStruct();
            meta.i64(2, info.offset);
            meta.beginStruct(3);
            meta.i32(1, physicalOf(frame.columns[c].type));
            meta.beginList(2, csec_parquet::kI32, 2);
            meta.listI32(csec_parquet::kPlain);
            meta.listI32(csec_parquet::kRle);
            meta.beginList(3, csec_parquet::kBinary, 1);
            meta.listString(frame.columns[c].name);
            meta.i32(4, csec_parquet::kUncompressed);
            meta.i64(5, info.values);
            meta.i64(6, info.size);
            meta.i64(7, info.size);
            meta.i64(9, info.offset);
            meta.endStruct();
            meta.endStruct();
        }
        meta.i64(2, totalSize);
        meta.i64(3, count);
        meta.endStruct();
    }
    meta.string(6, "csec std.dataframe");
    meta.stop();

    file += meta.out;
    putLe32(file, static_cast<uint32_t>(meta.out.size()));
    file += "PAR1";
    out.write(file.data(), static_cast<std::streamsize>(file.size()));
    if (!out) {
        fail(std::string("cannot write ") + path);
        return false;
    }
    return true;
}

// ---- CSV ---------------------------------------------------------------------------------------

// Split CSV text into records of fields (RFC 4180 quoting; CRLF or LF line ends).
std::vector<std::vector<std::string>> parseCsv(const std::string& text) {
    std::vector<std::vector<std::string>> records;
    std::vector<std::string> record;
    std::string field;
    bool quoted = false;
    bool fieldStarted = false;
    for (size_t k = 0; k < text.size(); ++k) {
        const char c = text[k];
        if (quoted) {
            if (c == '"') {
                if (k + 1 < text.size() && text[k + 1] == '"') {
                    field.push_back('"');
                    ++k;
                } else {
                    quoted = false;
                }
            } else {
                field.push_back(c);
            }
            continue;
        }
        if (c == '"' && field.empty()) {
            quoted = true;
            fieldStarted = true;
        } else if (c == ',') {
            record.push_back(std::move(field));
            field.clear();
            fieldStarted = true;
        } else if (c == '\n' || c == '\r') {
            if (c == '\r' && k + 1 < text.size() && text[k + 1] == '\n') ++k;
            if (fieldStarted || !field.empty() || !record.empty()) {
                record.push_back(std::move(field));
                records.push_back(std::move(record));
            }
            record.clear();
            field.clear();
            fieldStarted = false;
        } else {
            field.push_back(c);
            fieldStarted = true;
        }
    }
    if (fieldStarted || !field.empty() || !record.empty()) {
        record.push_back(std::move(field));
        records.push_back(std::move(record));
    }
    return records;
}

bool parseLongStrict(const std::string& text, long long& value) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    value = std::strtoll(text.c_str(), &end, 10);
    return errno == 0 && end && *end == '\0' && end != text.c_str();
}

bool parseBoolStrict(const std::string& text, bool& value) {
    std::string lower;
    for (char c : text) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "true") value = true;
    else if (lower == "false") value = false;
    else return false;
    return true;
}

bool parseDoubleStrict(const std::string& text, double& value) {
    if (text.empty()) return false;
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end && *end == '\0' && end != text.c_str();
}

long long frameFromCsv(const std::string& text) {
    lastError.clear();
    const auto records = parseCsv(text);
    if (records.empty()) return fail("CSV input has no header row");
    const auto& header = records[0];
    DataFrame frame;
    for (size_t c = 0; c < header.size(); ++c) {
        const std::string name = trim(header[c]);
        bool allLong = true;
        bool allDouble = true;
        bool allBool = true;
        for (size_t r = 1; r < records.size(); ++r) {
            const std::string cell = c < records[r].size() ? trim(records[r][c]) : std::string();
            if (cell.empty()) continue;
            long long l;
            double d;
            bool b;
            if (allBool && !parseBoolStrict(cell, b)) allBool = false;
            if (allLong && !parseLongStrict(cell, l)) allLong = false;
            if (allDouble && !parseDoubleStrict(cell, d)) allDouble = false;
            if (!allLong && !allDouble && !allBool) break;
        }
        bool anyValue = false;
        for (size_t r = 1; r < records.size() && !anyValue; ++r) anyValue = c < records[r].size() && !trim(records[r][c]).empty();
        Column column(name, (allBool && anyValue) ? ColType::Bool : allLong ? ColType::Long : allDouble ? ColType::Double : ColType::String);
        column.reserve(records.size() - 1);
        for (size_t r = 1; r < records.size(); ++r) {
            const std::string raw = c < records[r].size() ? records[r][c] : std::string();
            const std::string cell = trim(raw);
            if (cell.empty()) {
                column.pushNull();
            } else if (column.type == ColType::Bool) {
                bool value = false;
                parseBoolStrict(cell, value);
                column.pushBool(value);
            } else if (column.type == ColType::Long) {
                long long value = 0;
                parseLongStrict(cell, value);
                column.pushLong(value);
            } else if (column.type == ColType::Double) {
                double value = 0;
                parseDoubleStrict(cell, value);
                column.pushDouble(value);
            } else {
                column.pushString(raw);
            }
        }
        frame.columns.push_back(std::move(column));
    }
    return store(std::move(frame));
}

std::string csvField(const std::string& text) {
    if (text.find_first_of(",\"\r\n") == std::string::npos) return text;
    std::string out = "\"";
    for (char c : text) {
        if (c == '"') out.push_back('"');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string frameToCsv(const DataFrame& frame) {
    std::string out;
    for (size_t c = 0; c < frame.columns.size(); ++c) {
        if (c) out.push_back(',');
        out += csvField(frame.columns[c].name);
    }
    out.push_back('\n');
    const size_t rows = frame.rows();
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < frame.columns.size(); ++c) {
            if (c) out.push_back(',');
            out += csvField(cellText(frame.columns[c], r, ""));
        }
        out.push_back('\n');
    }
    return out;
}

// ---- Comparison helpers ------------------------------------------------------------------------

enum class Cmp { Eq, Ne, Lt, Le, Gt, Ge, Contains, StartsWith, EndsWith, Invalid };

Cmp parseCmp(const char* op) {
    const std::string text = op ? trim(op) : std::string();
    if (text == "==" || text == "=") return Cmp::Eq;
    if (text == "!=" || text == "<>") return Cmp::Ne;
    if (text == "<") return Cmp::Lt;
    if (text == "<=") return Cmp::Le;
    if (text == ">") return Cmp::Gt;
    if (text == ">=") return Cmp::Ge;
    if (text == "contains") return Cmp::Contains;
    if (text == "startswith") return Cmp::StartsWith;
    if (text == "endswith") return Cmp::EndsWith;
    return Cmp::Invalid;
}

template <typename T>
bool compareOrdered(const T& a, const T& b, Cmp cmp) {
    switch (cmp) {
    case Cmp::Eq: return a == b;
    case Cmp::Ne: return a != b;
    case Cmp::Lt: return a < b;
    case Cmp::Le: return a <= b;
    case Cmp::Gt: return a > b;
    case Cmp::Ge: return a >= b;
    default: return false;
    }
}

// -1, 0, 1 between two non-null cells of the same column type.
int compareCells(const Column& column, size_t a, size_t b) {
    if (column.usesLongs()) return column.longs[a] < column.longs[b] ? -1 : column.longs[a] > column.longs[b] ? 1 : 0;
    if (column.type == ColType::Double) {
        const double x = column.doubles[a], y = column.doubles[b];
        if (std::isnan(x) || std::isnan(y)) return std::isnan(x) ? (std::isnan(y) ? 0 : 1) : -1;
        return x < y ? -1 : x > y ? 1 : 0;
    }
    const int c = column.strings[a].compare(column.strings[b]);
    return c < 0 ? -1 : c > 0 ? 1 : 0;
}

// Binary key for hashing a row's values in `columns` (nulls group together).
void appendKey(std::string& key, const Column& column, size_t row, bool asDouble) {
    if (column.isNull(row)) {
        key.push_back('\0');
        return;
    }
    if (column.type == ColType::String) {
        key.push_back('s');
        const uint32_t length = static_cast<uint32_t>(column.strings[row].size());
        key.append(reinterpret_cast<const char*>(&length), sizeof length);
        key += column.strings[row];
    } else if (column.usesLongs() && !asDouble) {
        key.push_back('l');
        key.append(reinterpret_cast<const char*>(&column.longs[row]), sizeof(long long));
    } else {
        double value = column.number(row);
        if (value == 0.0) value = 0.0;  // -0 == 0
        key.push_back('d');
        key.append(reinterpret_cast<const char*>(&value), sizeof value);
    }
}

// ---- Aggregation -------------------------------------------------------------------------------

enum class Agg { Count, Sum, Mean, Min, Max, Std, Var, Median, NullCount, First, Last, NUnique, Invalid };

Agg parseAgg(const std::string& name) {
    if (name == "count") return Agg::Count;
    if (name == "sum") return Agg::Sum;
    if (name == "mean" || name == "avg") return Agg::Mean;
    if (name == "min") return Agg::Min;
    if (name == "max") return Agg::Max;
    if (name == "std") return Agg::Std;
    if (name == "var") return Agg::Var;
    if (name == "median") return Agg::Median;
    if (name == "null_count") return Agg::NullCount;
    if (name == "first") return Agg::First;
    if (name == "last") return Agg::Last;
    if (name == "nunique") return Agg::NUnique;
    return Agg::Invalid;
}

ColType aggType(Agg agg, const Column* source) {
    switch (agg) {
    case Agg::Count:
    case Agg::NullCount:
    case Agg::NUnique: return ColType::Long;
    case Agg::Mean:
    case Agg::Std:
    case Agg::Var:
    case Agg::Median: return ColType::Double;
    case Agg::Sum: return source && source->type == ColType::Bool ? ColType::Long : (source ? source->type : ColType::Long);
    default: return source ? source->type : ColType::Long;
    }
}

// Aggregate `rows` of `source` (null for a plain row count) into one cell of `out`.
void aggregate(Agg agg, const Column* source, const std::vector<size_t>& rows, Column& out) {
    if (agg == Agg::Count) {
        long long count = 0;
        for (size_t r : rows) count += (!source || !source->isNull(r)) ? 1 : 0;
        out.pushLong(count);
        return;
    }
    if (agg == Agg::NullCount) {
        long long count = 0;
        for (size_t r : rows) count += (source && source->isNull(r)) ? 1 : 0;
        out.pushLong(count);
        return;
    }
    if (agg == Agg::Median) {
        std::vector<double> values;
        values.reserve(rows.size());
        for (size_t r : rows) {
            if (!source->isNull(r)) values.push_back(source->number(r));
        }
        if (values.empty()) {
            out.pushNull();
            return;
        }
        const size_t mid = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + mid, values.end());
        double median = values[mid];
        if (values.size() % 2 == 0) {
            const double lower = *std::max_element(values.begin(), values.begin() + mid);
            median = (median + lower) / 2.0;
        }
        out.pushDouble(median);
        return;
    }
    if (agg == Agg::NUnique) {
        std::unordered_set<std::string> seen;
        for (size_t r : rows) {
            if (source->isNull(r)) continue;
            std::string key;
            appendKey(key, *source, r, false);
            seen.insert(key);
        }
        out.pushLong(static_cast<long long>(seen.size()));
        return;
    }
    if (agg == Agg::First || agg == Agg::Last || agg == Agg::Min || agg == Agg::Max) {
        bool found = false;
        size_t best = 0;
        for (size_t r : rows) {
            if (source->isNull(r)) continue;
            if (!found) {
                best = r;
                found = true;
                if (agg == Agg::First) break;
                continue;
            }
            if (agg == Agg::Last) best = r;
            else if (agg == Agg::Min && compareCells(*source, r, best) < 0) best = r;
            else if (agg == Agg::Max && compareCells(*source, r, best) > 0) best = r;
        }
        if (found) out.appendFrom(*source, best);
        else out.pushNull();
        return;
    }
    // Sum, Mean, Std need numbers.
    if (source->usesLongs() && agg == Agg::Sum) {
        long long sum = 0;
        for (size_t r : rows) {
            if (!source->isNull(r)) sum += source->longs[r];
        }
        out.pushLong(sum);
        return;
    }
    double sum = 0.0;
    double compensation = 0.0;  // Kahan summation
    size_t n = 0;
    for (size_t r : rows) {
        if (source->isNull(r)) continue;
        const double y = source->number(r) - compensation;
        const double t = sum + y;
        compensation = (t - sum) - y;
        sum = t;
        ++n;
    }
    if (agg == Agg::Sum) {
        out.pushDouble(sum);
        return;
    }
    if (n == 0 || ((agg == Agg::Std || agg == Agg::Var) && n < 2)) {
        out.pushNull();
        return;
    }
    const double mean = sum / static_cast<double>(n);
    if (agg == Agg::Mean) {
        out.pushDouble(mean);
        return;
    }
    double squares = 0.0;
    for (size_t r : rows) {
        if (source->isNull(r)) continue;
        const double d = source->number(r) - mean;
        squares += d * d;
    }
    const double variance = squares / static_cast<double>(n - 1);
    out.pushDouble(agg == Agg::Var ? variance : std::sqrt(variance));
}

struct AggSpec {
    int column = -1;  // -1: count rows
    Agg agg = Agg::Count;
    std::string outName;
};

bool parseAggSpecs(const DataFrame& frame, const char* spec, std::vector<AggSpec>& specs) {
    for (const auto& item : splitList(spec)) {
        AggSpec s;
        const auto colon = item.rfind(':');
        const std::string columnName = colon == std::string::npos ? std::string() : trim(item.substr(0, colon));
        const std::string aggName = trim(colon == std::string::npos ? item : item.substr(colon + 1));
        s.agg = parseAgg(aggName);
        if (s.agg == Agg::Invalid) {
            fail("unknown aggregation '" + aggName + "' (count, sum, mean, min, max, std, var, median, null_count, first, last, nunique)");
            return false;
        }
        if (columnName.empty() || columnName == "*") {
            if (s.agg != Agg::Count) {
                fail("aggregation '" + aggName + "' needs a column (column:" + aggName + ")");
                return false;
            }
            s.outName = "count";
        } else {
            s.column = frame.find(columnName);
            if (s.column < 0) {
                fail("no column '" + columnName + "'");
                return false;
            }
            const Column& source = frame.columns[s.column];
            if (!source.numeric() && (s.agg == Agg::Sum || s.agg == Agg::Mean || s.agg == Agg::Std || s.agg == Agg::Var ||
                                      s.agg == Agg::Median)) {
                fail("cannot take the " + aggName + " of string column '" + columnName + "'");
                return false;
            }
            s.outName = columnName + "_" + aggName;
        }
        specs.push_back(s);
    }
    if (specs.empty()) {
        fail("no aggregations given");
        return false;
    }
    return true;
}

bool resolveColumns(const DataFrame& frame, const char* spec, std::vector<int>& indices) {
    for (const auto& name : splitList(spec)) {
        const int index = frame.find(name);
        if (index < 0) {
            fail("no column '" + name + "'");
            return false;
        }
        indices.push_back(index);
    }
    return true;
}

long long groupBy(const DataFrame& frame, const char* keySpec, const char* aggSpec) {
    std::vector<int> keys;
    if (!resolveColumns(frame, keySpec, keys)) return 0;
    std::vector<AggSpec> specs;
    if (!parseAggSpecs(frame, aggSpec, specs)) return 0;
    const size_t rows = frame.rows();
    std::unordered_map<std::string, size_t> groupIndex;
    std::vector<std::vector<size_t>> groups;
    std::string key;
    for (size_t r = 0; r < rows; ++r) {
        key.clear();
        for (int k : keys) appendKey(key, frame.columns[k], r, false);
        auto it = groupIndex.find(key);
        if (it == groupIndex.end()) {
            it = groupIndex.emplace(key, groups.size()).first;
            groups.emplace_back();
        }
        groups[it->second].push_back(r);
    }
    if (keys.empty() && groups.empty()) groups.emplace_back();  // whole-frame aggregate of an empty frame
    DataFrame out;
    for (int k : keys) {
        Column column = frame.columns[k].emptyLike();
        for (const auto& g : groups) column.appendFrom(frame.columns[k], g.front());
        out.columns.push_back(std::move(column));
    }
    for (const auto& s : specs) {
        const Column* source = s.column >= 0 ? &frame.columns[s.column] : nullptr;
        Column column(s.outName, aggType(s.agg, source));
        for (const auto& g : groups) aggregate(s.agg, source, g, column);
        out.columns.push_back(std::move(column));
    }
    return store(std::move(out));
}

} // namespace csec_df

// ---- C API ---------------------------------------------------------------------------------------

extern "C" {

char* csec_df_last_error(void) {
    return csec_microthread::ownedText(csec_df::lastError);
}

int csec_df_zstd_supported(void) {
    return csec_parquet::zstdAvailable() ? 1 : 0;
}

long long csec_df_read_parquet(const char* path, const char* columns) {
    try {
        return csec_df::readParquet(path, columns);
    } catch (const std::exception& e) {
        return csec_df::fail(std::string("reading Parquet failed: ") + e.what());
    }
}

int csec_df_write_parquet(long long handle, const char* path) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    try {
        return csec_df::writeParquet(*frame, path) ? 1 : 0;
    } catch (const std::exception& e) {
        csec_df::fail(std::string("writing Parquet failed: ") + e.what());
        return 0;
    }
}

long long csec_df_read_csv(const char* path) {
    std::string text;
    if (!path || !csec_df::readFile(path, text)) return csec_df::fail(std::string("cannot read ") + (path ? path : ""));
    return csec_df::frameFromCsv(text);
}

long long csec_df_from_csv_text(const char* text) {
    return csec_df::frameFromCsv(text ? text : "");
}

int csec_df_write_csv(long long handle, const char* path) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    std::ofstream out(path ? path : "", std::ios::binary | std::ios::trunc);
    if (!out) {
        csec_df::fail(std::string("cannot write ") + (path ? path : ""));
        return 0;
    }
    const std::string text = csec_df::frameToCsv(*frame);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return out ? 1 : 0;
}

char* csec_df_to_csv_text(long long handle) {
    auto frame = csec_df::frameOf(handle);
    return csec_microthread::ownedText(frame ? csec_df::frameToCsv(*frame) : std::string());
}

void csec_df_free(long long handle) {
    csec_df::frames().take(handle);
}

long long csec_df_rows(long long handle) {
    auto frame = csec_df::frames().get(handle);
    return frame ? static_cast<long long>(frame->rows()) : 0;
}

int csec_df_columns(long long handle) {
    auto frame = csec_df::frames().get(handle);
    return frame ? static_cast<int>(frame->columns.size()) : 0;
}

char* csec_df_column_name(long long handle, int index) {
    auto frame = csec_df::frames().get(handle);
    const bool ok = frame && index >= 0 && static_cast<size_t>(index) < frame->columns.size();
    return csec_microthread::ownedText(ok ? frame->columns[index].name : std::string());
}

int csec_df_column_index(long long handle, const char* name) {
    auto frame = csec_df::frames().get(handle);
    return frame && name ? frame->find(name) : -1;
}

char* csec_df_column_type(long long handle, const char* name) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    return csec_microthread::ownedText(index >= 0 ? csec_df::typeName(frame->columns[index].type) : "");
}

// Cell access. A missing column or row reads as null.
int csec_df_is_null(long long handle, const char* name, long long row) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0 || row < 0) return 1;
    return frame->columns[index].isNull(static_cast<size_t>(row)) ? 1 : 0;
}

long long csec_df_get_long(long long handle, const char* name, long long row) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0 || row < 0) return 0;
    const auto& column = frame->columns[index];
    const size_t r = static_cast<size_t>(row);
    if (column.isNull(r)) return 0;
    if (column.usesLongs()) return column.longs[r];
    if (column.type == csec_df::ColType::Double) return static_cast<long long>(column.doubles[r]);
    long long value = 0;
    return csec_df::parseLongStrict(csec_df::trim(column.strings[r]), value) ? value : 0;
}

double csec_df_get_double(long long handle, const char* name, long long row) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    if (index < 0 || row < 0) return nan;
    const auto& column = frame->columns[index];
    const size_t r = static_cast<size_t>(row);
    if (column.isNull(r)) return nan;
    if (column.numeric()) return column.number(r);
    double value = 0;
    return csec_df::parseDoubleStrict(csec_df::trim(column.strings[r]), value) ? value : nan;
}

char* csec_df_get_string(long long handle, const char* name, long long row) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0 || row < 0) return csec_microthread::ownedText("");
    return csec_microthread::ownedText(csec_df::cellText(frame->columns[index], static_cast<size_t>(row), ""));
}

// ---- Building frames ----

long long csec_df_new(void) {
    return csec_df::store(csec_df::DataFrame());
}

int csec_df_add_column(long long handle, const char* name, const char* type) {
    auto frame = csec_df::frameOf(handle);
    if (!frame || !name || !type) return 0;
    const std::string t = csec_df::trim(type);
    csec_df::ColType columnType;
    if (t == "long") columnType = csec_df::ColType::Long;
    else if (t == "double") columnType = csec_df::ColType::Double;
    else if (t == "string") columnType = csec_df::ColType::String;
    else if (t == "bool") columnType = csec_df::ColType::Bool;
    else {
        csec_df::fail("unknown column type '" + t + "' (long, double, string)");
        return 0;
    }
    if (frame->find(name) >= 0) {
        csec_df::fail(std::string("column '") + name + "' already exists");
        return 0;
    }
    csec_df::Column column(name, columnType);
    const size_t rows = frame->rows();
    for (size_t k = 0; k < rows; ++k) column.pushNull();
    frame->columns.push_back(std::move(column));
    return 1;
}

static csec_df::Column* csecDfAppendTarget(long long handle, const char* name, csec_df::FramePtr& frame) {
    frame = csec_df::frames().get(handle);
    if (!frame) csec_df::lastError = "unknown data frame handle";
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0) {
        if (frame) csec_df::fail(std::string("no column '") + (name ? name : "") + "'");
        return nullptr;
    }
    return &frame->columns[index];
}

int csec_df_append_long(long long handle, const char* name, long long value) {
    csec_df::FramePtr frame;
    csec_df::Column* column = csecDfAppendTarget(handle, name, frame);
    if (!column) return 0;
    if (column->type == csec_df::ColType::Long) column->pushLong(value);
    else if (column->type == csec_df::ColType::Bool) column->pushBool(value != 0);
    else if (column->type == csec_df::ColType::Double) column->pushDouble(static_cast<double>(value));
    else column->pushString(std::to_string(value));
    return 1;
}

int csec_df_append_double(long long handle, const char* name, double value) {
    csec_df::FramePtr frame;
    csec_df::Column* column = csecDfAppendTarget(handle, name, frame);
    if (!column) return 0;
    if (column->type == csec_df::ColType::Double) column->pushDouble(value);
    else if (column->type == csec_df::ColType::Long) column->pushLong(static_cast<long long>(value));
    else if (column->type == csec_df::ColType::Bool) column->pushBool(value != 0.0);
    else column->pushString(csec_df::formatDouble(value));
    return 1;
}

int csec_df_append_string(long long handle, const char* name, const char* value) {
    csec_df::FramePtr frame;
    csec_df::Column* column = csecDfAppendTarget(handle, name, frame);
    if (!column) return 0;
    const std::string text = value ? value : "";
    if (column->type == csec_df::ColType::String) {
        column->pushString(text);
        return 1;
    }
    long long l;
    double d;
    bool b;
    if (column->type == csec_df::ColType::Bool && csec_df::parseBoolStrict(csec_df::trim(text), b)) column->pushBool(b);
    else if (column->type == csec_df::ColType::Long && csec_df::parseLongStrict(csec_df::trim(text), l)) column->pushLong(l);
    else if (column->type == csec_df::ColType::Double && csec_df::parseDoubleStrict(csec_df::trim(text), d)) column->pushDouble(d);
    else column->pushNull();
    return 1;
}

int csec_df_append_null(long long handle, const char* name) {
    csec_df::FramePtr frame;
    csec_df::Column* column = csecDfAppendTarget(handle, name, frame);
    if (!column) return 0;
    column->pushNull();
    return 1;
}

// ---- Frame operations (each returns a new frame, 0 on failure) ----

long long csec_df_select(long long handle, const char* spec) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    std::vector<int> indices;
    if (!csec_df::resolveColumns(*frame, spec, indices)) return 0;
    csec_df::DataFrame out;
    for (int k : indices) out.columns.push_back(frame->columns[k]);
    return csec_df::store(std::move(out));
}

long long csec_df_drop(long long handle, const char* spec) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    std::vector<int> indices;
    if (!csec_df::resolveColumns(*frame, spec, indices)) return 0;
    csec_df::DataFrame out;
    for (size_t k = 0; k < frame->columns.size(); ++k) {
        if (std::find(indices.begin(), indices.end(), static_cast<int>(k)) == indices.end()) out.columns.push_back(frame->columns[k]);
    }
    return csec_df::store(std::move(out));
}

long long csec_df_rename(long long handle, const char* from, const char* to) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    const int index = from ? frame->find(from) : -1;
    if (index < 0) return csec_df::fail(std::string("no column '") + (from ? from : "") + "'");
    if (!to || !*to) return csec_df::fail("empty column name");
    const int clash = frame->find(to);
    if (clash >= 0 && clash != index) return csec_df::fail(std::string("column '") + to + "' already exists");
    csec_df::DataFrame out = *frame;
    out.columns[index].name = to;
    return csec_df::store(std::move(out));
}

// Rows [start, start + count); a negative start counts from the end.
long long csec_df_slice(long long handle, long long start, long long count) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    const long long rows = static_cast<long long>(frame->rows());
    if (start < 0) start = std::max(0LL, rows + start);
    start = std::min(start, rows);
    const long long end = count < 0 ? rows : std::min(rows, start + count);
    std::vector<size_t> picked;
    for (long long r = start; r < end; ++r) picked.push_back(static_cast<size_t>(r));
    return csec_df::store(csec_df::take(*frame, picked));
}

long long csec_df_filter(long long handle, const char* name, const char* op, double value) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    const int index = name ? frame->find(name) : -1;
    if (index < 0) return csec_df::fail(std::string("no column '") + (name ? name : "") + "'");
    const auto cmp = csec_df::parseCmp(op);
    if (cmp == csec_df::Cmp::Invalid || cmp >= csec_df::Cmp::Contains) {
        return csec_df::fail(std::string("unknown numeric comparison '") + (op ? op : "") + "' (==, !=, <, <=, >, >=)");
    }
    const auto& column = frame->columns[index];
    if (!column.numeric()) return csec_df::fail("column '" + column.name + "' is a string column; use dfFilterString");
    std::vector<size_t> picked;
    const size_t rows = frame->rows();
    const bool integral = column.usesLongs() && std::floor(value) == value && std::fabs(value) < 9.2e18;
    for (size_t r = 0; r < rows; ++r) {
        if (column.isNull(r)) continue;
        const bool keep = integral ? csec_df::compareOrdered(column.longs[r], static_cast<long long>(value), cmp)
                                   : csec_df::compareOrdered(column.number(r), value, cmp);
        if (keep) picked.push_back(r);
    }
    return csec_df::store(csec_df::take(*frame, picked));
}

long long csec_df_filter_string(long long handle, const char* name, const char* op, const char* value) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    const int index = name ? frame->find(name) : -1;
    if (index < 0) return csec_df::fail(std::string("no column '") + (name ? name : "") + "'");
    const auto cmp = csec_df::parseCmp(op);
    if (cmp == csec_df::Cmp::Invalid) {
        return csec_df::fail(std::string("unknown comparison '") + (op ? op : "") + "' (==, !=, <, <=, >, >=, contains, startswith, endswith)");
    }
    const auto& column = frame->columns[index];
    const std::string needle = value ? value : "";
    std::vector<size_t> picked;
    const size_t rows = frame->rows();
    for (size_t r = 0; r < rows; ++r) {
        if (column.isNull(r)) continue;
        const std::string text = column.type == csec_df::ColType::String ? column.strings[r] : csec_df::cellText(column, r, "");
        bool keep = false;
        switch (cmp) {
        case csec_df::Cmp::Contains: keep = text.find(needle) != std::string::npos; break;
        case csec_df::Cmp::StartsWith: keep = text.compare(0, needle.size(), needle) == 0; break;
        case csec_df::Cmp::EndsWith:
            keep = text.size() >= needle.size() && text.compare(text.size() - needle.size(), needle.size(), needle) == 0;
            break;
        default: keep = csec_df::compareOrdered(text, needle, cmp); break;
        }
        if (keep) picked.push_back(r);
    }
    return csec_df::store(csec_df::take(*frame, picked));
}

// Keep the rows where none of the listed columns (all columns for "") is null.
long long csec_df_drop_nulls(long long handle, const char* spec) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    std::vector<int> indices;
    if (!csec_df::resolveColumns(*frame, spec, indices)) return 0;
    if (indices.empty()) {
        for (size_t k = 0; k < frame->columns.size(); ++k) indices.push_back(static_cast<int>(k));
    }
    std::vector<size_t> picked;
    const size_t rows = frame->rows();
    for (size_t r = 0; r < rows; ++r) {
        bool keep = true;
        for (int k : indices) keep = keep && !frame->columns[k].isNull(r);
        if (keep) picked.push_back(r);
    }
    return csec_df::store(csec_df::take(*frame, picked));
}

// Stable sort by "a,-b" (a prefix '-' sorts that key descending). Nulls sort last.
long long csec_df_sort(long long handle, const char* spec) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    std::vector<std::pair<int, bool>> keys;
    for (auto name : csec_df::splitList(spec)) {
        bool descending = false;
        if (name[0] == '-' || name[0] == '+') {
            descending = name[0] == '-';
            name = csec_df::trim(name.substr(1));
        }
        const int index = frame->find(name);
        if (index < 0) return csec_df::fail("no column '" + name + "'");
        keys.emplace_back(index, descending);
    }
    if (keys.empty()) return csec_df::fail("no sort keys given");
    std::vector<size_t> order(frame->rows());
    for (size_t r = 0; r < order.size(); ++r) order[r] = r;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        for (const auto& key : keys) {
            const auto& column = frame->columns[key.first];
            const bool nullA = column.isNull(a), nullB = column.isNull(b);
            if (nullA || nullB) {
                if (nullA == nullB) continue;
                return nullB;  // non-null first
            }
            const int c = csec_df::compareCells(column, a, b);
            if (c != 0) return key.second ? c > 0 : c < 0;
        }
        return false;
    });
    return csec_df::store(csec_df::take(*frame, order));
}

long long csec_df_group_by(long long handle, const char* keys, const char* aggregations) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    return csec_df::groupBy(*frame, keys, aggregations);
}

// Hash join on "key" or "leftKey=rightKey"; how is "inner" or "left". Right-hand columns whose names
// clash with the left get the suffix "_right".
long long csec_df_join(long long leftHandle, long long rightHandle, const char* on, const char* how) {
    auto left = csec_df::frameOf(leftHandle);
    auto right = csec_df::frameOf(rightHandle);
    if (!left || !right) return 0;
    const std::string spec = on ? csec_df::trim(on) : std::string();
    const auto eq = spec.find('=');
    const std::string leftKey = csec_df::trim(eq == std::string::npos ? spec : spec.substr(0, eq));
    const std::string rightKey = csec_df::trim(eq == std::string::npos ? spec : spec.substr(eq + 1));
    const int li = left->find(leftKey);
    const int ri = right->find(rightKey);
    if (li < 0) return csec_df::fail("no column '" + leftKey + "' in the left frame");
    if (ri < 0) return csec_df::fail("no column '" + rightKey + "' in the right frame");
    const std::string mode = how ? csec_df::trim(how) : std::string("inner");
    if (mode != "inner" && mode != "left") return csec_df::fail("unknown join '" + mode + "' (inner, left)");
    const auto& lk = left->columns[li];
    const auto& rk = right->columns[ri];
    if (lk.numeric() != rk.numeric()) return csec_df::fail("join keys '" + leftKey + "' and '" + rightKey + "' have different types");
    const bool asDouble = lk.type != rk.type;

    std::unordered_map<std::string, std::vector<size_t>> index;
    std::string key;
    const size_t rightRows = right->rows();
    for (size_t r = 0; r < rightRows; ++r) {
        if (rk.isNull(r)) continue;
        key.clear();
        csec_df::appendKey(key, rk, r, asDouble);
        index[key].push_back(r);
    }
    std::vector<size_t> leftRows;
    std::vector<long long> rightMatch;  // -1: no match (left join)
    const size_t rowsLeft = left->rows();
    for (size_t r = 0; r < rowsLeft; ++r) {
        const std::vector<size_t>* matches = nullptr;
        if (!lk.isNull(r)) {
            key.clear();
            csec_df::appendKey(key, lk, r, asDouble);
            auto it = index.find(key);
            if (it != index.end()) matches = &it->second;
        }
        if (matches) {
            for (size_t m : *matches) {
                leftRows.push_back(r);
                rightMatch.push_back(static_cast<long long>(m));
            }
        } else if (mode == "left") {
            leftRows.push_back(r);
            rightMatch.push_back(-1);
        }
    }
    csec_df::DataFrame out = csec_df::take(*left, leftRows);
    for (size_t c = 0; c < right->columns.size(); ++c) {
        if (static_cast<int>(c) == ri) continue;
        const auto& source = right->columns[c];
        csec_df::Column column = source.emptyLike();
        if (out.find(column.name) >= 0) column.name += "_right";
        column.reserve(rightMatch.size());
        for (long long m : rightMatch) {
            if (m < 0) column.pushNull();
            else column.appendFrom(source, static_cast<size_t>(m));
        }
        out.columns.push_back(std::move(column));
    }
    return csec_df::store(std::move(out));
}

static long long csecDfCompute(long long handle, const char* name, const char* a, const char* op, const char* b, bool scalar, double value) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    if (!name || !*name) return csec_df::fail("empty column name");
    const int ai = a ? frame->find(a) : -1;
    if (ai < 0) return csec_df::fail(std::string("no column '") + (a ? a : "") + "'");
    const int bi = scalar ? -1 : (b ? frame->find(b) : -1);
    if (!scalar && bi < 0) return csec_df::fail(std::string("no column '") + (b ? b : "") + "'");
    const std::string o = op ? csec_df::trim(op) : std::string();
    if (o != "+" && o != "-" && o != "*" && o != "/") return csec_df::fail("unknown operator '" + o + "' (+, -, *, /)");
    const auto& x = frame->columns[ai];
    const csec_df::Column* y = scalar ? nullptr : &frame->columns[bi];
    const size_t rows = frame->rows();
    csec_df::Column result;
    if (!x.numeric() || (y && !y->numeric())) {
        if (o != "+" || !y || x.type != csec_df::ColType::String || y->type != csec_df::ColType::String) {
            return csec_df::fail("string columns only support + with another string column");
        }
        result = csec_df::Column(name, csec_df::ColType::String);
        for (size_t r = 0; r < rows; ++r) {
            if (x.isNull(r) || y->isNull(r)) result.pushNull();
            else result.pushString(x.strings[r] + y->strings[r]);
        }
    } else {
        const bool integral = o != "/" && x.usesLongs() &&
                              (scalar ? (std::floor(value) == value && std::fabs(value) < 9.2e18) : y->usesLongs());
        result = csec_df::Column(name, integral ? csec_df::ColType::Long : csec_df::ColType::Double);
        for (size_t r = 0; r < rows; ++r) {
            if (x.isNull(r) || (y && y->isNull(r))) {
                result.pushNull();
                continue;
            }
            if (integral) {
                const long long p = x.longs[r];
                const long long q = y ? y->longs[r] : static_cast<long long>(value);
                result.pushLong(o == "+" ? p + q : o == "-" ? p - q : p * q);
            } else {
                const double p = x.number(r);
                const double q = y ? y->number(r) : value;
                result.pushDouble(o == "+" ? p + q : o == "-" ? p - q : o == "*" ? p * q : p / q);
            }
        }
    }
    csec_df::DataFrame out = *frame;
    const int existing = out.find(name);
    if (existing >= 0) out.columns[existing] = std::move(result);
    else out.columns.push_back(std::move(result));
    return csec_df::store(std::move(out));
}

// New (or replaced) column `name` = a op b, elementwise; nulls propagate.
long long csec_df_compute(long long handle, const char* name, const char* a, const char* op, const char* b) {
    return csecDfCompute(handle, name, a, op, b, false, 0.0);
}

long long csec_df_compute_scalar(long long handle, const char* name, const char* a, const char* op, double value) {
    return csecDfCompute(handle, name, a, op, nullptr, true, value);
}

// Rows of `first` followed by rows of `second`. Columns are matched by name; a Long column meeting a
// Double column becomes Double.
long long csec_df_concat(long long firstHandle, long long secondHandle) {
    auto first = csec_df::frameOf(firstHandle);
    auto second = csec_df::frameOf(secondHandle);
    if (!first || !second) return 0;
    if (first->columns.size() != second->columns.size()) return csec_df::fail("frames have different columns");
    csec_df::DataFrame out;
    for (const auto& a : first->columns) {
        const int bi = second->find(a.name);
        if (bi < 0) return csec_df::fail("column '" + a.name + "' is missing from the second frame");
        const auto& b = second->columns[bi];
        if (a.type == b.type) {
            csec_df::Column column = a;
            column.appendAll(b);
            out.columns.push_back(std::move(column));
        } else if (a.numeric() && b.numeric()) {
            csec_df::Column column(a.name, csec_df::ColType::Double);
            for (const auto* source : {&a, &b}) {
                for (size_t r = 0; r < source->size(); ++r) {
                    if (source->isNull(r)) column.pushNull();
                    else column.pushDouble(source->number(r));
                }
            }
            out.columns.push_back(std::move(column));
        } else {
            return csec_df::fail("column '" + a.name + "' is a string column in only one frame");
        }
    }
    return csec_df::store(std::move(out));
}

// Distinct values of `name` with their counts, most frequent first.
long long csec_df_value_counts(long long handle, const char* name) {
    auto frame = csec_df::frameOf(handle);
    if (!frame) return 0;
    const long long grouped = csec_df::groupBy(*frame, name, "count");
    if (!grouped) return 0;
    const long long sorted = csec_df_sort(grouped, "-count");
    csec_df::frames().take(grouped);
    return sorted;
}

// Whole-column aggregate as a Double (NaN when there is no value, e.g. the mean of no rows).
double csec_df_aggregate(long long handle, const char* name, const char* aggregation) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    auto frame = csec_df::frameOf(handle);
    if (!frame) return nan;
    const std::string spec = std::string(name ? name : "") + ":" + (aggregation ? aggregation : "");
    std::vector<csec_df::AggSpec> specs;
    if (!csec_df::parseAggSpecs(*frame, spec.c_str(), specs)) return nan;
    const auto& s = specs[0];
    if (s.agg == csec_df::Agg::First || s.agg == csec_df::Agg::Last || s.agg == csec_df::Agg::Min || s.agg == csec_df::Agg::Max) {
        if (!frame->columns[s.column].numeric()) {
            csec_df::fail("the " + std::string(aggregation) + " of a string column is not a number");
            return nan;
        }
    }
    std::vector<size_t> rows(frame->rows());
    for (size_t r = 0; r < rows.size(); ++r) rows[r] = r;
    const csec_df::Column* source = s.column >= 0 ? &frame->columns[s.column] : nullptr;
    csec_df::Column cell("", csec_df::aggType(s.agg, source));
    csec_df::aggregate(s.agg, source, rows, cell);
    if (cell.isNull(0)) return nan;
    return cell.usesLongs() ? static_cast<double>(cell.longs[0]) : cell.doubles[0];
}

// Non-null cells in the column (-1 for a missing column).
long long csec_df_count(long long handle, const char* name) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0) return -1;
    long long count = 0;
    for (unsigned char v : frame->columns[index].valid) count += v ? 1 : 0;
    return count;
}

long long csec_df_null_count(long long handle, const char* name) {
    auto frame = csec_df::frames().get(handle);
    const int index = frame && name ? frame->find(name) : -1;
    if (index < 0) return 0;
    long long count = 0;
    for (unsigned char v : frame->columns[index].valid) count += v ? 0 : 1;
    return count;
}

// A text table of at most `maxRows` rows (the first and last rows when the frame is longer).
char* csec_df_to_string(long long handle, int maxRows) {
    auto frame = csec_df::frames().get(handle);
    if (!frame) return csec_microthread::ownedText("<invalid data frame>");
    const size_t rows = frame->rows();
    const size_t limit = maxRows < 0 ? rows : static_cast<size_t>(maxRows);
    std::vector<size_t> shown;
    bool elided = false;
    if (rows <= limit) {
        for (size_t r = 0; r < rows; ++r) shown.push_back(r);
    } else {
        const size_t head = (limit + 1) / 2;
        const size_t tail = limit - head;
        for (size_t r = 0; r < head; ++r) shown.push_back(r);
        for (size_t r = rows - tail; r < rows; ++r) shown.push_back(r);
        elided = true;
    }
    const size_t columns = frame->columns.size();
    std::vector<std::vector<std::string>> cells(shown.size() + 2, std::vector<std::string>(columns));
    std::vector<size_t> width(columns, 0);
    std::vector<bool> rightAlign(columns, false);
    for (size_t c = 0; c < columns; ++c) {
        const auto& column = frame->columns[c];
        cells[0][c] = column.name;
        cells[1][c] = csec_df::typeName(column.type);
        rightAlign[c] = column.numeric();
        for (size_t k = 0; k < shown.size(); ++k) cells[k + 2][c] = csec_df::cellText(column, shown[k], "null");
        for (const auto& row : cells) width[c] = std::max(width[c], row[c].size());
    }
    std::string out;
    auto emitRow = [&](const std::vector<std::string>& row) {
        for (size_t c = 0; c < columns; ++c) {
            if (c) out += "  ";
            const std::string pad(width[c] - row[c].size(), ' ');
            out += rightAlign[c] ? pad + row[c] : row[c] + (c + 1 < columns ? pad : std::string());
        }
        out.push_back('\n');
    };
    emitRow(cells[0]);
    emitRow(cells[1]);
    const size_t head = elided ? (limit + 1) / 2 : shown.size();
    for (size_t k = 0; k < shown.size(); ++k) {
        if (elided && k == head) out += "...\n";
        emitRow(cells[k + 2]);
    }
    out += "[" + std::to_string(rows) + " rows x " + std::to_string(columns) + " columns]";
    return csec_microthread::ownedText(out);
}

} // extern "C"

