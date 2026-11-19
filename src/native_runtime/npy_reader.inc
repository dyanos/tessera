#include <cctype>
#include <stdexcept>

namespace csec_df {
namespace npy {

struct PyValue {
    enum class Kind { None, Bool, Int, Str, Tuple, List, Dict };
    Kind kind = Kind::None;
    bool boolean = false;
    long long integer = 0;
    std::string text;
    std::vector<PyValue> items;
    std::vector<std::pair<std::string, PyValue>> entries;

    const PyValue* get(const std::string& key) const {
        for (const auto& e : entries) {
            if (e.first == key) return &e.second;
        }
        return nullptr;
    }
    bool sequence() const { return kind == Kind::Tuple || kind == Kind::List; }
};

class LiteralParser {
public:
    explicit LiteralParser(const std::string& source) : s(source) {}

    PyValue parseAll() {
        PyValue value = parseValue();
        skipSpace();
        if (p != s.size()) throw std::runtime_error("unexpected text in .npy header");
        return value;
    }

private:
    const std::string& s;
    size_t p = 0;
    int depth = 0;

    struct Nest {
        int& depth;
        explicit Nest(int& d) : depth(d) {
            if (++depth > 64) throw std::runtime_error(".npy header is nested too deeply");
        }
        ~Nest() { --depth; }
    };

    void skipSpace() {
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r')) ++p;
    }
    bool consume(char c) {
        skipSpace();
        if (p < s.size() && s[p] == c) {
            ++p;
            return true;
        }
        return false;
    }
    void expect(char c) {
        if (!consume(c)) throw std::runtime_error(std::string("malformed .npy header: expected '") + c + "'");
    }
    bool word(const char* w) {
        const size_t n = std::strlen(w);
        if (s.compare(p, n, w) != 0) return false;
        const size_t end = p + n;
        if (end < s.size() && (std::isalnum(static_cast<unsigned char>(s[end])) || s[end] == '_')) return false;
        p = end;
        return true;
    }

    PyValue parseValue() {
        Nest nest(depth);
        skipSpace();
        if (p >= s.size()) throw std::runtime_error("truncated .npy header");
        const char c = s[p];
        PyValue v;
        if (c == '\'' || c == '"') {
            v.kind = PyValue::Kind::Str;
            v.text = parseString();
            return v;
        }
        if (c == '(' || c == '[') {
            const char close = c == '(' ? ')' : ']';
            v.kind = c == '(' ? PyValue::Kind::Tuple : PyValue::Kind::List;
            ++p;
            while (!consume(close)) {
                v.items.push_back(parseValue());
                if (!consume(',')) {
                    expect(close);
                    break;
                }
            }
            return v;
        }
        if (c == '{') {
            v.kind = PyValue::Kind::Dict;
            ++p;
            while (!consume('}')) {
                PyValue key = parseValue();
                if (key.kind != PyValue::Kind::Str) throw std::runtime_error("malformed .npy header: non-string key");
                expect(':');
                v.entries.emplace_back(key.text, parseValue());
                if (!consume(',')) {
                    expect('}');
                    break;
                }
            }
            return v;
        }
        if (word("True")) {
            v.kind = PyValue::Kind::Bool;
            v.boolean = true;
            return v;
        }
        if (word("False")) {
            v.kind = PyValue::Kind::Bool;
            return v;
        }
        if (word("None")) return v;
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            const size_t start = p;
            if (c == '-') ++p;
            while (p < s.size() && std::isdigit(static_cast<unsigned char>(s[p]))) ++p;
            const std::string digits = s.substr(start, p - start);
            if (p < s.size() && (s[p] == 'L' || s[p] == 'l')) ++p;
            errno = 0;
            v.kind = PyValue::Kind::Int;
            v.integer = std::strtoll(digits.c_str(), nullptr, 10);
            if (errno == ERANGE || digits == "-") throw std::runtime_error("bad integer in .npy header");
            return v;
        }
        throw std::runtime_error("malformed .npy header");
    }

    std::string parseString() {
        const char quote = s[p++];
        std::string out;
        while (p < s.size() && s[p] != quote) {
            char c = s[p++];
            if (c == '\\' && p < s.size()) {
                const char e = s[p++];
                switch (e) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '0': c = '\0'; break;
                default: c = e; break;
                }
            }
            out.push_back(c);
        }
        if (p >= s.size()) throw std::runtime_error("unterminated string in .npy header");
        ++p;
        return out;
    }
};

