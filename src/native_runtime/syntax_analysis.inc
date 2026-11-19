char* csec_token_text_at(const char* tokens, int ordinal) {
    const char* value = tokens ? tokens : "";
    if (ordinal < 0) {
        return const_cast<char*>("");
    }

    const auto& texts = tokenTexts(value);
    if (static_cast<size_t>(ordinal) >= texts.size()) {
        return const_cast<char*>("");
    }
    return const_cast<char*>(texts[static_cast<size_t>(ordinal)].c_str());
}

static bool csec_native_token_is(const char* tokens, int ordinal, char kind, const char* text) {
    if (csec_token_kind_at(tokens, ordinal) != kind) return false;
    const char* tokenText = csec_token_text_at(tokens, ordinal);
    return std::strcmp(tokenText ? tokenText : "", text ? text : "") == 0;
}

int csec_token_is(const char* tokens, int ordinal, char kind, const char* text) {
    return csec_native_token_is(tokens, ordinal, kind, text) ? 1 : 0;
}

// A comma separates arguments of the call closing at `close` only when it sits outside every
// nested group, so `outer(inner(1, 2), 3)` keeps `inner(1, 2)` as a single argument.
static bool csec_is_argument_separator(const char* tokens, int close, int ordinal) {
    if (!csec_native_token_is(tokens, ordinal, 'O', ",")) return false;
    int depth = 0;
    for (int cursor = ordinal + 1; cursor < close; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "(") || csec_native_token_is(tokens, cursor, 'O', "[") ||
            csec_native_token_is(tokens, cursor, 'O', "{")) {
            ++depth;
        } else if (csec_native_token_is(tokens, cursor, 'O', ")") || csec_native_token_is(tokens, cursor, 'O', "]") ||
                   csec_native_token_is(tokens, cursor, 'O', "}")) {
            if (depth == 0) return false;
            --depth;
        }
    }
    return depth == 0;
}

static char* csec_copy_literal(const char* text) {
    return const_cast<char*>(text);
}

char* csec_statement_kind(const char* tokens, int ordinal) {
    const char kind = csec_token_kind_at(tokens, ordinal);
    const char* text = csec_token_text_at(tokens, ordinal);
    if (kind == 'K') {
        if (std::strcmp(text, "return") == 0) return csec_copy_literal("return");
        if (std::strcmp(text, "val") == 0) return csec_copy_literal("value");
        if (std::strcmp(text, "var") == 0) return csec_copy_literal("variable");
        if (std::strcmp(text, "if") == 0) return csec_copy_literal("if");
        if (std::strcmp(text, "for") == 0) return csec_copy_literal("for");
        if (std::strcmp(text, "while") == 0) return csec_copy_literal("while");
        if (std::strcmp(text, "map") == 0) return csec_copy_literal("map");
        if (std::strcmp(text, "pmap") == 0) return csec_copy_literal("pmap");
        if (std::strcmp(text, "reduce") == 0) return csec_copy_literal("reduce");
        if (std::strcmp(text, "preduce") == 0) return csec_copy_literal("preduce");
        if (std::strcmp(text, "filter") == 0) return csec_copy_literal("filter");
        if (std::strcmp(text, "def") == 0) return csec_copy_literal("function");
        if (std::strcmp(text, "object") == 0) return csec_copy_literal("object");
        if (std::strcmp(text, "unsafe") == 0) return csec_copy_literal("unsafe");
        if (std::strcmp(text, "unatomic") == 0) return csec_copy_literal("unatomic");
        if (std::strcmp(text, "constexpr") == 0) return csec_copy_literal("constexpr");
    }
    if (kind == 'E') return csec_copy_literal("eof");
    return csec_copy_literal("expression");
}

char* csec_expression_leaf_kind(const char* tokens, int start, int end) {
    if (end <= start) return csec_copy_literal("empty");
    switch (csec_token_kind_at(tokens, start)) {
        case 'N': return csec_copy_literal("integer");
        case 'F': return csec_copy_literal("float");
        case 'S': return csec_copy_literal("string");
        case 'R': return csec_copy_literal("regex");
        case 'C': return csec_copy_literal("char");
        case 'B': return csec_copy_literal("bool");
        case 'I': return csec_copy_literal("identifier");
        case 'K': {
            const char* text = csec_token_text_at(tokens, start);
            if (std::strcmp(text, "this") == 0) return csec_copy_literal("this");
            if (std::strcmp(text, "super") == 0) return csec_copy_literal("super");
            if (std::strcmp(text, "new") == 0) return csec_copy_literal("new");
            if (std::strcmp(text, "match") == 0) return csec_copy_literal("match");
            break;
        }
        default: break;
    }
    return csec_copy_literal("unknown");
}

char* csec_declared_value_type(const char* tokens, int declStart, int declEnd) {
    for (int cursor = declStart; cursor < declEnd && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', ":")) {
            return csec_token_text_at(tokens, cursor + 1);
        }
        if (csec_native_token_is(tokens, cursor, 'O', "=") || csec_native_token_is(tokens, cursor, 'O', ";")) {
            return csec_copy_literal("infer");
        }
    }
    return csec_copy_literal("infer");
}

char* csec_infer_expression_type(const char* tokens, int start, int end) {
    while (start < end && csec_token_kind_at(tokens, start) == 'M') ++start;
    while (end > start && csec_token_kind_at(tokens, end - 1) == 'M') --end;
    if (end <= start) return csec_copy_literal("Unit");
    switch (csec_token_kind_at(tokens, start)) {
        case 'N': return csec_copy_literal("Int");
        case 'F': return csec_copy_literal("Double");
        case 'S': return csec_copy_literal("String");
        case 'C': return csec_copy_literal("Char");
        case 'B': return csec_copy_literal("Boolean");
        case 'R': return csec_copy_literal("Regex");
        case 'I': {
            const char* name = csec_token_text_at(tokens, start);
            char* cached = csec_lookup_function_return_type(tokens, name);
            if (cached && std::strcmp(cached, "unknown") != 0) return cached;
            break;
        }
        default: break;
    }
    return csec_copy_literal("unknown");
}

char* csec_declared_local_type(const char* tokens, int start, int end) {
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', ":")) return csec_token_text_at(tokens, cursor + 1);
        if (csec_native_token_is(tokens, cursor, 'O', "=") || csec_native_token_is(tokens, cursor, 'O', ";")) break;
    }
    return csec_copy_literal("");
}

static int csec_class_member_start(const char* tokens, int ordinal) {
    if (csec_native_token_is(tokens, ordinal, 'K', "override") ||
        csec_native_token_is(tokens, ordinal, 'K', "unsafe") ||
        csec_native_token_is(tokens, ordinal, 'K', "constexpr")) return ordinal + 1;
    return ordinal;
}

char* csec_class_member_kind(const char* tokens, int ordinal) {
    const int start = csec_class_member_start(tokens, ordinal);
    if (csec_native_token_is(tokens, start, 'K', "def")) return csec_copy_literal("method");
    if (csec_native_token_is(tokens, start, 'K', "val")) return csec_copy_literal("field");
    if (csec_native_token_is(tokens, start, 'K', "var")) return csec_copy_literal("mutable-field");
    return csec_copy_literal("member");
}

char* csec_class_member_name(const char* tokens, int ordinal) {
    const int start = csec_class_member_start(tokens, ordinal);
    if (csec_native_token_is(tokens, start, 'K', "def")) {
        if (csec_native_token_is(tokens, start + 1, 'K', "operator")) {
            return csec_string_concat("operator", csec_token_text_at(tokens, start + 2));
        }
        return csec_token_text_at(tokens, start + 1);
    }
    if (csec_native_token_is(tokens, start, 'K', "val") || csec_native_token_is(tokens, start, 'K', "var")) {
        return csec_token_text_at(tokens, start + 1);
    }
    return csec_copy_literal("<member>");
}

