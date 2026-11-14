// A generated block is closed only when its last instruction is a terminator. Testing for a
// `ret` anywhere in the body is not enough: a nested `if` that returns from one arm still
// leaves its own `if.end` label open, and that block still needs a branch.
static bool csec_body_is_terminated(const std::string& body) {
    const size_t last = body.find_last_not_of('\n');
    if (last == std::string::npos) return false;
    const size_t lineStart = body.rfind('\n', last) == std::string::npos ? 0 : body.rfind('\n', last) + 1;
    const std::string line = body.substr(lineStart, last - lineStart + 1);
    return line.compare(0, 6, "  ret ") == 0 || line.compare(0, 5, "  br ") == 0 ||
        line == "  unreachable";
}

// Emits `return <expression>` for whichever result type the enclosing function declares. Only
// the return paths differ between result types; every other statement lowers identically.
static std::string csec_emit_typed_return(const char* tokens, int start, int end,
                                          const char* resultType, const std::string& value) {
    if (std::strcmp(resultType, "ptr") == 0) {
        return csec_emit_ptr_expression(tokens, start, end, value) + "  ret ptr " + value + "\n";
    }
    if (std::strcmp(resultType, "i1") == 0) {
        return csec_emit_i1_expression(tokens, start, end, value) + "  ret i1 " + value + "\n";
    }
    if (std::strcmp(resultType, "double") == 0) {
        return csec_emit_f64_expression(tokens, start, end, value) + "  ret double " + value + "\n";
    }
    if (std::strcmp(resultType, "float") == 0) {
        return csec_emit_f32_expression(tokens, start, end, value) + "  ret float " + value + "\n";
    }
    if (std::strcmp(resultType, "i64") == 0) {
        return csec_emit_i64_expression(tokens, start, end, value) + "  ret i64 " + value + "\n";
    }
    if (std::strcmp(resultType, "i16") == 0) {
        return csec_emit_i16_expression(tokens, start, end, value) + "  ret i16 " + value + "\n";
    }
    if (std::strcmp(resultType, "i8") == 0) {
        return csec_emit_i8_expression(tokens, start, end, value) + "  ret i8 " + value + "\n";
    }
    return csec_emit_i32_expression(tokens, start, end, value) + "  ret i32 " + value + "\n";
}