struct Field {
    std::string name;
    char kind = 'f';
    size_t size = 8;
    size_t offset = 0;
    bool bigEndian = false;
    bool skip = false;
    ColType out = ColType::Double;
};

bool hostBigEndian() {
    const unsigned short probe = 1;
    unsigned char first;
    std::memcpy(&first, &probe, 1);
    return first == 0;
}

Field parseTypeString(const std::string& descr) {
    Field f;
    size_t i = 0;
    if (i < descr.size() && (descr[i] == '<' || descr[i] == '>' || descr[i] == '|' || descr[i] == '=')) {
        f.bigEndian = descr[i] == '>' || (descr[i] == '=' && hostBigEndian());
        ++i;
    }
    if (i >= descr.size()) throw std::runtime_error("empty dtype in .npy header");
    f.kind = descr[i++];
    if (f.kind == 'O') throw std::runtime_error("object arrays (dtype 'O') need pickle and are not supported");
    size_t count = 0;
    const size_t digitsStart = i;
    while (i < descr.size() && std::isdigit(static_cast<unsigned char>(descr[i]))) {
        if (count > (std::numeric_limits<size_t>::max() / 4 - 9) / 10) throw std::runtime_error("dtype size too large in .npy header: " + descr);
        count = count * 10 + static_cast<size_t>(descr[i++] - '0');
    }
    if (i == digitsStart) throw std::runtime_error("dtype without size in .npy header: " + descr);
    if (i < descr.size() && descr[i] != '[') throw std::runtime_error("unsupported .npy dtype: " + descr);
    f.size = f.kind == 'U' ? count * 4 : count;
    switch (f.kind) {
    case 'b':
        if (f.size != 1) throw std::runtime_error("unsupported .npy dtype: " + descr);
        f.out = ColType::Bool;
        break;
    case 'i':
    case 'u':
        if (f.size != 1 && f.size != 2 && f.size != 4 && f.size != 8) throw std::runtime_error("unsupported .npy dtype: " + descr);
        f.out = ColType::Long;
        break;
    case 'f':
        if (f.size != 2 && f.size != 4 && f.size != 8) throw std::runtime_error("unsupported .npy dtype: " + descr);
        f.out = ColType::Double;
        break;
    case 'M':
    case 'm':
        if (f.size != 8) throw std::runtime_error("unsupported .npy dtype: " + descr);
        f.out = ColType::Long;
        break;
    case 'S':
    case 'U':
        f.out = ColType::String;
        break;
    case 'V':
        f.skip = true;
        break;
    case 'c': throw std::runtime_error("complex .npy dtypes are not supported: " + descr);
    default: throw std::runtime_error("unsupported .npy dtype: " + descr);
    }
    return f;
}

std::vector<Field> parseFields(const PyValue& descr, bool& structured) {
    std::vector<Field> fields;
    if (descr.kind == PyValue::Kind::Str) {
        structured = false;
        Field f = parseTypeString(descr.text);
        if (f.skip) throw std::runtime_error("void .npy dtypes are not supported: " + descr.text);
        fields.push_back(f);
        return fields;
    }
    if (descr.kind != PyValue::Kind::List) throw std::runtime_error("malformed 'descr' in .npy header");
    structured = true;
    size_t offset = 0;
    for (const auto& item : descr.items) {
        if (item.kind != PyValue::Kind::Tuple || item.items.size() < 2) throw std::runtime_error("malformed field in .npy header");
        const PyValue& nameValue = item.items[0];
        std::string name;
        if (nameValue.kind == PyValue::Kind::Str) name = nameValue.text;
        else if (nameValue.kind == PyValue::Kind::Tuple && nameValue.items.size() == 2 && nameValue.items[1].kind == PyValue::Kind::Str) name = nameValue.items[1].text;
        else throw std::runtime_error("malformed field name in .npy header");
        if (item.items[1].kind != PyValue::Kind::Str) throw std::runtime_error("nested structured field '" + name + "' is not supported");
        if (item.items.size() > 2) {
            const PyValue& shape = item.items[2];
            bool scalar = false;
            if (shape.sequence()) {
                scalar = true;
                for (const auto& d : shape.items) scalar = scalar && d.kind == PyValue::Kind::Int && d.integer == 1;
            } else if (shape.kind == PyValue::Kind::Int) {
                scalar = shape.integer == 1;
            }
            if (!scalar) throw std::runtime_error("sub-array field '" + name + "' is not supported");
        }
        Field f = parseTypeString(item.items[1].text);
        f.name = name;
        f.offset = offset;
        if (f.size > std::numeric_limits<size_t>::max() - offset) throw std::runtime_error("structured .npy dtype too large");
        offset += f.size;
        if (name.empty() && f.kind == 'V') f.skip = true;
        else if (f.skip) throw std::runtime_error("void field '" + name + "' is not supported");
        fields.push_back(f);
    }
    return fields;
}