int csec_parse_return_integer_in_range(const char* tokens, int start, int end) {
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'K', "return") && csec_token_kind_at(tokens, cursor + 1) == 'N') {
            return csec_to_int(csec_token_text_at(tokens, cursor + 1));
        }
    }
    return 0;
}

static int csec_native_find_closing_token(const char* tokens, int openOrdinal, int limit, const char* openText, const char* closeText) {
    int depth = 0;
    int cursor = openOrdinal;
    while (cursor < limit && csec_token_kind_at(tokens, cursor) != 'E') {
        const char* text = csec_token_text_at(tokens, cursor);
        if (std::strcmp(text, openText) == 0) {
            ++depth;
        } else if (std::strcmp(text, closeText) == 0) {
            --depth;
            if (depth == 0) return cursor;
        }
        ++cursor;
    }
    return -1;
}

int csec_advance_statement(const char* tokens, int ordinal, int bodyEnd) {
    int cursor = ordinal;
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    while (cursor < bodyEnd && csec_token_kind_at(tokens, cursor) != 'E') {
        const char* text = csec_token_text_at(tokens, cursor);
        if (csec_token_kind_at(tokens, cursor) == 'O') {
            if (std::strcmp(text, "(") == 0) {
                ++paren;
            }
            if (std::strcmp(text, ")") == 0) {
                --paren;
            }
            if (std::strcmp(text, "{") == 0) {
                ++brace;
            }
            if (std::strcmp(text, "}") == 0) {
                if (brace == 0 && paren == 0 && bracket == 0) {
                    return cursor;
                }
                --brace;
                if (brace == 0 && paren == 0 && bracket == 0) {
                    int possibleElse = cursor + 1;
                    while (possibleElse < bodyEnd && csec_token_kind_at(tokens, possibleElse) == 'M') {
                        ++possibleElse;
                    }
                    if (csec_native_token_is(tokens, ordinal, 'K', "if") &&
                        csec_native_token_is(tokens, possibleElse, 'K', "else")) {
                        int afterElse = possibleElse + 1;
                        while (afterElse < bodyEnd && csec_token_kind_at(tokens, afterElse) == 'M') {
                            ++afterElse;
                        }
                        if (csec_native_token_is(tokens, afterElse, 'K', "if")) {
                            return csec_advance_statement(tokens, afterElse, bodyEnd);
                        }
                        if (csec_native_token_is(tokens, afterElse, 'O', "{")) {
                            int elseEnd = csec_native_find_closing_token(tokens, afterElse, bodyEnd, "{", "}");
                            if (elseEnd > afterElse) return elseEnd + 1;
                        }
                    }
                    return cursor + 1;
                }
            }
            if (std::strcmp(text, "[") == 0) {
                ++bracket;
            }
            if (std::strcmp(text, "]") == 0) {
                --bracket;
            }
            if (std::strcmp(text, ";") == 0 && brace == 0 && paren == 0 && bracket == 0) {
                return cursor + 1;
            }
        }
        ++cursor;
    }
    return cursor;
}

static bool csec_native_starts_top_level_declaration(const char* tokens, int ordinal) {
    char kind = csec_token_kind_at(tokens, ordinal);
    const char* text = csec_token_text_at(tokens, ordinal);
    if (kind == 'E') return true;
    if (kind == 'O' && std::strcmp(text, "[@") == 0) return true;
    if (kind != 'K') return false;
    return std::strcmp(text, "import") == 0 || std::strcmp(text, "class") == 0 ||
           std::strcmp(text, "object") == 0 || std::strcmp(text, "external") == 0 ||
           std::strcmp(text, "def") == 0 || std::strcmp(text, "template") == 0 ||
           std::strcmp(text, "val") == 0 || std::strcmp(text, "var") == 0 ||
           std::strcmp(text, "unsafe") == 0 || std::strcmp(text, "unatomic") == 0 ||
           std::strcmp(text, "constexpr") == 0 ||
           (std::strcmp(text, "async") == 0 && csec_native_token_is(tokens, ordinal + 1, 'K', "def"));
}

int csec_starts_top_level_declaration(const char* tokens, int ordinal) {
    return csec_native_starts_top_level_declaration(tokens, ordinal) ? 1 : 0;
}

int csec_validate_top_level(const char* tokens) {
    int cursor = 0;
    while (csec_token_kind_at(tokens, cursor) == 'M') ++cursor;
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    bool expectingDecl = true;
    while (csec_token_kind_at(tokens, cursor) != 'E') {
        char kind = csec_token_kind_at(tokens, cursor);
        const char* text = csec_token_text_at(tokens, cursor);
        if (expectingDecl && paren == 0 && brace == 0 && bracket == 0 &&
            !csec_native_starts_top_level_declaration(tokens, cursor)) {
            return 0;
        }
        if (expectingDecl && paren == 0 && brace == 0 && bracket == 0) expectingDecl = false;
        if (kind == 'O') {
            if (std::strcmp(text, "(") == 0) ++paren;
            if (std::strcmp(text, ")") == 0) --paren;
            if (std::strcmp(text, "{") == 0) ++brace;
            if (std::strcmp(text, "}") == 0) {
                --brace;
                if (brace == 0 && paren == 0 && bracket == 0) expectingDecl = true;
            }
            if (std::strcmp(text, "[") == 0 || std::strcmp(text, "[@") == 0) ++bracket;
            if (std::strcmp(text, "]") == 0) --bracket;
            if (std::strcmp(text, ";") == 0 && brace == 0 && paren == 0 && bracket == 0) expectingDecl = true;
        }
        ++cursor;
        while (csec_token_kind_at(tokens, cursor) == 'M') ++cursor;
    }
    return 1;
}

char* csec_top_level_decl_kind(const char* tokens, int ordinal) {
    for (;;) {
        const char* text = csec_token_text_at(tokens, ordinal);
        const char kind = csec_token_kind_at(tokens, ordinal);
        if (kind == 'O' && std::strcmp(text, "[@") == 0) return const_cast<char*>("attribute");
        if (kind != 'K') return const_cast<char*>("unknown");
        if (std::strcmp(text, "external") == 0) {
            const char* next = csec_token_text_at(tokens, ordinal + 1);
            if (std::strcmp(next, "class") == 0) return const_cast<char*>("external-class");
            if (std::strcmp(next, "object") == 0) return const_cast<char*>("external-object");
            if (std::strcmp(next, "def") == 0) return const_cast<char*>("external-function");
            return const_cast<char*>("external");
        }
        if (std::strcmp(text, "unsafe") == 0 || std::strcmp(text, "unatomic") == 0 ||
            std::strcmp(text, "constexpr") == 0 || std::strcmp(text, "async") == 0) {
            ++ordinal;
            continue;
        }
        if (std::strcmp(text, "template") == 0) return const_cast<char*>("template");
        if (std::strcmp(text, "import") == 0) return const_cast<char*>("import");
        if (std::strcmp(text, "class") == 0) return const_cast<char*>("class");
        if (std::strcmp(text, "object") == 0) return const_cast<char*>("object");
        if (std::strcmp(text, "def") == 0) return const_cast<char*>("function");
        if (std::strcmp(text, "val") == 0) return const_cast<char*>("value");
        if (std::strcmp(text, "var") == 0) return const_cast<char*>("variable");
        return const_cast<char*>("unknown");
    }
}

