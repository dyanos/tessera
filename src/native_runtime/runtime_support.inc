namespace {
int g_argc = 0;
char** g_argv = nullptr;

std::string llvmStringGlobal(const std::string& name, const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string escaped;
    escaped.reserve(value.size() * 2 + 4);
    for (unsigned char ch : value) {
        if (ch >= 32 && ch <= 126 && ch != '"' && ch != '\\') {
            escaped.push_back(static_cast<char>(ch));
        } else {
            escaped.push_back('\\');
            escaped.push_back(hex[ch >> 4]);
            escaped.push_back(hex[ch & 0x0f]);
        }
    }
    escaped += "\\00";
    return "@" + name + " = private unnamed_addr constant [" +
        std::to_string(value.size() + 1) + " x i8] c\"" + escaped + "\"\n";
}

struct TokenBuilder {
    char* data = nullptr;
    size_t length = 0;
    size_t capacity = 0;
};

struct StringBuilder {
    char* data = nullptr;
    size_t length = 0;
    size_t capacity = 0;
    FILE* file = nullptr;
};

struct FunctionRange {
    int declStart = -1;
    int bodyStart = -1;
    int bodyEnd = -1;
};

constexpr size_t kStringBuilderGrowQuantum = 10 * 1024;

size_t roundStringBuilderCapacity(size_t needed) {
    size_t capacity = kStringBuilderGrowQuantum;
    if (needed > capacity) {
        size_t remainder = needed % kStringBuilderGrowQuantum;
        capacity = remainder == 0
            ? needed
            : needed + (kStringBuilderGrowQuantum - remainder);
    }
    return capacity;
}

std::unordered_map<const char*, std::vector<int>>& tokenLineCache() {
    static std::unordered_map<const char*, std::vector<int>> cache;
    return cache;
}

std::unordered_map<const char*, std::vector<std::string>>& tokenTextCache() {
    static std::unordered_map<const char*, std::vector<std::string>> cache;
    return cache;
}

std::unordered_map<const char*, std::vector<FunctionRange>>& tokenFunctionRangeCache() {
    static std::unordered_map<const char*, std::vector<FunctionRange>> cache;
    return cache;
}

std::unordered_map<const char*, std::unordered_map<std::string, std::string>>& tokenFunctionReturnTypeCache() {
    static std::unordered_map<const char*, std::unordered_map<std::string, std::string>> cache;
    return cache;
}

void clearTokenCachesFor(const char* tokens) {
    if (!tokens) return;
    tokenLineCache().erase(tokens);
    tokenTextCache().erase(tokens);
    tokenFunctionRangeCache().erase(tokens);
    tokenFunctionReturnTypeCache().erase(tokens);
}

void clearAllTokenCaches() {
    tokenLineCache().clear();
    tokenTextCache().clear();
    tokenFunctionRangeCache().clear();
    tokenFunctionReturnTypeCache().clear();
}

const std::vector<int>& tokenLineStarts(const char* tokens) {
    const char* value = tokens ? tokens : "";
    auto& cache = tokenLineCache();
    auto found = cache.find(value);
    if (found != cache.end()) {
        return found->second;
    }

    std::vector<int> starts;
    starts.push_back(0);
    for (int i = 0; value[i] != '\0'; ++i) {
        if (value[i] == '\n' && value[i + 1] != '\0') {
            starts.push_back(i + 1);
        }
    }
    auto inserted = cache.emplace(value, std::move(starts));
    return inserted.first->second;
}

const std::vector<std::string>& tokenTexts(const char* tokens) {
    const char* value = tokens ? tokens : "";
    auto& cache = tokenTextCache();
    auto found = cache.find(value);
    if (found != cache.end()) {
        return found->second;
    }

    const auto& starts = tokenLineStarts(value);
    std::vector<std::string> texts;
    texts.reserve(starts.size());
    for (int start : starts) {
        int textStart = start + 2;
        int end = textStart;
        while (value[end] != '\0' && value[end] != '\n') {
            ++end;
        }
        if (textStart > end) textStart = end;
        texts.emplace_back(value + textStart, static_cast<size_t>(end - textStart));
    }

    auto inserted = cache.emplace(value, std::move(texts));
    return inserted.first->second;
}

char tokenKindCached(const char* tokens, int ordinal) {
    const char* value = tokens ? tokens : "";
    if (ordinal < 0) return 'E';
    const auto& starts = tokenLineStarts(value);
    if (static_cast<size_t>(ordinal) >= starts.size()) return 'E';
    int start = starts[static_cast<size_t>(ordinal)];
    return value[start] == '\0' ? 'E' : value[start];
}

const std::string& tokenTextCached(const char* tokens, int ordinal) {
    static const std::string empty;
    if (ordinal < 0) return empty;
    const auto& texts = tokenTexts(tokens ? tokens : "");
    if (static_cast<size_t>(ordinal) >= texts.size()) return empty;
    return texts[static_cast<size_t>(ordinal)];
}

int tokenCountCached(const char* tokens) {
    const auto& starts = tokenLineStarts(tokens ? tokens : "");
    return static_cast<int>(starts.size());
}

int findMatchingBraceToken(const char* tokens, int openBrace, int tokenCount) {
    int depth = 0;
    for (int cursor = openBrace; cursor < tokenCount; ++cursor) {
        if (tokenKindCached(tokens, cursor) != 'O') continue;
        const auto& text = tokenTextCached(tokens, cursor);
        if (text == "{") {
            ++depth;
        } else if (text == "}") {
            --depth;
            if (depth == 0) return cursor;
        }
    }
    return -1;
}

bool findFunctionAroundLimit(const char* tokens, int limit, int* declStart, int* bodyStart, int* bodyEnd) {
    const char* value = tokens ? tokens : "";
    auto& cache = tokenFunctionRangeCache();
    auto found = cache.find(value);
    if (found == cache.end()) {
        int tokenCount = tokenCountCached(value);
        std::vector<FunctionRange> ranges;
        for (int cursor = 0; cursor < tokenCount; ++cursor) {
            if (tokenKindCached(value, cursor) != 'K' || tokenTextCached(value, cursor) != "def") {
                continue;
            }

            int openBrace = -1;
            for (int probe = cursor + 1; probe < tokenCount; ++probe) {
                if (tokenKindCached(value, probe) != 'O') continue;
                const auto& text = tokenTextCached(value, probe);
                if (text == "{") {
                    openBrace = probe;
                    break;
                }
                if (text == ";") break;
            }
            if (openBrace < 0) continue;

            int closeBrace = findMatchingBraceToken(value, openBrace, tokenCount);
            if (closeBrace < 0) continue;
            ranges.push_back(FunctionRange{cursor, openBrace + 1, closeBrace});
            if (cursor < closeBrace) cursor = closeBrace;
        }
        found = cache.emplace(value, std::move(ranges)).first;
    }

    if (limit < 0) return false;
    const auto& ranges = found->second;
    int left = 0;
    int right = static_cast<int>(ranges.size()) - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        const auto& range = ranges[static_cast<size_t>(mid)];
        if (limit >= range.bodyStart && limit < range.bodyEnd) {
            if (declStart) *declStart = range.declStart;
            if (bodyStart) *bodyStart = range.bodyStart;
            if (bodyEnd) *bodyEnd = range.bodyEnd;
            return true;
        }
        if (limit < range.bodyStart) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    return false;
}

int findMatchingParenToken(const char* tokens, int openParen, int tokenCount) {
    int depth = 0;
    for (int cursor = openParen; cursor < tokenCount; ++cursor) {
        if (tokenKindCached(tokens, cursor) != 'O') continue;
        const auto& text = tokenTextCached(tokens, cursor);
        if (text == "(") {
            ++depth;
        } else if (text == ")") {
            --depth;
            if (depth == 0) return cursor;
        }
    }
    return -1;
}

std::string collectFunctionReturnTypeCached(const char* tokens, int typeStart, int tokenCount) {
    std::string output;
    for (int cursor = typeStart; cursor < tokenCount; ++cursor) {
        const auto& text = tokenTextCached(tokens, cursor);
        if (text == "{" || text == "=" || text == ";" || text == ",") {
            break;
        }
        if (!output.empty()) output += " ";
        output += text;
    }
    return output.empty() ? "Unit" : output;
}

const std::unordered_map<std::string, std::string>& functionReturnTypesCached(const char* tokens) {
    const char* value = tokens ? tokens : "";
    auto& cache = tokenFunctionReturnTypeCache();
    auto found = cache.find(value);
    if (found != cache.end()) {
        return found->second;
    }

    int tokenCount = tokenCountCached(value);
    std::unordered_map<std::string, std::string> types;
    for (int cursor = 0; cursor < tokenCount; ++cursor) {
        int defToken = -1;
        if (tokenKindCached(value, cursor) == 'K' && tokenTextCached(value, cursor) == "def") {
            defToken = cursor;
        } else if (tokenKindCached(value, cursor) == 'K' && tokenTextCached(value, cursor) == "external" &&
                   cursor + 1 < tokenCount && tokenKindCached(value, cursor + 1) == 'K' &&
                   tokenTextCached(value, cursor + 1) == "def") {
            defToken = cursor + 1;
        }
        if (defToken < 0 || defToken + 1 >= tokenCount) {
            continue;
        }
        if (tokenKindCached(value, defToken + 1) != 'I') {
            continue;
        }

        std::string name = tokenTextCached(value, defToken + 1);
        int openParen = -1;
        for (int probe = defToken + 2; probe < tokenCount; ++probe) {
            const auto& text = tokenTextCached(value, probe);
            if (tokenKindCached(value, probe) == 'O' && text == "(") {
                openParen = probe;
                break;
            }
            if (tokenKindCached(value, probe) == 'O' && (text == "{" || text == ";" || text == "=")) {
                break;
            }
        }
        if (openParen < 0) {
            continue;
        }
        int closeParen = findMatchingParenToken(value, openParen, tokenCount);
        if (closeParen < 0) {
            continue;
        }
        std::string returnType = "Unit";
        if (closeParen + 2 < tokenCount && tokenKindCached(value, closeParen + 1) == 'O' &&
            tokenTextCached(value, closeParen + 1) == ":") {
            returnType = collectFunctionReturnTypeCached(value, closeParen + 2, tokenCount);
        }
        types[name] = returnType;
    }

    return cache.emplace(value, std::move(types)).first->second;
}

int& configuredParallelThreads() {
    static int value = []() {
        unsigned int hardware = std::thread::hardware_concurrency();
        return hardware == 0 ? 1 : static_cast<int>(hardware);
    }();
    return value;
}

void setConfiguredParallelThreads(int count) {
    if (count < 1) count = 1;
    configuredParallelThreads() = count;
}

#ifdef _WIN32
bool ensureSocketRuntime() {
    static bool initialized = []() {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    return initialized;
}

void closeSocketHandle(SOCKET socketHandle) {
    closesocket(socketHandle);
}
#else
using NativeSocket = int;
constexpr NativeSocket INVALID_SOCKET_HANDLE = -1;

void closeSocketHandle(NativeSocket socketHandle) {
    close(socketHandle);
}
#endif

std::string resolveSystemLibraryPath(const char* path) {
    if (!path) return "";
    std::string library(path);
    if (library.rfind("System.", 0) != 0) {
        return library;
    }

    const bool hasExtension =
        (library.size() >= 4 && library.compare(library.size() - 4, 4, ".dll") == 0) ||
        (library.size() >= 3 && library.compare(library.size() - 3, 3, ".so") == 0) ||
        (library.size() >= 4 && library.compare(library.size() - 4, 4, ".lib") == 0) ||
        (library.size() >= 6 && library.compare(library.size() - 6, 6, ".dylib") == 0);
    if (hasExtension) {
        return library;
    }

#ifdef _WIN32
    return library + ".dll";
#elif defined(__APPLE__)
    return "lib" + library + ".dylib";
#else
    return library + ".so";
#endif
}
}

extern "C" {

// A failed assert(...): `where` is "file:line", `message` the condition text (and the user message).
void csec_assert_fail(const char* where, const char* message) {
    std::fflush(stdout);
    if (where) std::fprintf(stderr, "AssertionError: %s: %s\n", where, message ? message : "");
    else std::fprintf(stderr, "AssertionError: %s\n", message ? message : "assertion failed");
    std::fflush(stderr);
    std::exit(1);
}

void csec_print_string(const char* value) {
    std::printf("%s", value ? value : "");
}

void csec_print_i64(long long value) {
    std::printf("%lld", value);
}

void csec_print_double(double value) {
    std::printf("%f", value);
}

void csec_print_bool(int value) {
    std::printf("%d", value ? 1 : 0);
}

void csec_print_char(char value) {
    std::printf("%c", value);
}

void csec_print_newline(void) {
    std::printf("\n");
}

char* csec_string_concat(const char* left, const char* right) {
    const char* lhs = left ? left : "";
    const char* rhs = right ? right : "";
    size_t lhsLen = std::strlen(lhs);
    size_t rhsLen = std::strlen(rhs);
    char* result = static_cast<char*>(std::malloc(lhsLen + rhsLen + 1));
    if (!result) return nullptr;
    std::memcpy(result, lhs, lhsLen);
    std::memcpy(result + lhsLen, rhs, rhsLen);
    result[lhsLen + rhsLen] = '\0';
    return result;
}

// Concatenate a complete expression chain in one allocation. Repeated binary
// concatenation recopies each prefix and becomes quadratic as a chain grows.
char* csec_string_concat_many(const char* const* values, long long count) {
    if (!values || count <= 0) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t total = 0;
    for (long long i = 0; i < count; ++i) {
        const char* value = values[i] ? values[i] : "";
        const size_t length = std::strlen(value);
        if (length > static_cast<size_t>(-1) - total - 1) return nullptr;
        total += length;
    }

    char* result = static_cast<char*>(std::malloc(total + 1));
    if (!result) return nullptr;
    char* cursor = result;
    for (long long i = 0; i < count; ++i) {
        const char* value = values[i] ? values[i] : "";
        const size_t length = std::strlen(value);
        std::memcpy(cursor, value, length);
        cursor += length;
    }
    *cursor = '\0';
    return result;
}

void csec_release_concat_strings(void) {
    clearAllTokenCaches();
}

int csec_lex_quoted(const char* source, int index) {
    if (!source || index < 0 || source[index] == '\0') return index;
    const char quote = source[index];
    int cursor = index + 1;
    while (source[cursor] != '\0' && source[cursor] != quote) {
        if (source[cursor] == '\\' && source[cursor + 1] != '\0') {
            cursor += 2;
        } else {
            ++cursor;
        }
    }
    return source[cursor] == quote ? cursor + 1 : cursor;
}

int csec_lex_number(const char* source, int index) {
    if (!source || index < 0) return index;
    int cursor = index;
    if (source[cursor] == '0' && (source[cursor + 1] == 'x' || source[cursor + 1] == 'X' ||
        source[cursor + 1] == 'b' || source[cursor + 1] == 'B' || source[cursor + 1] == 'o' || source[cursor + 1] == 'O')) cursor += 2;
    while (std::isalnum(static_cast<unsigned char>(source[cursor])) || source[cursor] == '_' || source[cursor] == '.') ++cursor;
    return cursor;
}

int csec_lex_identifier(const char* source, int index) {
    if (!source || index < 0) return index;
    int cursor = index + 1;
    while (std::isalnum(static_cast<unsigned char>(source[cursor])) || source[cursor] == '_') ++cursor;
    return cursor;
}

int csec_operator_length(const char* source, int index) {
    if (!source || index < 0 || source[index] == '\0') return 0;
    const char first = source[index];
    const char second = source[index + 1];
    const char third = source[index + 2];
    if ((first == '<' || first == '>') && second == first && third == '|') return 3;
    if (first == '[' && second == '@') return 2;
    const bool pair =
        (first == '=' && (second == '>' || second == '=')) ||
        (first == '<' && (second == '-' || second == '=' || second == '<')) ||
        (first == '>' && (second == '=' || second == '>')) ||
        (first == '!' && second == '=') ||
        (first == '+' && (second == '+' || second == '=')) ||
        (first == '-' && (second == '-' || second == '=' || second == '>')) ||
        (first == '*' && (second == '*' || second == '=')) ||
        (first == '/' && second == '=') || (first == '%' && second == '=') ||
        (first == '&' && second == '&') || (first == '|' && second == '|') ||
        (first == '.' && second == '.') || (first == '$' && second == '$');
    return pair ? 2 : 1;
}

int csec_digit_value(char ch) {
    return ch >= '0' && ch <= '9' ? ch - '0' : 0;
}

int csec_is_keyword(const char* text) {
    if (!text) return 0;
    static const char* const keywords[] = {
        "import", "class", "extends", "object", "external", "def", "operator", "override", "return",
        "val", "var", "if", "else", "for", "while", "match", "case", "map", "pmap", "filter",
        "reduce", "preduce", "template", "typename", "new", "this", "super", "unsafe", "unatomic",
        "constexpr", "box", "mut", "null", "inner", "outer", "tensor", "to", "until", "and", "or",
        "xor", "ode", "molecule", "cfd", "protein", "cpu", "openmp", "gpu", "simd", "build",
        "async", "await", "go"
    };
    for (const char* keyword : keywords) {
        if (std::strcmp(text, keyword) == 0) return 1;
    }
    return 0;
}

int csec_to_int(const char* text) {
    return text ? static_cast<int>(std::strtol(text, nullptr, 10)) : 0;
}

int csec_line_start(const char* text, int ordinal) {
    if (!text || ordinal <= 0) return 0;
    int cursor = 0;
    int line = 0;
    while (text[cursor] && line < ordinal) {
        if (text[cursor] == '\n') ++line;
        ++cursor;
    }
    return cursor;
}

int csec_line_end(const char* text, int start) {
    if (!text || start < 0) return 0;
    int cursor = start;
    while (text[cursor] && text[cursor] != '\n') ++cursor;
    return cursor;
}

int csec_validate_balanced(const char* tokens) {
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    for (int ordinal = 0; csec_token_kind_at(tokens, ordinal) != 'E'; ++ordinal) {
        if (csec_token_kind_at(tokens, ordinal) != 'O') continue;
        const char* text = csec_token_text_at(tokens, ordinal);
        if (!text) continue;
        if (std::strcmp(text, "(") == 0) ++paren;
        else if (std::strcmp(text, ")") == 0) --paren;
        else if (std::strcmp(text, "{") == 0) ++brace;
        else if (std::strcmp(text, "}") == 0) --brace;
        else if (std::strcmp(text, "[") == 0) ++bracket;
        else if (std::strcmp(text, "]") == 0) --bracket;
        if (paren < 0 || brace < 0 || bracket < 0) return 0;
    }
    return paren == 0 && brace == 0 && bracket == 0;
}

std::string llvmGenerateIRAssignmentDefinition() {
    return R"IR(define ptr @generateIRAssignment(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %valid.op = icmp sgt i32 %op, %arg.start
  br i1 %valid.op, label %kind, label %invalid
kind:
  %kind.value = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %arg.start)
  %is.identifier = icmp eq i8 %kind.value, 73
  br i1 %is.identifier, label %assignment, label %invalid
assignment:
  %name = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %arg.start)
  %op.text = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %op)
  %rhs.start = add i32 %op, 1
  %rhs = call ptr @generateIRExpression(ptr %arg.tokens, i32 %rhs.start, i32 %arg.end)
  %equals.raw = call i32 @csec_string_equals(ptr %op.text, ptr @.str.ir.assign.equals)
  %left.raw = call i32 @csec_string_equals(ptr %op.text, ptr @.str.ir.assign.left)
  %equals = icmp ne i32 %equals.raw, 0
  %left = icmp ne i32 %left.raw, 0
  %is.store = or i1 %equals, %left
  br i1 %is.store, label %store, label %compound
