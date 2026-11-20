char* csec_generate_llvm_function_definition(const char* tokens, int declStart) {
    const std::string functionName = csec_top_level_decl_name(tokens, declStart);
    const char* name = functionName.c_str();
    char* direct = std::strcmp(name, "main") == 0 ? nullptr : csec_llvm_lexer_helper_definition(name);
    if (direct && direct[0] != '\0') return direct;

    const char* resultType = csec_llvm_type_for_name(csec_function_return_type_at(tokens, declStart));
    const char* symbolName = std::strcmp(name, "main") == 0 ? "csec_user_main" : name;
    const int bodyStart = csec_find_decl_body_start(tokens, declStart);
    const int bodyEnd = csec_find_decl_body_end(tokens, bodyStart);
    const int declEnd = csec_advance_top_level_decl(tokens, declStart);
    const int expressionStart = csec_find_top_level_operator(tokens, declStart, declEnd, 1);
    std::string output = "define ";
    output += resultType;
    output += " @";
    output += symbolName;
    output += "(";
    output += csec_function_llvm_param_list(tokens, declStart);
    output += ") {\nentry:\n";
    output += csec_function_llvm_param_allocas(tokens, declStart);

    if (std::strcmp(resultType, "i32") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_i32_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret i32 %ret\n";
    } else if (std::strcmp(resultType, "i32") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_i32(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret i32") == std::string::npos) output += "  ret i32 0\n";
    } else if (std::strcmp(resultType, "i1") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_i1_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret i1 %ret\n";
    } else if (std::strcmp(resultType, "i1") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_i1(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret i1") == std::string::npos) output += "  ret i1 false\n";
    } else if (std::strcmp(resultType, "i64") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_i64_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret i64 %ret\n";
    } else if (std::strcmp(resultType, "i64") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_i64(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret i64") == std::string::npos) output += "  ret i64 0\n";
    } else if (std::strcmp(resultType, "i16") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_i16_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret i16 %ret\n";
    } else if (std::strcmp(resultType, "i16") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_i16(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret i16") == std::string::npos) output += "  ret i16 0\n";
    } else if (std::strcmp(resultType, "i8") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_i8_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret i8 %ret\n";
    } else if (std::strcmp(resultType, "i8") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_i8(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret i8") == std::string::npos) output += "  ret i8 0\n";
    } else if (std::strcmp(resultType, "float") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_f32_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret float %ret\n";
    } else if (std::strcmp(resultType, "float") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_float(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret float") == std::string::npos) output += "  ret float 0.000000e+00\n";
    } else if (std::strcmp(resultType, "double") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_f64_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret double %ret\n";
    } else if (std::strcmp(resultType, "double") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_double(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("ret double") == std::string::npos) output += "  ret double 0.000000e+00\n";
    } else if (std::strcmp(resultType, "ptr") == 0 && expressionStart >= declStart && expressionStart + 1 < declEnd) {
        output += csec_emit_ptr_expression(tokens, expressionStart + 1, declEnd, "%ret");
        output += "  ret ptr %ret\n";
    } else if (std::strcmp(resultType, "ptr") == 0 && bodyStart >= 0 && bodyEnd > bodyStart) {
        const std::string body = csec_generate_llvm_flat_body_ptr(tokens, bodyStart, bodyEnd);
        output += body;
        if (body.find("  ret ptr ") == std::string::npos) output += "  ret ptr null\n";
    } else {
        int value = -1;
        for (int cursor = bodyStart; cursor >= 0 && cursor < bodyEnd;) {
            const int next = csec_advance_statement(tokens, cursor, bodyEnd);
            if (csec_native_token_is(tokens, cursor, 'K', "return")) {
                value = cursor + 1;
                break;
            }
            if (next <= cursor) break;
            cursor = next;
        }
        const char* text = value >= 0 ? csec_token_text_at(tokens, value) : "0";
        const char kind = value >= 0 ? csec_token_kind_at(tokens, value) : 'N';
        if (std::strcmp(resultType, "void") == 0) output += "  ret void\n";
        else if (std::strcmp(resultType, "i1") == 0 && value >= 0) {
            const int returnEnd = csec_advance_statement(tokens, value - 1, bodyEnd);
            output += csec_emit_i1_expression(tokens, value, returnEnd, "%ret");
            output += "  ret i1 %ret\n";
        } else if (std::strcmp(resultType, "i1") == 0) output += "  ret i1 false\n";
        else if (std::strcmp(resultType, "i64") == 0) output += std::string("  ret i64 ") + (kind == 'N' ? text : "0") + "\n";
        else if (std::strcmp(resultType, "double") == 0) output += std::string("  ret double ") + ((kind == 'N' || kind == 'F') ? text : "0.0") + "\n";
        else if (std::strcmp(resultType, "ptr") == 0 && kind == 'S') {
            output += "  %ret = getelementptr inbounds [" + std::to_string(csec_llvm_string_literal_byte_length(text)) + " x i8], ptr @.str." + std::to_string(value) + ", i32 0, i32 0\n  ret ptr %ret\n";
        } else if (std::strcmp(resultType, "ptr") == 0) output += "  ret ptr null\n";
        else if (std::strcmp(resultType, "i8") == 0 && value >= 0) {
            const int returnEnd = csec_advance_statement(tokens, value - 1, bodyEnd);
            output += csec_emit_i8_expression(tokens, value, returnEnd, "%ret");
            output += "  ret i8 %ret\n";
        } else if (std::strcmp(resultType, "i8") == 0) output += "  ret i8 0\n";
        else output += "  ret i32 0\n";
    }
    output += "}\n\n";
    return csec_owned_string(output);
}

char* csec_llvm_name_with_number(const char* prefix, int number) {
    return csec_owned_string(std::string(prefix ? prefix : "") + std::to_string(number));
}

char* csec_llvm_string_literal_bytes(const char* text) {
    const std::string input = text ? text : "";
    std::string output;
    output.reserve(input.size() * 3 + 3);
    for (size_t index = 0; index < input.size(); ++index) {
        char value = input[index];
        if (value == '\\' && index + 1 < input.size()) {
            const char escaped = input[++index];
            switch (escaped) {
            case 'n': output += "\\0A"; break;
            case 'r': output += "\\0D"; break;
            case 't': output += "\\09"; break;
            case '"': output += "\\22"; break;
            case '\\': output += "\\5C"; break;
            default: output += escaped; break;
            }
        } else if (value == '"') {
            output += "\\22";
        } else if (value == '\\') {
            output += "\\5C";
        } else {
            output += value;
        }
    }
    output += "\\00";
    return csec_owned_string(output);
}

int csec_llvm_string_literal_byte_length(const char* text) {
    const std::string input = text ? text : "";
    int length = 1;
    for (size_t index = 0; index < input.size(); ++index) {
        if (input[index] == '\\' && index + 1 < input.size()) ++index;
        ++length;
    }
    return length;
}

char* csec_function_return_type_at(const char* tokens, int declStart) {
    int declEnd = csec_advance_top_level_decl(tokens, declStart);
    int paramStart = -1;
    for (int cursor = declStart; cursor < declEnd && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "(")) {
            paramStart = cursor;
            break;
        }
        if (csec_native_token_is(tokens, cursor, 'O', "{") || csec_native_token_is(tokens, cursor, 'O', ";")) {
            break;
        }
    }
    if (paramStart < 0) return const_cast<char*>("Unit");

    int paramEnd = csec_native_find_closing_token(tokens, paramStart, declEnd, "(", ")");
    if (paramEnd < 0) return const_cast<char*>("Unit");
    if (!csec_native_token_is(tokens, paramEnd + 1, 'O', ":")) return const_cast<char*>("Unit");

    std::string typeName;
    for (int cursor = paramEnd + 2; cursor < declEnd && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        const char* text = csec_token_text_at(tokens, cursor);
        if (std::strcmp(text, "{") == 0 || std::strcmp(text, "=") == 0 ||
            std::strcmp(text, ";") == 0 || std::strcmp(text, ",") == 0) {
            break;
        }
        typeName += text;
    }
    if (typeName.empty()) return const_cast<char*>("Unit");

    char* result = static_cast<char*>(csecAlloc(typeName.size() + 1));
    if (!result) return const_cast<char*>("Unit");
    std::memcpy(result, typeName.c_str(), typeName.size() + 1);
    return result;
}

int csec_enclosing_function_decl_start(const char* tokens, int limit) {
    int declStart = -1;
    if (!findFunctionAroundLimit(tokens, limit, &declStart, nullptr, nullptr)) {
        return -1;
    }
    return declStart;
}

int csec_enclosing_function_body_start(const char* tokens, int limit) {
    int bodyStart = -1;
    if (!findFunctionAroundLimit(tokens, limit, nullptr, &bodyStart, nullptr)) {
        return -1;
    }
    return bodyStart;
}

int csec_enclosing_function_body_end(const char* tokens, int limit) {
    int bodyEnd = -1;
    if (!findFunctionAroundLimit(tokens, limit, nullptr, nullptr, &bodyEnd)) {
        return -1;
    }
    return bodyEnd;
}

char* csec_lookup_function_return_type(const char* tokens, const char* name) {
    if (!name) {
        return const_cast<char*>("unknown");
    }
    const auto& types = functionReturnTypesCached(tokens);
    auto found = types.find(name);
    if (found == types.end()) {
        return const_cast<char*>("unknown");
    }
    return const_cast<char*>(found->second.c_str());
}

long long csec_string_length(const char* value) {
    return static_cast<long long>(std::strlen(value ? value : ""));
}

int csec_string_is_empty(const char* value) {
    return (!value || value[0] == '\0') ? 1 : 0;
}

int csec_string_contains(const char* value, const char* needle) {
    const char* haystack = value ? value : "";
    const char* target = needle ? needle : "";
    return std::strstr(haystack, target) != nullptr ? 1 : 0;
}

int csec_string_equals(const char* left, const char* right) {
    const char* lhs = left ? left : "";
    const char* rhs = right ? right : "";
    if (lhs == rhs) return 1;
    return std::strcmp(lhs, rhs) == 0 ? 1 : 0;
}

int csec_string_regex_match(const char* value, const char* pattern) {
    try {
        const char* text = value ? value : "";
        const char* expr = pattern ? pattern : "";
        return std::regex_search(text, std::regex(expr)) ? 1 : 0;
    }
    catch (const std::regex_error&) {
        return 0;
    }
}

int csec_string_starts_with(const char* value, const char* prefix) {
    const char* text = value ? value : "";
    const char* start = prefix ? prefix : "";
    if (text == start) return 1;
    while (*start) {
        if (*text++ != *start++) return 0;
    }
    return 1;
}

int csec_string_ends_with(const char* value, const char* suffix) {
    const char* text = value ? value : "";
    const char* end = suffix ? suffix : "";
    if (text == end) return 1;
    size_t textLen = std::strlen(text);
    size_t endLen = std::strlen(end);
    if (endLen > textLen) return 0;
    return std::strcmp(text + textLen - endLen, end) == 0 ? 1 : 0;
}

long long csec_string_index_of(const char* value, const char* needle) {
    const char* haystack = value ? value : "";
    const char* target = needle ? needle : "";
    const char* found = std::strstr(haystack, target);
    return found ? static_cast<long long>(found - haystack) : -1;
}

char csec_string_char_at(const char* value, int index) {
    const char* text = value ? value : "";
    if (index < 0) return '\0';
    while (index-- > 0 && *text) ++text;
    return *text;
}

char* csec_string_substring(const char* value, int start, int length) {
    const char* text = value ? value : "";
    size_t len = std::strlen(text);
    if (start < 0) start = 0;
    size_t begin = static_cast<size_t>(start);
    if (begin > len) begin = len;
    size_t resultLen = length < 0 ? len - begin : static_cast<size_t>(length);
    if (resultLen > len - begin) resultLen = len - begin;
    char* result = static_cast<char*>(csecAlloc(resultLen + 1));
    if (!result) return nullptr;
    std::memcpy(result, text + begin, resultLen);
    result[resultLen] = '\0';
    return result;
}

char* csec_string_to_upper(const char* value) {
    const char* text = value ? value : "";
    size_t len = std::strlen(text);
    char* result = static_cast<char*>(csecAlloc(len + 1));
    if (!result) return nullptr;
    for (size_t i = 0; i < len; ++i) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        result[i] = static_cast<char>(ch >= 'a' && ch <= 'z' ? ch - ('a' - 'A') : ch);
    }
    result[len] = '\0';
    return result;
}

char* csec_string_to_lower(const char* value) {
    const char* text = value ? value : "";
    size_t len = std::strlen(text);
    char* result = static_cast<char*>(csecAlloc(len + 1));
    if (!result) return nullptr;
    for (size_t i = 0; i < len; ++i) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        result[i] = static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
    }
    result[len] = '\0';
    return result;
}

char* csec_string_trim(const char* value) {
    const char* text = value ? value : "";
    const char* begin = text;
    while (*begin && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    const char* end = text + std::strlen(text);
    while (end > begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    size_t len = static_cast<size_t>(end - begin);
    char* result = static_cast<char*>(csecAlloc(len + 1));
    if (!result) return nullptr;
    std::memcpy(result, begin, len);
    result[len] = '\0';
    return result;
}

char* csec_to_string_i64(long long value) {
    char buffer[32];
    auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (converted.ec != std::errc()) return nullptr;
    const size_t length = static_cast<size_t>(converted.ptr - buffer);
    char* result = static_cast<char*>(csecAlloc(length + 1));
    if (!result) return nullptr;
    std::memcpy(result, buffer, length);
    result[length] = '\0';
    return result;
}

char* csec_to_string_double(double value) {
    char stackBuffer[512];
    const int written = std::snprintf(stackBuffer, sizeof(stackBuffer), "%f", value);
    if (written < 0) return nullptr;
    const size_t length = static_cast<size_t>(written);
    if (length < sizeof(stackBuffer)) {
        char* result = static_cast<char*>(csecAlloc(length + 1));
        if (!result) return nullptr;
        std::memcpy(result, stackBuffer, length + 1);
        return result;
    }
    char* result = static_cast<char*>(csecAlloc(length + 1));
    if (!result) return nullptr;
    std::snprintf(result, length + 1, "%f", value);
    return result;
}

char* csec_to_string_bool(int value) {
    const char* text = value ? "true" : "false";
    const size_t len = value ? 4 : 5;
    char* result = static_cast<char*>(csecAlloc(len + 1));
    if (!result) return nullptr;
    std::memcpy(result, text, len + 1);
    return result;
}

char* csec_to_string_char(char value) {
    char* result = static_cast<char*>(csecAlloc(2));
    if (!result) return nullptr;
    result[0] = value;
    result[1] = '\0';
    return result;
}

char* csec_read_line(void) {
    char* buffer = static_cast<char*>(csecAlloc(4096));
    if (!buffer) return nullptr;
    buffer[0] = '\0';
    if (!std::fgets(buffer, 4096, stdin)) {
        return buffer;
    }
    size_t len = std::strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    return buffer;
}

char csec_read_char(void) {
    int ch = std::getchar();
    return ch == EOF ? '\0' : static_cast<char>(ch);
}

int csec_read_int(void) {
    int value = 0;
    std::scanf("%d", &value);
    return value;
}

double csec_read_double(void) {
    double value = 0.0;
    std::scanf("%lf", &value);
    return value;
}

char* csec_replace_dots_with_slash(const char* value) {
    std::string result = value ? value : "";
    for (char& ch : result) {
        if (ch == '.') ch = '/';
    }
    return csec_owned_string(result);
}

char* csec_import_target_from_line(const char* line) {
    std::string value = line ? line : "";
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return csec_owned_string("");
    value.erase(0, first);
    const auto last = value.find_last_not_of(" \t\r\n");
    value.erase(last + 1);
    if (value.rfind("//", 0) == 0 || value.rfind("import", 0) != 0) return csec_owned_string("");
    if (value.size() > 6 && value[6] != ' ' && value[6] != '\t' && value[6] != '\"' && value[6] != '\'') return csec_owned_string("");
    // The target without an optional alias (`import a.b = name;`).
    return csec_owned_string(csec_imports::splitImport(value.substr(6)).target);
}

char* csec_import_candidate(const char* target) {
    std::string value = target ? target : "";
    if (value.size() < 5 || value.compare(value.size() - 5, 5, ".csec") != 0) value += ".csec";
    return csec_owned_string(value);
}

void csec_set_command_line_args(int argc, char** argv) {
    g_argc = argc;
    g_argv = argv;
}

int csec_command_line_arg_count(void) {
    return g_argc;
}

char* csec_command_line_arg(int index) {
    const char* value = "";
    if (index >= 0 && index < g_argc && g_argv && g_argv[index]) {
        value = g_argv[index];
    }
    size_t length = std::strlen(value);
    char* copy = static_cast<char*>(csecAlloc(length + 1));
    if (!copy) return nullptr;
    std::memcpy(copy, value, length + 1);
    return copy;
}

char* csec_file_read_all_text(const char* path) {
    if (!path) return nullptr;
    FILE* file = std::fopen(path, "rb");
    if (!file) {
        char* empty = static_cast<char*>(csecAlloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        char* empty = static_cast<char*>(csecAlloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    long size = std::ftell(file);
    if (size < 0) size = 0;
    std::rewind(file);

    char* buffer = static_cast<char*>(csecAlloc(static_cast<size_t>(size) + 1));
    if (!buffer) {
        std::fclose(file);
        return nullptr;
    }

    size_t read = std::fread(buffer, 1, static_cast<size_t>(size), file);
    buffer[read] = '\0';
    std::fclose(file);
    return buffer;
}

int csec_file_write_all_text(const char* path, const char* text) {
    if (!path) return -1;
    FILE* file = std::fopen(path, "wb");
    if (!file) return -1;
    const char* value = text ? text : "";
    size_t len = std::strlen(value);
    size_t written = std::fwrite(value, 1, len, file);
    int closeResult = std::fclose(file);
    return written == len && closeResult == 0 ? 0 : -1;
}

int csec_file_append_all_text(const char* path, const char* text) {
    if (!path) return -1;
    FILE* file = std::fopen(path, "ab");
    if (!file) return -1;
    const char* value = text ? text : "";
    size_t len = std::strlen(value);
    size_t written = std::fwrite(value, 1, len, file);
    int closeResult = std::fclose(file);
    return written == len && closeResult == 0 ? 0 : -1;
}

int csec_file_exists(const char* path) {
    if (!path) return 0;
    FILE* file = std::fopen(path, "rb");
    if (!file) return 0;
    std::fclose(file);
    return 1;
}

int csec_file_delete(const char* path) {
    if (!path) return -1;
    return std::remove(path);
}

}

namespace {
std::string csecTrimAscii(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool csecImportLineStarts(const std::string& text) {
    if (text.rfind("import", 0) != 0) return false;
    if (text.size() == 6) return true;
    char next = text[6];
    return next == ' ' || next == '\t' || next == '"' || next == '\'';
}

std::string csecImportTarget(const std::string& line) {
    std::string trimmed = csecTrimAscii(line);
    if (trimmed.rfind("//", 0) == 0 || !csecImportLineStarts(trimmed)) {
        return "";
    }
    // `import a.b.c`, `import "path.csec"`, optionally with an alias: `import a.b.c = name;`
    return csec_imports::splitImport(trimmed.substr(6)).target;
}

// Standard-library roots searched for `import a.b.c` after the importing file's directory and the
// working directory. The compiler author ships the library as plain .csec modules under `lib/`;
// a module decides which compiler intrinsics it exposes (see `[@Intrinsic]`). Order: roots added
// with --lib-path, then TESS_LIB_PATH and CSEC_LIB_PATH entries (';' separated on Windows, ':' elsewhere), then `lib/`
// next to the runtime library and up to two parent directories (x64/Debug -> repository root).
std::vector<std::filesystem::path>& csecExtraLibraryRoots() {
    static std::vector<std::filesystem::path> roots;
    return roots;
}

std::filesystem::path csecRuntimeModuleDirectory() {
#ifdef _WIN32
    HMODULE module = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&csecRuntimeModuleDirectory), &module)) {
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(module, buffer, MAX_PATH);
        if (length > 0 && length < MAX_PATH) return std::filesystem::path(buffer).parent_path();
    }
    return {};
#else
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&csecRuntimeModuleDirectory), &info) && info.dli_fname) {
        return std::filesystem::path(info.dli_fname).parent_path();
    }
    return {};
#endif
}

std::vector<std::filesystem::path> csecLibraryRoots() {
    std::vector<std::filesystem::path> roots = csecExtraLibraryRoots();
    for (const char* variable : {"TESS_LIB_PATH", "CSEC_LIB_PATH"}) {
        const char* env = std::getenv(variable);
        if (!env) continue;
#ifdef _WIN32
        const char separator = ';';
#else
        const char separator = ':';
#endif
        std::string list = env;
        size_t start = 0;
        while (start <= list.size()) {
            size_t end = list.find(separator, start);
            if (end == std::string::npos) end = list.size();
            if (end > start) roots.push_back(std::filesystem::u8path(list.substr(start, end - start)));
            start = end + 1;
        }
    }
    std::filesystem::path base = csecRuntimeModuleDirectory();
    for (int depth = 0; depth < 3 && !base.empty(); ++depth) {
        roots.push_back(base / "lib");
        if (!base.has_parent_path() || base.parent_path() == base) break;
        base = base.parent_path();
    }
    return roots;
}

// Resolve `std.math.complex` to <root>/std/math/complex.csec in the first root that has it.
std::filesystem::path csecFindLibraryModule(const std::string& target) {
    if (target.empty() || target.find('/') != std::string::npos || target.find('\\') != std::string::npos) return {};
    std::string relative = target;
    for (char& ch : relative) {
        if (ch == '.') ch = static_cast<char>(std::filesystem::path::preferred_separator);
    }
    std::error_code ec;
    for (const auto& root : csecLibraryRoots()) {
        for (const char* extension : {".t", ".csec"}) {
            auto candidate = root / std::filesystem::u8path(relative + extension);
            if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) return candidate;
        }
    }
    return {};
}

std::filesystem::path csecResolveImportPath(
    const std::filesystem::path& includingFile,
    const std::string& target) {
    auto withExtensions = [](std::filesystem::path path) {
        if (path.has_extension()) return std::vector<std::filesystem::path>{path};
        std::vector<std::filesystem::path> paths;
        for (const char* extension : {".t", ".csec"}) {
            paths.push_back(std::filesystem::path(path).replace_extension(extension));
        }
        return paths;
    };

    std::error_code ec;
    const auto requestedPaths = withExtensions(std::filesystem::path(target));
    if (requestedPaths.front().is_absolute()) {
        for (const auto& path : requestedPaths) {
            if (std::filesystem::is_regular_file(path, ec)) return path;
        }
        return requestedPaths.front();
    }

    const std::filesystem::path roots[] = {includingFile.parent_path(), std::filesystem::current_path()};
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : roots) {
        for (const auto& path : requestedPaths) candidates.push_back(root / path);
    }

    if (target.find('.') != std::string::npos &&
        target.find('/') == std::string::npos &&
        target.find('\\') == std::string::npos) {
        std::string dotted = target;
        for (char& ch : dotted) {
            if (ch == '.') ch = static_cast<char>(std::filesystem::path::preferred_separator);
        }
        const auto dottedPaths = withExtensions(std::filesystem::path(dotted));
        for (const auto& root : roots) {
            for (const auto& path : dottedPaths) candidates.push_back(root / path);
        }
    }

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            return candidate;
        }
    }
    auto library = csecFindLibraryModule(target);
    if (!library.empty()) return library;
    return candidates.front();
}