char* csec_top_level_decl_name(const char* tokens, int ordinal) {
    // Declaration modifiers precede the declaring keyword: `async def f`, `unsafe def f`.
    while (csec_native_token_is(tokens, ordinal, 'K', "async") || csec_native_token_is(tokens, ordinal, 'K', "unsafe") ||
           csec_native_token_is(tokens, ordinal, 'K', "unatomic") || csec_native_token_is(tokens, ordinal, 'K', "constexpr")) {
        ++ordinal;
    }
    const char* kind = csec_top_level_decl_kind(tokens, ordinal);
    if (std::strcmp(kind, "external-class") == 0 || std::strcmp(kind, "external-object") == 0 ||
        std::strcmp(kind, "external-function") == 0) {
        return csec_top_level_decl_name(tokens, ordinal + 1);
    }
    if (std::strcmp(kind, "template") == 0) {
        int depth = 0;
        for (int cursor = ordinal + 1; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
            const char* text = csec_token_text_at(tokens, cursor);
            if (std::strcmp(text, "<") == 0) ++depth;
            if (std::strcmp(text, ">") == 0 && --depth == 0) return csec_top_level_decl_name(tokens, cursor + 1);
        }
        return const_cast<char*>("<template>");
    }
    if (std::strcmp(kind, "import") == 0) return csec_token_text_at(tokens, ordinal + 1);
    if (std::strcmp(kind, "function") == 0) {
        if (std::strcmp(csec_token_text_at(tokens, ordinal + 1), "operator") != 0) {
            return csec_token_text_at(tokens, ordinal + 1);
        }
        const char* suffix = csec_token_text_at(tokens, ordinal + 2);
        const size_t length = std::strlen(suffix) + 9;
        char* result = static_cast<char*>(csecAlloc(length));
        if (!result) return const_cast<char*>("operator");
        std::snprintf(result, length, "operator%s", suffix);
        return result;
    }
    if (std::strcmp(kind, "class") == 0 || std::strcmp(kind, "object") == 0 ||
        std::strcmp(kind, "value") == 0 || std::strcmp(kind, "variable") == 0) {
        return csec_token_text_at(tokens, ordinal + 1);
    }
    return const_cast<char*>("<anonymous>");
}

int csec_count_function_params(const char* tokens, int declStart) {
    const int paramStart = csec_function_param_end(tokens, declStart) < 0
        ? -1
        : csec_find_token_text_in_range(tokens, declStart, csec_function_param_end(tokens, declStart), "(");
    if (paramStart < 0) return 0;
    const int paramEnd = csec_function_param_end(tokens, declStart);
    if (paramEnd <= paramStart + 1) return 0;
    int count = 1;
    for (int cursor = paramStart + 1; cursor < paramEnd; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', ",")) ++count;
    }
    return count;
}

int csec_token_is_top_level_operator(const char* tokens, int ordinal, int group) {
    const char kind = csec_token_kind_at(tokens, ordinal);
    const char* text = csec_token_text_at(tokens, ordinal);
    const auto is = [text](const char* candidate) { return std::strcmp(text, candidate) == 0; };
    if (group == 1 && kind == 'O') return is("=") || is("<-") || is("+=") || is("-=") || is("*=") || is("/=") || is("%=");
    if (group == 2) return (kind == 'K' && is("or")) || (kind == 'O' && (is("||") || is("|")));
    if (group == 3) return (kind == 'K' && is("and")) || (kind == 'O' && is("&&"));
    if (group == 8) return (kind == 'K' && is("xor")) || (kind == 'O' && is("^"));
    if (group == 9 && kind == 'O') return is("&");
    if (group == 4 && kind == 'O') return is("==") || is("!=") || is("<") || is(">") || is("<=") || is(">=");
    if (group == 10 && kind == 'O') return is("<<") || is(">>");
    if (group == 5) return (kind == 'O' && is("..")) || (kind == 'K' && (is("to") || is("until")));
    if (group == 6 && kind == 'O') return is("+") || is("-");
    if (group == 7 && kind == 'O') return is("*") || is("/") || is("%") || is("@");
    if (group == 11 && kind == 'K') return is("inner") || is("outer") || is("tensor");
    return group == 12 && kind == 'O' && is(",");
}

int csec_find_top_level_operator(const char* tokens, int start, int end, int group) {
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    for (int cursor = end - 1; cursor >= start; --cursor) {
        const char* text = csec_token_text_at(tokens, cursor);
        if (csec_token_kind_at(tokens, cursor) == 'O') {
            if (std::strcmp(text, ")") == 0) ++paren;
            else if (std::strcmp(text, "(") == 0) --paren;
            else if (std::strcmp(text, "}") == 0) ++brace;
            else if (std::strcmp(text, "{") == 0) --brace;
            else if (std::strcmp(text, "]") == 0) ++bracket;
            else if (std::strcmp(text, "[") == 0) --bracket;
        }
        if (paren == 0 && brace == 0 && bracket == 0 &&
            csec_token_is_top_level_operator(tokens, cursor, group)) {
            return cursor;
        }
    }
    return -1;
}

int csec_find_top_level_match(const char* tokens, int start, int end) {
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        const char* text = csec_token_text_at(tokens, cursor);
        if (csec_token_kind_at(tokens, cursor) == 'O') {
            if (std::strcmp(text, "(") == 0) ++paren;
            else if (std::strcmp(text, ")") == 0) --paren;
            else if (std::strcmp(text, "{") == 0) ++brace;
            else if (std::strcmp(text, "}") == 0) --brace;
            else if (std::strcmp(text, "[") == 0) ++bracket;
            else if (std::strcmp(text, "]") == 0) --bracket;
        }
        if (paren == 0 && brace == 0 && bracket == 0 &&
            csec_token_is(tokens, cursor, 'K', "match")) {
            return cursor;
        }
    }
    return -1;
}

int csec_find_lambda_arrow(const char* tokens, int start, int end) {
    if (!tokens || start < 0 || end <= start) return -1;
    const int tokenCount = tokenCountCached(tokens);
    if (start >= tokenCount) return -1;
    if (end > tokenCount) end = tokenCount;
    if (end <= start) return -1;
    if (!csec_token_is(tokens, start, 'O', "[")) return -1;
    const int closeCapture = csec_find_closing_token(tokens, start, end, "[", "]");
    if (closeCapture < start || closeCapture + 1 >= end) return -1;
    int openParams = closeCapture + 1;
    while (openParams < end && csec_token_kind_at(tokens, openParams) == 'M') ++openParams;
    if (openParams >= end || !csec_token_is(tokens, openParams, 'O', "(")) return -1;
    const int closeParams = csec_find_closing_token(tokens, openParams, end, "(", ")");
    if (closeParams < openParams || closeParams + 1 >= end) return -1;
    int arrow = closeParams + 1;
    while (arrow < end && csec_token_kind_at(tokens, arrow) == 'M') ++arrow;
    return arrow < end && csec_token_is(tokens, arrow, 'O', "->") ? arrow : -1;
}

char* csec_generate_symbol_table(const char* tokens) {
    std::string output;
    if (!csec_validate_top_level(tokens)) {
        output = "error: parse failed\n";
    } else {
        output = "Symbols\n";
        int cursor = 0;
        while (csec_token_kind_at(tokens, cursor) == 'M') ++cursor;
        while (csec_token_kind_at(tokens, cursor) != 'E') {
            const char* kind = csec_top_level_decl_kind(tokens, cursor);
            const char* name = csec_top_level_decl_name(tokens, cursor);
            const int declEnd = csec_advance_top_level_decl(tokens, cursor);
            if (std::strcmp(kind, "function") == 0 || std::strcmp(kind, "external-function") == 0) {
                output += "Symbol function ";
                output += name;
                output += " params=" + std::to_string(csec_count_function_params(tokens, cursor));
                output += " returns=";
                output += csec_function_return_type_at(tokens, cursor);
                output += "\n";
                if (std::strcmp(kind, "external-function") == 0) {
                    output += "  External function ";
                    output += name;
                    output += "\n";
                }
            } else if (std::strcmp(kind, "value") == 0 || std::strcmp(kind, "variable") == 0) {
                output += "Symbol ";
                output += kind;
                output += " ";
                output += name;
                output += "\n";
            } else if (std::strcmp(kind, "import") == 0) {
                output += "Symbol import ";
                output += name;
                output += "\n";
            } else if (std::strcmp(kind, "class") == 0 || std::strcmp(kind, "object") == 0 ||
                       std::strcmp(kind, "external-class") == 0 || std::strcmp(kind, "external-object") == 0) {
                output += "Symbol type ";
                output += name;
                output += " kind=";
                output += kind;
                output += "\n";
            } else {
                output += "Symbol ";
                output += kind;
                output += " ";
                output += name;
                output += "\n";
            }
            cursor = declEnd;
            while (csec_token_kind_at(tokens, cursor) == 'M') ++cursor;
        }
    }
    char* result = static_cast<char*>(csecAlloc(output.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, output.c_str(), output.size() + 1);
    return result;
}

char* csec_append_expression_until(const char* tokens, int start, int end) {
    std::string output;
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        output += csec_token_text_at(tokens, cursor);
        if (cursor + 1 < end) output += " ";
    }
    char* result = static_cast<char*>(csecAlloc(output.size() + 1));
    if (!result) return nullptr;
    std::memcpy(result, output.c_str(), output.size() + 1);
    return result;
}

