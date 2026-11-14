// Self-host lowering of dictionaries: the `{key: value, ...}` literal and `Dict<K, V>` locals and
// parameters (the direct compiler's counterpart is src/dict_support.cpp). A dictionary is a `ptr`
// handle into dict_runtime.inc; keys and values cross as i64 payloads. The self-host supports scalar
// keys (String, Int, Long, Char, Boolean) and scalar values (those plus Double/Float); list and nested
// dictionary values are compiled by the direct compiler only. Sets (`{1, 2, 3}`, `Set<T>`) reuse this
// machinery with `isSet`: only keys, created by csec_set_new and filled by csec_set_add.
//
// Every hook here first checks that the tokens name a dictionary, so modules without dictionaries
// (including the self-host compiler itself) lower exactly as before.

// This file is included inside an extern "C" region; its helpers are internal C++ functions.
extern "C++" {

struct CsecDictTypes {
    std::string key;
    std::string value;
    bool isSet = false;
};

static std::string csec_dict_canonical_type(const std::string& name) {
    if (name == "Bool") return "Boolean";
    if (name == "Real") return "Double";
    if (name == "Natural" || name == "Integer") return "Long";
    return name;
}

static bool csec_dict_scalar_type(const std::string& name) {
    return name == "Int" || name == "Long" || name == "Short" || name == "Byte" || name == "Double" ||
        name == "Float" || name == "Boolean" || name == "String" || name == "Char";
}

static bool csec_dict_types_valid(const CsecDictTypes& types) {
    return csec_dict_scalar_type(types.key) && (types.isSet || csec_dict_scalar_type(types.value));
}

static int csec_dict_kind(const std::string& name) {
    if (name == "Double" || name == "Float") return 1;
    if (name == "Boolean") return 2;
    if (name == "String") return 3;
    if (name == "Char") return 4;
    return 0;
}

// `Dict < K , V >` starting at `at`; on success fills `types` and returns the index after '>'.
static int csec_dict_parse_type(const char* tokens, int at, CsecDictTypes& types) {
    types.isSet = false;
    if (csec_native_token_is(tokens, at, 'I', "Set") && csec_native_token_is(tokens, at + 1, 'O', "<") &&
        csec_token_kind_at(tokens, at + 2) == 'I' &&
        (csec_native_token_is(tokens, at + 3, 'O', ">") || csec_native_token_is(tokens, at + 3, 'O', ">>"))) {
        types.isSet = true;
        types.key = csec_dict_canonical_type(csec_token_text_at(tokens, at + 2));
        types.value.clear();
        return at + 4;
    }
    if (!csec_native_token_is(tokens, at, 'I', "Dict") || !csec_native_token_is(tokens, at + 1, 'O', "<") ||
        csec_token_kind_at(tokens, at + 2) != 'I' || !csec_native_token_is(tokens, at + 3, 'O', ",") ||
        csec_token_kind_at(tokens, at + 4) != 'I') {
        return -1;
    }
    if (!csec_native_token_is(tokens, at + 5, 'O', ">") && !csec_native_token_is(tokens, at + 5, 'O', ">>")) return -1;
    types.key = csec_dict_canonical_type(csec_token_text_at(tokens, at + 2));
    types.value = csec_dict_canonical_type(csec_token_text_at(tokens, at + 4));
    return at + 6;
}

// The index of the ':' separating key and value in `[start, end)`, or -1.
static int csec_dict_entry_colon(const char* tokens, int start, int end) {
    int depth = 0;
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!std::strcmp(text, "(") || !std::strcmp(text, "[") || !std::strcmp(text, "{")) ++depth;
        else if (!std::strcmp(text, ")") || !std::strcmp(text, "]") || !std::strcmp(text, "}")) --depth;
        else if (depth == 0 && !std::strcmp(text, ":")) return cursor;
    }
    return -1;
}