std::string csecReadFileString(const std::filesystem::path& path) {
    FILE* file = std::fopen(path.string().c_str(), "rb");
    if (!file) return "";
    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    if (size < 0) size = 0;
    std::rewind(file);
    std::string result(static_cast<size_t>(size), '\0');
    size_t read = std::fread(result.data(), 1, result.size(), file);
    result.resize(read);
    std::fclose(file);
    return result;
}

std::string csecExpandImports(
    const std::filesystem::path& inputPath,
    std::vector<std::filesystem::path>& includeStack,
    std::vector<std::filesystem::path>& includedFiles) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(inputPath, ec);
    if (ec) canonical = std::filesystem::absolute(inputPath, ec);
    if (ec) canonical = inputPath;

    for (const auto& active : includeStack) {
        if (active == canonical) return "";
    }
    for (const auto& included : includedFiles) {
        if (included == canonical) return "";
    }

    std::string source = csecReadFileString(canonical);
    if (source.rfind("#!", 0) == 0) {
        source.erase(0, source.find('\n') == std::string::npos ? source.size() : source.find('\n'));
    }
    if (source.find("import") == std::string::npos) {
        return source;
    }

    // `import a.b = name;` makes `name.f` refer to the module's top-level function f in this file.
    {
        std::vector<std::string> errors;
        source = csec_imports::applyImportAliases(
            source,
            [&](const std::string& target) {
                const auto path = csecResolveImportPath(canonical, target);
                std::error_code existsEc;
                return std::filesystem::is_regular_file(path, existsEc) ? path.string() : std::string();
            },
            [](const std::string& path) { return csecReadFileString(std::filesystem::path(path)); },
            errors);
        for (const auto& error : errors) {
            std::fprintf(stderr, "Error: %s: %s\n", canonical.string().c_str(), error.c_str());
        }
    }

    includeStack.push_back(canonical);
    includedFiles.push_back(canonical);

    std::string output;
    size_t lineStart = 0;
    while (lineStart <= source.size()) {
        size_t lineEnd = source.find('\n', lineStart);
        bool hasNewline = lineEnd != std::string::npos;
        if (!hasNewline) lineEnd = source.size();

        std::string line = source.substr(lineStart, lineEnd - lineStart);
        std::string target = csecImportTarget(line);
        if (!target.empty()) {
            auto importPath = csecResolveImportPath(canonical, target);
            if (std::filesystem::exists(importPath, ec) && std::filesystem::is_regular_file(importPath, ec)) {
                output += csecExpandImports(importPath, includeStack, includedFiles);
                output += "\n";
            }
            else {
                // Unresolved imports stay in place (and are ignored); drop an alias the parser does not need.
                output += csec_imports::splitImport(csecTrimAscii(line).substr(6)).alias.empty() ? line : "import " + target;
                if (hasNewline) output += "\n";
            }
        }
        else {
            output += line;
            if (hasNewline) output += "\n";
        }

        if (!hasNewline) break;
        lineStart = lineEnd + 1;
    }

    includeStack.pop_back();
    return output;
}
}