char* csec_ir_operator_name(const char* text) {
    const char* value = text ? text : "";
    if (std::strcmp(value, "+") == 0) return const_cast<char*>("add");
    if (std::strcmp(value, "-") == 0) return const_cast<char*>("sub");
    if (std::strcmp(value, "*") == 0) return const_cast<char*>("mul");
    if (std::strcmp(value, "/") == 0) return const_cast<char*>("sdiv");
    if (std::strcmp(value, "%") == 0) return const_cast<char*>("srem");
    if (std::strcmp(value, "<<") == 0) return const_cast<char*>("shl");
    if (std::strcmp(value, ">>") == 0) return const_cast<char*>("ashr");
    if (std::strcmp(value, "&") == 0 || std::strcmp(value, "and") == 0 || std::strcmp(value, "&&") == 0) return const_cast<char*>("and");
    if (std::strcmp(value, "|") == 0 || std::strcmp(value, "or") == 0 || std::strcmp(value, "||") == 0) return const_cast<char*>("or");
    if (std::strcmp(value, "^") == 0 || std::strcmp(value, "xor") == 0) return const_cast<char*>("xor");
    if (std::strcmp(value, "==") == 0) return const_cast<char*>("icmp_eq");
    if (std::strcmp(value, "!=") == 0) return const_cast<char*>("icmp_ne");
    if (std::strcmp(value, "<") == 0) return const_cast<char*>("icmp_slt");
    if (std::strcmp(value, "<=") == 0) return const_cast<char*>("icmp_sle");
    if (std::strcmp(value, ">") == 0) return const_cast<char*>("icmp_sgt");
    if (std::strcmp(value, ">=") == 0) return const_cast<char*>("icmp_sge");
    if (std::strcmp(value, "..") == 0) return const_cast<char*>("range");
    if (std::strcmp(value, "to") == 0) return const_cast<char*>("range_to");
    if (std::strcmp(value, "until") == 0) return const_cast<char*>("range_until");
    if (std::strcmp(value, "@") == 0) return const_cast<char*>("matmul");
    return const_cast<char*>(value);
}

int csec_expression_top_level_operator(const char* tokens, int start, int end) {
    static const int groups[] = {1, 2, 3, 8, 9, 4, 10, 5, 6, 7, 11};
    for (int group : groups) {
        const int ordinal = csec_find_top_level_operator(tokens, start, end, group);
        if (ordinal >= start) return ordinal;
    }
    return -1;
}

int csec_advance_top_level_decl(const char* tokens, int ordinal) {
    int cursor = ordinal;
    int paren = 0;
    int brace = 0;
    int bracket = 0;
    while (csec_token_kind_at(tokens, cursor) != 'E') {
        // A declaring keyword right after a modifier (`async def`, `unsafe def`) continues the
        // same declaration.
        const bool afterModifier = cursor > ordinal &&
            (csec_native_token_is(tokens, cursor - 1, 'K', "async") || csec_native_token_is(tokens, cursor - 1, 'K', "unsafe") ||
             csec_native_token_is(tokens, cursor - 1, 'K', "external") ||
             csec_native_token_is(tokens, cursor - 1, 'K', "unatomic") || csec_native_token_is(tokens, cursor - 1, 'K', "constexpr"));
        if (cursor > ordinal && paren == 0 && brace == 0 && bracket == 0 && !afterModifier &&
            csec_native_starts_top_level_declaration(tokens, cursor)) {
            return cursor;
        }
        if (csec_token_kind_at(tokens, cursor) == 'O') {
            const char* text = csec_token_text_at(tokens, cursor);
            if (std::strcmp(text, "(") == 0) ++paren;
            if (std::strcmp(text, ")") == 0) --paren;
            if (std::strcmp(text, "{") == 0) ++brace;
            if (std::strcmp(text, "}") == 0) {
                --brace;
                if (brace == 0 && paren == 0 && bracket == 0) return cursor + 1;
            }
            if (std::strcmp(text, "[") == 0 || std::strcmp(text, "[@") == 0) ++bracket;
            if (std::strcmp(text, "]") == 0) --bracket;
            if (std::strcmp(text, ";") == 0 && brace == 0 && paren == 0 && bracket == 0) {
                return cursor + 1;
            }
        }
        ++cursor;
    }
    return cursor;
}

// ---- Intrinsics (`[@Intrinsic] external def name(...)`, see LANGUAGE_SPEC.md 1.2) ----------------
// True when the declaration starting at `ordinal` (an `external` keyword) is preceded by
// `[@Intrinsic]` or `[@Intrinsic(...)]`.
static bool csec_decl_has_intrinsic_attribute(const char* tokens, int ordinal) {
    if (ordinal < 2 || !csec_native_token_is(tokens, ordinal - 1, 'O', "]")) return false;
    int depth = 0;
    for (int cursor = ordinal - 1; cursor >= 0; --cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "]")) ++depth;
        else if (csec_native_token_is(tokens, cursor, 'O', "[")) --depth;
        else if (csec_native_token_is(tokens, cursor, 'O', "[@")) {
            --depth;
            if (depth == 0) return csec_native_token_is(tokens, cursor + 1, 'I', "Intrinsic");
        }
        if (depth == 0) return false;
    }
    return false;
}

// True when some imported module declared `name` as an intrinsic. Only then does the self-host lower
// a call of `name` with its built-in meaning (mirrors the native compiler's declaredIntrinsics()).
static bool csec_is_declared_intrinsic(const char* tokens, const char* name) {
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'K', "external") &&
            csec_native_token_is(tokens, cursor + 1, 'K', "def") &&
            csec_native_token_is(tokens, cursor + 2, 'I', name) &&
            csec_decl_has_intrinsic_attribute(tokens, cursor)) {
            return true;
        }
    }
    return false;
}

int csec_find_decl_body_start(const char* tokens, int ordinal) {
    int tokenCount = tokenCountCached(tokens ? tokens : "");
    while (csec_native_token_is(tokens, ordinal, 'K', "async") || csec_native_token_is(tokens, ordinal, 'K', "unsafe") ||
           csec_native_token_is(tokens, ordinal, 'K', "unatomic") || csec_native_token_is(tokens, ordinal, 'K', "constexpr")) {
        ++ordinal;
    }
    for (int cursor = ordinal; cursor < tokenCount; ++cursor) {
        if (cursor > ordinal && csec_native_starts_top_level_declaration(tokens, cursor)) return -1;
        if (csec_native_token_is(tokens, cursor, 'O', "{")) return cursor + 1;
        if (csec_native_token_is(tokens, cursor, 'O', ";")) return -1;
    }
    return -1;
}