// `{` at `open` whose matching `}` is `close` encloses a dictionary literal (`{}` or `{k: v, ...}`).
static bool csec_dict_literal_at(const char* tokens, int open, int close) {
    if (!csec_native_token_is(tokens, open, 'O', "{") || close <= open) return false;
    if (close == open + 1) return true;
    if (csec_token_kind_at(tokens, open + 1) == 'K') return false;
    int depth = 0;
    for (int cursor = open + 1; cursor < close; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!std::strcmp(text, "(") || !std::strcmp(text, "[") || !std::strcmp(text, "{")) ++depth;
        else if (!std::strcmp(text, ")") || !std::strcmp(text, "]") || !std::strcmp(text, "}")) --depth;
        else if (depth == 0) {
            if (!std::strcmp(text, ":")) return true;
            if (!std::strcmp(text, ";") || !std::strcmp(text, "=") || !std::strcmp(text, "->")) return false;
        }
    }
    return false;
}

// `{` at `open` whose matching `}` is `close` encloses a set literal: comma-separated elements
// (`{1, 2}` or `{x,}`) with no top-level ':', ';', '=', '->' or '<-'.
static bool csec_set_literal_at(const char* tokens, int open, int close) {
    if (!csec_native_token_is(tokens, open, 'O', "{") || close <= open + 1) return false;
    if (csec_token_kind_at(tokens, open + 1) == 'K') return false;
    int depth = 0;
    bool comma = false;
    for (int cursor = open + 1; cursor < close; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!std::strcmp(text, "(") || !std::strcmp(text, "[") || !std::strcmp(text, "{")) ++depth;
        else if (!std::strcmp(text, ")") || !std::strcmp(text, "]") || !std::strcmp(text, "}")) --depth;
        else if (depth == 0) {
            if (!std::strcmp(text, ",")) comma = true;
            else if (!std::strcmp(text, ":") || !std::strcmp(text, ";") || !std::strcmp(text, "=") ||
                     !std::strcmp(text, "->") || !std::strcmp(text, "<-")) return false;
        }
    }
    return comma;
}

// Split `[start, end)` at top-level commas.
static std::vector<std::pair<int, int>> csec_dict_split_items(const char* tokens, int start, int end) {
    std::vector<std::pair<int, int>> items;
    int depth = 0;
    int itemStart = start;
    for (int cursor = start; cursor < end; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!std::strcmp(text, "(") || !std::strcmp(text, "[") || !std::strcmp(text, "{")) ++depth;
        else if (!std::strcmp(text, ")") || !std::strcmp(text, "]") || !std::strcmp(text, "}")) --depth;
        else if (depth == 0 && !std::strcmp(text, ",")) {
            if (cursor > itemStart) items.emplace_back(itemStart, cursor);
            itemStart = cursor + 1;
        }
    }
    if (end > itemStart) items.emplace_back(itemStart, end);
    return items;
}

// The scalar type of an expression, as far as the self-host can tell from its tokens.
static std::string csec_dict_expression_type(const char* tokens, int start, int end) {
    if (end - start == 2 && csec_native_token_is(tokens, start, 'O', "-")) ++start;
    if (end - start == 1) {
        switch (csec_token_kind_at(tokens, start)) {
            case 'N': return "Int";
            case 'F': return "Double";
            case 'S': return "String";
            case 'C': return "Char";
            case 'B': return "Boolean";
            case 'I': {
                char* type = csec_lookup_visible_value_type(tokens, start, csec_token_text_at(tokens, start));
                std::string result = csec_dict_canonical_type(type && *type ? type : "Int");
                if (type && *type) std::free(type);
                return result;
            }
            default: break;
        }
    }
    if (csec_is_string_expression(tokens, start, end)) return "String";
    for (int cursor = start; cursor < end; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) == 'F') return "Double";
        if (csec_token_kind_at(tokens, cursor) == 'O') {
            const char* text = csec_token_text_at(tokens, cursor);
            if (!std::strcmp(text, "==") || !std::strcmp(text, "!=") || !std::strcmp(text, "<") ||
                !std::strcmp(text, ">") || !std::strcmp(text, "<=") || !std::strcmp(text, ">=")) return "Boolean";
        }
    }
    return "Int";
}

static std::string csec_dict_unify(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a == b) return a;
    const bool aNumber = a == "Int" || a == "Long" || a == "Short" || a == "Byte" || a == "Double" || a == "Float";
    const bool bNumber = b == "Int" || b == "Long" || b == "Short" || b == "Byte" || b == "Double" || b == "Float";
    if (aNumber && bNumber) {
        if (a == "Double" || b == "Double" || a == "Float" || b == "Float") return "Double";
        return "Long";
    }
    return a;
}