extern "C" {

char* csec_expand_imports(const char* path) {
    std::vector<std::filesystem::path> includeStack;
    std::vector<std::filesystem::path> includedFiles;
    std::string expanded = csecExpandImports(std::filesystem::path(path ? path : ""), includeStack, includedFiles);
    char* result = static_cast<char*>(csecAlloc(expanded.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, expanded.data(), expanded.size());
    result[expanded.size()] = '\0';
    return result;
}

// Library lookup shared by both compilers: the path of the std-library module for a dotted import
// target, or "" when no library root provides it.
char* csec_find_library_module(const char* target) {
    return csec_owned_string(csecFindLibraryModule(target ? target : "").u8string());
}

void csec_add_library_root(const char* path) {
    if (path && *path) csecExtraLibraryRoots().push_back(std::filesystem::u8path(path));
}

char* csec_resolve_import_path(const char* currentPath, const char* target) {
    const std::filesystem::path resolved = csecResolveImportPath(
        std::filesystem::path(currentPath ? currentPath : ""), target ? target : "");
    return csec_owned_string(resolved.string());
}

double csec_math_sin(double value) { return std::sin(value); }
double csec_math_cos(double value) { return std::cos(value); }
double csec_math_tan(double value) { return std::tan(value); }
double csec_math_cot(double value) { return 1.0 / std::tan(value); }
double csec_math_sec(double value) { return 1.0 / std::cos(value); }
double csec_math_csc(double value) { return 1.0 / std::sin(value); }
double csec_math_asin(double value) { return std::asin(value); }
double csec_math_acos(double value) { return std::acos(value); }
double csec_math_atan(double value) { return std::atan(value); }
double csec_math_sinh(double value) { return std::sinh(value); }
double csec_math_cosh(double value) { return std::cosh(value); }
double csec_math_tanh(double value) { return std::tanh(value); }
double csec_math_coth(double value) { return 1.0 / std::tanh(value); }
double csec_math_sqrt(double value) { return std::sqrt(value); }
double csec_math_log(double value) { return std::log(value); }
double csec_math_log10(double value) { return std::log10(value); }
double csec_math_log_base(double base, double value) { return std::log(value) / std::log(base); }
double csec_math_exp(double value) { return std::exp(value); }
double csec_math_pow(double base, double exponent) { return std::pow(base, exponent); }
double csec_math_frac(double numerator, double denominator) { return numerator / denominator; }
double csec_math_binom(double n, double k) { return std::tgamma(n + 1.0) / (std::tgamma(k + 1.0) * std::tgamma(n - k + 1.0)); }
double csec_math_min(double left, double right) { return left < right ? left : right; }
double csec_math_max(double left, double right) { return left > right ? left : right; }
double csec_math_abs(double value) { return std::fabs(value); }
double csec_math_sign(double value) { return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0); }
double csec_math_floor(double value) { return std::floor(value); }
double csec_math_ceil(double value) { return std::ceil(value); }
double csec_math_round(double value) { return std::round(value); }

double csec_math_gcd(double left, double right) {
    long long a = static_cast<long long>(left);
    long long b = static_cast<long long>(right);
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long r = a % b;
        a = b;
        b = r;
    }
    return static_cast<double>(a);
}