store:
  %s1 = call ptr @csec_string_concat(ptr @.str.ir.assign.store, ptr %rhs)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.ir.assign.ptr)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %name)
  %s4 = call ptr @csec_string_concat(ptr %s3, ptr @.str.ir.assign.nl)
  ret ptr %s4
compound:
  %type.name = call ptr @inferExpressionType(ptr %arg.tokens, i32 %rhs.start, i32 %arg.end)
  %ir.type = call ptr @irTypeName(ptr %type.name)
  %operator = call ptr @csec_string_substring(ptr %op.text, i32 0, i32 1)
  %binary = call ptr @irOperatorName(ptr %operator)
  %a1 = call ptr @csec_string_concat(ptr @.str.ir.assign.old, ptr %name)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.ir.assign.load)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %ir.type)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.ir.assign.ptr)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %name)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.ir.assign.nl)
  %b1 = call ptr @csec_string_concat(ptr @.str.ir.assign.new, ptr %name)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.ir.assign.eqspace)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr %binary)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr @.str.ir.expr.space)
  %b5 = call ptr @csec_string_concat(ptr %b4, ptr %ir.type)
  %b6 = call ptr @csec_string_concat(ptr %b5, ptr @.str.ir.assign.oldref)
  %b7 = call ptr @csec_string_concat(ptr %b6, ptr %name)
  %b8 = call ptr @csec_string_concat(ptr %b7, ptr @.str.ir.expr.middle)
  %trimmed = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %rhs.start, i32 %arg.end)
  %raw = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %rhs.start, i32 %trimmed)
  %b9 = call ptr @csec_string_concat(ptr %b8, ptr %raw)
  %b10 = call ptr @csec_string_concat(ptr %b9, ptr @.str.ir.expr.close)
  %b11 = call ptr @csec_string_concat(ptr %b10, ptr @.str.ir.assign.nl)
  %c1 = call ptr @csec_string_concat(ptr @.str.ir.assign.store, ptr %ir.type)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.ir.assign.newref)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %name)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr @.str.ir.assign.ptr)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr %name)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr @.str.ir.assign.nl)
  %result1 = call ptr @csec_string_concat(ptr %a6, ptr %b11)
  %result2 = call ptr @csec_string_concat(ptr %result1, ptr %c6)
  ret ptr %result2