// Infer K and V from the entries of the literal `{ ... }` at [open, close].
static bool csec_dict_literal_types(const char* tokens, int open, int close, CsecDictTypes& types) {
    types.key.clear();
    types.value.clear();
    types.isSet = csec_set_literal_at(tokens, open, close);
    if (types.isSet) {
        for (const auto& item : csec_dict_split_items(tokens, open + 1, close)) {
            types.key = csec_dict_unify(types.key, csec_dict_expression_type(tokens, item.first, item.second));
        }
        return !types.key.empty();
    }
    for (const auto& item : csec_dict_split_items(tokens, open + 1, close)) {
        const int colon = csec_dict_entry_colon(tokens, item.first, item.second);
        if (colon < 0) return false;
        types.key = csec_dict_unify(types.key, csec_dict_expression_type(tokens, item.first, colon));
        types.value = csec_dict_unify(types.value, csec_dict_expression_type(tokens, colon + 1, item.second));
    }
    return !types.key.empty() && !types.value.empty();
}

// The key/value types of the dictionary named by the identifier at `at` (a local or a parameter).
static bool csec_dict_named_types(const char* tokens, int at, CsecDictTypes& types) {
    if (csec_token_kind_at(tokens, at) != 'I') return false;
    const char* name = csec_token_text_at(tokens, at);
    const int declaration = csec_find_visible_local(tokens, at, name);
    if (declaration >= 0) {
        if (csec_native_token_is(tokens, declaration + 2, 'O', ":")) {
            return csec_dict_parse_type(tokens, declaration + 3, types) >= 0;
        }
        if (!csec_native_token_is(tokens, declaration + 2, 'O', "=")) return false;
        const int value = declaration + 3;
        if (csec_native_token_is(tokens, value, 'O', "{")) {
            const int close = csec_find_closing_token(tokens, value, 1000000000, "{", "}");
            return close > value && (csec_dict_literal_at(tokens, value, close) || csec_set_literal_at(tokens, value, close)) &&
                csec_dict_literal_types(tokens, value, close, types);
        }
        // `val d = makeIndex(...)` where `def makeIndex(...): Dict<K, V>`.
        if (csec_token_kind_at(tokens, value) == 'I' && csec_native_token_is(tokens, value + 1, 'O', "(")) {
            const char* callee = csec_token_text_at(tokens, value);
            for (int def = 0; csec_token_kind_at(tokens, def) != 'E'; ++def) {
                if (!csec_native_token_is(tokens, def, 'K', "def") || !csec_native_token_is(tokens, def + 1, 'I', callee) ||
                    !csec_native_token_is(tokens, def + 2, 'O', "(")) continue;
                const int close = csec_find_closing_token(tokens, def + 2, 1000000000, "(", ")");
                return close > def && csec_native_token_is(tokens, close + 1, 'O', ":") &&
                    csec_dict_parse_type(tokens, close + 2, types) >= 0;
            }
        }
        return false;
    }
    const int function = csec_enclosing_function_declaration(tokens, at);
    if (function < 0) return false;
    for (int cursor = function; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "{") || csec_native_token_is(tokens, cursor, 'O', "=")) break;
        if (csec_native_token_is(tokens, cursor, 'I', name) && csec_native_token_is(tokens, cursor + 1, 'O', ":")) {
            return csec_dict_parse_type(tokens, cursor + 2, types) >= 0;
        }
    }
    return false;
}