int csec_find_decl_body_end(const char* tokens, int bodyStart) {
    if (bodyStart < 0) return -1;
    int tokenCount = tokenCountCached(tokens ? tokens : "");
    int openBrace = bodyStart - 1;
    if (!csec_native_token_is(tokens, openBrace, 'O', "{")) return -1;
    int closeBrace = findMatchingBraceToken(tokens ? tokens : "", openBrace, tokenCount);
    return closeBrace;
}

int csec_find_token_text_in_range(const char* tokens, int start, int end, const char* text) {
    if (!tokens || !text) return -1;
    for (int ordinal = start < 0 ? 0 : start; ordinal < end && csec_token_kind_at(tokens, ordinal) != 'E'; ++ordinal) {
        const char* candidate = csec_token_text_at(tokens, ordinal);
        if (candidate && std::strcmp(candidate, text) == 0) return ordinal;
    }
    return -1;
}

int csec_find_closing_token(const char* tokens, int openOrdinal, int end, const char* openText, const char* closeText) {
    return csec_native_find_closing_token(tokens, openOrdinal, end, openText, closeText);
}

int csec_find_statement_paren_start(const char* tokens, int start, int end) {
    return csec_find_token_text_in_range(tokens, start, end, "(");
}

int csec_find_statement_paren_end(const char* tokens, int start, int end) {
    int openParen = csec_find_statement_paren_start(tokens, start, end);
    return openParen < 0 ? -1 : csec_native_find_closing_token(tokens, openParen, end, "(", ")");
}

int csec_find_statement_block_start(const char* tokens, int start, int end) {
    int closeParen = csec_find_statement_paren_end(tokens, start, end);
    return closeParen >= 0
        ? csec_find_token_text_in_range(tokens, closeParen + 1, end, "{")
        : csec_find_token_text_in_range(tokens, start, end, "{");
}

int csec_find_statement_block_end(const char* tokens, int start, int end) {
    int openBrace = csec_find_statement_block_start(tokens, start, end);
    return openBrace < 0 ? -1 : csec_native_find_closing_token(tokens, openBrace, end, "{", "}");
}

int csec_count_comma_separated(const char* tokens, int start, int end) {
    if (!tokens || end <= start) return 0;
    int count = 1;
    int paren = 0;
    int bracket = 0;
    int brace = 0;
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'O') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        if (!text) continue;
        if (std::strcmp(text, "(") == 0) ++paren;
        else if (std::strcmp(text, ")") == 0) --paren;
        else if (std::strcmp(text, "[") == 0) ++bracket;
        else if (std::strcmp(text, "]") == 0) --bracket;
        else if (std::strcmp(text, "{") == 0) ++brace;
        else if (std::strcmp(text, "}") == 0) --brace;
        else if (std::strcmp(text, ",") == 0 && paren == 0 && bracket == 0 && brace == 0) ++count;
    }
    return count;
}

int csec_find_last_top_level_token(const char* tokens, int start, int end, const char* text) {
    if (!tokens || !text) return -1;
    int paren = 0;
    int bracket = 0;
    int brace = 0;
    for (int cursor = end - 1; cursor >= start; --cursor) {
        const char* current = csec_token_text_at(tokens, cursor);
        if (csec_token_kind_at(tokens, cursor) == 'O' && current) {
            if (std::strcmp(current, ")") == 0) ++paren;
            else if (std::strcmp(current, "(") == 0) --paren;
            else if (std::strcmp(current, "]") == 0) ++bracket;
            else if (std::strcmp(current, "[") == 0) --bracket;
            else if (std::strcmp(current, "}") == 0) ++brace;
            else if (std::strcmp(current, "{") == 0) --brace;
        }
        if (paren == 0 && bracket == 0 && brace == 0 && current && std::strcmp(current, text) == 0) return cursor;
    }
    return -1;
}

int csec_function_param_end(const char* tokens, int declStart) {
    int declEnd = csec_advance_top_level_decl(tokens, declStart);
    for (int cursor = declStart; cursor < declEnd && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "(")) {
            return csec_native_find_closing_token(tokens, cursor, declEnd, "(", ")");
        }
        if (csec_native_token_is(tokens, cursor, 'O', "{") || csec_native_token_is(tokens, cursor, 'O', ";")) {
            break;
        }
    }
    return -1;
}

int csec_find_function_param_start(const char* tokens, int declStart) {
    for (int cursor = declStart; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "(")) return cursor;
        if (csec_native_token_is(tokens, cursor, 'O', "{") || csec_native_token_is(tokens, cursor, 'O', ";")) return -1;
    }
    return -1;
}

}

static bool csec_is_i64_type_name(const std::string& typeName) {
    return typeName == "Long" || typeName == "Natural" || typeName == "Integer";
}

static bool csec_is_i8_type_name(const std::string& typeName) {
    return typeName == "Char" || typeName == "Byte";
}

static bool csec_is_i16_type_name(const std::string& typeName) {
    return typeName == "Short";
}

// Method names may contain operator characters (`operator+`), which are not valid in an LLVM
// symbol. Map each operator character to a readable word so the generated name is a legal
// identifier and stays stable between the definition and its call sites. Names without operator
// characters pass through unchanged, so ordinary methods are unaffected.
static std::string csec_mangle_method_name(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') { result += c; continue; }
        switch (c) {
            case '+': result += "_add"; break;
            case '-': result += "_sub"; break;
            case '*': result += "_mul"; break;
            case '/': result += "_div"; break;
            case '%': result += "_mod"; break;
            case '=': result += "_eq"; break;
            case '<': result += "_lt"; break;
            case '>': result += "_gt"; break;
            case '!': result += "_bang"; break;
            case '&': result += "_amp"; break;
            case '|': result += "_pipe"; break;
            case '^': result += "_caret"; break;
            case '~': result += "_tilde"; break;
            default: result += "_op"; break;
        }
    }
    return result;
}

static const char* csec_llvm_type_for_name(const std::string& typeName) {
    if (typeName == "Int") return "i32";
    if (csec_is_i16_type_name(typeName)) return "i16";
    if (csec_is_i64_type_name(typeName)) return "i64";
    if (typeName == "Boolean") return "i1";
    if (csec_is_i8_type_name(typeName)) return "i8";
    if (typeName == "Double") return "double";
    if (typeName == "Float") return "float";
    return "ptr";
}

static char* csec_owned_string(const std::string& text) {
    char* result = static_cast<char*>(csecAlloc(text.size() + 1));
    if (!result) return const_cast<char*>("");
    std::memcpy(result, text.c_str(), text.size() + 1);
    return result;
}

static int csec_expression_ast_skip_trivia(const char* tokens, int ordinal) {
    while (csec_token_kind_at(tokens, ordinal) == 'M') ++ordinal;
    return ordinal;
}

static int csec_expression_ast_trim_end(const char* tokens, int start, int end) {
    while (end > start && csec_token_kind_at(tokens, end - 1) == 'M') --end;
    return end > start && csec_native_token_is(tokens, end - 1, 'O', ";") ? end - 1 : end;
}

static std::string csec_expression_ast_leaf_kind(const char* tokens, int start, int end) {
    if (end <= start) return "empty";
    switch (csec_token_kind_at(tokens, start)) {
    case 'N': return "integer";
    case 'F': return "float";
    case 'S': return "string";
    case 'R': return "regex";
    case 'C': return "char";
    case 'B': return "bool";
    case 'I': return "identifier";
    case 'K': {
        const char* text = csec_token_text_at(tokens, start);
        if (std::strcmp(text, "this") == 0 || std::strcmp(text, "super") == 0 ||
            std::strcmp(text, "new") == 0 || std::strcmp(text, "match") == 0) return text;
        break;
    }
    default: break;
    }
    return "unknown";
}