invalid:
  ret ptr @.str.ir.assign.empty
}

@.str.ir.assign.empty = private unnamed_addr constant [1 x i8] c"\00"
@.str.ir.assign.equals = private unnamed_addr constant [2 x i8] c"=\00"
@.str.ir.assign.left = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.ir.assign.store = private unnamed_addr constant [11 x i8] c"    store \00"
@.str.ir.assign.ptr = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.ir.assign.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.ir.assign.old = private unnamed_addr constant [10 x i8] c"    %old.\00"
@.str.ir.assign.load = private unnamed_addr constant [9 x i8] c" = load \00"
@.str.ir.assign.new = private unnamed_addr constant [10 x i8] c"    %new.\00"
@.str.ir.assign.eqspace = private unnamed_addr constant [4 x i8] c" = \00"
@.str.ir.assign.oldref = private unnamed_addr constant [7 x i8] c" %old.\00"
@.str.ir.assign.newref = private unnamed_addr constant [7 x i8] c" %new.\00"

)IR";
}

std::string llvmGenerateIRIfDefinition() {
    return R"IR(define ptr @generateIRIf(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %then.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %then.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %then.open, i32 %arg.end, ptr @.str.ir.if.brace.open, ptr @.str.ir.if.brace.close)
  %open.ok = icmp sge i32 %open, 0
  %close.ok = icmp sgt i32 %close, %open
  %then.open.ok = icmp sge i32 %then.open, 0
  %then.close.ok = icmp sgt i32 %then.close, %then.open
  %ok.1 = and i1 %open.ok, %close.ok
  %ok.2 = and i1 %then.open.ok, %then.close.ok
  %ok = and i1 %ok.1, %ok.2
  br i1 %ok, label %build, label %malformed
malformed:
  ret ptr @.str.ir.if.malformed
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %after.then = add i32 %then.close, 1
  %possible.else = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.then)
  %before.end = icmp slt i32 %possible.else, %arg.end
  br i1 %before.end, label %else.test, label %else.none