// The local type of an initializer `[start, end)` that is a dictionary literal ("Dict") or starts
// with a read from a dictionary (`d[k]`, `d.get(...)` -> V; `d.size()` -> Long; `d.contains(k)` ->
// Boolean); a top-level comparison makes it Boolean. "" for every other initializer.
static std::string csec_dict_initializer_type(const char* tokens, int start, int end) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "";
    if (csec_native_token_is(tokens, start, 'O', "{")) {
        const int close = csec_find_closing_token(tokens, start, end, "{", "}");
        if (close != end - 1) return "";
        if (csec_dict_literal_at(tokens, start, close)) return "Dict";
        return csec_set_literal_at(tokens, start, close) ? "Set" : "";
    }
    if (csec_token_kind_at(tokens, start) != 'I') return "";
    const bool indexed = csec_native_token_is(tokens, start + 1, 'O', "[");
    const bool method = csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(");
    if (!indexed && !method) return "";
    CsecDictTypes types;
    if (!csec_dict_named_types(tokens, start, types) || !csec_dict_types_valid(types)) return "";
    if (types.isSet) {
        if (indexed) return "";
        const std::string setMethod = csec_token_text_at(tokens, start + 2);
        if (setMethod == "size" || setMethod == "length" || setMethod == "count") return "Long";
        if (setMethod == "contains" || setMethod == "has" || setMethod == "isEmpty" || setMethod == "add" ||
            setMethod == "remove" || setMethod == "discard") return "Boolean";
        return "";
    }
    int depth = 0;
    for (int cursor = start; cursor < end; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!std::strcmp(text, "(") || !std::strcmp(text, "[") || !std::strcmp(text, "{")) ++depth;
        else if (!std::strcmp(text, ")") || !std::strcmp(text, "]") || !std::strcmp(text, "}")) --depth;
        else if (depth == 0 && (!std::strcmp(text, "==") || !std::strcmp(text, "!=") || !std::strcmp(text, "<") ||
                                !std::strcmp(text, ">") || !std::strcmp(text, "<=") || !std::strcmp(text, ">="))) {
            return "Boolean";
        }
    }
    if (indexed) return types.value;
    const std::string methodName = csec_token_text_at(tokens, start + 2);
    if (methodName == "get") return types.value;
    if (methodName == "size" || methodName == "length" || methodName == "count") return "Long";
    if (methodName == "contains" || methodName == "has" || methodName == "containsKey" || methodName == "isEmpty" ||
        methodName == "remove") return "Boolean";
    return "";
}

// ---- assert(condition[, message]) ----

static bool csec_module_uses_assert(const char* tokens) {
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'I', "assert") && csec_native_token_is(tokens, cursor + 1, 'O', "(")) return true;
    }
    return false;
}

static std::string csec_assert_definitions(const char* tokens) {
    return csec_module_uses_assert(tokens) ? "declare void @csec_assert_fail(ptr, ptr)\n\n" : "";
}

// The statement `assert(c)` / `assert(c, "message")` at [cursor, next): a false condition calls
// csec_assert_fail (prints AssertionError and exits 1). "" when the statement is not an assert.
static std::string csec_emit_assert_statement(const char* tokens, int cursor, int next) {
    if (!csec_native_token_is(tokens, cursor, 'I', "assert") || !csec_native_token_is(tokens, cursor + 1, 'O', "(")) return "";
    const int close = csec_find_closing_token(tokens, cursor + 1, next, "(", ")");
    if (close <= cursor + 2) return "";
    const auto arguments = csec_dict_split_items(tokens, cursor + 2, close);
    if (arguments.empty()) return "";
    const std::string seed = std::to_string(cursor);
    std::string output = csec_emit_i1_expression(tokens, arguments[0].first, arguments[0].second, "%assert.cond." + seed);
    output += "  br i1 %assert.cond." + seed + ", label %assert.ok." + seed + ", label %assert.fail." + seed + "\n";
    output += "assert.fail." + seed + ":\n";
    std::string message = "null";
    if (arguments.size() >= 2) {
        output += csec_emit_ptr_expression(tokens, arguments[1].first, arguments[1].second, "%assert.message." + seed);
        message = "%assert.message." + seed;
    }
    output += "  call void @csec_assert_fail(ptr null, ptr " + message + ")\n";
    output += "  br label %assert.ok." + seed + "\n";
    output += "assert.ok." + seed + ":\n";
    return output;
}

static bool csec_module_uses_dicts(const char* tokens) {
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if ((csec_native_token_is(tokens, cursor, 'I', "Dict") || csec_native_token_is(tokens, cursor, 'I', "Set")) &&
            csec_native_token_is(tokens, cursor + 1, 'O', "<")) return true;
        if (csec_native_token_is(tokens, cursor, 'O', "{") && cursor > 0 &&
            (csec_native_token_is(tokens, cursor - 1, 'O', "=") || csec_native_token_is(tokens, cursor - 1, 'O', "(") ||
             csec_native_token_is(tokens, cursor - 1, 'O', ","))) {
            const int close = csec_find_closing_token(tokens, cursor, 1000000000, "{", "}");
            if (close > cursor && (csec_dict_literal_at(tokens, cursor, close) || csec_set_literal_at(tokens, cursor, close))) return true;
        }
    }
    return false;
}