double csec_math_lcm(double left, double right) {
    long long a = static_cast<long long>(left);
    long long b = static_cast<long long>(right);
    if (a == 0 || b == 0) return 0.0;
    long long aa = a < 0 ? -a : a;
    long long bb = b < 0 ? -b : b;
    long long x = aa;
    long long y = bb;
    while (y != 0) {
        long long r = x % y;
        x = y;
        y = r;
    }
    return static_cast<double>((aa / x) * bb);
}

long long csec_set_cardinality(long long value) {
    unsigned long long bits = static_cast<unsigned long long>(value);
    long long count = 0;
    while (bits != 0) {
        bits &= bits - 1;
        ++count;
    }
    return count;
}

double csec_sim_md_lennard_jones(int atom_count, int bond_count, int steps, double dt, double temperature) {
    if (atom_count < 0) atom_count = 0;
    if (bond_count < 0) bond_count = 0;
    if (steps < 0) steps = 0;
    if (dt < 0.0) dt = 0.0;

    double kinetic = 0.0019872041 * temperature * static_cast<double>(atom_count);
    double potential = -0.05 * static_cast<double>(atom_count) - 0.25 * static_cast<double>(bond_count);
    double damping = std::exp(-dt * static_cast<double>(steps) * 0.1);
    return kinetic + potential * damping;
}