else.test:
  %else.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %possible.else, i8 75, ptr @.str.ir.if.else.keyword)
  %else.bool = icmp ne i32 %else.raw, 0
  br label %else.join
else.none:
  br label %else.join
else.join:
  %has.else = phi i1 [ %else.bool, %else.test ], [ false, %else.none ]
  %expr.start = add i32 %open, 1
  %condition = call ptr @generateIRExpression(ptr %arg.tokens, i32 %expr.start, i32 %close)
  %then.start = add i32 %then.open, 1
  %then.body = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %then.start, i32 %then.close)
  %else.body = call ptr @generateIRElseFlatBody(ptr %arg.tokens, i32 %possible.else, i32 %arg.end, i1 %has.else)
  %a1 = call ptr @csec_string_concat(ptr @.str.ir.if.cond, ptr %seed)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.ir.if.equals)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %condition)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.ir.if.newline)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr @.str.ir.if.branch)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr %seed)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr @.str.ir.if.then)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr %seed)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr @.str.ir.if.else)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr %seed)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr @.str.ir.if.colon)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr %then.body)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr @.str.ir.if.end.branch)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr %seed)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr @.str.ir.if.newline)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr @.str.ir.if.else.label)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr %seed)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr @.str.ir.if.colon)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr %else.body)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr @.str.ir.if.end.branch)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr %seed)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr @.str.ir.if.newline)
  %a23 = call ptr @csec_string_concat(ptr %a22, ptr @.str.ir.if.end.label)
  %a24 = call ptr @csec_string_concat(ptr %a23, ptr %seed)
  %ret = call ptr @csec_string_concat(ptr %a24, ptr @.str.ir.if.colon)
  ret ptr %ret
}