static std::string csec_generate_llvm_flat_body_typed(const char* tokens, int bodyStart, int bodyEnd,
                                                      const char* resultType) {
    std::string output;
    int cursor = csec_expression_ast_skip_trivia(tokens, bodyStart);
    while (cursor >= 0 && cursor < bodyEnd && csec_token_kind_at(tokens, cursor) != 'E') {
        const int next = csec_advance_statement(tokens, cursor, bodyEnd);
        if (next <= cursor) break;
        const bool declaration = csec_native_token_is(tokens, cursor, 'K', "val") ||
            csec_native_token_is(tokens, cursor, 'K', "var");
        if (csec_native_token_is(tokens, cursor, 'K', "if")) {
            const int openParen = csec_find_statement_paren_start(tokens, cursor, next);
            const int closeParen = csec_find_statement_paren_end(tokens, cursor, next);
            const int thenOpen = csec_find_statement_block_start(tokens, cursor, next);
            const int thenClose = thenOpen >= 0 ? csec_find_closing_token(tokens, thenOpen, next, "{", "}") : -1;
            const bool booleanCall = openParen >= 0 && closeParen == openParen + 4 &&
                csec_token_kind_at(tokens, openParen + 1) == 'I' && csec_native_token_is(tokens, openParen + 2, 'O', "(") &&
                csec_native_token_is(tokens, openParen + 3, 'O', ")");
            if (openParen >= 0 && closeParen > openParen && thenClose > thenOpen) {
                const std::string seed = std::to_string(cursor);
                const std::string left = "%ifcond." + seed + ".left";
                if (booleanCall) {
                    output += "  %ifcond." + seed + " = call i1 @" + std::string(csec_token_text_at(tokens, openParen + 1)) + "()\n";
                } else if (csec_expression_is_boolean(tokens, openParen + 1, closeParen)) {
                    output += csec_emit_i1_expression(tokens, openParen + 1, closeParen, "%ifcond." + seed);
                } else {
                    output += csec_emit_i32_expression(tokens, openParen + 1, closeParen, left);
                    output += "  %ifcond." + seed + " = icmp ne i32 " + left + ", 0\n";
                }
                output += "  br i1 %ifcond." + seed + ", label %if.then." + seed + ", label %if.else." + seed + "\n";
                output += "if.then." + seed + ":\n";
                const std::string thenBody = csec_generate_llvm_flat_body_typed(tokens, thenOpen + 1, thenClose, resultType);
                output += thenBody;
                const bool thenReturn = csec_body_is_terminated(thenBody);
                if (!thenReturn) output += "  br label %if.end." + seed + "\n";
                output += "if.else." + seed + ":\n";
                int elseOpen = -1;
                int elseIf = -1;
                for (int pos = thenClose + 1; pos < next; ++pos) {
                    if (csec_native_token_is(tokens, pos, 'K', "else")) {
                        if (csec_native_token_is(tokens, pos + 1, 'K', "if")) elseIf = pos + 1;
                        else elseOpen = csec_find_token_text_in_range(tokens, pos + 1, next, "{");
                        break;
                    }
                }
                if (elseIf >= 0) {
                    // The nested `else if` ends at its own `if.end` label, so chain that block
                    // into this one instead of assuming the whole chain is unreachable.
                    const std::string elseBody = csec_generate_llvm_flat_body_typed(tokens, elseIf, next, resultType);
                    output += elseBody;
                    const bool elseReturn = csec_body_is_terminated(elseBody);
                    if (!elseReturn) output += "  br label %if.end." + seed + "\n";
                    output += "if.end." + seed + ":\n";
                    if (thenReturn && elseReturn) output += "  unreachable\n";
                } else {
                    const int elseClose = elseOpen >= 0 ? csec_find_closing_token(tokens, elseOpen, next, "{", "}") : -1;
                    const std::string elseBody = elseClose > elseOpen
                        ? csec_generate_llvm_flat_body_typed(tokens, elseOpen + 1, elseClose, resultType) : "";
                    output += elseBody;
                    const bool elseReturn = csec_body_is_terminated(elseBody);
                    if (!elseReturn) output += "  br label %if.end." + seed + "\n";
                    output += "if.end." + seed + ":\n";
                    if (thenReturn && elseReturn) output += "  unreachable\n";
                }
            }
        } else if (csec_native_token_is(tokens, cursor, 'K', "for")) {
            const int openParen = csec_find_statement_paren_start(tokens, cursor, next);
            const int closeParen = csec_find_statement_paren_end(tokens, cursor, next);
            const int arrow = openParen >= 0 ? csec_find_token_text_in_range(tokens, openParen + 1, closeParen, "<-") : -1;
            int range = -1;
            bool inclusive = true;
            for (int pos = arrow + 1; arrow >= 0 && pos < closeParen; ++pos) {
                if (csec_native_token_is(tokens, pos, 'K', "to")) { range = pos; inclusive = true; break; }
                if (csec_native_token_is(tokens, pos, 'K', "until")) { range = pos; inclusive = false; break; }
            }
            const int loopOpen = csec_find_statement_block_start(tokens, cursor, next);
            const int loopClose = loopOpen >= 0 ? csec_find_closing_token(tokens, loopOpen, next, "{", "}") : -1;
            if (openParen >= 0 && arrow > openParen + 1 && range > arrow + 1 && loopClose > loopOpen) {
                const std::string seed = std::to_string(cursor);
                const std::string variable = csec_token_text_at(tokens, openParen + 1);
                const std::string startValue = "%forstart." + seed;
                const std::string endValue = "%forend." + seed;
                output += "  %" + variable + " = alloca i32\n";
                output += csec_emit_i32_expression(tokens, arrow + 1, range, startValue);
                output += "  store i32 " + startValue + ", ptr %" + variable + "\n";
                output += "  br label %for.cond." + seed + "\n";
                output += "for.cond." + seed + ":\n";
                output += "  %forvalue." + seed + " = load i32, ptr %" + variable + "\n";
                output += csec_emit_i32_expression(tokens, range + 1, closeParen, endValue);
                output += "  %forcond." + seed + " = icmp " + (inclusive ? "sle" : "slt") + " i32 %forvalue." + seed + ", " + endValue + "\n";
                output += "  br i1 %forcond." + seed + ", label %for.body." + seed + ", label %for.end." + seed + "\n";
                output += "for.body." + seed + ":\n";
                output += csec_generate_llvm_flat_body_typed(tokens, loopOpen + 1, loopClose, resultType);
                output += "  %fornext." + seed + " = add i32 %forvalue." + seed + ", 1\n";
                output += "  store i32 %fornext." + seed + ", ptr %" + variable + "\n";
                output += "  br label %for.cond." + seed + "\n";
                output += "for.end." + seed + ":\n";
            }
        } else if (csec_native_token_is(tokens, cursor, 'K', "while")) {
            const int openParen = csec_find_statement_paren_start(tokens, cursor, next);
            const int closeParen = csec_find_statement_paren_end(tokens, cursor, next);
            const int loopOpen = csec_find_statement_block_start(tokens, cursor, next);
            const int loopClose = loopOpen >= 0 ? csec_find_closing_token(tokens, loopOpen, next, "{", "}") : -1;
            if (openParen >= 0 && closeParen > openParen && loopClose > loopOpen) {
                const std::string seed = std::to_string(cursor);
                const std::string value = "%whilecond." + seed + ".value";
                output += "  br label %while.cond." + seed + "\n";
                output += "while.cond." + seed + ":\n";
                if (csec_expression_is_boolean(tokens, openParen + 1, closeParen)) {
                    output += csec_emit_i1_expression(tokens, openParen + 1, closeParen, "%whilecond." + seed);
                } else {
                    output += csec_emit_i32_expression(tokens, openParen + 1, closeParen, value);
                    output += "  %whilecond." + seed + " = icmp ne i32 " + value + ", 0\n";
                }
                output += "  br i1 %whilecond." + seed + ", label %while.body." + seed + ", label %while.end." + seed + "\n";
                output += "while.body." + seed + ":\n";
                output += csec_generate_llvm_flat_body_typed(tokens, loopOpen + 1, loopClose, resultType);
                output += "  br label %while.cond." + seed + "\n";
                output += "while.end." + seed + ":\n";
            }
        } else if (declaration && csec_token_kind_at(tokens, cursor + 1) == 'I') {
            const std::string name = csec_token_text_at(tokens, cursor + 1);
            const std::string storage = name + ".addr." + std::to_string(cursor);
            const int initializer = csec_find_top_level_operator(tokens, cursor, next, 1);
            CsecDictTypes dictTypes;
            if (csec_dict_declaration(tokens, cursor, next, initializer, dictTypes)) {
                output += csec_dict_emit_declaration(tokens, cursor, next, initializer, dictTypes, storage);
            } else if (csec_lambda_declaration(tokens, cursor, next)) {
                const std::string closure = "%" + name + ".closure." + std::to_string(cursor);
                const std::string closureType = csec_lambda_closure_type_name(cursor);
                const auto captures = csec_lambda_capture_names(tokens, cursor, next);
                const bool byReference = csec_lambda_captures_by_reference(tokens, cursor, next);
                output += "  " + closure + " = alloca " + closureType + "\n";
                output += "  %" + storage + " = alloca ptr\n";
                output += "  %" + name + ".closure.code." + std::to_string(cursor) + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 0\n";
                output += "  store ptr @" + csec_lambda_function_name(cursor) + ", ptr %" + name + ".closure.code." + std::to_string(cursor) + "\n";
                output += "  %" + name + ".closure.env." + std::to_string(cursor) + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 1\n";
                if (captures.empty()) {
                    output += "  store ptr null, ptr %" + name + ".closure.env." + std::to_string(cursor) + "\n";
                } else {
                    const std::string environment = "%" + name + ".environment." + std::to_string(cursor);
                    const std::string environmentType = csec_lambda_environment_type_name(cursor);
                    output += "  " + environment + " = alloca " + environmentType + "\n";
                    for (size_t index = 0; index < captures.size(); ++index) {
                        const std::string field = "%" + name + ".capture." + std::to_string(cursor) + "." + std::to_string(index);
                        const char* source = csec_lookup_visible_storage_name(tokens, cursor, captures[index].c_str());
                        output += "  " + field + " = getelementptr inbounds " + environmentType + ", ptr " + environment + ", i32 0, i32 " + std::to_string(index) + "\n";
                        if (byReference) {
                            output += "  store ptr %" + std::string(source) + ", ptr " + field + "\n";
                        } else {
                            const std::string value = "%" + name + ".capture.value." + std::to_string(cursor) + "." + std::to_string(index);
                            output += "  " + value + " = load i32, ptr %" + std::string(source) + "\n";
                            output += "  store i32 " + value + ", ptr " + field + "\n";
                        }
                    }
                    output += "  store ptr " + environment + ", ptr %" + name + ".closure.env." + std::to_string(cursor) + "\n";
                }
                output += "  store ptr " + closure + ", ptr %" + storage + "\n";
            } else if (csec_is_i1_array_initializer(tokens, initializer, next)) {
                output += csec_emit_i1_array_local(tokens, cursor, next);
            } else if (csec_is_i8_array_initializer(tokens, initializer, next)) {
                output += csec_emit_i8_array_local(tokens, cursor, next);
            } else if (csec_is_i16_array_initializer(tokens, initializer, next)) {
                output += csec_emit_i16_array_local(tokens, cursor, next);
            } else if (csec_is_i64_array_initializer(tokens, initializer, next)) {
                output += csec_emit_i64_array_local(tokens, cursor, next);
            } else if (csec_is_f32_array_initializer(tokens, initializer, next)) {
                output += csec_emit_f32_array_local(tokens, cursor, next);
            } else if (csec_is_f64_array_initializer(tokens, initializer, next)) {
                output += csec_emit_f64_array_local(tokens, cursor, next);
            } else if (csec_is_ptr_array_initializer(tokens, initializer, next)) {
                output += csec_emit_ptr_array_local(tokens, cursor, next);
            } else if (csec_is_i32_array_initializer(tokens, initializer, next)) {
                output += csec_emit_i32_array_local(tokens, cursor, next);
            } else if (initializer >= cursor && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
                       csec_token_kind_at(tokens, initializer + 2) == 'I' && csec_native_token_is(tokens, initializer + 3, 'O', "(")) {
                const char* className = csec_token_text_at(tokens, initializer + 2);
                const int classStart = csec_find_class_declaration(tokens, className);
                const int constructorClose = csec_find_closing_token(tokens, initializer + 3, next, "(", ")");
                if (classStart >= 0 && constructorClose >= initializer + 3 && csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const std::string object = name + ".object." + std::to_string(cursor);
                    output += "  %" + object + " = alloca %class." + std::string(className) + "\n";
                    output += "  %" + storage + " = alloca ptr\n";
                    output += "  store ptr %" + object + ", ptr %" + storage + "\n";
                    int fieldIndex = 0;
                    int parameterIndex = 0;
                    const auto parameters = csec_class_constructor_params(tokens, classStart);
                    int argumentStart = initializer + 4;
                    for (int position = argumentStart; position <= constructorClose; ++position) {
                        if (position == constructorClose || csec_is_argument_separator(tokens, constructorClose, position)) {
                            if (position > argumentStart && parameterIndex < static_cast<int>(parameters.size())) {
                                const char* fieldType = csec_llvm_type_for_name(parameters[parameterIndex].second);
                                const std::string value = "%" + name + ".ctor." + std::to_string(fieldIndex);
                                const std::string field = "%" + name + ".field." + std::to_string(fieldIndex);
                                if (std::strcmp(fieldType, "i1") == 0) {
                                    output += csec_emit_i1_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "ptr") == 0) {
                                    output += csec_emit_ptr_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "i64") == 0) {
                                    output += csec_emit_i64_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "double") == 0) {
                                    output += csec_emit_f64_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "float") == 0) {
                                    output += csec_emit_f32_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "i16") == 0) {
                                    output += csec_emit_i16_expression(tokens, argumentStart, position, value);
                                } else if (std::strcmp(fieldType, "i8") == 0) {
                                    output += csec_emit_i8_expression(tokens, argumentStart, position, value);
                                } else {
                                    output += csec_emit_i32_expression(tokens, argumentStart, position, value);
                                }
                                output += "  " + field + " = getelementptr inbounds %class." + std::string(className) + ", ptr %" + object + ", i32 0, i32 " + std::to_string(fieldIndex++) + "\n";
                                output += "  store " + std::string(fieldType) + " " + value + ", ptr " + field + "\n";
                            }
                            argumentStart = position + 1;
                            ++parameterIndex;
                        }
                    }
                    output += csec_emit_class_field_initializers(tokens, classStart, className, object, name, fieldIndex);
                } else {
                    output += "  %" + storage + " = alloca i32\n";
                    output += "  store i32 0, ptr %" + storage + "\n";
                }
            } else if (std::strcmp(csec_llvm_type_for_name(csec_c_local_type(tokens, cursor, next, initializer)), "ptr") == 0) {
                output += "  %" + storage + " = alloca ptr\n";
                if (initializer >= cursor && initializer + 1 < next) {
                    const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                    output += csec_emit_ptr_expression(tokens, initializer + 1, next, initialValue);
                    output += "  store ptr " + initialValue + ", ptr %" + storage + "\n";
                } else output += "  store ptr null, ptr %" + storage + "\n";
            } else if (csec_c_local_type(tokens, cursor, next, initializer) == "Float") {
                output += "  %" + storage + " = alloca float\n";
                if (initializer >= cursor && initializer + 1 < next) {
                    const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                    output += csec_emit_f32_expression(tokens, initializer + 1, next, initialValue);
                    output += "  store float " + initialValue + ", ptr %" + storage + "\n";
                } else output += "  store float 0.000000e+00, ptr %" + storage + "\n";
            } else if (csec_c_local_type(tokens, cursor, next, initializer) == "Double") {
                output += "  %" + storage + " = alloca double\n";
                if (initializer >= cursor && initializer + 1 < next) {
                    const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                    output += csec_emit_f64_expression(tokens, initializer + 1, next, initialValue);
                    output += "  store double " + initialValue + ", ptr %" + storage + "\n";
                } else output += "  store double 0.000000e+00, ptr %" + storage + "\n";
            } else if (csec_is_i16_type_name(csec_c_local_type(tokens, cursor, next, initializer))) {
                output += "  %" + storage + " = alloca i16\n";
                if (initializer >= cursor && initializer + 1 < next) { const std::string initialValue = "%" + name + ".init." + std::to_string(cursor); output += csec_emit_i16_expression(tokens, initializer + 1, next, initialValue); output += "  store i16 " + initialValue + ", ptr %" + storage + "\n"; }
                else output += "  store i16 0, ptr %" + storage + "\n";
            } else if (csec_is_i8_type_name(csec_c_local_type(tokens, cursor, next, initializer))) {
                output += "  %" + storage + " = alloca i8\n";
                if (initializer >= cursor && initializer + 1 < next) {
                    const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                    output += csec_emit_i8_expression(tokens, initializer + 1, next, initialValue);
                    output += "  store i8 " + initialValue + ", ptr %" + storage + "\n";
                } else output += "  store i8 0, ptr %" + storage + "\n";
            } else if (csec_is_i64_type_name(csec_c_local_type(tokens, cursor, next, initializer))) {
                output += "  %" + storage + " = alloca i64\n";
                if (initializer >= cursor && initializer + 1 < next) {
                    const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                    output += csec_emit_i64_expression(tokens, initializer + 1, next, initialValue);
                    output += "  store i64 " + initialValue + ", ptr %" + storage + "\n";
                } else output += "  store i64 0, ptr %" + storage + "\n";
            } else {
                const bool boolean = csec_c_local_type(tokens, cursor, next, initializer) == "Boolean";
                output += "  %" + storage + " = alloca " + std::string(boolean ? "i1" : "i32") + "\n";
                if (initializer >= cursor && initializer + 1 < next) {
                const std::string initialValue = "%" + name + ".init." + std::to_string(cursor);
                output += boolean
                    ? csec_emit_i1_expression(tokens, initializer + 1, next, initialValue)
                    : csec_emit_i32_expression(tokens, initializer + 1, next, initialValue);
                output += std::string("  store ") + (boolean ? "i1 " : "i32 ") + initialValue + ", ptr %" + storage + "\n";
                } else {
                    output += std::string("  store ") + (boolean ? "i1 false" : "i32 0") + ", ptr %" + storage + "\n";
                }
            }
        } else if (csec_native_token_is(tokens, cursor, 'K', "return") && cursor + 1 < next) {
            const std::string value = "%ret." + std::to_string(cursor);
            output += csec_emit_typed_return(tokens, cursor + 1, next, resultType, value);
            return output;
        } else if (csec_token_kind_at(tokens, cursor) == 'I' &&
                   csec_native_token_is(tokens, cursor + 1, 'O', "[") &&
                   !csec_emit_dict_store(tokens, cursor, next).empty()) {
            output += csec_emit_dict_store(tokens, cursor, next);
        } else if (csec_token_kind_at(tokens, cursor) == 'I' &&
                   csec_native_token_is(tokens, cursor + 1, 'O', "[")) {
            const int close = csec_find_closing_token(tokens, cursor + 1, next, "[", "]");
            const char* assignment = close >= 0 && close + 1 < next ? csec_token_text_at(tokens, close + 1) : "";
            const bool compound = std::strcmp(assignment, "+=") == 0 || std::strcmp(assignment, "-=") == 0 ||
                std::strcmp(assignment, "*=") == 0 || std::strcmp(assignment, "/=") == 0 || std::strcmp(assignment, "%=") == 0;
            const bool booleanArray = csec_visible_i1_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool charArray = csec_visible_i8_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool shortArray = csec_visible_i16_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool longArray = csec_visible_i64_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool floatArray = csec_visible_f32_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool doubleArray = csec_visible_f64_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            const bool pointerArray = csec_visible_ptr_array(tokens, cursor, csec_token_text_at(tokens, cursor));
            if (close > cursor + 1 && (std::strcmp(assignment, "=") == 0 || compound) &&
                (csec_visible_i32_array(tokens, cursor, csec_token_text_at(tokens, cursor)) || booleanArray || charArray || shortArray || longArray || floatArray || doubleArray || pointerArray) &&
                (!booleanArray || !compound) && (!charArray || !compound) && (!shortArray || !compound) && (!longArray || !compound) && (!floatArray || !compound) && (!doubleArray || !compound) && (!pointerArray || !compound)) {
                const std::string name = csec_token_text_at(tokens, cursor);
                const std::string storage = csec_lookup_visible_storage_name(tokens, cursor, name.c_str());
                const std::string seed = std::to_string(cursor);
                const std::string index = "%" + name + ".index." + seed;
                const std::string data = "%" + name + ".data." + seed;
                const std::string element = "%" + name + ".element." + seed;
                const std::string rhs = "%" + name + ".rhs." + seed;
                output += csec_emit_i32_expression(tokens, cursor + 2, close, index);
                output += "  " + data + " = load ptr, ptr %" + storage + "\n";
                output += "  " + element + " = getelementptr inbounds " + std::string(booleanArray ? "i1" : (charArray ? "i8" : (shortArray ? "i16" : (longArray ? "i64" : (floatArray ? "float" : (doubleArray ? "double" : (pointerArray ? "ptr" : "i32"))))))) + ", ptr " + data + ", i32 " + index + "\n";
                output += booleanArray ? csec_emit_i1_expression(tokens, close + 2, next, rhs)
                    : (charArray ? csec_emit_i8_expression(tokens, close + 2, next, rhs)
                    : (shortArray ? csec_emit_i16_expression(tokens, close + 2, next, rhs)
                    : (longArray ? csec_emit_i64_expression(tokens, close + 2, next, rhs)
                    : (floatArray ? csec_emit_f32_expression(tokens, close + 2, next, rhs)
                    : (doubleArray ? csec_emit_f64_expression(tokens, close + 2, next, rhs)
                    : (pointerArray ? csec_emit_ptr_expression(tokens, close + 2, next, rhs)
                                    : csec_emit_i32_expression(tokens, close + 2, next, rhs)))))));
                if (compound) {
                    const std::string old = "%" + name + ".old." + seed;
                    const std::string value = "%" + name + ".next." + seed;
                    const char* instruction = std::strcmp(assignment, "+=") == 0 ? "add" :
                        (std::strcmp(assignment, "-=") == 0 ? "sub" : (std::strcmp(assignment, "*=") == 0 ? "mul" :
                        (std::strcmp(assignment, "/=") == 0 ? "sdiv" : "srem")));
                    output += "  " + old + " = load i32, ptr " + element + "\n";
                    output += "  " + value + " = " + instruction + " i32 " + old + ", " + rhs + "\n";
                    output += "  store i32 " + value + ", ptr " + element + "\n";
                } else output += std::string("  store ") + (booleanArray ? "i1 " : (charArray ? "i8 " : (shortArray ? "i16 " : (longArray ? "i64 " : (floatArray ? "float " : (doubleArray ? "double " : (pointerArray ? "ptr " : "i32 "))))))) + rhs + ", ptr " + element + "\n";
            }
        } else if (csec_token_kind_at(tokens, cursor) == 'I' &&
                   (csec_native_token_is(tokens, cursor + 1, 'O', "+=") ||
                    csec_native_token_is(tokens, cursor + 1, 'O', "-=") ||
                    csec_native_token_is(tokens, cursor + 1, 'O', "*=") ||
                    csec_native_token_is(tokens, cursor + 1, 'O', "/=") ||
                    csec_native_token_is(tokens, cursor + 1, 'O', "%="))) {
            const std::string name = csec_token_text_at(tokens, cursor);
            const std::string storage = csec_lookup_visible_storage_name(tokens, cursor, name.c_str());
            const std::string left = "%" + name + ".compound.left." + std::to_string(cursor);
            const std::string right = "%" + name + ".compound.right." + std::to_string(cursor);
            const std::string value = "%" + name + ".compound." + std::to_string(cursor);
            const char* operation = csec_token_text_at(tokens, cursor + 1);
            const char* instruction = std::strcmp(operation, "+=") == 0 ? "add" :
                (std::strcmp(operation, "-=") == 0 ? "sub" : (std::strcmp(operation, "*=") == 0 ? "mul" :
                (std::strcmp(operation, "/=") == 0 ? "sdiv" : "srem")));
            output += "  " + left + " = load i32, ptr %" + storage + "\n";
            output += csec_emit_i32_expression(tokens, cursor + 2, next, right);
            output += "  " + value + " = " + instruction + " i32 " + left + ", " + right + "\n";
            output += "  store i32 " + value + ", ptr %" + storage + "\n";
        } else if (csec_token_kind_at(tokens, cursor) == 'I' &&
                   csec_native_token_is(tokens, cursor + 1, 'O', "=")) {
            const std::string name = csec_token_text_at(tokens, cursor);
            const std::string storage = csec_lookup_visible_storage_name(tokens, cursor, name.c_str());
            const std::string value = "%" + name + ".assign." + std::to_string(cursor);
            const bool boolean = csec_is_boolean_local(tokens, cursor, name.c_str()) ||
                csec_is_boolean_class_constructor_param(tokens, cursor, name.c_str());
            const bool floating = csec_is_float_local(tokens, cursor, name.c_str()) ||
                csec_is_float_parameter(tokens, cursor, name.c_str());
            const bool doubling = csec_is_double_local(tokens, cursor, name.c_str()) ||
                csec_is_double_parameter(tokens, cursor, name.c_str());
            const bool pointer = csec_is_string_local(tokens, cursor, name.c_str()) ||
                csec_is_string_parameter(tokens, cursor, name.c_str()) ||
                csec_is_pointer_local(tokens, cursor, name.c_str()) ||
                csec_is_pointer_parameter(tokens, cursor, name.c_str());
            // A name that is neither a local nor a parameter may be a field of the enclosing
            // class, in which case the slot width comes from the class layout.
            const std::string fieldType =
                (boolean || pointer || floating || doubling || csec_find_visible_local(tokens, cursor, name.c_str()) >= 0)
                    ? std::string() : csec_class_field_llvm_type(tokens, cursor, name.c_str());
            // A Long local or parameter is assigned a 64-bit value (previously an i32 was stored into
            // its 8-byte slot, dropping the high half).
            bool wideLocal = false;
            if (!boolean && !pointer && !floating && !doubling) {
                const int declaration = csec_find_visible_local(tokens, cursor, name.c_str());
                if (declaration >= 0) {
                    const int declarationEnd = csec_advance_statement(tokens, declaration, 1000000000);
                    const int initializer = csec_find_top_level_operator(tokens, declaration, declarationEnd, 1);
                    wideLocal = csec_is_i64_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer));
                } else {
                    const int function = csec_enclosing_function_declaration(tokens, cursor);
                    if (function >= 0) {
                        for (const auto& parameter : csec_function_params(tokens, function)) {
                            if (parameter.first == name && csec_is_i64_type_name(parameter.second)) wideLocal = true;
                        }
                    }
                }
            }
            if (wideLocal) {
                output += csec_emit_i64_expression(tokens, cursor + 2, next, value);
                output += "  store i64 " + value + ", ptr %" + storage + "\n";
                cursor = csec_expression_ast_skip_trivia(tokens, next);
                continue;
            }
            if (!fieldType.empty() && fieldType != "i32") {
                if (fieldType == "i64") output += csec_emit_i64_expression(tokens, cursor + 2, next, value);
                else if (fieldType == "double") output += csec_emit_f64_expression(tokens, cursor + 2, next, value);
                else if (fieldType == "float") output += csec_emit_f32_expression(tokens, cursor + 2, next, value);
                else if (fieldType == "i16") output += csec_emit_i16_expression(tokens, cursor + 2, next, value);
                else if (fieldType == "i1") output += csec_emit_i1_expression(tokens, cursor + 2, next, value);
                else if (fieldType == "ptr") output += csec_emit_ptr_expression(tokens, cursor + 2, next, value);
                else output += csec_emit_i8_expression(tokens, cursor + 2, next, value);
                output += "  store " + fieldType + " " + value + ", ptr %" + storage + "\n";
                cursor = csec_expression_ast_skip_trivia(tokens, next);
                continue;
            }
            output += boolean ? csec_emit_i1_expression(tokens, cursor + 2, next, value)
                : (pointer ? csec_emit_ptr_expression(tokens, cursor + 2, next, value)
                : (doubling ? csec_emit_f64_expression(tokens, cursor + 2, next, value)
                : (floating ? csec_emit_f32_expression(tokens, cursor + 2, next, value)
                            : csec_emit_i32_expression(tokens, cursor + 2, next, value))));
            output += std::string("  store ") + (boolean ? "i1 " : (pointer ? "ptr " : (doubling ? "double " : (floating ? "float " : "i32 ")))) + value + ", ptr %" + storage + "\n";
        } else if ((csec_native_token_is(tokens, cursor, 'K', "go") || csec_native_token_is(tokens, cursor, 'K', "await")) &&
                   cursor + 1 < next) {
            output += csec_emit_i32_expression(tokens, cursor, next, "%discard." + std::to_string(cursor));
        } else if (!csec_emit_assert_statement(tokens, cursor, next).empty()) {
            output += csec_emit_assert_statement(tokens, cursor, next);
        } else if (!csec_emit_println_statement(tokens, cursor, next).empty()) {
            output += csec_emit_println_statement(tokens, cursor, next);
        } else if (csec_native_token_is(tokens, cursor, 'I', "free") &&
                   csec_native_token_is(tokens, cursor + 1, 'O', "(")) {
            // free(x): the self-host emitter allocates collections on the stack, so there is nothing
            // to release — treat it as a no-op (the direct compiler, which heap-allocates via
            // malloc, performs the real free). The comment consumes the statement so it is not
            // lowered as a generic i32 call to an undeclared @free.
            output += "  ; free() no-op (self-host stack allocation)\n";
        } else if (csec_token_kind_at(tokens, cursor) == 'I' && cursor + 3 < next &&
                   csec_native_token_is(tokens, cursor + 1, 'O', ".") &&
                   csec_token_kind_at(tokens, cursor + 2) == 'I' &&
                   csec_native_token_is(tokens, cursor + 3, 'O', "(")) {
            output += csec_emit_i32_expression(tokens, cursor, next, "%discard." + std::to_string(cursor));
        } else if (csec_token_kind_at(tokens, cursor) == 'I' && cursor + 1 < next &&
                   csec_native_token_is(tokens, cursor + 1, 'O', "(")) {
            output += csec_emit_i32_expression(tokens, cursor, next, "%discard." + std::to_string(cursor));
        }
        cursor = csec_expression_ast_skip_trivia(tokens, next);
    }
    return output;
}