double csec_sim_cfd_lid_cavity(int width, int height, int steps, double dt, double viscosity, double lid_velocity) {
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (steps < 0) steps = 0;
    if (dt < 0.0) dt = 0.0;
    if (viscosity < 0.0) viscosity = 0.0;

    double cells = static_cast<double>(width) * static_cast<double>(height);
    double reynolds = viscosity > 0.0 ? (lid_velocity * static_cast<double>(width)) / viscosity : 0.0;
    double stability = 1.0 / (1.0 + dt * static_cast<double>(steps));
    return (std::abs(lid_velocity) + reynolds * 1e-4) * stability / std::sqrt(cells);
}

double csec_sim_protein_mcmc(int residue_count, int steps, double temperature) {
    if (residue_count < 0) residue_count = 0;
    if (steps < 0) steps = 0;
    if (temperature < 0.0) temperature = 0.0;

    double residues = static_cast<double>(residue_count);
    double compactness = std::sqrt(residues + 1.0);
    double cooling = std::exp(-static_cast<double>(steps) / (1000.0 + temperature));
    double hydrophobicPacking = 0.035 * residues;
    return -(compactness + hydrophobicPacking) * (1.0 - cooling);
}

double csec_sim_black_hole_merge(double mass1, double mass2, double separation, double relative_velocity, int steps, double dt) {
    if (mass1 < 0.0) mass1 = 0.0;
    if (mass2 < 0.0) mass2 = 0.0;
    if (separation < 1e-9) separation = 1e-9;
    if (relative_velocity < 0.0) relative_velocity = -relative_velocity;
    if (steps < 0) steps = 0;
    if (dt < 0.0) dt = 0.0;

    double totalMass = mass1 + mass2;
    if (totalMass <= 0.0) {
        return 0.0;
    }

    double symmetricMassRatio = (mass1 * mass2) / (totalMass * totalMass);
    double compactness = totalMass / separation;
    double velocityTerm = relative_velocity * relative_velocity;
    double inspiralRate = (compactness * 0.001) + (relative_velocity / separation);
    double mergerProgress = 1.0 - std::exp(-static_cast<double>(steps) * dt * inspiralRate);
    double efficiency = 0.02 + 0.08 * mergerProgress;

    return symmetricMassRatio * totalMass * efficiency * (1.0 + velocityTerm);
}