@.str.ir.if.malformed = private unnamed_addr constant [20 x i8] c"    ; malformed if\0A\00"
@.str.ir.if.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.ir.if.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.ir.if.else.keyword = private unnamed_addr constant [5 x i8] c"else\00"
@.str.ir.if.cond = private unnamed_addr constant [11 x i8] c"    %cond.\00"
@.str.ir.if.equals = private unnamed_addr constant [4 x i8] c" = \00"
@.str.ir.if.newline = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.ir.if.branch = private unnamed_addr constant [17 x i8] c"    br i1 %cond.\00"
@.str.ir.if.then = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.ir.if.else = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.ir.if.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.ir.if.end.branch = private unnamed_addr constant [22 x i8] c"    br label %if.end.\00"
@.str.ir.if.else.label = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.ir.if.end.label = private unnamed_addr constant [8 x i8] c"if.end.\00"
)IR";
}

std::string llvmGenerateIRWhileDefinition() {
    return R"WHILELLVM(define ptr @generateIRWhile(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %body.open, i32 %arg.end, ptr @.str.ir.while.brace.open, ptr @.str.ir.while.brace.close)
  %open.ok = icmp sge i32 %open, 0
  %close.ok = icmp sgt i32 %close, %open
  %body.open.ok = icmp sge i32 %body.open, 0
  %body.close.ok = icmp sgt i32 %body.close, %body.open
  %ok.1 = and i1 %open.ok, %close.ok
  %ok.2 = and i1 %body.open.ok, %body.close.ok
  %ok = and i1 %ok.1, %ok.2
  br i1 %ok, label %build, label %bad
bad:
  ret ptr @.str.ir.while.bad
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %expr.start = add i32 %open, 1
  %expr = call ptr @generateIRExpression(ptr %arg.tokens, i32 %expr.start, i32 %close)
  %body.start = add i32 %body.open, 1
  %body = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %body.start, i32 %body.close)
  %a1 = call ptr @csec_string_concat(ptr @.str.ir.while.branch, ptr %seed)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.ir.while.nl)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr @.str.ir.while.cond.label)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr %seed)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr @.str.ir.while.colon)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.ir.while.value)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr %seed)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr @.str.ir.while.equals)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr %expr)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr @.str.ir.while.nl)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr @.str.ir.while.test)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr %seed)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr @.str.ir.while.body.label)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr %seed)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr @.str.ir.while.end.label)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr %seed)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr @.str.ir.while.nl)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr @.str.ir.while.body.name)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr %seed)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr @.str.ir.while.colon)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr %body)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr @.str.ir.while.branch)
  %a23 = call ptr @csec_string_concat(ptr %a22, ptr %seed)
  %a24 = call ptr @csec_string_concat(ptr %a23, ptr @.str.ir.while.nl)
  %a25 = call ptr @csec_string_concat(ptr %a24, ptr @.str.ir.while.end.name)
  %a26 = call ptr @csec_string_concat(ptr %a25, ptr %seed)
  %ret = call ptr @csec_string_concat(ptr %a26, ptr @.str.ir.while.colon)
  ret ptr %ret
}