char* csec_generate_llvm_flat_body_i32(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_owned_string(csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "i32"));
}

static std::string csec_generate_llvm_flat_body_ptr(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "ptr");
}

static std::string csec_generate_llvm_flat_body_i1(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "i1");
}

static std::string csec_generate_llvm_flat_body_float(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "float");
}

static std::string csec_generate_llvm_flat_body_double(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "double");
}

static std::string csec_generate_llvm_flat_body_i64(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "i64");
}

static std::string csec_generate_llvm_flat_body_i16(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "i16");
}

static std::string csec_generate_llvm_flat_body_i8(const char* tokens, int bodyStart, int bodyEnd) {
    return csec_generate_llvm_flat_body_typed(tokens, bodyStart, bodyEnd, "i8");
}

char* csec_generate_llvm_string_literal_globals(const char* tokens) {
    std::string output;
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_token_kind_at(tokens, cursor) != 'S') continue;
        const char* text = csec_token_text_at(tokens, cursor);
        output += "@.str." + std::to_string(cursor) + " = private unnamed_addr constant [";
        output += std::to_string(csec_llvm_string_literal_byte_length(text));
        output += " x i8] c\"";
        output += csec_llvm_string_literal_bytes(text);
        output += "\"\n";
    }
    if (!output.empty()) output += "\n";
    return csec_owned_string(output);
}