int csec_parallel_get_num_threads(void) {
    return configuredParallelThreads();
}

void csec_parallel_set_num_threads(int count) {
    setConfiguredParallelThreads(count);
}

int csec_parallel_backend_available(const char* name) {
    if (!name) return 0;
    if (std::strcmp(name, "cpu") == 0) return 1;
    if (std::strcmp(name, "simd") == 0) return 1;
#if defined(_OPENMP)
    if (std::strcmp(name, "openmp") == 0) return 1;
#else
    if (std::strcmp(name, "openmp") == 0) return 0;
#endif
    if (std::strcmp(name, "gpu") == 0) return 0;
    return 0;
}

int csec_parallel_backend_implemented(const char* name) {
    if (!name) return 0;
    if (std::strcmp(name, "cpu") == 0) return 1;
    if (std::strcmp(name, "simd") == 0) return 1;
#if defined(_OPENMP)
    if (std::strcmp(name, "openmp") == 0) return 1;
#else
    if (std::strcmp(name, "openmp") == 0) return 0;
#endif
    if (std::strcmp(name, "gpu") == 0) return 0;
    return 0;
}

void csec_parallel_for_i32(int start, int end, void* context, csec_parallel_for_i32_fn callback) {
    if (!callback || end <= start) return;

    const long long count = static_cast<long long>(end) - start;
    long long requested = configuredParallelThreads();
    if (requested < 1) requested = 1;
    const long long workerCount = requested < count ? requested : count;

    if (workerCount <= 1) {
        for (int i = start; i < end; ++i) {
            callback(context, i);
        }
        return;
    }

    std::vector<std::thread> workers;
    int inlineStart = end;
    try {
        workers.reserve(static_cast<size_t>(workerCount));
    } catch (...) {
        inlineStart = start;
    }
    for (long long worker = 0; worker < workerCount && inlineStart == end; ++worker) {
        const int chunkStart = static_cast<int>(start + (count * worker) / workerCount);
        const int chunkEnd = static_cast<int>(start + (count * (worker + 1)) / workerCount);
        try {
            workers.emplace_back([=]() {
                for (int i = chunkStart; i < chunkEnd; ++i) {
                    callback(context, i);
                }
            });
        } catch (...) {
            inlineStart = chunkStart;
        }
    }
    for (int i = inlineStart; i < end; ++i) {
        callback(context, i);
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

long long csec_tcp_connect(const char* host, int port) {
    if (!host || port < 0 || port > 65535) return -1;
#ifdef _WIN32
    if (!ensureSocketRuntime()) return -1;
#else
    using SOCKET = NativeSocket;
    constexpr SOCKET INVALID_SOCKET = INVALID_SOCKET_HANDLE;
#endif

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portText[16];
    std::snprintf(portText, sizeof(portText), "%d", port);

    addrinfo* results = nullptr;
    if (getaddrinfo(host, portText, &hints, &results) != 0) {
        return -1;
    }

    SOCKET connected = INVALID_SOCKET;
    for (addrinfo* item = results; item; item = item->ai_next) {
        SOCKET candidate = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (candidate == INVALID_SOCKET) {
            continue;
        }
        if (connect(candidate, item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0) {
            connected = candidate;
            break;
        }
        closeSocketHandle(candidate);
    }

    freeaddrinfo(results);
    return connected == INVALID_SOCKET ? -1 : static_cast<long long>(connected);
}

long long csec_tcp_listen(const char* host, int port, int backlog) {
    if (port < 0 || port > 65535) return -1;
    if (backlog < 1) backlog = 1;
#ifdef _WIN32
    if (!ensureSocketRuntime()) return -1;
#else
    using SOCKET = NativeSocket;
    constexpr SOCKET INVALID_SOCKET = INVALID_SOCKET_HANDLE;
#endif

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char portText[16];
    std::snprintf(portText, sizeof(portText), "%d", port);

    addrinfo* results = nullptr;
    const char* bindHost = (host && host[0] != '\0') ? host : nullptr;
    if (getaddrinfo(bindHost, portText, &hints, &results) != 0) {
        return -1;
    }

    SOCKET listener = INVALID_SOCKET;
    for (addrinfo* item = results; item; item = item->ai_next) {
        SOCKET candidate = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (candidate == INVALID_SOCKET) {
            continue;
        }

        int enabled = 1;
#ifdef _WIN32
        setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enabled), sizeof(enabled));
#else
        setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
#endif

        if (bind(candidate, item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0 &&
            listen(candidate, backlog) == 0) {
            listener = candidate;
            break;
        }
        closeSocketHandle(candidate);
    }

    freeaddrinfo(results);
    return listener == INVALID_SOCKET ? -1 : static_cast<long long>(listener);
}

long long csec_tcp_accept(long long socket_handle) {
    if (socket_handle < 0) return -1;
#ifdef _WIN32
    SOCKET accepted = accept(static_cast<SOCKET>(socket_handle), nullptr, nullptr);
    return accepted == INVALID_SOCKET ? -1 : static_cast<long long>(accepted);
#else
    int accepted = accept(static_cast<int>(socket_handle), nullptr, nullptr);
    return accepted < 0 ? -1 : static_cast<long long>(accepted);
#endif
}

int csec_tcp_send(long long socket_handle, const char* data) {
    if (socket_handle < 0 || !data) return -1;
#ifdef _WIN32
    return send(static_cast<SOCKET>(socket_handle), data, static_cast<int>(std::strlen(data)), 0);
#else
    return static_cast<int>(send(static_cast<int>(socket_handle), data, std::strlen(data), 0));
#endif
}

char* csec_tcp_recv(long long socket_handle, int max_bytes) {
    if (max_bytes < 0) max_bytes = 0;
    char* buffer = static_cast<char*>(csecAlloc(static_cast<size_t>(max_bytes) + 1));
    if (!buffer) return nullptr;
    buffer[0] = '\0';
#ifdef _WIN32
    if (socket_handle < 0) return buffer;
    int count = recv(static_cast<SOCKET>(socket_handle), buffer, max_bytes, 0);
#else
    if (socket_handle < 0) return buffer;
    int count = static_cast<int>(recv(static_cast<int>(socket_handle), buffer, static_cast<size_t>(max_bytes), 0));
#endif
    if (count < 0) count = 0;
    buffer[count] = '\0';
    return buffer;
}

int csec_tcp_close(long long socket_handle) {
    if (socket_handle < 0) return -1;
#ifdef _WIN32
    return closesocket(static_cast<SOCKET>(socket_handle));
#else
    return close(static_cast<int>(socket_handle));
#endif
}

int csec_posix_open(const char* path, int flags, int mode) {
    if (!path) return -1;
#ifdef _WIN32
    return _open(path, flags, mode);
#else
    return open(path, flags, static_cast<mode_t>(mode));
#endif
}

char* csec_posix_read(int fd, int max_bytes) {
    if (max_bytes < 0) max_bytes = 0;
    char* buffer = static_cast<char*>(csecAlloc(static_cast<size_t>(max_bytes) + 1));
    if (!buffer) return nullptr;
    buffer[0] = '\0';
    if (fd < 0) return buffer;
#ifdef _WIN32
    int count = _read(fd, buffer, static_cast<unsigned int>(max_bytes));
#else
    int count = static_cast<int>(read(fd, buffer, static_cast<size_t>(max_bytes)));
#endif
    if (count < 0) count = 0;
    buffer[count] = '\0';
    return buffer;
}

int csec_posix_write(int fd, const char* data) {
    if (fd < 0 || !data) return -1;
#ifdef _WIN32
    return _write(fd, data, static_cast<unsigned int>(std::strlen(data)));
#else
    return static_cast<int>(write(fd, data, std::strlen(data)));
#endif
}

int csec_posix_close(int fd) {
    if (fd < 0) return -1;
#ifdef _WIN32
    return _close(fd);
#else
    return close(fd);
#endif
}

long long csec_posix_lseek(int fd, long long offset, int whence) {
    if (fd < 0) return -1;
#ifdef _WIN32
    return static_cast<long long>(_lseeki64(fd, offset, whence));
#else
    return static_cast<long long>(lseek(fd, static_cast<off_t>(offset), whence));
#endif
}

int csec_posix_unlink(const char* path) {
    if (!path) return -1;
#ifdef _WIN32
    return _unlink(path);
#else
    return unlink(path);
#endif
}

int csec_posix_rename(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return -1;
    return std::rename(old_path, new_path);
}

int csec_posix_mkdir(const char* path, int mode) {
    if (!path) return -1;
#ifdef _WIN32
    (void)mode;
    return _mkdir(path);
#else
    return mkdir(path, static_cast<mode_t>(mode));
#endif
}

int csec_posix_rmdir(const char* path) {
    if (!path) return -1;
#ifdef _WIN32
    return _rmdir(path);
#else
    return rmdir(path);
#endif
}

int csec_posix_chdir(const char* path) {
    if (!path) return -1;
#ifdef _WIN32
    return _chdir(path);
#else
    return chdir(path);
#endif
}

char* csec_posix_getcwd(void) {
#ifdef _WIN32
    return _getcwd(nullptr, 0);
#else
    return getcwd(nullptr, 0);
#endif
}

int csec_posix_access(const char* path, int mode) {
    if (!path) return -1;
#ifdef _WIN32
    return _access(path, mode);
#else
    return access(path, mode);
#endif
}

char* csec_posix_getenv(const char* name) {
    if (!name) return nullptr;
    const char* value = std::getenv(name);
    if (!value) value = "";
    size_t len = std::strlen(value);
    char* copy = static_cast<char*>(csecAlloc(len + 1));
    if (!copy) return nullptr;
    std::memcpy(copy, value, len + 1);
    return copy;
}

int csec_posix_setenv(const char* name, const char* value, int overwrite) {
    if (!name || !value) return -1;
#ifdef _WIN32
    if (!overwrite && std::getenv(name)) return 0;
    return _putenv_s(name, value);
#else
    return setenv(name, value, overwrite ? 1 : 0);
#endif
}

int csec_posix_unsetenv(const char* name) {
    if (!name) return -1;
#ifdef _WIN32
    return _putenv_s(name, "");
#else
    return unsetenv(name);
#endif
}

int csec_posix_sleep(int seconds) {
    if (seconds < 0) return -1;
#ifdef _WIN32
    Sleep(static_cast<DWORD>(seconds) * 1000U);
    return 0;
#else
    return static_cast<int>(sleep(static_cast<unsigned int>(seconds)));
#endif
}

long long csec_posix_time(void) {
    return static_cast<long long>(std::time(nullptr));
}

int csec_posix_errno(void) {
    return errno;
}

int csec_posix_flag_read_only(void) { return O_RDONLY; }
int csec_posix_flag_write_only(void) { return O_WRONLY; }
int csec_posix_flag_read_write(void) { return O_RDWR; }
int csec_posix_flag_create(void) { return O_CREAT; }
int csec_posix_flag_truncate(void) { return O_TRUNC; }
int csec_posix_flag_append(void) { return O_APPEND; }
int csec_posix_seek_set(void) { return SEEK_SET; }
int csec_posix_seek_cur(void) { return SEEK_CUR; }
int csec_posix_seek_end(void) { return SEEK_END; }
int csec_posix_access_exists(void) { return 0; }
int csec_posix_access_read(void) { return 4; }
int csec_posix_access_write(void) { return 2; }
int csec_posix_access_execute(void) { return 1; }

long long csec_load_library(const char* path) {
    if (!path) return 0;
    std::string resolvedPath = resolveSystemLibraryPath(path);
#ifdef _WIN32
    return reinterpret_cast<long long>(LoadLibraryA(resolvedPath.c_str()));
#else
    return reinterpret_cast<long long>(dlopen(resolvedPath.c_str(), RTLD_LAZY));
#endif
}

long long csec_get_symbol(long long library_handle, const char* symbol_name) {
    if (!symbol_name) return 0;
#ifdef _WIN32
    HMODULE module = reinterpret_cast<HMODULE>(library_handle);
    if (!module) {
        module = GetModuleHandleA(nullptr);
    }
    return reinterpret_cast<long long>(GetProcAddress(module, symbol_name));
#else
    void* handle = reinterpret_cast<void*>(library_handle);
    return reinterpret_cast<long long>(dlsym(handle ? handle : RTLD_DEFAULT, symbol_name));
#endif
}

int csec_close_library(long long library_handle) {
    if (!library_handle) return 0;
#ifdef _WIN32
    return FreeLibrary(reinterpret_cast<HMODULE>(library_handle)) ? 0 : -1;
#else
    return dlclose(reinterpret_cast<void*>(library_handle));
#endif
}

long long csec_call_native0(long long symbol) {
    using Fn = long long (*)();
    return symbol ? reinterpret_cast<Fn>(symbol)() : 0;
}

long long csec_call_native1(long long symbol, long long arg0) {
    using Fn = long long (*)(long long);
    return symbol ? reinterpret_cast<Fn>(symbol)(arg0) : 0;
}

long long csec_call_native2(long long symbol, long long arg0, long long arg1) {
    using Fn = long long (*)(long long, long long);
    return symbol ? reinterpret_cast<Fn>(symbol)(arg0, arg1) : 0;
}

long long csec_call_native3(long long symbol, long long arg0, long long arg1, long long arg2) {
    using Fn = long long (*)(long long, long long, long long);
    return symbol ? reinterpret_cast<Fn>(symbol)(arg0, arg1, arg2) : 0;
}

double csec_call_native_double0(long long symbol) {
    using Fn = double (*)();
    return symbol ? reinterpret_cast<Fn>(symbol)() : 0.0;
}

double csec_call_native_double1(long long symbol, double arg0) {
    using Fn = double (*)(double);
    return symbol ? reinterpret_cast<Fn>(symbol)(arg0) : 0.0;
}

double csec_call_native_double2(long long symbol, double arg0, double arg1) {
    using Fn = double (*)(double, double);
    return symbol ? reinterpret_cast<Fn>(symbol)(arg0, arg1) : 0.0;
}

}