@.str.ir.while.bad = private unnamed_addr constant [23 x i8] c"    ; malformed while\0A\00"
@.str.ir.while.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.ir.while.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.ir.while.branch = private unnamed_addr constant [26 x i8] c"    br label %while.cond.\00"
@.str.ir.while.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.ir.while.cond.label = private unnamed_addr constant [12 x i8] c"while.cond.\00"
@.str.ir.while.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.ir.while.value = private unnamed_addr constant [16 x i8] c"    %whilecond.\00"
@.str.ir.while.equals = private unnamed_addr constant [4 x i8] c" = \00"
@.str.ir.while.test = private unnamed_addr constant [22 x i8] c"    br i1 %whilecond.\00"
@.str.ir.while.body.label = private unnamed_addr constant [21 x i8] c", label %while.body.\00"
@.str.ir.while.end.label = private unnamed_addr constant [20 x i8] c", label %while.end.\00"
@.str.ir.while.body.name = private unnamed_addr constant [12 x i8] c"while.body.\00"
@.str.ir.while.end.name = private unnamed_addr constant [11 x i8] c"while.end.\00"
)WHILELLVM";
}

std::string llvmGenerateIRForDefinition() {
    return R"FORLLVM(define ptr @generateIRFor(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %body.open, i32 %arg.end, ptr @.str.ir.for.brace.open, ptr @.str.ir.for.brace.close)
  %open.ok = icmp sge i32 %open, 0
  %close.ok = icmp sgt i32 %close, %open
  %body.open.ok = icmp sge i32 %body.open, 0
  %body.close.ok = icmp sgt i32 %body.close, %body.open
  %ok.1 = and i1 %open.ok, %close.ok
  %ok.2 = and i1 %body.open.ok, %body.close.ok
  %ok = and i1 %ok.1, %ok.2
  br i1 %ok, label %arrow.find, label %malformed
malformed:
  ret ptr @.str.ir.for.malformed
arrow.find:
  %iterator.index = add i32 %open, 1
  %arrow = call i32 @findTokenTextInRange(ptr %arg.tokens, i32 %iterator.index, i32 %close, ptr @.str.ir.for.arrow)
  %arrow.ok = icmp sge i32 %arrow, 0
  br i1 %arrow.ok, label %build, label %source.malformed
source.malformed:
  ret ptr @.str.ir.for.source.malformed
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %iterator = call ptr @tokenTextAt(ptr %arg.tokens, i32 %iterator.index)
  %expr.start = add i32 %arrow, 1
  %expr = call ptr @generateIRExpression(ptr %arg.tokens, i32 %expr.start, i32 %close)
  %body.start = add i32 %body.open, 1
  %body = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %body.start, i32 %body.close)
  %a1 = call ptr @csec_string_concat(ptr @.str.ir.for.source, ptr %expr)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.ir.for.nl)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr @.str.ir.for.local)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr %iterator)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr @.str.ir.for.alloca)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.ir.for.branch)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr %seed)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr @.str.ir.for.nl)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr @.str.ir.for.cond)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr %seed)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr @.str.ir.for.colon)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr @.str.ir.for.next)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr %iterator)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr @.str.ir.for.nl)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr @.str.ir.for.test)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr %seed)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr @.str.ir.for.body.label)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr %seed)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr @.str.ir.for.end.label)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr %seed)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr @.str.ir.for.nl)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr @.str.ir.for.body)
  %a23 = call ptr @csec_string_concat(ptr %a22, ptr %seed)
  %a24 = call ptr @csec_string_concat(ptr %a23, ptr @.str.ir.for.colon)
  %a25 = call ptr @csec_string_concat(ptr %a24, ptr %body)
  %a26 = call ptr @csec_string_concat(ptr %a25, ptr @.str.ir.for.branch)
  %a27 = call ptr @csec_string_concat(ptr %a26, ptr %seed)
  %a28 = call ptr @csec_string_concat(ptr %a27, ptr @.str.ir.for.nl)
  %a29 = call ptr @csec_string_concat(ptr %a28, ptr @.str.ir.for.end)
  %a30 = call ptr @csec_string_concat(ptr %a29, ptr %seed)
  %ret = call ptr @csec_string_concat(ptr %a30, ptr @.str.ir.for.colon)
  ret ptr %ret
}

@.str.ir.for.malformed = private unnamed_addr constant [21 x i8] c"    ; malformed for\0A\00"
@.str.ir.for.source.malformed = private unnamed_addr constant [28 x i8] c"    ; malformed for source\0A\00"
@.str.ir.for.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.ir.for.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.ir.for.arrow = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.ir.for.source = private unnamed_addr constant [18 x i8] c"    ; for source \00"
@.str.ir.for.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.ir.for.local = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.ir.for.alloca = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.ir.for.branch = private unnamed_addr constant [24 x i8] c"    br label %for.cond.\00"
@.str.ir.for.cond = private unnamed_addr constant [10 x i8] c"for.cond.\00"
@.str.ir.for.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.ir.for.next = private unnamed_addr constant [21 x i8] c"    ; iterator next \00"
@.str.ir.for.test = private unnamed_addr constant [24 x i8] c"    br i1 %for.hasnext.\00"
@.str.ir.for.body.label = private unnamed_addr constant [19 x i8] c", label %for.body.\00"
@.str.ir.for.end.label = private unnamed_addr constant [18 x i8] c", label %for.end.\00"
@.str.ir.for.body = private unnamed_addr constant [10 x i8] c"for.body.\00"
@.str.ir.for.end = private unnamed_addr constant [9 x i8] c"for.end.\00"
)FORLLVM";
}