int csec_generate_llvm_module_into(const char* tokens, long long builder) {
    static constexpr const char* header =
        "; ModuleID = 'csec.selfhost'\n"
        "source_filename = \"csec.selfhost\"\n\n"
        "declare ptr @csec_append_expression_until(ptr, i32, i32)\n"
        "declare ptr @csec_generate_symbol_table(ptr)\n"
        "declare ptr @csec_ir_type_name(ptr)\n"
        "declare ptr @csec_collect_type_name(ptr, i32, i32)\n"
        "declare ptr @csec_lookup_visible_value_type(ptr, i32, ptr)\n"
        "declare ptr @csec_lookup_visible_storage_name(ptr, i32, ptr)\n"
        "declare i32 @csec_find_function_param_start(ptr, i32)\n"
        "declare i32 @csec_count_comma_separated(ptr, i32, i32)\n"
        "declare i32 @csec_count_function_params(ptr, i32)\n"
        "declare i32 @csec_expression_top_level_operator(ptr, i32, i32)\n"
        "declare i32 @csec_find_closing_token(ptr, i32, i32, ptr, ptr)\n"
        "declare i32 @csec_find_last_top_level_token(ptr, i32, i32, ptr)\n"
        "declare i32 @csec_find_statement_block_end(ptr, i32, i32)\n"
        "declare i32 @csec_find_statement_block_start(ptr, i32, i32)\n"
        "declare i32 @csec_find_statement_paren_end(ptr, i32, i32)\n"
        "declare i32 @csec_find_statement_paren_start(ptr, i32, i32)\n"
        "declare i32 @csec_find_token_text_in_range(ptr, i32, i32, ptr)\n"
        "declare i32 @csec_find_top_level_match(ptr, i32, i32)\n"
        "declare i32 @csec_find_top_level_operator(ptr, i32, i32, i32)\n"
        "declare ptr @csec_generate_body_ast(ptr, i32, i32)\n"
        "declare ptr @csec_generate_c_body(ptr, i32, i32, ptr)\n"
        "declare ptr @csec_generate_c_expression(ptr, i32, i32)\n"
        "declare ptr @csec_generate_expression_ast(ptr, i32, i32)\n"
        "declare ptr @csec_generate_statement_ast(ptr, i32, i32)\n"
        "declare ptr @csec_ir_operator_name(ptr)\n"
        "declare i32 @csec_is_keyword(ptr)\n"
        "declare i32 @csec_line_end(ptr, i32)\n"
        "declare i32 @csec_line_start(ptr, i32)\n"
        "declare i32 @csec_starts_top_level_declaration(ptr, i32)\n"
        "declare i8 @csec_string_char_at(ptr, i32)\n"
        "declare ptr @csec_string_concat(ptr, ptr)\n"
        "declare i32 @csec_string_contains(ptr, ptr)\n"
        "declare i32 @csec_string_ends_with(ptr, ptr)\n"
        "declare i32 @csec_string_equals(ptr, ptr)\n"
        "declare i64 @csec_string_index_of(ptr, ptr)\n"
        "declare i32 @csec_string_is_empty(ptr)\n"
        "declare i64 @csec_string_length(ptr)\n"
        "declare i32 @csec_string_starts_with(ptr, ptr)\n"
        "declare ptr @csec_string_substring(ptr, i32, i32)\n"
        "declare ptr @csec_string_to_lower(ptr)\n"
        "declare ptr @csec_string_to_upper(ptr)\n"
        "declare ptr @csec_string_trim(ptr)\n"
        "declare void @csec_print_i64(i64)\n"
        "declare void @csec_print_newline()\n"
        "declare void @csec_print_string(ptr)\n"
        "declare i32 @csec_command_line_arg_count()\n"
        "declare ptr @csec_command_line_arg(i32)\n"
        "declare i32 @csec_to_int(ptr)\n"
        "declare ptr @csec_to_string_char(i8)\n"
        "declare ptr @csec_to_string_i64(i64)\n"
        "declare i32 @csec_token_is_top_level_operator(ptr, i32, i32)\n"
        "declare ptr @csec_top_level_decl_kind(ptr, i32)\n"
        "declare ptr @csec_top_level_decl_name(ptr, i32)\n"
        "declare i32 @csec_validate_balanced(ptr)\n"
        "declare i32 @csec_validate_top_level(ptr)\n\n";
    if (csec_string_builder_append(builder, header) != 0) return -1;
    char* stringGlobals = csec_generate_llvm_string_literal_globals(tokens);
    if (stringGlobals && csec_string_builder_append(builder, stringGlobals) != 0) return -1;
    const std::string lambdaTypeDefinitions = csec_lambda_type_definitions(tokens);
    if (!lambdaTypeDefinitions.empty() && csec_string_builder_append(builder, lambdaTypeDefinitions.c_str()) != 0) return -1;
    const std::string lambdaDefinitions = csec_lambda_definitions(tokens);
    if (!lambdaDefinitions.empty() && csec_string_builder_append(builder, lambdaDefinitions.c_str()) != 0) return -1;
    const std::string concurrencyDefinitions = csec_concurrency_definitions(tokens);
    if (!concurrencyDefinitions.empty() && csec_string_builder_append(builder, concurrencyDefinitions.c_str()) != 0) return -1;
    const std::string assertDefinitions = csec_assert_definitions(tokens);
    if (!assertDefinitions.empty() && csec_string_builder_append(builder, assertDefinitions.c_str()) != 0) return -1;
    const std::string dictDefinitions = csec_dict_definitions(tokens);
    if (!dictDefinitions.empty() && csec_string_builder_append(builder, dictDefinitions.c_str()) != 0) return -1;

    bool compilerDriver = false;
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E';) {
        const int next = csec_advance_top_level_decl(tokens, cursor);
        if (next <= cursor) break;
        if (std::strcmp(csec_top_level_decl_kind(tokens, cursor), "function") == 0 &&
            std::strcmp(csec_top_level_decl_name(tokens, cursor), "compileFile") == 0) {
            compilerDriver = true;
            break;
        }
        cursor = next;
    }
    bool hasUserMain = false;
    int userMainDeclaration = -1;
    if (!compilerDriver) {
        for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
            if (csec_native_token_is(tokens, cursor, 'K', "def") &&
                csec_token_kind_at(tokens, cursor + 1) == 'I' &&
                std::strcmp(csec_token_text_at(tokens, cursor + 1), "main") == 0) {
                userMainDeclaration = cursor;
                break;
            }
        }
    }
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E';) {
        const int next = csec_advance_top_level_decl(tokens, cursor);
        if (next <= cursor) break;
        const char* declarationKind = csec_top_level_decl_kind(tokens, cursor);
        if (std::strcmp(declarationKind, "external-function") == 0 && csec_decl_has_intrinsic_attribute(tokens, cursor)) {
            // An intrinsic declaration only enables the built-in lowering; it has no symbol.
        } else if (std::strcmp(declarationKind, "external-function") == 0) {
            std::string declaration = "declare ";
            declaration += csec_llvm_type_for_name(csec_function_return_type_at(tokens, cursor));
            declaration += " @";
            declaration += csec_top_level_decl_name(tokens, cursor);
            declaration += "(";
            declaration += csec_function_llvm_param_list(tokens, cursor);
            declaration += ")\n";
            if (csec_string_builder_append(builder, declaration.c_str()) != 0) return -1;
        } else if (std::strcmp(declarationKind, "object") == 0) {
            const int objectBody = csec_find_decl_body_start(tokens, cursor);
            const int objectEnd = csec_find_decl_body_end(tokens, objectBody);
            // csec_find_decl_body_start already returns the first token inside the body.
            for (int member = objectBody; objectBody >= 0 && member < objectEnd; ++member) {
                if (csec_native_token_is(tokens, member, 'K', "def")) {
                    const std::string definition = csec_object_method_definition(tokens, cursor, member);
                    if (definition.empty() || csec_string_builder_append(builder, definition.c_str()) != 0) return -1;
                }
            }
        } else if (std::strcmp(declarationKind, "class") == 0) {
            const std::string className = csec_top_level_decl_name(tokens, cursor);
            if (csec_class_uses_pointer_receiver(tokens, cursor)) {
                const std::string layout = csec_class_i32_layout(tokens, cursor);
                if (layout.empty() || csec_string_builder_append(builder, ("%class." + className + " = type { " + layout + " }\n\n").c_str()) != 0) return -1;
            }
            const int classBody = csec_find_decl_body_start(tokens, cursor);
            const int classEnd = csec_find_decl_body_end(tokens, classBody);
            // Expression-bodied methods do not always have a statement boundary before the
            // following member, so inspect every token in the class body for method starts.
            for (int member = classBody; classBody >= 0 && member < classEnd; ++member) {
                if (csec_native_token_is(tokens, member, 'K', "def")) {
                    const std::string definition = csec_class_method_definition(tokens, cursor, member);
                    if (definition.empty() || csec_string_builder_append(builder, definition.c_str()) != 0) return -1;
                }
            }
        } else if (std::strcmp(declarationKind, "function") == 0) {
            const std::string functionName = csec_top_level_decl_name(tokens, cursor);
            const char* name = functionName.c_str();
            if (std::strncmp(name, "csec_", 5) == 0) {
                cursor = next;
                continue;
            }
            if (std::strcmp(name, "main") == 0) {
                if (compilerDriver) {
                    std::string mainDefinition = "define i32 @main() {\nentry:\n";
                    mainDefinition += csec_llvm_main_body();
                    mainDefinition += "}\n\n";
                    if (csec_string_builder_append(builder, mainDefinition.c_str()) != 0) return -1;
                } else {
                    char* userMain = csec_generate_llvm_function_definition(tokens, cursor);
                    if (!userMain || csec_string_builder_append(builder, userMain) != 0) return -1;
                    hasUserMain = true;
                }
                cursor = next;
                continue;
            }
            char* definition = csec_llvm_lexer_helper_definition(name);
            if (definition && definition[0] != '\0') {
                if (csec_string_builder_append(builder, definition) != 0) return -1;
            } else {
                char* generated = csec_generate_llvm_function_definition(tokens, cursor);
                if (!generated || csec_string_builder_append(builder, generated) != 0) return -1;
            }
        }
        cursor = next;
    }
    if (!compilerDriver && !hasUserMain && userMainDeclaration >= 0) {
        char* userMain = csec_generate_llvm_function_definition(tokens, userMainDeclaration);
        if (!userMain || csec_string_builder_append(builder, userMain) != 0) return -1;
        hasUserMain = true;
    }
    if (hasUserMain) {
        static constexpr const char* entryWrapper =
            "define i32 @main() {\n"
            "entry:\n"
            "  %ret = call i32 @csec_user_main()\n"
            "  ret i32 %ret\n"
            "}\n\n";
        if (csec_string_builder_append(builder, entryWrapper) != 0) return -1;
    }
    return 0;
}