static std::string csec_dict_definitions(const char* tokens) {
    if (!csec_module_uses_dicts(tokens)) return "";
    return
        "declare ptr @csec_dict_new(i32, i32, i32, i32)\n"
        "declare void @csec_dict_set(ptr, i64, i64)\n"
        "declare i64 @csec_dict_get(ptr, i64)\n"
        "declare i64 @csec_dict_get_or(ptr, i64, i64)\n"
        "declare i32 @csec_dict_contains(ptr, i64)\n"
        "declare i32 @csec_dict_remove(ptr, i64)\n"
        "declare i64 @csec_dict_size(ptr)\n"
        "declare i64 @csec_dict_key_at(ptr, i64)\n"
        "declare i64 @csec_dict_value_at(ptr, i64)\n"
        "declare void @csec_dict_clear(ptr)\n"
        "declare ptr @csec_dict_copy(ptr)\n"
        "declare ptr @csec_dict_to_string(ptr)\n"
        "declare ptr @csec_set_new(i32)\n"
        "declare i32 @csec_set_add(ptr, i64)\n\n";
}

// Emit `[start, end)` as a value of scalar `type` and widen it to an i64 payload named `payload`.
static std::string csec_dict_emit_payload(const char* tokens, int start, int end, const std::string& type, const std::string& payload) {
    const std::string raw = payload + ".raw";
    if (type == "String") {
        return csec_emit_ptr_expression(tokens, start, end, raw) + "  " + payload + " = ptrtoint ptr " + raw + " to i64\n";
    }
    if (type == "Long") return csec_emit_i64_expression(tokens, start, end, payload);
    if (type == "Double") {
        return csec_emit_f64_expression(tokens, start, end, raw) + "  " + payload + " = bitcast double " + raw + " to i64\n";
    }
    if (type == "Float") {
        return csec_emit_f32_expression(tokens, start, end, raw) +
            "  " + payload + ".wide = fpext float " + raw + " to double\n" +
            "  " + payload + " = bitcast double " + payload + ".wide to i64\n";
    }
    if (type == "Boolean") {
        return csec_emit_i1_expression(tokens, start, end, raw) + "  " + payload + " = zext i1 " + raw + " to i64\n";
    }
    if (type == "Char" || type == "Byte") {
        return csec_emit_i8_expression(tokens, start, end, raw) +
            "  " + payload + " = " + (type == "Char" ? "zext" : "sext") + " i8 " + raw + " to i64\n";
    }
    if (type == "Short") {
        return csec_emit_i16_expression(tokens, start, end, raw) + "  " + payload + " = sext i16 " + raw + " to i64\n";
    }
    return csec_emit_i32_expression(tokens, start, end, raw) + "  " + payload + " = sext i32 " + raw + " to i64\n";
}

// Convert the i64 payload `payload` of a value of `type` to the LLVM type `target`.
static std::string csec_dict_convert_payload(const std::string& payload, const std::string& type, const std::string& target, const std::string& result) {
    const bool floating = type == "Double" || type == "Float";
    if (target == "ptr") return "  " + result + " = inttoptr i64 " + payload + " to ptr\n";
    if (target == "double" || target == "float") {
        std::string output;
        std::string asDouble = payload + ".double";
        if (floating) output += "  " + asDouble + " = bitcast i64 " + payload + " to double\n";
        else output += "  " + asDouble + " = sitofp i64 " + payload + " to double\n";
        if (target == "double") return output + "  " + result + " = fadd double " + asDouble + ", 0.000000e+00\n";
        return output + "  " + result + " = fptrunc double " + asDouble + " to float\n";
    }
    std::string integer = payload;
    std::string output;
    if (floating) {
        integer = payload + ".int";
        output += "  " + payload + ".double = bitcast i64 " + payload + " to double\n";
        output += "  " + integer + " = fptosi double " + payload + ".double to i64\n";
    }
    if (target == "i64") return output + "  " + result + " = add i64 " + integer + ", 0\n";
    if (target == "i1") return output + "  " + result + " = icmp ne i64 " + integer + ", 0\n";
    return output + "  " + result + " = trunc i64 " + integer + " to " + target + "\n";
}