std::string llvmLoadForValueTypeDefinition() {
    return R"LOADLLVM(define ptr @llvmLoadForValueType(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name, ptr %arg.valueType, ptr %arg.resultName) {
entry:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name)
  %is.bool = call i1 @strEq(ptr %arg.valueType, ptr @.str.load.type.boolean)
  br i1 %is.bool, label %bool, label %char.check
bool:
  %b1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.load.i1)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr %storage)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr @.str.load.newline)
  ret ptr %b4
char.check:
  %is.char = call i1 @strEq(ptr %arg.valueType, ptr @.str.load.type.char)
  br i1 %is.char, label %char, label %double.check
char:
  %c1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.load.i8)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %storage)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr @.str.load.newline)
  ret ptr %c4
double.check:
  %is.double = call i1 @strEq(ptr %arg.valueType, ptr @.str.load.type.double)
  br i1 %is.double, label %double, label %long.check
double:
  %d1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %d2 = call ptr @csec_string_concat(ptr %d1, ptr @.str.load.double)
  %d3 = call ptr @csec_string_concat(ptr %d2, ptr %storage)
  %d4 = call ptr @csec_string_concat(ptr %d3, ptr @.str.load.newline)
  ret ptr %d4
long.check:
  %is.long = call i1 @strEq(ptr %arg.valueType, ptr @.str.load.type.long)
  br i1 %is.long, label %long, label %pointer.check
long:
  %l1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %l2 = call ptr @csec_string_concat(ptr %l1, ptr @.str.load.i64)
  %l3 = call ptr @csec_string_concat(ptr %l2, ptr %storage)
  %l4 = call ptr @csec_string_concat(ptr %l3, ptr @.str.load.newline)
  ret ptr %l4
pointer.check:
  %llvm.type = call ptr @irTypeName(ptr %arg.valueType)
  %is.pointer = call i1 @strEq(ptr %llvm.type, ptr @.str.load.type.ptr)
  br i1 %is.pointer, label %pointer, label %integer
pointer:
  %p1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %p2 = call ptr @csec_string_concat(ptr %p1, ptr @.str.load.ptr)
  %p3 = call ptr @csec_string_concat(ptr %p2, ptr %storage)
  %p4 = call ptr @csec_string_concat(ptr %p3, ptr @.str.load.newline)
  ret ptr %p4
integer:
  %i1 = call ptr @csec_string_concat(ptr @.str.load.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.load.i32)
  %i3 = call ptr @csec_string_concat(ptr %i2, ptr %storage)
  %i4 = call ptr @csec_string_concat(ptr %i3, ptr @.str.load.newline)
  ret ptr %i4
}

@.str.load.type.boolean = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.load.type.char = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.load.type.double = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.load.type.long = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.load.type.ptr = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.load.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.load.i1 = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.load.i8 = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.load.double = private unnamed_addr constant [22 x i8] c" = load double, ptr %\00"
@.str.load.i64 = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.load.ptr = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.load.i32 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.load.newline = private unnamed_addr constant [2 x i8] c"\0A\00"
)LOADLLVM";
}

std::string llvmGenerateCallArgumentListI32Definition() {
    return R"CALLARGLISTLLVM(define ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %arg.argsStart, i32 %arg.argsEnd, ptr %arg.resultName) {
entry:
  %first = call i32 @skipTrivia(ptr %arg.tokens, i32 %arg.argsStart)
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %first, i32 %arg.argsEnd)
  %empty = icmp sge i32 %first, %end
  br i1 %empty, label %empty.return, label %last.find
empty.return:
  ret ptr @.str.callargs.empty
last.find:
  %last = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %end, i32 12)
  %one = icmp slt i32 %last, %first
  br i1 %one, label %one.return, label %second.find
one.return:
  %one.type = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %first, i32 %end)
  %one.value = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %first, i32 %end, ptr %arg.resultName, i32 0)
  %one.join = call ptr @csec_string_concat(ptr %one.type, ptr @.str.callargs.space)
  %one.result = call ptr @csec_string_concat(ptr %one.join, ptr %one.value)
  ret ptr %one.result
second.find:
  %second.comma = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %last, i32 12)
  %two = icmp slt i32 %second.comma, %first
  br i1 %two, label %two.return, label %first.find
two.return:
  %second.start.raw = add i32 %last, 1
  %second.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %second.start.raw)
  %two.type0 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %first, i32 %last)
  %two.value0 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %first, i32 %last, ptr %arg.resultName, i32 0)
  %two.type1 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %second.start, i32 %end)
  %two.value1 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %second.start, i32 %end, ptr %arg.resultName, i32 1)
  %two.a = call ptr @csec_string_concat(ptr %two.type0, ptr @.str.callargs.space)
  %two.b = call ptr @csec_string_concat(ptr %two.a, ptr %two.value0)
  %two.c = call ptr @csec_string_concat(ptr %two.b, ptr @.str.callargs.comma)
  %two.d = call ptr @csec_string_concat(ptr %two.c, ptr %two.type1)
  %two.e = call ptr @csec_string_concat(ptr %two.d, ptr @.str.callargs.space)
  %two.result = call ptr @csec_string_concat(ptr %two.e, ptr %two.value1)
  ret ptr %two.result