char* csec_ir_type_name(const char* typeName) {
    return csec_owned_string(csec_llvm_type_for_name(typeName ? typeName : ""));
}

char* csec_collect_type_name(const char* tokens, int start, int end) {
    std::string output;
    for (int cursor = start < 0 ? 0 : start; cursor < end && csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        const char* text = csec_token_text_at(tokens, cursor);
        if (std::strcmp(text, "{") == 0 || std::strcmp(text, "=") == 0 ||
            std::strcmp(text, ";") == 0 || std::strcmp(text, ",") == 0) break;
        output += text;
    }
    return csec_owned_string(output);
}

char* csec_lookup_visible_value_type(const char* tokens, int limit, const char* name) {
    const int declaration = csec_find_visible_local(tokens, limit, name);
    if (declaration < 0) return csec_owned_string("Int");
    const int end = limit < declaration ? declaration : limit;
    const int initializer = csec_find_top_level_operator(tokens, declaration, end, 1);
    return csec_owned_string(csec_c_local_type(tokens, declaration, end, initializer));
}

char* csec_lookup_visible_storage_name(const char* tokens, int limit, const char* name) {
    const int declaration = csec_find_visible_local(tokens, limit, name);
    if (declaration < 0) return csec_owned_string(name ? name : "");
    return csec_owned_string(std::string(name ? name : "") + ".addr." + std::to_string(declaration));
}

char* csec_function_llvm_param_list(const char* tokens, int declStart) {
    std::string output;
    for (const auto& param : csec_function_params(tokens, declStart)) {
        if (!output.empty()) output += ", ";
        output += csec_llvm_type_for_name(param.second);
        output += " %arg.";
        output += param.first;
    }
    return csec_owned_string(output);
}

char* csec_function_llvm_param_allocas(const char* tokens, int declStart) {
    std::string output;
    for (const auto& param : csec_function_params(tokens, declStart)) {
        const char* llvmType = csec_llvm_type_for_name(param.second);
        output += "  %" + param.first + " = alloca ";
        output += llvmType;
        output += "\n  store ";
        output += llvmType;
        output += " %arg." + param.first + ", ptr %" + param.first + "\n";
    }
    return csec_owned_string(output);
}