// A new dictionary from the literal at [open, close] with entry types `types`, as ptr `resultName`.
static std::string csec_dict_emit_literal(const char* tokens, int open, int close, const CsecDictTypes& types, const std::string& resultName) {
    if (types.isSet) {
        std::string output = "  " + resultName + " = call ptr @csec_set_new(i32 " + std::to_string(csec_dict_kind(types.key)) + ")\n";
        int index = 0;
        if (close > open + 1) {
            for (const auto& item : csec_dict_split_items(tokens, open + 1, close)) {
                const std::string element = resultName + ".element" + std::to_string(index++);
                output += csec_dict_emit_payload(tokens, item.first, item.second, types.key, element);
                output += "  " + element + ".added = call i32 @csec_set_add(ptr " + resultName + ", i64 " + element + ")\n";
            }
        }
        return output;
    }
    std::string output = "  " + resultName + " = call ptr @csec_dict_new(i32 " + std::to_string(csec_dict_kind(types.key)) +
        ", i32 " + std::to_string(csec_dict_kind(types.value)) + ", i32 0, i32 8)\n";
    int index = 0;
    for (const auto& item : csec_dict_split_items(tokens, open + 1, close)) {
        const int colon = csec_dict_entry_colon(tokens, item.first, item.second);
        if (colon < 0) continue;
        const std::string seed = resultName + ".entry" + std::to_string(index++);
        output += csec_dict_emit_payload(tokens, item.first, colon, types.key, seed + ".key");
        output += csec_dict_emit_payload(tokens, colon + 1, item.second, types.value, seed + ".value");
        output += "  call void @csec_dict_set(ptr " + resultName + ", i64 " + seed + ".key, i64 " + seed + ".value)\n";
    }
    return output;
}

// `val d = {...}`, `val d: Dict<K, V> = ...`, `val d = makeDict(...)`: the declaration at `cursor`.
static bool csec_dict_declaration(const char* tokens, int cursor, int next, int initializer, CsecDictTypes& types) {
    if (csec_native_token_is(tokens, cursor + 2, 'O', ":")) {
        return csec_dict_parse_type(tokens, cursor + 3, types) >= 0 && csec_dict_types_valid(types);
    }
    if (initializer < cursor) return false;
    if (csec_native_token_is(tokens, initializer + 1, 'O', "{")) {
        const int close = csec_find_closing_token(tokens, initializer + 1, next, "{", "}");
        return close > initializer + 1 &&
            (csec_dict_literal_at(tokens, initializer + 1, close) || csec_set_literal_at(tokens, initializer + 1, close)) &&
            csec_dict_literal_types(tokens, initializer + 1, close, types) && csec_dict_types_valid(types);
    }
    return csec_dict_named_types(tokens, cursor + 1, types) && csec_dict_types_valid(types);
}

static std::string csec_dict_emit_declaration(const char* tokens, int cursor, int next, int initializer,
                                              const CsecDictTypes& types, const std::string& storage) {
    std::string output = "  %" + storage + " = alloca ptr\n";
    const std::string name = csec_token_text_at(tokens, cursor + 1);
    const std::string value = "%" + name + ".dict." + std::to_string(cursor);
    if (initializer >= cursor && initializer + 1 < next) {
        const int close = csec_native_token_is(tokens, initializer + 1, 'O', "{")
            ? csec_find_closing_token(tokens, initializer + 1, next, "{", "}") : -1;
        if (close > initializer + 1) output += csec_dict_emit_literal(tokens, initializer + 1, close, types, value);
        else output += csec_emit_ptr_expression(tokens, initializer + 1, next, value);
        output += "  store ptr " + value + ", ptr %" + storage + "\n";
    } else {
        output += "  store ptr null, ptr %" + storage + "\n";
    }
    return output;
}