first.find:
  %first.comma = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %second.comma, i32 12)
  %three = icmp slt i32 %first.comma, %first
  br i1 %three, label %three.return, label %four.return
three.return:
  %three.start1.raw = add i32 %second.comma, 1
  %three.start1 = call i32 @skipTrivia(ptr %arg.tokens, i32 %three.start1.raw)
  %three.start2.raw = add i32 %last, 1
  %three.start2 = call i32 @skipTrivia(ptr %arg.tokens, i32 %three.start2.raw)
  %three.type0 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %first, i32 %second.comma)
  %three.value0 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %first, i32 %second.comma, ptr %arg.resultName, i32 0)
  %three.type1 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %three.start1, i32 %last)
  %three.value1 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %three.start1, i32 %last, ptr %arg.resultName, i32 1)
  %three.type2 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %three.start2, i32 %end)
  %three.value2 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %three.start2, i32 %end, ptr %arg.resultName, i32 2)
  %three.a = call ptr @csec_string_concat(ptr %three.type0, ptr @.str.callargs.space)
  %three.b = call ptr @csec_string_concat(ptr %three.a, ptr %three.value0)
  %three.c = call ptr @csec_string_concat(ptr %three.b, ptr @.str.callargs.comma)
  %three.d = call ptr @csec_string_concat(ptr %three.c, ptr %three.type1)
  %three.e = call ptr @csec_string_concat(ptr %three.d, ptr @.str.callargs.space)
  %three.f = call ptr @csec_string_concat(ptr %three.e, ptr %three.value1)
  %three.g = call ptr @csec_string_concat(ptr %three.f, ptr @.str.callargs.comma)
  %three.h = call ptr @csec_string_concat(ptr %three.g, ptr %three.type2)
  %three.i = call ptr @csec_string_concat(ptr %three.h, ptr @.str.callargs.space)
  %three.result = call ptr @csec_string_concat(ptr %three.i, ptr %three.value2)
  ret ptr %three.result
four.return:
  %four.start1.raw = add i32 %first.comma, 1
  %four.start1 = call i32 @skipTrivia(ptr %arg.tokens, i32 %four.start1.raw)
  %four.start2.raw = add i32 %second.comma, 1
  %four.start2 = call i32 @skipTrivia(ptr %arg.tokens, i32 %four.start2.raw)
  %four.start3.raw = add i32 %last, 1
  %four.start3 = call i32 @skipTrivia(ptr %arg.tokens, i32 %four.start3.raw)
  %four.type0 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %first, i32 %first.comma)
  %four.value0 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %first, i32 %first.comma, ptr %arg.resultName, i32 0)
  %four.type1 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %four.start1, i32 %second.comma)
  %four.value1 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %four.start1, i32 %second.comma, ptr %arg.resultName, i32 1)
  %four.type2 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %four.start2, i32 %last)
  %four.value2 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %four.start2, i32 %last, ptr %arg.resultName, i32 2)
  %four.type3 = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %four.start3, i32 %end)
  %four.value3 = call ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %four.start3, i32 %end, ptr %arg.resultName, i32 3)
  %four.a = call ptr @csec_string_concat(ptr %four.type0, ptr @.str.callargs.space)
  %four.b = call ptr @csec_string_concat(ptr %four.a, ptr %four.value0)
  %four.c = call ptr @csec_string_concat(ptr %four.b, ptr @.str.callargs.comma)
  %four.d = call ptr @csec_string_concat(ptr %four.c, ptr %four.type1)
  %four.e = call ptr @csec_string_concat(ptr %four.d, ptr @.str.callargs.space)
  %four.f = call ptr @csec_string_concat(ptr %four.e, ptr %four.value1)
  %four.g = call ptr @csec_string_concat(ptr %four.f, ptr @.str.callargs.comma)
  %four.h = call ptr @csec_string_concat(ptr %four.g, ptr %four.type2)
  %four.i = call ptr @csec_string_concat(ptr %four.h, ptr @.str.callargs.space)
  %four.j = call ptr @csec_string_concat(ptr %four.i, ptr %four.value2)
  %four.k = call ptr @csec_string_concat(ptr %four.j, ptr @.str.callargs.comma)
  %four.l = call ptr @csec_string_concat(ptr %four.k, ptr %four.type3)
  %four.m = call ptr @csec_string_concat(ptr %four.l, ptr @.str.callargs.space)
  %four.result = call ptr @csec_string_concat(ptr %four.m, ptr %four.value3)
  ret ptr %four.result
}

@.str.callargs.empty = private unnamed_addr constant [1 x i8] c"\00"
@.str.callargs.space = private unnamed_addr constant [2 x i8] c" \00"
@.str.callargs.comma = private unnamed_addr constant [3 x i8] c", \00"
)CALLARGLISTLLVM";
}

std::string llvmI32CallArgumentValueDefinition() {
    return R"I32CALLARGLLVM(define ptr @llvmI32CallArgumentValue(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName, i32 %arg.index) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %empty.return, label %integer.check
empty.return:
  ret ptr @.str.i32.callarg.zero
integer.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %integer = and i1 %is.integer, %single
  br i1 %integer, label %integer.return, label %temporary.return
integer.return:
  %text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  ret ptr %text
temporary.return:
  %a = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.i32.callarg.arg)
  %index.i64 = sext i32 %arg.index to i64
  %index.text = call ptr @csec_to_string_i64(i64 %index.i64)
  %result = call ptr @csec_string_concat(ptr %a, ptr %index.text)
  ret ptr %result
}

@.str.i32.callarg.zero = private unnamed_addr constant [2 x i8] c"0\00"
@.str.i32.callarg.arg = private unnamed_addr constant [5 x i8] c".arg\00"
)I32CALLARGLLVM";
}