unsigned long long readUnsigned(const unsigned char* p, size_t size, bool big) {
    unsigned long long v = 0;
    for (size_t k = 0; k < size; ++k) {
        const unsigned char b = big ? p[k] : p[size - 1 - k];
        v = (v << 8) | b;
    }
    return v;
}

void appendUtf8(std::string& out, unsigned long cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

bool decodeCell(const Field& f, const unsigned char* p, Column& column, std::string& error) {
    switch (f.kind) {
    case 'b':
        column.pushBool(p[0] != 0);
        return true;
    case 'i': {
        const unsigned long long raw = readUnsigned(p, f.size, f.bigEndian);
        const unsigned bits = static_cast<unsigned>(f.size * 8);
        long long v;
        if (bits == 64) {
            std::memcpy(&v, &raw, sizeof v);
        } else {
            const unsigned long long sign = 1ULL << (bits - 1);
            v = static_cast<long long>(raw & (sign - 1)) - static_cast<long long>(raw & sign);
        }
        column.pushLong(v);
        return true;
    }
    case 'u': {
        const unsigned long long raw = readUnsigned(p, f.size, f.bigEndian);
        if (raw > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            error = "uint64 value " + std::to_string(raw) + " does not fit in a long";
            return false;
        }
        column.pushLong(static_cast<long long>(raw));
        return true;
    }
    case 'M':
    case 'm': {
        const unsigned long long raw = readUnsigned(p, 8, f.bigEndian);
        long long v;
        std::memcpy(&v, &raw, sizeof v);
        if (v == std::numeric_limits<long long>::min()) column.pushNull();
        else column.pushLong(v);
        return true;
    }
    case 'f': {
        const unsigned long long raw = readUnsigned(p, f.size, f.bigEndian);
        if (f.size == 8) {
            double d;
            std::memcpy(&d, &raw, sizeof d);
            column.pushDouble(d);
        } else if (f.size == 4) {
            const unsigned int bits = static_cast<unsigned int>(raw);
            float x;
            std::memcpy(&x, &bits, sizeof x);
            column.pushDouble(static_cast<double>(x));
        } else {
            std::string le(2, '\0');
            le[0] = static_cast<char>(raw & 0xFF);
            le[1] = static_cast<char>((raw >> 8) & 0xFF);
            column.pushDouble(halfToDouble(le));
        }
        return true;
    }
    case 'S': {
        size_t n = f.size;
        while (n > 0 && p[n - 1] == 0) --n;
        column.pushString(std::string(reinterpret_cast<const char*>(p), n));
        return true;
    }
    case 'U': {
        size_t n = f.size / 4;
        while (n > 0 && readUnsigned(p + (n - 1) * 4, 4, f.bigEndian) == 0) --n;
        std::string out;
        out.reserve(n);
        for (size_t k = 0; k < n; ++k) {
            const unsigned long long cp = readUnsigned(p + k * 4, 4, f.bigEndian);
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                error = "invalid code point in a unicode .npy string";
                return false;
            }
            appendUtf8(out, static_cast<unsigned long>(cp));
        }
        column.pushString(std::move(out));
        return true;
    }
    default:
        error = "unsupported .npy dtype";
        return false;
    }
}