static std::string csec_expression_ast_postfix(const char* tokens, int start, int end) {
    if (end <= start) return "";
    if (csec_native_token_is(tokens, end - 1, 'O', ")")) {
        const int open = csec_find_last_top_level_token(tokens, start, end, "(");
        if (open > start) return "Expr call target=" + std::string(csec_append_expression_until(tokens, start, open)) +
            " argc=" + std::to_string(csec_count_comma_separated(tokens, open + 1, end - 1));
    }
    if (csec_native_token_is(tokens, end - 1, 'O', "]")) {
        const int open = csec_find_last_top_level_token(tokens, start, end, "[");
        if (open > start) return "Expr index target=" + std::string(csec_append_expression_until(tokens, start, open)) +
            " argc=" + std::to_string(csec_count_comma_separated(tokens, open + 1, end - 1));
    }
    const int dot = csec_find_last_top_level_token(tokens, start, end, ".");
    if (dot > start) return "Expr member " + std::string(csec_append_expression_until(tokens, start, dot)) + "." +
        csec_append_expression_until(tokens, dot + 1, end);
    return "";
}

static std::string csec_generate_expression_ast_impl(const char* tokens, int rawStart, int rawEnd) {
    const int start = csec_expression_ast_skip_trivia(tokens, rawStart);
    const int end = csec_expression_ast_trim_end(tokens, start, rawEnd);
    if (end <= start) return "Expr empty";
    const auto expressionText = [&] { return std::string(csec_append_expression_until(tokens, start, end)); };
    if (end - start > 32) return "Expr " + csec_expression_ast_leaf_kind(tokens, start, end) + " " + expressionText();

    if (csec_native_token_is(tokens, start, 'O', "[")) {
        const int arrow = csec_find_lambda_arrow(tokens, start, end);
        if (arrow >= 0) {
            const int captureClose = csec_find_closing_token(tokens, start, end, "[", "]");
            const std::string capture = captureClose <= start + 1 ? "none" : csec_append_expression_until(tokens, start + 1, captureClose);
            const int paramOpen = csec_expression_ast_skip_trivia(tokens, captureClose + 1);
            const int paramClose = csec_find_closing_token(tokens, paramOpen, end, "(", ")");
            return "Expr lambda capture=" + capture + " params=" + std::to_string(csec_count_comma_separated(tokens, paramOpen + 1, paramClose));
        }
    }

    const int matchAt = csec_find_top_level_match(tokens, start, end);
    if (matchAt > start) {
        const int open = csec_find_token_text_in_range(tokens, matchAt, end, "{");
        int cases = 0;
        if (open >= 0) {
            const int close = csec_find_closing_token(tokens, open, end, "{", "}");
            for (int cursor = open + 1; cursor < close && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
                if (csec_native_token_is(tokens, cursor, 'K', "case")) ++cases;
            }
        }
        return "Expr match cases=" + std::to_string(cases) + " [" + csec_generate_expression_ast_impl(tokens, start, matchAt) + "]";
    }
    if (csec_native_token_is(tokens, start, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start, end, "(", ")");
        if (close == end - 1) return "Expr group (" + csec_generate_expression_ast_impl(tokens, start + 1, end - 1) + ")";
    }

    const int groups[] = {1, 2, 3, 8, 9, 4, 10, 5, 6, 7, 11};
    for (const int group : groups) {
        const int op = csec_find_top_level_operator(tokens, start, end, group);
        if (op < start || ((group == 6 || group == 7 || group == 11) && op <= start)) continue;
        const char* category = group == 1 ? "assign" : group == 4 ? "compare" : group == 10 ? "shift" :
            group == 5 ? "range" : group == 11 ? "tensor" : "binary";
        return "Expr " + std::string(category) + " " + csec_token_text_at(tokens, op) + " [" +
            csec_generate_expression_ast_impl(tokens, start, op) + "] [" + csec_generate_expression_ast_impl(tokens, op + 1, end) + "]";
    }

    if (csec_token_kind_at(tokens, start) == 'O') {
        const char* text = csec_token_text_at(tokens, start);
        if (std::strcmp(text, "!") == 0 || std::strcmp(text, "-") == 0 || std::strcmp(text, "+") == 0 ||
            std::strcmp(text, "~") == 0 || std::strcmp(text, "*") == 0 || std::strcmp(text, "&") == 0 ||
            std::strcmp(text, "<-") == 0 || std::strcmp(text, "++") == 0 || std::strcmp(text, "--") == 0) {
            return "Expr unary " + std::string(text) + " [" + csec_generate_expression_ast_impl(tokens, start + 1, end) + "]";
        }
    }
    if (end > start + 1 && csec_token_kind_at(tokens, end - 1) == 'O') {
        const char* text = csec_token_text_at(tokens, end - 1);
        if (std::strcmp(text, "++") == 0 || std::strcmp(text, "--") == 0) {
            return "Expr postfix " + std::string(text) + " [" + csec_generate_expression_ast_impl(tokens, start, end - 1) + "]";
        }
    }
    const std::string postfix = csec_expression_ast_postfix(tokens, start, end);
    if (!postfix.empty()) return postfix;
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(") &&
        csec_find_closing_token(tokens, start + 1, end, "(", ")") == end - 1) {
        return "Expr call " + std::string(csec_token_text_at(tokens, start)) + "(" + csec_append_expression_until(tokens, start + 2, end - 1) + ")";
    }
    if (csec_native_token_is(tokens, start, 'O', "[")) return "Expr array count=" +
        std::to_string(csec_count_comma_separated(tokens, start + 1, end - 1)) + " [" + csec_append_expression_until(tokens, start + 1, end - 1) + "]";
    if (csec_native_token_is(tokens, start, 'K', "new")) return "Expr new " + expressionText();
    if (csec_native_token_is(tokens, start, 'K', "match")) return "Expr match";
    return "Expr " + csec_expression_ast_leaf_kind(tokens, start, end) + " " + expressionText();
}

static std::string csec_generate_statement_ast_impl(const char* tokens, int start, int end) {
    const bool keyword = csec_token_kind_at(tokens, start) == 'K';
    const std::string kind = keyword ? csec_token_text_at(tokens, start) : "expression";
    if (kind == "return") return "  Stmt return " + csec_generate_expression_ast_impl(tokens, start + 1, end) + "\n";
    if (kind == "val" || kind == "var") {
        const int initializer = csec_find_top_level_operator(tokens, start, end, 1);
        std::string output = "  Stmt " + std::string(kind == "val" ? "value" : "variable") + " " + csec_token_text_at(tokens, start + 1);
        if (initializer >= start) output += " = " + csec_generate_expression_ast_impl(tokens, initializer + 1, end);
        return output + "\n";
    }
    if (kind == "if" || kind == "for" || kind == "while" || kind == "map" || kind == "pmap" || kind == "reduce" || kind == "preduce" || kind == "filter") {
        const int open = csec_find_statement_paren_start(tokens, start, end);
        const int close = csec_find_statement_paren_end(tokens, start, end);
        return "  Stmt " + kind + " (" + (open >= 0 && close > open ? csec_append_expression_until(tokens, open + 1, close) : "") + ")\n";
    }
    if (kind == "unsafe" || kind == "unatomic" || kind == "constexpr") return "  Stmt " + kind + "\n";
    if (kind == "def" || kind == "object") return "  Stmt " + std::string(kind == "def" ? "function" : "object") + " " + csec_token_text_at(tokens, start + 1) + "\n";
    return "  Stmt expression " + csec_generate_expression_ast_impl(tokens, start, end) + "\n";
}