// Reads with result LLVM type `target` (i32, i64, i1, i8, i16, double, float or ptr):
//   d[key], d.get(key), d.get(key, default), d.size()/length()/count(), d.isEmpty(),
//   d.contains(key)/has/containsKey, d.remove(key), d.clear(), and a dictionary literal (ptr only).
// Returns "" when `[start, end)` is none of these.
static std::string csec_emit_dict_expression(const char* tokens, int start, int end, const std::string& target, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "";
    if (target == "ptr" && csec_native_token_is(tokens, start, 'O', "{")) {
        const int close = csec_find_closing_token(tokens, start, end, "{", "}");
        CsecDictTypes types;
        if (close == end - 1 && close > start + 1 &&
            (csec_dict_literal_at(tokens, start, close) || csec_set_literal_at(tokens, start, close)) &&
            csec_dict_literal_types(tokens, start, close, types)) {
            return csec_dict_emit_literal(tokens, start, close, types, resultName);
        }
        return "";
    }
    if (csec_token_kind_at(tokens, start) != 'I') return "";
    const bool indexed = csec_native_token_is(tokens, start + 1, 'O', "[");
    const bool method = csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(");
    if (!indexed && !method) return "";
    const int open = indexed ? start + 1 : start + 3;
    const int close = csec_find_closing_token(tokens, open, end, indexed ? "[" : "(", indexed ? "]" : ")");
    if (close != end - 1) return "";
    CsecDictTypes types;
    if (!csec_dict_named_types(tokens, start, types) || !csec_dict_types_valid(types)) {
        return "";
    }
    if (types.isSet && indexed) return "";
    const std::string handle = resultName + ".dict";
    std::string output = csec_emit_ptr_expression(tokens, start, start + 1, handle);
    const std::string methodName = method ? csec_token_text_at(tokens, start + 2) : "";
    const auto arguments = csec_dict_split_items(tokens, open + 1, close);
    if (types.isSet && (methodName == "add" || methodName == "discard") && arguments.size() == 1) {
        output += csec_dict_emit_payload(tokens, arguments[0].first, arguments[0].second, types.key, resultName + ".key");
        output += "  " + resultName + ".flag = call i32 @csec_" + std::string(methodName == "add" ? "set_add" : "dict_remove") +
            "(ptr " + handle + ", i64 " + resultName + ".key)\n";
        output += "  " + resultName + ".payload = sext i32 " + resultName + ".flag to i64\n";
        return output + csec_dict_convert_payload(resultName + ".payload", "Boolean", target, resultName);
    }
    if (types.isSet && methodName == "get") return "";
    if (indexed || (methodName == "get" && arguments.size() == 1)) {
        if (arguments.size() != 1) return "";
        output += csec_dict_emit_payload(tokens, arguments[0].first, arguments[0].second, types.key, resultName + ".key");
        output += "  " + resultName + ".payload = call i64 @csec_dict_get(ptr " + handle + ", i64 " + resultName + ".key)\n";
        return output + csec_dict_convert_payload(resultName + ".payload", types.value, target, resultName);
    }
    if (methodName == "get" && arguments.size() == 2) {
        output += csec_dict_emit_payload(tokens, arguments[0].first, arguments[0].second, types.key, resultName + ".key");
        output += csec_dict_emit_payload(tokens, arguments[1].first, arguments[1].second, types.value, resultName + ".fallback");
        output += "  " + resultName + ".payload = call i64 @csec_dict_get_or(ptr " + handle + ", i64 " + resultName +
            ".key, i64 " + resultName + ".fallback)\n";
        return output + csec_dict_convert_payload(resultName + ".payload", types.value, target, resultName);
    }
    if ((methodName == "size" || methodName == "length" || methodName == "count") && arguments.empty()) {
        output += "  " + resultName + ".payload = call i64 @csec_dict_size(ptr " + handle + ")\n";
        return output + csec_dict_convert_payload(resultName + ".payload", "Long", target, resultName);
    }
    if (methodName == "isEmpty" && arguments.empty()) {
        output += "  " + resultName + ".size = call i64 @csec_dict_size(ptr " + handle + ")\n";
        output += "  " + resultName + ".empty = icmp eq i64 " + resultName + ".size, 0\n";
        output += "  " + resultName + ".payload = zext i1 " + resultName + ".empty to i64\n";
        return output + csec_dict_convert_payload(resultName + ".payload", "Boolean", target, resultName);
    }
    if ((methodName == "contains" || methodName == "has" || methodName == "containsKey" || methodName == "remove") &&
        arguments.size() == 1) {
        output += csec_dict_emit_payload(tokens, arguments[0].first, arguments[0].second, types.key, resultName + ".key");
        output += "  " + resultName + ".flag = call i32 @csec_dict_" + std::string(methodName == "remove" ? "remove" : "contains") +
            "(ptr " + handle + ", i64 " + resultName + ".key)\n";
        output += "  " + resultName + ".payload = sext i32 " + resultName + ".flag to i64\n";
        return output + csec_dict_convert_payload(resultName + ".payload", "Boolean", target, resultName);
    }
    if (methodName == "clear" && arguments.empty()) {
        output += "  call void @csec_dict_clear(ptr " + handle + ")\n";
        return output + csec_dict_convert_payload("0", "Long", target, resultName);
    }
    return "";
}

// Statement `d[key] = value` or `d[key] op= value` (op one of + - * / %), `[cursor, next)`.
static std::string csec_emit_dict_store(const char* tokens, int cursor, int next) {
    if (csec_token_kind_at(tokens, cursor) != 'I' || !csec_native_token_is(tokens, cursor + 1, 'O', "[")) return "";
    const int close = csec_find_closing_token(tokens, cursor + 1, next, "[", "]");
    if (close <= cursor + 1 || close + 1 >= next) return "";
    const char* assignment = csec_token_text_at(tokens, close + 1);
    const bool plain = std::strcmp(assignment, "=") == 0;
    const bool compound = std::strcmp(assignment, "+=") == 0 || std::strcmp(assignment, "-=") == 0 ||
        std::strcmp(assignment, "*=") == 0 || std::strcmp(assignment, "/=") == 0 || std::strcmp(assignment, "%=") == 0;
    if (!plain && !compound) return "";
    CsecDictTypes types;
    if (!csec_dict_named_types(tokens, cursor, types) || !csec_dict_types_valid(types) || types.isSet) {
        return "";
    }
    const std::string seed = "%" + std::string(csec_token_text_at(tokens, cursor)) + ".dictset." + std::to_string(cursor);
    std::string output = csec_emit_ptr_expression(tokens, cursor, cursor + 1, seed + ".dict");
    output += csec_dict_emit_payload(tokens, cursor + 2, close, types.key, seed + ".key");
    int valueEnd = next;
    while (valueEnd > close + 2 && csec_native_token_is(tokens, valueEnd - 1, 'O', ";")) --valueEnd;
    if (plain) {
        output += csec_dict_emit_payload(tokens, close + 2, valueEnd, types.value, seed + ".value");
    } else {
        const bool floating = types.value == "Double" || types.value == "Float";
        if (types.value == "String" || types.value == "Boolean" || types.value == "Char") return "";
        output += "  " + seed + ".old = call i64 @csec_dict_get(ptr " + seed + ".dict, i64 " + seed + ".key)\n";
        const char op = assignment[0];
        if (floating) {
            output += csec_emit_f64_expression(tokens, close + 2, valueEnd, seed + ".rhs");
            output += "  " + seed + ".oldf = bitcast i64 " + seed + ".old to double\n";
            const char* instruction = op == '+' ? "fadd" : op == '-' ? "fsub" : op == '*' ? "fmul" : op == '/' ? "fdiv" : "frem";
            output += "  " + seed + ".nextf = " + instruction + " double " + seed + ".oldf, " + seed + ".rhs\n";
            output += "  " + seed + ".value = bitcast double " + seed + ".nextf to i64\n";
        } else {
            output += csec_dict_emit_payload(tokens, close + 2, valueEnd, "Long", seed + ".rhs");
            const char* instruction = op == '+' ? "add" : op == '-' ? "sub" : op == '*' ? "mul" : op == '/' ? "sdiv" : "srem";
            output += "  " + seed + ".value = " + instruction + " i64 " + seed + ".old, " + seed + ".rhs\n";
        }
    }
    output += "  call void @csec_dict_set(ptr " + seed + ".dict, i64 " + seed + ".key, i64 " + seed + ".value)\n";
    return output;
}

} // extern "C++"