long long readNpy(const std::string& path) {
    std::string data;
    if (!readFile(path, data)) return fail("cannot read " + path);
    const auto* bytes = reinterpret_cast<const unsigned char*>(data.data());
    if (data.size() < 10 || data.compare(0, 6, "\x93NUMPY") != 0) return fail(path + ": not a .npy file");
    const int major = bytes[6];
    size_t headerLength;
    size_t headerStart;
    if (major == 1) {
        headerLength = static_cast<size_t>(readUnsigned(bytes + 8, 2, false));
        headerStart = 10;
    } else if (major == 2 || major == 3) {
        if (data.size() < 12) return fail(path + ": truncated .npy header");
        headerLength = static_cast<size_t>(readUnsigned(bytes + 8, 4, false));
        headerStart = 12;
    } else {
        return fail(path + ": unsupported .npy format version " + std::to_string(major));
    }
    if (headerLength > data.size() - headerStart) return fail(path + ": truncated .npy header");

    const PyValue header = LiteralParser(data.substr(headerStart, headerLength)).parseAll();
    if (header.kind != PyValue::Kind::Dict) return fail(path + ": .npy header is not a dict");
    const PyValue* descr = header.get("descr");
    const PyValue* order = header.get("fortran_order");
    const PyValue* shapeValue = header.get("shape");
    if (!descr || !order || order->kind != PyValue::Kind::Bool || !shapeValue || shapeValue->kind != PyValue::Kind::Tuple) {
        return fail(path + ": .npy header lacks descr, fortran_order, or shape");
    }

    std::vector<size_t> shape;
    for (const auto& d : shapeValue->items) {
        if (d.kind != PyValue::Kind::Int || d.integer < 0) return fail(path + ": bad shape in .npy header");
        shape.push_back(static_cast<size_t>(d.integer));
    }
    if (shape.size() > 2) return fail(path + ": .npy arrays with more than 2 dimensions cannot become a frame");

    bool structured = false;
    const std::vector<Field> fields = parseFields(*descr, structured);
    if (structured && shape.size() > 1) return fail(path + ": 2-D structured .npy arrays are not supported");
    size_t itemSize = 0;
    for (const auto& f : fields) {
        if (f.size > std::numeric_limits<size_t>::max() - itemSize) return fail(path + ": .npy dtype too large");
        itemSize += f.size;
    }

    const size_t rows = shape.empty() ? 1 : shape[0];
    const size_t width = shape.size() == 2 ? shape[1] : 1;
    if (width > (size_t(1) << 20)) return fail(path + ": .npy array has too many columns");
    size_t count = rows;
    if (width != 0 && count > std::numeric_limits<size_t>::max() / width) return fail(path + ": .npy array too large");
    count *= width;
    if (itemSize == 0 && count > 0) return fail(path + ": .npy dtype has no bytes");
    if (itemSize != 0 && count > std::numeric_limits<size_t>::max() / itemSize) return fail(path + ": .npy array too large");
    const size_t dataStart = headerStart + headerLength;
    if (count * itemSize > data.size() - dataStart) return fail(path + ": .npy data is shorter than its shape");
    const unsigned char* base = bytes + dataStart;
    const bool fortran = order->boolean;

    struct Target {
        const Field* field;
        size_t columnIndex;
        std::string name;
    };
    std::vector<Target> targets;
    if (structured) {
        for (const auto& f : fields) {
            if (!f.skip) targets.push_back({&f, 0, f.name});
        }
    } else {
        for (size_t c = 0; c < width; ++c) targets.push_back({&fields[0], c, "column_" + std::to_string(c)});
    }

    DataFrame frame;
    frame.columns.resize(targets.size());
    std::vector<std::string> errors(targets.size());
    parallelFor(targets.size(), count, [&](size_t k) {
        const Target& t = targets[k];
        Column column(t.name, t.field->out);
        column.reserve(rows);
        for (size_t r = 0; r < rows; ++r) {
            size_t element;
            if (structured) element = r;
            else element = fortran ? t.columnIndex * rows + r : r * width + t.columnIndex;
            const unsigned char* cell = base + element * itemSize + t.field->offset;
            if (!decodeCell(*t.field, cell, column, errors[k])) return;
        }
        frame.columns[k] = std::move(column);
    });
    for (size_t k = 0; k < targets.size(); ++k) {
        if (!errors[k].empty()) return fail(path + ": column " + targets[k].name + ": " + errors[k]);
    }
    return store(std::move(frame));
}

} // namespace npy
} // namespace csec_df

extern "C" {

long long csec_df_read_npy(const char* path) {
    try {
        return csec_df::npy::readNpy(path ? path : "");
    } catch (const std::exception& e) {
        return csec_df::fail(std::string("reading .npy failed: ") + e.what());
    }
}

}