static std::string csec_generate_body_ast_impl(const char* tokens, int bodyStart, int bodyEnd) {
    std::string output;
    int cursor = csec_expression_ast_skip_trivia(tokens, bodyStart);
    while (cursor >= 0 && cursor < bodyEnd && csec_token_kind_at(tokens, cursor) != 'E') {
        const int next = csec_advance_statement(tokens, cursor, bodyEnd);
        if (next <= cursor) return output + "  Stmt error\n";
        output += csec_generate_statement_ast_impl(tokens, cursor, next);
        cursor = csec_expression_ast_skip_trivia(tokens, next);
    }
    return output;
}

static std::string csec_c_expression(const char* tokens, int start, int rawEnd) {
    const int end = csec_expression_ast_trim_end(tokens, start, rawEnd);
    std::string output;
    for (int cursor = start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        const char* text = csec_token_text_at(tokens, cursor);
        if (csec_native_token_is(tokens, cursor, 'K', "and")) output += "&&";
        else if (csec_native_token_is(tokens, cursor, 'K', "or")) output += "||";
        else if (csec_native_token_is(tokens, cursor, 'K', "xor")) output += "^";
        else output += text;
        if (cursor + 1 < end) output += " ";
    }
    return output;
}

static const char* csec_c_type(const std::string& type) {
    if (type == "Boolean" || type == "Int") return "int";
    if (type == "Char" || type == "Byte") return "char";
    if (csec_is_i64_type_name(type)) return "long long";
    if (type == "Float") return "float";
    if (type == "Double") return "double";
    if (type == "String" || type == "Regex") return "const char*";
    if (type.rfind("Array", 0) == 0) return "void*";
    if (type == "Unit") return "void";
    return "void*";
}

char* csec_c_type_name(const char* typeName) {
    return csec_copy_literal(csec_c_type(typeName ? typeName : ""));
}

char* csec_empty_string(void) {
    return csec_copy_literal("");
}

char* csec_llvm_main_body_fallback(void) {
    return csec_copy_literal("  ret i32 0\n");
}

char* csec_llvm_main_body(void) {
    return csec_copy_literal(
        "  %argc = call i32 @csec_command_line_arg_count()\n"
        "  %provided = icmp sge i32 %argc, 4\n"
        "  br i1 %provided, label %compile, label %missing\n"
        "compile:\n"
        "  %input = call ptr @csec_command_line_arg(i32 1)\n"
        "  %output = call ptr @csec_command_line_arg(i32 2)\n"
        "  %mode = call ptr @csec_command_line_arg(i32 3)\n"
        "  %status = call i32 @compileFile(ptr %input, ptr %output, ptr %mode)\n"
        "  ret i32 %status\n"
        "missing:\n"
        "  ret i32 1\n");
}

char* csec_llvm_body_fallback(int kind) {
    switch (kind) {
        case 1: return csec_copy_literal("  ret i1 false\n");
        case 2: return csec_copy_literal("  ret i8 0\n");
        case 3: return csec_copy_literal("  ret double 0.0\n");
        case 4: return csec_copy_literal("  ret i64 0\n");
        case 5: return csec_copy_literal("  ret ptr null\n");
        default: return csec_copy_literal("  ret i32 0\n");
    }
}

// Defined in llvm_dicts.inc: the type of a dictionary-valued initializer, or "" (see there).
extern "C++" {
static std::string csec_dict_initializer_type(const char* tokens, int start, int end);
}

static std::string csec_c_local_type(const char* tokens, int start, int end, int initializer) {
    // `val d = {k: v}` is a Dict; `val x = d[k]` / `d.get(..)` / `d.size()` take the dictionary's
    // types. Checked first because the literal's own ':' would otherwise be read as an annotation.
    if (initializer >= start && !csec_native_token_is(tokens, start + 2, 'O', ":")) {
        const std::string dictType = csec_dict_initializer_type(tokens, initializer + 1, end);
        if (!dictType.empty()) return dictType;
    }
    for (int cursor = start; cursor < end; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', ":")) {
            std::string type;
            for (int pos = cursor + 1; pos < end && !csec_native_token_is(tokens, pos, 'O', "=") && !csec_native_token_is(tokens, pos, 'O', ";"); ++pos) type += csec_token_text_at(tokens, pos);
            if (!type.empty()) return type;
        }
    }
    if (initializer >= start) {
        const char kind = csec_token_kind_at(tokens, initializer + 1);
        // `val f = async work(1)` holds a Future handle (a ptr).
        if (csec_native_token_is(tokens, initializer + 1, 'K', "async")) return "Future";
        if (kind == 'N') return "Int";
        if (kind == 'F') return "Double";
        if (kind == 'S') return "String";
        if (kind == 'C') return "Char";
        if (kind == 'B') return "Boolean";
        if (kind == 'I' && csec_native_token_is(tokens, initializer + 2, 'O', "(")) {
            const char* name = csec_token_text_at(tokens, initializer + 1);
            for (int declaration = 0; csec_token_kind_at(tokens, declaration) != 'E'; ++declaration) {
                if (!csec_native_token_is(tokens, declaration, 'K', "def") ||
                    !csec_native_token_is(tokens, declaration + 1, 'I', name) ||
                    !csec_native_token_is(tokens, declaration + 2, 'O', "(")) continue;
                const int close = csec_find_closing_token(tokens, declaration + 2, 1000000000, "(", ")");
                if (close < declaration + 2 || !csec_native_token_is(tokens, close + 1, 'O', ":")) break;
                std::string returnType;
                for (int pos = close + 2; csec_token_kind_at(tokens, pos) != 'E' &&
                     !csec_native_token_is(tokens, pos, 'O', "{") && !csec_native_token_is(tokens, pos, 'O', "="); ++pos) {
                    returnType += csec_token_text_at(tokens, pos);
                }
                if (!returnType.empty()) return returnType;
                break;
            }
        }
        if (kind == 'I' && csec_native_token_is(tokens, initializer + 2, 'O', "[")) {
            const char* name = csec_token_text_at(tokens, initializer + 1);
            for (int declaration = 0; declaration < start; ++declaration) {
                if ((!csec_native_token_is(tokens, declaration, 'K', "val") && !csec_native_token_is(tokens, declaration, 'K', "var")) ||
                    !csec_native_token_is(tokens, declaration + 1, 'I', name) ||
                    !csec_native_token_is(tokens, declaration + 2, 'O', ":")) continue;
                const int typeEnd = csec_find_token_text_in_range(tokens, declaration + 3, start, "=");
                const int open = csec_find_token_text_in_range(tokens, declaration + 3, typeEnd >= 0 ? typeEnd : start, "[");
                const int close = open >= 0 ? csec_find_closing_token(tokens, open, typeEnd >= 0 ? typeEnd : start, "[", "]") : -1;
                if (close == open + 2 && csec_token_kind_at(tokens, open + 1) == 'I') return csec_token_text_at(tokens, open + 1);
            }
        }
        if (kind == 'I' && csec_native_token_is(tokens, initializer + 2, 'O', ".") &&
            csec_token_kind_at(tokens, initializer + 3) == 'I') {
            const char* method = csec_token_text_at(tokens, initializer + 3);
            if (std::strcmp(method, "trim") == 0 || std::strcmp(method, "toUpper") == 0 ||
                std::strcmp(method, "toLower") == 0 || std::strcmp(method, "toString") == 0 ||
                std::strcmp(method, "substring") == 0) return "String";
            // A user class instance method call whose resolved overload returns String makes the
            // local a String; otherwise `val g = obj.greet(...)` was typed Int and lowered through
            // the i32 path, allocating an i32 slot for a pointer result. (csec_instance_call_
            // returns_string is declared in NativeRuntime.h so it is callable from this region.)
            if (csec_native_token_is(tokens, initializer + 4, 'O', "(")) {
                const int methodClose = csec_find_closing_token(tokens, initializer + 4, end, "(", ")");
                if (methodClose > initializer + 4 &&
                    csec_instance_call_returns_string(tokens, initializer + 1, methodClose) != 0) {
                    return "String";
                }
            }
        }
    }
    return "Int";
}

static std::string csec_generate_c_body_impl(const char* tokens, int bodyStart, int bodyEnd, const std::string& indent) {
    std::string output;
    int cursor = csec_expression_ast_skip_trivia(tokens, bodyStart);
    while (cursor >= 0 && cursor < bodyEnd && csec_token_kind_at(tokens, cursor) != 'E') {
        const int next = csec_advance_statement(tokens, cursor, bodyEnd);
        if (next <= cursor) break;
        const char* kind = csec_token_text_at(tokens, cursor);
        if (csec_native_token_is(tokens, cursor, 'K', "return")) {
            output += indent + "return " + csec_c_expression(tokens, cursor + 1, next) + ";\n";
        } else if (csec_native_token_is(tokens, cursor, 'K', "val") || csec_native_token_is(tokens, cursor, 'K', "var")) {
            const int init = csec_find_top_level_operator(tokens, cursor, next, 1);
            output += indent + csec_c_type(csec_c_local_type(tokens, cursor, next, init)) + " " + csec_token_text_at(tokens, cursor + 1);
            output += init >= cursor ? " = " + csec_c_expression(tokens, init + 1, next) + ";\n" : " = 0;\n";
        } else if (std::strcmp(kind, "if") == 0 || std::strcmp(kind, "while") == 0) {
            const int open = csec_find_statement_paren_start(tokens, cursor, next);
            const int close = csec_find_statement_paren_end(tokens, cursor, next);
            const int brace = csec_find_statement_block_start(tokens, cursor, next);
            const int braceEnd = csec_find_closing_token(tokens, brace, next, "{", "}");
            if (open >= cursor && close > open && brace > close && braceEnd > brace) {
                output += indent + kind + " (" + csec_c_expression(tokens, open + 1, close) + ") {\n";
                output += csec_generate_c_body_impl(tokens, brace + 1, braceEnd, indent + "    ") + indent + "}\n";
            }
        } else if (std::strcmp(kind, "for") == 0) {
            const int open = csec_find_statement_paren_start(tokens, cursor, next);
            const int close = csec_find_statement_paren_end(tokens, cursor, next);
            const int brace = csec_find_statement_block_start(tokens, cursor, next);
            const int braceEnd = csec_find_statement_block_end(tokens, cursor, next);
            const int arrow = csec_find_token_text_in_range(tokens, open + 1, close, "<-");
            const int range = csec_find_top_level_operator(tokens, arrow + 1, close, 5);
            if (open >= cursor && arrow > open && range > arrow && braceEnd > brace) {
                const char* name = csec_token_text_at(tokens, open + 1);
                const char* op = csec_token_text_at(tokens, range);
                output += indent + "for (int " + std::string(name) + " = " + csec_c_expression(tokens, arrow + 1, range) + "; " + name +
                    (std::strcmp(op, "to") == 0 || std::strcmp(op, "..") == 0 ? " <= " : " < ") + csec_c_expression(tokens, range + 1, close) + "; " + name + "++) {\n";
                output += csec_generate_c_body_impl(tokens, brace + 1, braceEnd, indent + "    ") + indent + "}\n";
            }
        } else if (std::strcmp(kind, "map") == 0 || std::strcmp(kind, "pmap") == 0 || std::strcmp(kind, "reduce") == 0 || std::strcmp(kind, "preduce") == 0 || std::strcmp(kind, "filter") == 0) {
            const int brace = csec_find_statement_block_start(tokens, cursor, next);
            const int braceEnd = csec_find_statement_block_end(tokens, cursor, next);
            output += indent + "/* " + std::string(kind) + " */\n";
            if (braceEnd > brace) output += csec_generate_c_body_impl(tokens, brace + 1, braceEnd, indent);
        } else {
            output += indent + csec_c_expression(tokens, cursor, next) + ";\n";
        }
        cursor = csec_expression_ast_skip_trivia(tokens, next);
    }
    return output;
}

static std::vector<std::pair<std::string, std::string>> csec_function_params(const char* tokens, int declStart) {
    std::vector<std::pair<std::string, std::string>> params;
    int paramEnd = csec_function_param_end(tokens, declStart);
    if (paramEnd < 0) return params;
    int paramStart = -1;
    for (int cursor = declStart; cursor < paramEnd; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'O', "(")) {
            paramStart = cursor;
            break;
        }
    }
    if (paramStart < 0) return params;
    int cursor = paramStart + 1;
    while (cursor < paramEnd) {
        if (csec_token_kind_at(tokens, cursor) == 'I') {
            std::string name = csec_token_text_at(tokens, cursor);
            std::string typeName = "String";
            if (csec_native_token_is(tokens, cursor + 1, 'O', ":")) {
                typeName = csec_token_text_at(tokens, cursor + 2);
                if (csec_native_token_is(tokens, cursor + 2, 'O', "(")) {
                    const int typeClose = csec_find_closing_token(tokens, cursor + 2, paramEnd, "(", ")");
                    if (typeClose >= cursor + 2) {
                        cursor = typeClose + 1;
                        if (csec_native_token_is(tokens, cursor, 'O', "=>")) ++cursor;
                        if (cursor < paramEnd) ++cursor;
                    }
                }
            }
            params.emplace_back(std::move(name), std::move(typeName));
        }
        int angleDepth = 0;
        while (cursor < paramEnd && (angleDepth > 0 || !csec_native_token_is(tokens, cursor, 'O', ","))) {
            if (csec_native_token_is(tokens, cursor, 'O', "<")) ++angleDepth;
            else if (csec_native_token_is(tokens, cursor, 'O', ">") && angleDepth > 0) --angleDepth;
            else if (csec_native_token_is(tokens, cursor, 'O', ">>")) angleDepth = angleDepth > 1 ? angleDepth - 2 : 0;
            ++cursor;
        }
        ++cursor;
    }
    return params;
}

static int csec_find_visible_local(const char* tokens, int limit, const char* name) {
    const int boundedLimit = limit < 0 ? 0 : limit;
    int begin = 0;
    int end = boundedLimit;
    int bodyStart = -1;
    int bodyEnd = -1;
    if (findFunctionAroundLimit(tokens, boundedLimit, nullptr, &bodyStart, &bodyEnd)) {
        begin = bodyStart;
        end = boundedLimit < bodyEnd ? boundedLimit : bodyEnd;
    }
    int visible = -1;
    for (int cursor = begin; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        const bool local = csec_native_token_is(tokens, cursor, 'K', "val") ||
            csec_native_token_is(tokens, cursor, 'K', "var");
        if (local && csec_token_kind_at(tokens, cursor + 1) == 'I' &&
            std::strcmp(csec_token_text_at(tokens, cursor + 1), name ? name : "") == 0) {
            visible = cursor;
        }
    }
    return visible;
}

extern "C" {

char* csec_generate_expression_ast(const char* tokens, int start, int end) {
    return csec_owned_string(csec_generate_expression_ast_impl(tokens, start, end));
}

char* csec_generate_statement_ast(const char* tokens, int start, int end) {
    return csec_owned_string(csec_generate_statement_ast_impl(tokens, start, end));
}

char* csec_generate_body_ast(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_owned_string(csec_generate_body_ast_impl(tokens, bodyStart, bodyEnd));
}

char* csec_generate_c_body(const char* tokens, int bodyStart, int bodyEnd, const char* indent) {
    return csec_owned_string(csec_generate_c_body_impl(tokens, bodyStart, bodyEnd, indent ? indent : ""));
}

char* csec_generate_c_expression(const char* tokens, int start, int end) {
    return csec_owned_string(csec_c_expression(tokens, start, end));
}

static int csec_i32_expression_end(const char* tokens, int start, int end) {
    while (end > start && csec_native_token_is(tokens, end - 1, 'O', ";")) --end;
    return end;
}
