extern "C++" {
// ---- Concurrency: std.microthread / std.thread intrinsics, `go`, `async`, `await` ------------------
// Mirrors the native compiler (LANGUAGE_SPEC.md sections 1.2 and 3). In the self-host, spawn/threadStart and
// go/async take named functions only. Every path here is guarded by an intrinsic declaration or a
// go/async/await keyword, so programs that use neither (including csec_compiler.csec) are unaffected.

static std::vector<std::pair<int, int>> csec_call_argument_ranges(const char* tokens, int open, int close) {
    std::vector<std::pair<int, int>> ranges;
    int argumentStart = open + 1;
    for (int cursor = argumentStart; cursor <= close; ++cursor) {
        if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
            if (cursor > argumentStart) ranges.push_back({ argumentStart, cursor });
            argumentStart = cursor + 1;
        }
    }
    return ranges;
}

// The closing paren when `name(...)` spans exactly [start, end); otherwise -1.
static int csec_whole_call_close(const char* tokens, int start, int end) {
    if (csec_token_kind_at(tokens, start) != 'I' || !csec_native_token_is(tokens, start + 1, 'O', "(")) return -1;
    const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
    return close == end - 1 ? close : -1;
}

static bool csec_is_intrinsic_call(const char* tokens, int start, int end, const char* name) {
    return csec_native_token_is(tokens, start, 'I', name) && csec_whole_call_close(tokens, start, end) >= 0 &&
        csec_is_declared_intrinsic(tokens, name);
}

static std::string csec_emit_value_of_llvm_type(const char* tokens, int start, int end, const std::string& llvmType, const std::string& name) {
    if (llvmType == "i64") return csec_emit_i64_expression(tokens, start, end, name);
    if (llvmType == "double") return csec_emit_f64_expression(tokens, start, end, name);
    if (llvmType == "float") return csec_emit_f32_expression(tokens, start, end, name);
    if (llvmType == "i1") return csec_emit_i1_expression(tokens, start, end, name);
    if (llvmType == "ptr") return csec_emit_ptr_expression(tokens, start, end, name);
    if (llvmType == "i8") return csec_emit_i8_expression(tokens, start, end, name);
    if (llvmType == "i16") return csec_emit_i16_expression(tokens, start, end, name);
    return csec_emit_i32_expression(tokens, start, end, name);
}

// Widen a value to the 64-bit Future payload.
static std::string csec_task_payload(const std::string& value, const std::string& llvmType, const std::string& out) {
    if (llvmType == "i64") return "  " + out + " = add i64 " + value + ", 0\n";
    if (llvmType == "i1") return "  " + out + " = zext i1 " + value + " to i64\n";
    if (llvmType == "i32" || llvmType == "i16" || llvmType == "i8") return "  " + out + " = sext " + llvmType + " " + value + " to i64\n";
    if (llvmType == "double") return "  " + out + " = bitcast double " + value + " to i64\n";
    if (llvmType == "float") return "  " + out + ".wide = fpext float " + value + " to double\n  " + out + " = bitcast double " + out + ".wide to i64\n";
    return "  " + out + " = ptrtoint ptr " + value + " to i64\n";
}

static std::string csec_task_env_type_name(int site) { return "%csec.task.env." + std::to_string(site); }
static std::string csec_task_function_name(int site) { return "csec_task_" + std::to_string(site); }

static bool csec_selfhost_future_type(const std::string& type) {
    return type == "Future" || type.rfind("Future<", 0) == 0 || type.rfind("Future <", 0) == 0;
}

static bool csec_task_managed_type(const std::string& type) {
    return csec_selfhost_future_type(type) || type == "Tensor" || type.rfind("Tensor<", 0) == 0 || type.rfind("Tensor <", 0) == 0;
}

static std::string csec_task_callee_symbol(const char* name) {
    return std::strcmp(name, "main") == 0 ? std::string("csec_user_main") : std::string(name);
}

// Start the call that follows the `go`/`async` keyword at `site` in a new microthread. The arguments
// are evaluated here and copied (with the future) into the task environment; `async` yields the
// Future in `resultName`, `go` yields the pid.
static std::string csec_emit_task_start(const char* tokens, int site, int end, const std::string& resultName, bool wantFuture) {
    const int callStart = site + 1;
    const int close = csec_whole_call_close(tokens, callStart, end);
    const int callee = close >= 0 ? csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, callStart)) : -1;
    if (callee < 0) {
        return wantFuture ? "  " + resultName + " = getelementptr i8, ptr null, i64 0\n" : "  " + resultName + " = add i32 0, 0\n";
    }
    const auto parameters = csec_function_params(tokens, callee);
    const auto arguments = csec_call_argument_ranges(tokens, callStart + 1, close);
    const std::string envType = csec_task_env_type_name(site);
    const std::string env = resultName + ".env";
    std::string output;
    std::vector<std::string> managedArguments;
    const std::string future = wantFuture ? resultName : std::string("null");
    if (wantFuture) {
        output += "  " + resultName + " = call ptr @csec_future_new()\n" +
            "  call void @csec_selfhost_scope_track(ptr %tess.managed.scope, ptr " + resultName + ")\n";
    }
    output += "  " + env + " = alloca " + envType + "\n";
    output += "  " + env + ".f = getelementptr inbounds " + envType + ", ptr " + env + ", i32 0, i32 0\n";
    output += "  store ptr " + future + ", ptr " + env + ".f\n";
    for (size_t index = 0; index < arguments.size() && index < parameters.size(); ++index) {
        const std::string llvmType = csec_llvm_type_for_name(parameters[index].second.c_str());
        const std::string value = resultName + ".arg" + std::to_string(index);
        output += csec_emit_value_of_llvm_type(tokens, arguments[index].first, arguments[index].second, llvmType, value);
        if (csec_task_managed_type(parameters[index].second)) {
            output += "  call void @csec_rc_retain(ptr " + value + ")\n";
            managedArguments.push_back(value);
        }
        output += "  " + value + ".slot = getelementptr inbounds " + envType + ", ptr " + env + ", i32 0, i32 " + std::to_string(index + 1) + "\n";
        output += "  store " + llvmType + " " + value + ", ptr " + value + ".slot\n";
    }
    const std::string pid = wantFuture ? resultName + ".pid" : resultName;
    output += "  " + pid + " = call i32 " +
        (wantFuture ? "@csec_future_start(ptr " + future + ", " : std::string("@csec_process_spawn(")) +
        "ptr @" + csec_task_function_name(site) + ", ptr " + env +
        ", i64 ptrtoint (ptr getelementptr (" + envType + ", ptr null, i32 1) to i64))\n";
    if (!managedArguments.empty()) {
        const std::string failed = resultName + ".failed";
        output += "  " + failed + " = icmp eq i32 " + pid + ", 0\n";
        for (const auto& value : managedArguments) {
            output += "  " + value + ".failed = select i1 " + failed + ", ptr " + value + ", ptr null\n";
            output += "  call void @csec_rc_release(ptr " + value + ".failed)\n";
        }
    }
    return output;
}

// `await e` where e is a Future local or an inline `async f(...)`: the raw 64-bit payload.
static std::string csec_emit_await_payload(const char* tokens, int start, int end, const std::string& rawName) {
    std::string output;
    const std::string future = rawName + ".future";
    if (csec_native_token_is(tokens, start + 1, 'K', "async")) {
        output += csec_emit_task_start(tokens, start + 1, end, future, true);
    } else if (csec_token_kind_at(tokens, start + 1) == 'I' && start + 2 == end) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start + 1, csec_token_text_at(tokens, start + 1));
        output += "  " + future + " = load ptr, ptr %" + std::string(storage) + "\n";
    } else {
        output += csec_emit_ptr_expression(tokens, start + 1, end, future);
    }
    output += "  " + rawName + " = call i64 @csec_future_await(ptr " + future + ")\n";
    return output;
}

// `await e` that waits on a Future: `e` is an inline `async f(...)` or a local holding a Future.
// Any other `await e` (e.g. awaiting a call of an `async def`) yields `e` unchanged, as in the native
// compiler.
static bool csec_is_await_expression(const char* tokens, int start, int end) {
    if (!csec_native_token_is(tokens, start, 'K', "await") || start + 1 >= end) return false;
    if (csec_native_token_is(tokens, start + 1, 'K', "async")) return csec_whole_call_close(tokens, start + 2, end) >= 0;
    if (csec_token_kind_at(tokens, start + 1) == 'I' && start + 2 == end) {
        // The local's own declaration statement decides (an annotation `: Future<...>` or an
        // `= async ...` initializer).
        const int declaration = csec_find_visible_local(tokens, start + 1, csec_token_text_at(tokens, start + 1));
        if (declaration < 0) return false;
        const int statementEnd = csec_advance_statement(tokens, declaration, start + 1);
        const int initializer = csec_find_top_level_operator(tokens, declaration, statementEnd, 1);
        return csec_selfhost_future_type(csec_c_local_type(tokens, declaration, statementEnd, initializer));
    }
    return false;
}

static bool csec_is_await_identity(const char* tokens, int start, int end) {
    return csec_native_token_is(tokens, start, 'K', "await") && start + 1 < end && !csec_is_await_expression(tokens, start, end);
}

static bool csec_is_task_expression(const char* tokens, int start, int end, const char* keyword) {
    return csec_native_token_is(tokens, start, 'K', keyword) && csec_whole_call_close(tokens, start + 1, end) >= 0;
}

// i32-valued microthread intrinsics; "" when [start, end) is not one of them.
static std::string csec_emit_i32_intrinsic(const char* tokens, int start, int end, const std::string& r) {
    if (csec_token_kind_at(tokens, start) != 'I') return "";
    const char* name = csec_token_text_at(tokens, start);
    static const char* const names[] = { "spawn", "self", "send", "receive", "receiveTimeout", "yield", "isAlive",
        "monitor", "mailboxSize", "processCount", "joinProcesses" };
    bool known = false;
    for (const char* candidate : names) known = known || std::strcmp(name, candidate) == 0;
    if (!known || !csec_is_intrinsic_call(tokens, start, end, name)) return "";
    const int close = end - 1;
    const auto args = csec_call_argument_ranges(tokens, start + 1, close);
    auto arg32 = [&](size_t index, const std::string& out) {
        return index < args.size() ? csec_emit_i32_expression(tokens, args[index].first, args[index].second, out)
                                   : "  " + out + " = add i32 0, 0\n";
    };
    auto arg64 = [&](size_t index, const std::string& out) {
        return arg32(index, out + ".narrow") + "  " + out + " = sext i32 " + out + ".narrow to i64\n";
    };
    const std::string n = name;
    if (n == "spawn") {
        if (args.size() == 1 && csec_token_kind_at(tokens, args[0].first) == 'I' && args[0].first + 1 == args[0].second) {
            const char* function = csec_token_text_at(tokens, args[0].first);
            return "  " + r + " = call i32 @csec_process_spawn(ptr @" + csec_task_callee_symbol(function) + ", ptr null, i64 0)\n";
        }
        return "  " + r + " = add i32 0, 0\n"; // the self-host spawns named functions only
    }
    if (n == "self") return "  " + r + " = call i32 @csec_process_self()\n";
    if (n == "send") return arg32(0, r + ".pid") + arg64(1, r + ".msg") +
        "  " + r + " = call i32 @csec_process_send(i32 " + r + ".pid, i64 " + r + ".msg)\n";
    if (n == "receive") return "  " + r + ".wide = call i64 @csec_process_receive()\n  " + r + " = trunc i64 " + r + ".wide to i32\n";
    if (n == "receiveTimeout") return arg64(0, r + ".ms") + arg64(1, r + ".fallback") +
        "  " + r + ".wide = call i64 @csec_process_receive_timeout(i64 " + r + ".ms, i64 " + r + ".fallback)\n  " + r + " = trunc i64 " + r + ".wide to i32\n";
    if (n == "yield") return "  call void @csec_process_yield()\n  " + r + " = add i32 0, 0\n";
    if (n == "isAlive") return arg32(0, r + ".pid") + "  " + r + " = call i32 @csec_process_alive(i32 " + r + ".pid)\n";
    if (n == "monitor") return arg32(0, r + ".pid") + arg64(1, r + ".msg") +
        "  " + r + " = call i32 @csec_process_monitor(i32 " + r + ".pid, i64 " + r + ".msg)\n";
    if (n == "mailboxSize") return "  " + r + " = call i32 @csec_process_mailbox_size()\n";
    if (n == "processCount") return "  " + r + " = call i32 @csec_process_count()\n";
    return "  " + r + " = call i32 @csec_process_join()\n";
}

// Declarations and task thunks for a module that uses concurrency; "" for every other module.
static bool csec_module_uses_concurrency(const char* tokens) {
    for (int cursor = 0; csec_token_kind_at(tokens, cursor) != 'E'; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'K', "go") || csec_native_token_is(tokens, cursor, 'K', "await") ||
            (csec_native_token_is(tokens, cursor, 'K', "async") && !csec_native_token_is(tokens, cursor + 1, 'K', "def")) ||
            (csec_native_token_is(tokens, cursor, 'O', "[@") && csec_native_token_is(tokens, cursor + 1, 'I', "Intrinsic"))) {
            return true;
        }
    }
    return false;
}

static std::string csec_concurrency_definitions(const char* tokens) {
    if (!csec_module_uses_concurrency(tokens)) return "";
    std::string output =
        "declare i32 @csec_process_spawn(ptr, ptr, i64)\n"
        "declare i32 @csec_process_self()\n"
        "declare i32 @csec_process_send(i32, i64)\n"
        "declare i64 @csec_process_receive()\n"
        "declare i64 @csec_process_receive_timeout(i64, i64)\n"
        "declare void @csec_process_yield()\n"
        "declare i32 @csec_process_alive(i32)\n"
        "declare i32 @csec_process_monitor(i32, i64)\n"
        "declare i32 @csec_process_mailbox_size()\n"
        "declare i32 @csec_process_count()\n"
        "declare i32 @csec_process_join()\n"
        "declare ptr @csec_future_new()\n"
        "declare i32 @csec_future_start(ptr, ptr, ptr, i64)\n"
        "declare void @csec_future_complete(ptr, i64)\n"
        "declare i64 @csec_future_await(ptr)\n"
        "declare void @csec_rc_retain(ptr)\n"
        "declare void @csec_rc_release(ptr)\n"
        "declare i64 @csec_thread_start(ptr, ptr, i64)\n\n";
    const int tokenCount = tokenCountCached(tokens ? tokens : "");
    for (int site = 0; site < tokenCount; ++site) {
        const bool isGo = csec_native_token_is(tokens, site, 'K', "go");
        const bool isAsync = csec_native_token_is(tokens, site, 'K', "async");
        if (!isGo && !isAsync) continue;
        if (csec_token_kind_at(tokens, site + 1) != 'I' || !csec_native_token_is(tokens, site + 2, 'O', "(")) continue;
        const int close = csec_find_closing_token(tokens, site + 2, tokenCount, "(", ")");
        const int callee = close >= 0 ? csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, site + 1)) : -1;
        if (callee < 0) continue;
        const auto parameters = csec_function_params(tokens, callee);
        const auto arguments = csec_call_argument_ranges(tokens, site + 2, close);
        const std::string envType = csec_task_env_type_name(site);
        std::string fields = "ptr";
        std::string loads;
        std::string releaseArguments;
        std::string callArguments;
        for (size_t index = 0; index < arguments.size() && index < parameters.size(); ++index) {
            const std::string llvmType = csec_llvm_type_for_name(parameters[index].second.c_str());
            const std::string value = "%arg" + std::to_string(index);
            fields += ", " + llvmType;
            loads += "  " + value + ".slot = getelementptr inbounds " + envType + ", ptr %env, i32 0, i32 " + std::to_string(index + 1) + "\n";
            loads += "  " + value + " = load " + llvmType + ", ptr " + value + ".slot\n";
            if (csec_task_managed_type(parameters[index].second)) {
                releaseArguments += "  call void @csec_rc_release(ptr " + value + ")\n";
            }
            if (!callArguments.empty()) callArguments += ", ";
            callArguments += llvmType + " " + value;
        }
        const std::string returnType = csec_llvm_type_for_name(csec_function_return_type_at(tokens, callee));
        output += envType + " = type { " + fields + " }\n\n";
        output += "define internal void @" + csec_task_function_name(site) + "(ptr %env) {\nentry:\n";
        output += "  %future.slot = getelementptr inbounds " + envType + ", ptr %env, i32 0, i32 0\n";
        output += "  %future = load ptr, ptr %future.slot\n";
        output += loads;
        output += (returnType == "void" ? "  call " : "  %result = call ") + returnType +
            " @" + csec_task_callee_symbol(csec_token_text_at(tokens, site + 1)) + "(" + callArguments + ")\n";
        output += releaseArguments;
        if (isAsync) {
            if (returnType != "void") output += csec_task_payload("%result", returnType, "%payload");
            output += "  call void @csec_future_complete(ptr %future, i64 ";
            output += returnType == "void" ? "0)\n" : "%payload)\n";
            output += "  call void @csec_rc_release(ptr %future)\n";
        }
        output += "  ret void\n}\n\n";
    }
    return output;
}


// A whole-range call `name(args)` of an `external def` (not an intrinsic, not shadowed by a `def`):
// arguments follow the declared parameter types and the result is converted to `contextType`.
// Returns "" when [start, end) is not such a call or the result cannot be converted.
static std::string csec_emit_external_call(const char* tokens, int start, int end, const std::string& resultName, const std::string& contextType) {
    const int close = csec_whole_call_close(tokens, start, end);
    if (close < 0) return "";
    const char* name = csec_token_text_at(tokens, start);
    int declaration = csec_top_level_function_declaration(tokens, name);
    if (declaration >= 0) {
        // A `def` callee takes this path only when it has a Long parameter (which the older call paths
        // pass as i32) and every parameter is a plain scalar or String.
        // The i64 call path also passes String arguments as i64, so String parameters qualify there.
        bool hasWide = false;
        bool hasString = false;
        for (const auto& parameter : csec_function_params(tokens, declaration)) {
            const std::string& type = parameter.second;
            if (type != "Int" && type != "Long" && type != "Boolean" && type != "Double" && type != "String") return "";
            hasWide = hasWide || type == "Long";
            hasString = hasString || type == "String";
        }
        if (!hasWide && !(contextType == "i64" && hasString)) return "";
    } else {
        declaration = csec_top_level_external_function_declaration(tokens, name);
    }
    if (declaration < 0) return "";
    const auto parameters = csec_function_params(tokens, declaration);
    const auto arguments = csec_call_argument_ranges(tokens, start + 1, close);
    if (arguments.size() != parameters.size()) return "";
    const std::string returnType = csec_llvm_type_for_name(csec_function_return_type_at(tokens, declaration));
    const bool integerReturn = returnType == "i32" || returnType == "i64" || returnType == "i1" || returnType == "i8" || returnType == "i16";
    const bool integerContext = contextType == "i32" || contextType == "i64" || contextType == "i1";
    // A Void/Unit external maps to `ptr`; as a statement it is lowered in i32 context (result discarded).
    const bool pointerAsInt = returnType == "ptr" && contextType == "i32";
    if (returnType != contextType && !(integerReturn && integerContext) && !pointerAsInt) return "";
    std::string output;
    std::string argumentList;
    for (size_t index = 0; index < arguments.size(); ++index) {
        const std::string type = csec_llvm_type_for_name(parameters[index].second);
        const std::string value = resultName + ".x" + std::to_string(index);
        output += csec_emit_value_of_llvm_type(tokens, arguments[index].first, arguments[index].second, type, value);
        if (!argumentList.empty()) argumentList += ", ";
        argumentList += type + " " + value;
    }
    const std::string raw = returnType == contextType ? resultName : resultName + ".raw";
    output += "  " + raw + " = call " + returnType + " @" + csec_task_callee_symbol(name) + "(" + argumentList + ")\n";
    if (raw == resultName) return output;
    if (pointerAsInt) {
        return output + "  " + resultName + ".address = ptrtoint ptr " + raw + " to i64\n" +
            "  " + resultName + " = trunc i64 " + resultName + ".address to i32\n";
    }
    if (contextType == "i1") return output + "  " + resultName + " = icmp ne " + returnType + " " + raw + ", 0\n";
    if (returnType == "i1") return output + "  " + resultName + " = zext i1 " + raw + " to " + contextType + "\n";
    const int fromBits = returnType == "i64" ? 64 : (returnType == "i16" ? 16 : (returnType == "i8" ? 8 : 32));
    const int toBits = contextType == "i64" ? 64 : 32;
    return output + "  " + resultName + " = " + std::string(fromBits < toBits ? "sext " : "trunc ") + returnType + " " + raw + " to " + contextType + "\n";
}

// `text[i]` on a String local or parameter: byte i, read with one load (as in the native compiler).
static std::string csec_emit_string_index(const char* tokens, int start, int end, const std::string& resultName) {
    if (csec_token_kind_at(tokens, start) != 'I' || !csec_native_token_is(tokens, start + 1, 'O', "[")) return "";
    const int close = csec_find_closing_token(tokens, start + 1, end, "[", "]");
    if (close != end - 1 || close <= start + 2) return "";
    const char* name = csec_token_text_at(tokens, start);
    if (!csec_is_string_local(tokens, start, name) && !csec_is_string_parameter(tokens, start, name)) return "";
    const char* storage = csec_lookup_visible_storage_name(tokens, start, name);
    return "  " + resultName + ".base = load ptr, ptr %" + std::string(storage) + "\n" +
        csec_emit_i64_expression(tokens, start + 2, close, resultName + ".index") +
        "  " + resultName + ".ptr = getelementptr inbounds i8, ptr " + resultName + ".base, i64 " + resultName + ".index\n" +
        "  " + resultName + " = load i8, ptr " + resultName + ".ptr\n";
}
} // extern "C++"

static std::string csec_emit_ptr_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "ptr", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_is_task_expression(tokens, start, end, "async")) return csec_emit_task_start(tokens, start, end, resultName, true);
    if (csec_is_await_identity(tokens, start, end)) return csec_emit_ptr_expression(tokens, start + 1, end, resultName);
    if (csec_is_await_expression(tokens, start, end)) {
        return csec_emit_await_payload(tokens, start, end, resultName + ".raw") +
            "  " + resultName + " = inttoptr i64 " + resultName + ".raw to ptr\n";
    }
    {
        const std::string external = csec_emit_external_call(tokens, start, end, resultName, "ptr");
        if (!external.empty()) return external;
    }
    const std::string indexedArray = csec_emit_ptr_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    // Use the string-literal-aware scan here so a concatenation chain whose operands include string
    // literals with operator-like text ("<", ">", "(") splits on the real `+` rather than on a
    // literal, which would otherwise drop the expression to the null-pointer fallback.
    const int operation = csec_ptr_top_level_operator(tokens, start, end);
    // Operator overloading returning String: `instance <op> operand` where the instance's class
    // defines an `operator<op>` returning String. Handled before string concatenation so the
    // instance is dispatched rather than mistaken for a concatenation operand.
    if (operation >= start && start + 1 == operation && csec_token_kind_at(tokens, start) == 'I') {
        const int classStart = csec_local_instance_class(tokens, start, csec_token_text_at(tokens, start));
        if (classStart >= 0) {
            const std::string operatorMethod = std::string("operator") + csec_token_text_at(tokens, operation);
            const int method = csec_resolve_class_method_by_args(tokens, classStart, operatorMethod.c_str(), operation + 1, end);
            if (method >= 0 && std::strcmp(csec_function_return_type_at(tokens, method), "String") == 0) {
                std::string output;
                std::string arguments;
                if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string receiver = resultName + ".oprecv";
                    output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                    arguments = "ptr " + receiver;
                }
                const auto parameters = csec_function_params(tokens, method);
                const std::string operandValue = resultName + ".oparg";
                const std::string operandType = parameters.empty() ? std::string("Int") : parameters.back().second;
                if (std::strcmp(csec_llvm_type_for_name(operandType), "ptr") == 0) {
                    output += csec_emit_ptr_expression(tokens, operation + 1, end, operandValue);
                    arguments += (arguments.empty() ? "" : ", ") + std::string("ptr ") + operandValue;
                } else {
                    output += csec_emit_i32_expression(tokens, operation + 1, end, operandValue);
                    arguments += (arguments.empty() ? "" : ", ") + std::string("i32 ") + operandValue;
                }
                return output + "  " + resultName + " = call ptr @" +
                    csec_class_method_symbol(tokens, classStart, method) + "(" + arguments + ")\n";
            }
        }
    }
    if (operation >= start && csec_native_token_is(tokens, operation, 'O', "+") &&
        csec_is_string_expression(tokens, start, operation) && csec_is_string_expression(tokens, operation + 1, end)) {
        const std::string left = resultName + ".left";
        const std::string right = resultName + ".right";
        return csec_emit_ptr_expression(tokens, start, operation, left) +
            csec_emit_ptr_expression(tokens, operation + 1, end, right) +
            "  " + resultName + " = call ptr @csec_string_concat(ptr " + left + ", ptr " + right + ")\n";
    }
    // `String + <integer>` (or the reverse): stringify the integer side with csec_to_string_i64,
    // then concatenate. Only fires when exactly one side is a string, so String + String above
    // and purely numeric additions are unaffected.
    if (operation >= start && csec_native_token_is(tokens, operation, 'O', "+") &&
        (csec_is_string_expression(tokens, start, operation) != csec_is_string_expression(tokens, operation + 1, end))) {
        const std::string left = resultName + ".left";
        const std::string right = resultName + ".right";
        auto stringifySide = [&](int from, int to, const std::string& name) -> std::string {
            if (csec_is_string_expression(tokens, from, to)) {
                return csec_emit_ptr_expression(tokens, from, to, name);
            }
            const std::string narrow = name + ".i32";
            const std::string wide = name + ".i64";
            return csec_emit_i32_expression(tokens, from, to, narrow) +
                "  " + wide + " = sext i32 " + narrow + " to i64\n" +
                "  " + name + " = call ptr @csec_to_string_i64(i64 " + wide + ")\n";
        };
        return stringifySide(start, operation, left) +
            stringifySide(operation + 1, end, right) +
            "  " + resultName + " = call ptr @csec_string_concat(ptr " + left + ", ptr " + right + ")\n";
    }
    if (end == start + 1 && csec_token_kind_at(tokens, start) == 'S') {
        return "  " + resultName + " = getelementptr inbounds [" +
            std::to_string(csec_llvm_string_literal_byte_length(csec_token_text_at(tokens, start))) +
            " x i8], ptr @.str." + std::to_string(start) + ", i32 0, i32 0\n";
    }
    if (end == start + 1 && csec_token_kind_at(tokens, start) == 'I' &&
        (csec_is_string_local(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_string_parameter(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_string_class_constructor_param(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_pointer_local(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_pointer_parameter(tokens, start, csec_token_text_at(tokens, start)))) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        return "  " + resultName + " = load ptr, ptr %" + std::string(storage) + "\n";
    }
    if (end == start + 1 && csec_token_kind_at(tokens, start) == 'I' &&
        (csec_visible_i32_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_i1_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_i8_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_i16_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_i64_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_f32_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_f64_array(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_visible_ptr_array(tokens, start, csec_token_text_at(tokens, start)))) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        return "  " + resultName + " = load ptr, ptr %" + std::string(storage) + "\n";
    }
    if ((csec_token_kind_at(tokens, start) == 'I' || csec_token_kind_at(tokens, start) == 'S') && start + 4 < end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(") && csec_is_string_expression(tokens, start, start + 1)) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        if (close == end - 1) {
            const std::string method = csec_token_text_at(tokens, start + 2);
            const std::string object = resultName + ".string";
            if ((method == "trim" || method == "toUpper" || method == "toLower" || method == "toString") && close == start + 4) {
                const char* runtime = method == "trim" ? "csec_string_trim" :
                    (method == "toUpper" ? "csec_string_to_upper" : "csec_string_to_lower");
                const std::string source = csec_emit_ptr_expression(tokens, start, start + 1, object);
                if (method == "toString") return source + "  " + resultName + " = getelementptr i8, ptr " + object + ", i32 0\n";
                return source + "  " + resultName + " = call ptr @" + runtime + "(ptr " + object + ")\n";
            }
            if (method == "substring" && close > start + 6) {
                const int comma = csec_find_token_text_in_range(tokens, start + 4, close, ",");
                if (comma > start + 4 && comma + 1 < close &&
                    csec_find_token_text_in_range(tokens, comma + 1, close, ",") < 0) {
                    const std::string offset = resultName + ".offset";
                    const std::string length = resultName + ".length";
                    return csec_emit_ptr_expression(tokens, start, start + 1, object) +
                        csec_emit_i32_expression(tokens, start + 4, comma, offset) +
                        csec_emit_i32_expression(tokens, comma + 1, close, length) +
                        "  " + resultName + " = call ptr @csec_string_substring(ptr " + object + ", i32 " + offset + ", i32 " + length + ")\n";
                }
            }
        }
    }
    std::string instanceCall;
    if (csec_emit_ptr_instance_call(tokens, start, end, resultName, instanceCall)) return instanceCall;
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (callee >= 0 && close == end - 1 &&
            std::strcmp(csec_llvm_type_for_name(csec_function_return_type_at(tokens, callee)), "ptr") == 0) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments;
            int argumentStart = start + 2, index = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string argument = resultName + ".arg" + std::to_string(index++);
                        const bool pointerArgument = index - 1 < static_cast<int>(parameters.size()) &&
                            std::strcmp(csec_llvm_type_for_name(parameters[index - 1].second), "ptr") == 0;
                        output += pointerArgument ? csec_emit_ptr_expression(tokens, argumentStart, cursor, argument)
                                                  : csec_emit_i32_expression(tokens, argumentStart, cursor, argument);
                        if (!arguments.empty()) arguments += ", ";
                        arguments += std::string(pointerArgument ? "ptr " : "i32 ") + argument;
                    }
                    argumentStart = cursor + 1;
                }
            }
            output += "  " + resultName + " = call ptr @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
            if (csec_selfhost_future_type(csec_function_return_type_at(tokens, callee))) {
                output += "  call void @csec_selfhost_scope_track(ptr %tess.managed.scope, ptr " + resultName + ")\n";
            }
            return output;
        }
    }
    return "  " + resultName + " = getelementptr i8, ptr null, i32 0\n";
}

static std::string csec_emit_i32_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = add i32 0, 0\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "i32", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_is_task_expression(tokens, start, end, "go")) return csec_emit_task_start(tokens, start, end, resultName, false);
    if (csec_is_await_identity(tokens, start, end)) return csec_emit_i32_expression(tokens, start + 1, end, resultName);
    if (csec_is_await_expression(tokens, start, end)) {
        return csec_emit_await_payload(tokens, start, end, resultName + ".raw") +
            "  " + resultName + " = trunc i64 " + resultName + ".raw to i32\n";
    }
    {
        const std::string intrinsic = csec_emit_i32_intrinsic(tokens, start, end, resultName);
        if (!intrinsic.empty()) return intrinsic;
    }
    {
        const std::string stringByte = csec_emit_string_index(tokens, start, end, resultName + ".byte");
        if (!stringByte.empty()) return stringByte + "  " + resultName + " = zext i8 " + resultName + ".byte to i32\n";
    }
    {
        const std::string external = csec_emit_external_call(tokens, start, end, resultName, "i32");
        if (!external.empty()) return external;
    }
    if (csec_native_token_is(tokens, start, 'O', "{") &&
        csec_find_closing_token(tokens, start, end, "{", "}") == end - 1) {
        return csec_emit_i32_block_expression(tokens, start, end, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "[")) {
        const int captureClose = csec_find_closing_token(tokens, start, end, "[", "]");
        const int parameterOpen = captureClose >= 0 ? captureClose + 1 : -1;
        const int parameterClose = parameterOpen >= 0 && csec_native_token_is(tokens, parameterOpen, 'O', "(")
            ? csec_find_closing_token(tokens, parameterOpen, end, "(", ")") : -1;
        if (parameterClose >= 0 && csec_native_token_is(tokens, parameterClose + 1, 'O', "->")) {
            return "  " + resultName + " = add i32 0, 0\n";
        }
    }
    for (int cursor = start + 1; cursor < end; ++cursor) {
        if (csec_native_token_is(tokens, cursor, 'K', "match")) {
            return csec_emit_i32_match_expression(tokens, start, end, cursor, resultName);
        }
    }
    if (csec_native_token_is(tokens, start, 'O', "(") &&
        csec_find_closing_token(tokens, start, end, "(", ")") == end - 1) {
        return csec_emit_i32_expression(tokens, start + 1, end - 1, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "+") && start + 1 < end) {
        return csec_emit_i32_expression(tokens, start + 1, end, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "-") && start + 1 < end) {
        const std::string valueName = resultName + ".value";
        return csec_emit_i32_expression(tokens, start + 1, end, valueName) +
            "  " + resultName + " = sub i32 0, " + valueName + "\n";
    }
    if (csec_native_token_is(tokens, start, 'O', "!") && start + 1 < end) {
        const std::string valueName = resultName + ".value";
        const std::string comparisonName = resultName + ".comparison";
        return csec_emit_i32_expression(tokens, start + 1, end, valueName) +
            "  " + comparisonName + " = icmp eq i32 " + valueName + ", 0\n" +
            "  " + resultName + " = zext i1 " + comparisonName + " to i32\n";
    }
    if (csec_native_token_is(tokens, start, 'O', "~") && start + 1 < end) {
        const std::string valueName = resultName + ".value";
        return csec_emit_i32_expression(tokens, start + 1, end, valueName) +
            "  " + resultName + " = xor i32 " + valueName + ", -1\n";
    }
    // Array postfixes are expression atoms. Split a following binary operation
    // here so the array index is lowered before the normal precedence walker.
    if (csec_token_kind_at(tokens, start) == 'I' && csec_native_token_is(tokens, start + 1, 'O', "[")) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "[", "]");
        if (close >= start + 2 && close + 2 < end) {
            const char* postfixOp = csec_token_text_at(tokens, close + 1);
            const char* instruction = std::strcmp(postfixOp, "+") == 0 ? "add" :
                (std::strcmp(postfixOp, "-") == 0 ? "sub" : (std::strcmp(postfixOp, "*") == 0 ? "mul" :
                (std::strcmp(postfixOp, "/") == 0 ? "sdiv" : (std::strcmp(postfixOp, "%") == 0 ? "srem" : nullptr))));
            if (instruction) {
                const std::string leftName = resultName + ".left";
                const std::string rightName = resultName + ".right";
                return csec_emit_i32_expression(tokens, start, close + 1, leftName) +
                    csec_emit_i32_expression(tokens, close + 2, end, rightName) +
                    "  " + resultName + " = " + instruction + " i32 " + leftName + ", " + rightName + "\n";
            }
        }
    }
    const int operation = csec_i32_top_level_operator(tokens, start, end);
    if (operation >= start) {
        const std::string leftName = resultName + ".left";
        const std::string rightName = resultName + ".right";
        const char* op = csec_token_text_at(tokens, operation);
        // Operator overloading: when the left operand is a single class-instance local whose
        // class defines an `operator<op>` method returning Int, dispatch to that method instead
        // of the primitive instruction. The guard (single identifier bound to `new Class(...)`
        // with a matching operator method) never fires on primitive operands.
        if (start + 1 == operation && csec_token_kind_at(tokens, start) == 'I') {
            const char* leftIdentifier = csec_token_text_at(tokens, start);
            const int declaration = csec_find_visible_local(tokens, start, leftIdentifier);
            const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
            const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
            if (initializer >= declaration && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
                csec_token_kind_at(tokens, initializer + 2) == 'I') {
                const std::string className = csec_token_text_at(tokens, initializer + 2);
                const int classStart = csec_find_class_declaration(tokens, className.c_str());
                const std::string operatorMethod = std::string("operator") + op;
                const int method = classStart >= 0 ?
                    csec_resolve_class_method_by_args(tokens, classStart, operatorMethod.c_str(), operation + 1, end) : -1;
                if (method >= 0 && std::strcmp(csec_function_return_type_at(tokens, method), "Int") == 0) {
                    std::string output;
                    std::string arguments;
                    if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                        const char* storage = csec_lookup_visible_storage_name(tokens, start, leftIdentifier);
                        const std::string receiver = resultName + ".oprecv";
                        output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                        arguments = "ptr " + receiver;
                    } else {
                        // Scalar-receiver class: its methods take the constructor state as leading
                        // parameters (this is how `c.get()` reads a field), so re-evaluate the
                        // `new Class(...)` constructor arguments and pass them as the operator's
                        // receiver state. Without this the operator body reads uninitialised state.
                        const int constructorOpen = initializer + 3;
                        if (csec_native_token_is(tokens, constructorOpen, 'O', "(")) {
                            const int constructorClose = csec_find_closing_token(tokens, constructorOpen, declarationEnd, "(", ")");
                            int stateIndex = 0;
                            for (int cursor = constructorOpen + 1, itemStart = constructorOpen + 1;
                                 constructorClose > constructorOpen && cursor <= constructorClose; ++cursor) {
                                if (cursor == constructorClose || csec_is_argument_separator(tokens, constructorClose, cursor)) {
                                    if (cursor > itemStart) {
                                        const std::string stateValue = resultName + ".oprecv" + std::to_string(stateIndex++);
                                        output += csec_emit_i32_expression(tokens, itemStart, cursor, stateValue);
                                        if (!arguments.empty()) arguments += ", ";
                                        arguments += "i32 " + stateValue;
                                    }
                                    itemStart = cursor + 1;
                                }
                            }
                        }
                    }
                    const std::string operandValue = resultName + ".oparg";
                    output += csec_emit_i32_expression(tokens, operation + 1, end, operandValue);
                    if (!arguments.empty()) arguments += ", ";
                    arguments += "i32 " + operandValue;
                    return output + "  " + resultName + " = call i32 @" +
                        csec_class_method_symbol(tokens, classStart, method) + "(" + arguments + ")\n";
                }
            }
        }
        if ((std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0) &&
            csec_is_string_expression(tokens, start, operation) &&
            csec_is_string_expression(tokens, operation + 1, end)) {
            const std::string equal = resultName + ".equals";
            const std::string comparison = resultName + ".comparison";
            return csec_emit_ptr_expression(tokens, start, operation, leftName) +
                csec_emit_ptr_expression(tokens, operation + 1, end, rightName) +
                "  " + equal + " = call i32 @csec_string_equals(ptr " + leftName + ", ptr " + rightName + ")\n" +
                "  " + comparison + " = icmp " + (std::strcmp(op, "==") == 0 ? "ne" : "eq") + " i32 " + equal + ", 0\n" +
                "  " + resultName + " = zext i1 " + comparison + " to i32\n";
        }
        std::string output = csec_emit_i32_expression(tokens, start, operation, leftName);
        output += csec_emit_i32_expression(tokens, operation + 1, end, rightName);
        const char* instruction = "ashr";
        if (std::strcmp(op, "+") == 0) instruction = "add";
        else if (std::strcmp(op, "-") == 0) instruction = "sub";
        else if (std::strcmp(op, "*") == 0) instruction = "mul";
        else if (std::strcmp(op, "/") == 0) instruction = "sdiv";
        else if (std::strcmp(op, "%") == 0) instruction = "srem";
        else if (std::strcmp(op, "&") == 0) instruction = "and";
        else if (std::strcmp(op, "|") == 0) instruction = "or";
        else if (std::strcmp(op, "^") == 0) instruction = "xor";
        else if (std::strcmp(op, "<<") == 0) instruction = "shl";
        else if (std::strcmp(op, "and") == 0 || std::strcmp(op, "&&") == 0) instruction = "and";
        else if (std::strcmp(op, "or") == 0 || std::strcmp(op, "||") == 0) instruction = "or";
        else if (std::strcmp(op, "xor") == 0) instruction = "xor";
        if (std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0 ||
            std::strcmp(op, "<") == 0 || std::strcmp(op, "<=") == 0 ||
            std::strcmp(op, ">") == 0 || std::strcmp(op, ">=") == 0) {
            const char* predicate = std::strcmp(op, "==") == 0 ? "eq" :
                (std::strcmp(op, "!=") == 0 ? "ne" : (std::strcmp(op, "<") == 0 ? "slt" :
                (std::strcmp(op, "<=") == 0 ? "sle" : (std::strcmp(op, ">") == 0 ? "sgt" : "sge"))));
            const std::string comparisonName = resultName + ".comparison";
            return output + "  " + comparisonName + " = icmp " + predicate + " i32 " + leftName + ", " + rightName + "\n" +
                "  " + resultName + " = zext i1 " + comparisonName + " to i32\n";
        }
        return output + "  " + resultName + " = " + instruction + " i32 " + leftName + ", " + rightName + "\n";
    }
    const char kind = csec_token_kind_at(tokens, start);
    if (kind == 'N') return "  " + resultName + " = add i32 0, " + csec_i32_literal(csec_token_text_at(tokens, start)) + "\n";
    if (std::strcmp(csec_token_text_at(tokens, start), "true") == 0) return "  " + resultName + " = add i32 0, 1\n";
    if (std::strcmp(csec_token_text_at(tokens, start), "false") == 0) return "  " + resultName + " = add i32 0, 0\n";
    if (csec_token_kind_at(tokens, start) == 'I' && start + 2 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_is_string_expression(tokens, start, start + 1) &&
        (std::strcmp(csec_token_text_at(tokens, start + 2), "length") == 0 ||
         std::strcmp(csec_token_text_at(tokens, start + 2), "size") == 0)) {
        const bool property = start + 3 == end;
        const bool call = start + 5 == end && csec_native_token_is(tokens, start + 3, 'O', "(") &&
            csec_native_token_is(tokens, start + 4, 'O', ")");
        if (property || call) {
            const std::string value = resultName + ".string";
            const std::string length = resultName + ".length64";
            return csec_emit_ptr_expression(tokens, start, start + 1, value) +
                "  " + length + " = call i64 @csec_string_length(ptr " + value + ")\n" +
                "  " + resultName + " = trunc i64 " + length + " to i32\n";
        }
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 3 < end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(") && csec_is_string_expression(tokens, start, start + 1)) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        if (close == end - 1) {
            const std::string method = csec_token_text_at(tokens, start + 2);
            const std::string object = resultName + ".string";
            if (method == "isEmpty" && close == start + 4) {
                return csec_emit_ptr_expression(tokens, start, start + 1, object) +
                    "  " + resultName + " = call i32 @csec_string_is_empty(ptr " + object + ")\n";
            }
            if ((method == "contains" || method == "startsWith" || method == "endsWith" || method == "indexOf") &&
                close > start + 4 && csec_find_token_text_in_range(tokens, start + 4, close, ",") < 0) {
                const std::string argument = resultName + ".needle";
                std::string output = csec_emit_ptr_expression(tokens, start, start + 1, object) +
                    csec_emit_ptr_expression(tokens, start + 4, close, argument);
                if (method == "contains") return output +
                    "  " + resultName + " = call i32 @csec_string_contains(ptr " + object + ", ptr " + argument + ")\n";
                if (method == "startsWith") return output +
                    "  " + resultName + " = call i32 @csec_string_starts_with(ptr " + object + ", ptr " + argument + ")\n";
                if (method == "endsWith") return output +
                    "  " + resultName + " = call i32 @csec_string_ends_with(ptr " + object + ", ptr " + argument + ")\n";
                const std::string index = resultName + ".index64";
                return output + "  " + index + " = call i64 @csec_string_index_of(ptr " + object + ", ptr " + argument + ")\n" +
                    "  " + resultName + " = trunc i64 " + index + " to i32\n";
            }
            if (method == "charAt" && close > start + 4 &&
                csec_find_token_text_in_range(tokens, start + 4, close, ",") < 0) {
                const std::string index = resultName + ".index";
                const std::string character = resultName + ".char";
                return csec_emit_ptr_expression(tokens, start, start + 1, object) +
                    csec_emit_i32_expression(tokens, start + 4, close, index) +
                    "  " + character + " = call i8 @csec_string_char_at(ptr " + object + ", i32 " + index + ")\n" +
                    "  " + resultName + " = zext i8 " + character + " to i32\n";
            }
        }
    }
    const std::string indexedLiteral = csec_emit_i32_literal_array_index(tokens, start, end, resultName);
    if (!indexedLiteral.empty()) return indexedLiteral;
    const std::string indexedArray = csec_emit_i32_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    const std::string indexedCharArray = csec_emit_i8_array_index(tokens, start, end, resultName + ".char");
    if (!indexedCharArray.empty()) {
        return indexedCharArray + "  " + resultName + " = zext i8 " + resultName + ".char to i32\n";
    }
    const std::string indexedShortArray = csec_emit_i16_array_index(tokens, start, end, resultName + ".short");
    if (!indexedShortArray.empty()) {
        return indexedShortArray + "  " + resultName + " = sext i16 " + resultName + ".short to i32\n";
    }
    const std::string indexedLongArray = csec_emit_i64_array_index(tokens, start, end, resultName + ".long");
    if (!indexedLongArray.empty()) {
        return indexedLongArray + "  " + resultName + " = trunc i64 " + resultName + ".long to i32\n";
    }
    if (csec_token_kind_at(tokens, start) == 'C' && start + 1 == end) {
        const std::string character = resultName + ".char";
        return csec_emit_i8_expression(tokens, start, end, character) +
            "  " + resultName + " = zext i8 " + character + " to i32\n";
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 == end) {
        const std::string name = csec_token_text_at(tokens, start);
        const int declaration = csec_find_visible_local(tokens, start, name.c_str());
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (declaration >= 0 && csec_c_local_type(tokens, declaration, declarationEnd, initializer) == "Char") {
            const std::string character = resultName + ".char";
            return csec_emit_i8_expression(tokens, start, end, character) +
                "  " + resultName + " = zext i8 " + character + " to i32\n";
        }
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        if (callee >= 0 && std::strcmp(csec_function_return_type_at(tokens, callee), "Char") == 0) {
            const std::string character = resultName + ".char";
            return csec_emit_i8_expression(tokens, start, end, character) +
                "  " + resultName + " = zext i8 " + character + " to i32\n";
        }
    }
    std::string instanceCall;
    if (csec_emit_i32_instance_call(tokens, start, end, resultName, instanceCall)) return instanceCall;
    if (csec_native_token_is(tokens, start, 'K', "this") && start + 3 == end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I') {
        const std::string field = csec_token_text_at(tokens, start + 2);
        return "  " + resultName + " = load i32, ptr %" + field + "\n";
    }
    if (csec_native_token_is(tokens, start, 'K', "super") && start + 4 < end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int classStart = csec_enclosing_class_declaration(tokens, start);
        const std::string parent = classStart >= 0 ? csec_class_parent_name(tokens, classStart) : "";
        if (close == end - 1 && !parent.empty()) {
            const int parentStart = csec_find_class_declaration(tokens, parent.c_str());
            if (csec_class_uses_pointer_receiver(tokens, classStart) && parentStart >= 0 &&
                csec_class_uses_pointer_receiver(tokens, parentStart)) {
                return "  " + resultName + " = call i32 @" + parent + "_" +
                    csec_token_text_at(tokens, start + 2) + "(ptr %arg.this)\n";
            }
            std::string output;
            std::string arguments;
            int argumentIndex = 0;
            for (const auto& parameter : csec_class_constructor_params(tokens, classStart)) {
                if (std::strcmp(csec_llvm_type_for_name(parameter.second.c_str()), "i32") != 0) continue;
                const std::string value = resultName + ".super.arg" + std::to_string(argumentIndex++);
                output += "  " + value + " = load i32, ptr %" + parameter.first + "\n";
                if (!arguments.empty()) arguments += ", ";
                arguments += "i32 " + value;
            }
            return output + "  " + resultName + " = call i32 @" + parent + "_" +
                csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
        }
    }
    if (csec_native_token_is(tokens, start, 'K', "this") && start + 4 < end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(")) {
        // `this.method(args)` calls another method of the enclosing class on the same receiver.
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int classStart = csec_enclosing_class_declaration(tokens, start);
        if (close == end - 1 && classStart >= 0) {
            const std::string className = csec_top_level_decl_name(tokens, classStart);
            const std::string method = csec_token_text_at(tokens, start + 2);
            std::string output;
            std::string arguments;
            if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                arguments = "ptr %arg.this";
            } else {
                int receiverIndex = 0;
                for (const auto& parameter : csec_class_constructor_params(tokens, classStart)) {
                    if (std::strcmp(csec_llvm_type_for_name(parameter.second.c_str()), "i32") != 0) continue;
                    const std::string value = resultName + ".this.recv" + std::to_string(receiverIndex++);
                    output += "  " + value + " = load i32, ptr %" + parameter.first + "\n";
                    if (!arguments.empty()) arguments += ", ";
                    arguments += "i32 " + value;
                }
            }
            int argumentStart = start + 4;
            int argumentIndex = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string value = resultName + ".marg" + std::to_string(argumentIndex++);
                        output += csec_emit_i32_expression(tokens, argumentStart, cursor, value);
                        if (!arguments.empty()) arguments += ", ";
                        arguments += "i32 " + value;
                    }
                    argumentStart = cursor + 1;
                }
            }
            return output + "  " + resultName + " = call i32 @" + className + "_" + method + "(" + arguments + ")\n";
        }
    }
    if (kind == 'I' && start + 3 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' && csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        if (close == end - 1) {
            std::string output;
            std::string arguments;
            int argumentStart = start + 4;
            int argumentIndex = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string argumentName = resultName + ".arg" + std::to_string(argumentIndex++);
                        output += csec_emit_i32_expression(tokens, argumentStart, cursor, argumentName);
                        if (!arguments.empty()) arguments += ", ";
                        arguments += "i32 " + argumentName;
                    }
                    argumentStart = cursor + 1;
                }
            }
            return output + "  " + resultName + " = call i32 @" + std::string(csec_token_text_at(tokens, start)) +
                "_" + csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(") &&
        csec_is_function_parameter(tokens, start, csec_token_text_at(tokens, start))) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (close == end - 1) {
            std::string output;
            std::string arguments;
            int argumentStart = start + 2;
            int argumentIndex = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string argumentName = resultName + ".arg" + std::to_string(argumentIndex++);
                        output += csec_emit_i32_expression(tokens, argumentStart, cursor, argumentName);
                        if (!arguments.empty()) arguments += ", ";
                        arguments += "i32 " + argumentName;
                    }
                    argumentStart = cursor + 1;
                }
            }
            const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
            const std::string closure = resultName + ".closure";
            const std::string codeField = resultName + ".code.field";
            const std::string environmentField = resultName + ".environment.field";
            const std::string functionValue = resultName + ".fn";
            const std::string environment = resultName + ".environment";
            return output + "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n" +
                "  " + codeField + " = getelementptr inbounds { ptr, ptr }, ptr " + closure + ", i32 0, i32 0\n" +
                "  " + functionValue + " = load ptr, ptr " + codeField + "\n" +
                "  " + environmentField + " = getelementptr inbounds { ptr, ptr }, ptr " + closure + ", i32 0, i32 1\n" +
                "  " + environment + " = load ptr, ptr " + environmentField + "\n" +
                "  " + resultName + " = call i32 " + functionValue + "(ptr " + environment + (arguments.empty() ? "" : ", " + arguments) + ")\n";
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        if (declaration >= 0 && csec_lambda_declaration(tokens, declaration, declarationEnd)) {
            const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
            if (close == end - 1) {
                std::string output;
                std::string arguments;
                const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                const std::string closure = resultName + ".closure";
                const std::string codeField = resultName + ".code.field";
                const std::string environmentField = resultName + ".environment.field";
                const std::string code = resultName + ".code";
                const std::string environment = resultName + ".environment";
                const std::string closureType = csec_lambda_closure_type_name(declaration);
                output += "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n";
                output += "  " + codeField + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 0\n";
                output += "  " + code + " = load ptr, ptr " + codeField + "\n";
                output += "  " + environmentField + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 1\n";
                output += "  " + environment + " = load ptr, ptr " + environmentField + "\n";
                arguments = "ptr " + environment;
                int argumentStart = start + 2;
                int argumentIndex = 0;
                for (int cursor = argumentStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > argumentStart) {
                            const std::string argumentName = resultName + ".arg" + std::to_string(argumentIndex++);
                            output += csec_emit_i32_expression(tokens, argumentStart, cursor, argumentName);
                            if (!arguments.empty()) arguments += ", ";
                            arguments += "i32 " + argumentName;
                        }
                        argumentStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call i32 " + code + "(" + arguments + ")\n";
            }
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        const auto parameters = callee >= 0 ? csec_function_params(tokens, callee) : std::vector<std::pair<std::string, std::string>>{};
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        bool hasFunctionParameter = false;
        for (const auto& parameter : parameters) if (parameter.second == "(") hasFunctionParameter = true;
        if (close == end - 1 && hasFunctionParameter) {
            std::string output;
            std::string arguments;
            int argumentStart = start + 2;
            int argumentIndex = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        if (!arguments.empty()) arguments += ", ";
                        const bool functionArgument = argumentIndex < static_cast<int>(parameters.size()) && parameters[argumentIndex].second == "(";
                        if (functionArgument && csec_token_kind_at(tokens, argumentStart) == 'I' && argumentStart + 1 == cursor) {
                            const int lambdaDeclaration = csec_find_visible_local(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                            const int lambdaEnd = lambdaDeclaration >= 0 ? csec_advance_statement(tokens, lambdaDeclaration, 1000000000) : -1;
                            if (lambdaDeclaration >= 0 && csec_lambda_declaration(tokens, lambdaDeclaration, lambdaEnd)) {
                                const std::string closure = resultName + ".closure.arg" + std::to_string(argumentIndex);
                                const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                                output += "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n";
                                arguments += "ptr " + closure;
                            } else arguments += "ptr null";
                        } else {
                            const std::string value = resultName + ".arg" + std::to_string(argumentIndex);
                            output += csec_emit_i32_expression(tokens, argumentStart, cursor, value);
                            arguments += "i32 " + value;
                        }
                        ++argumentIndex;
                    }
                    argumentStart = cursor + 1;
                }
            }
            if (std::strcmp(csec_function_return_type_at(tokens, callee), "Boolean") == 0) {
                const std::string booleanResult = resultName + ".i1";
                return output + "  " + booleanResult + " = call i1 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n" +
                    "  " + resultName + " = zext i1 " + booleanResult + " to i32\n";
            }
            return output + "  " + resultName + " = call i32 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (close > start + 1) {
            const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
            const auto parameters = callee >= 0 ? csec_function_params(tokens, callee) : std::vector<std::pair<std::string, std::string>>{};
            // 64-bit parameters (Long, also of `external def` callees) must receive i64 arguments; this
            // is the only use of the external declaration here, so other argument lowering is unchanged.
            const int wideDeclaration = callee >= 0 ? callee : csec_top_level_external_function_declaration(tokens, csec_token_text_at(tokens, start));
            const auto wideParameters = wideDeclaration >= 0 ? csec_function_params(tokens, wideDeclaration) : std::vector<std::pair<std::string, std::string>>{};
            std::string output;
            std::string arguments;
            int argumentStart = start + 2;
            int argumentIndex = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string argumentName = resultName + ".arg" + std::to_string(argumentIndex);
                        const bool pointerArgument = argumentIndex < static_cast<int>(parameters.size()) &&
                            std::strcmp(csec_llvm_type_for_name(parameters[argumentIndex].second), "ptr") == 0;
                        if (!arguments.empty()) arguments += ", ";
                        if (pointerArgument && (parameters[argumentIndex].second == "Dict" || parameters[argumentIndex].second == "Set")) {
                            // A `Dict<K, V>` / `Set<T>` parameter takes the handle.
                            output += csec_emit_ptr_expression(tokens, argumentStart, cursor, argumentName);
                            arguments += "ptr " + argumentName;
                        } else if (pointerArgument && csec_token_kind_at(tokens, argumentStart) == 'I' && argumentStart + 1 == cursor &&
                            csec_visible_i32_array(tokens, argumentStart, csec_token_text_at(tokens, argumentStart))) {
                            const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                            output += "  " + argumentName + " = load ptr, ptr %" + std::string(storage) + "\n";
                            arguments += "ptr " + argumentName;
                        } else if (pointerArgument &&
                            (csec_token_kind_at(tokens, argumentStart) == 'S' ||
                             (csec_token_kind_at(tokens, argumentStart) == 'I' && argumentStart + 1 == cursor &&
                              (csec_is_string_local(tokens, argumentStart, csec_token_text_at(tokens, argumentStart)) ||
                               csec_is_string_parameter(tokens, argumentStart, csec_token_text_at(tokens, argumentStart)) ||
                               csec_is_string_class_constructor_param(tokens, argumentStart, csec_token_text_at(tokens, argumentStart)))))) {
                            output += csec_emit_ptr_expression(tokens, argumentStart, cursor, argumentName);
                            arguments += "ptr " + argumentName;
                        } else if (argumentIndex < static_cast<int>(wideParameters.size()) &&
                                   std::strcmp(csec_llvm_type_for_name(wideParameters[argumentIndex].second), "i64") == 0) {
                            output += csec_emit_i64_expression(tokens, argumentStart, cursor, argumentName);
                            arguments += "i64 " + argumentName;
                        } else {
                            const bool booleanArgument = argumentIndex < static_cast<int>(parameters.size()) && parameters[argumentIndex].second == "Boolean";
                            output += booleanArgument
                                ? csec_emit_i1_expression(tokens, argumentStart, cursor, argumentName)
                                : csec_emit_i32_expression(tokens, argumentStart, cursor, argumentName);
                            arguments += std::string(booleanArgument ? "i1 " : "i32 ") + argumentName;
                        }
                        ++argumentIndex;
                    }
                    argumentStart = cursor + 1;
                }
            }
            if (callee >= 0 && std::strcmp(csec_function_return_type_at(tokens, callee), "Boolean") == 0) {
                const std::string booleanResult = resultName + ".i1";
                output += "  " + booleanResult + " = call i1 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
                output += "  " + resultName + " = zext i1 " + booleanResult + " to i32\n";
            } else output += "  " + resultName + " = call i32 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
            return output;
        }
    }
    if (kind == 'I') {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        if (csec_is_boolean_local(tokens, start, csec_token_text_at(tokens, start)) ||
            csec_is_boolean_class_constructor_param(tokens, start, csec_token_text_at(tokens, start))) {
            const std::string booleanValue = resultName + ".i1";
            return "  " + booleanValue + " = load i1, ptr %" + std::string(storage) + "\n" +
                "  " + resultName + " = zext i1 " + booleanValue + " to i32\n";
        }
        return "  " + resultName + " = load i32, ptr %" + std::string(storage) + "\n";
    }
    return "  " + resultName + " = add i32 0, 0\n";
}

static std::string csec_emit_f32_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = fadd float 0.000000e+00, 0.000000e+00\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "float", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_native_token_is(tokens, start, 'O', "(") &&
        csec_find_closing_token(tokens, start, end, "(", ")") == end - 1) {
        return csec_emit_f32_expression(tokens, start + 1, end - 1, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "+") && start + 1 < end) {
        return csec_emit_f32_expression(tokens, start + 1, end, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "-") && start + 1 < end) {
        const std::string value = resultName + ".value";
        return csec_emit_f32_expression(tokens, start + 1, end, value) +
            "  " + resultName + " = fsub float 0.000000e+00, " + value + "\n";
    }
    const int operation = csec_i32_top_level_operator(tokens, start, end);
    if (operation >= start) {
        const char* op = csec_token_text_at(tokens, operation);
        const char* instruction = std::strcmp(op, "+") == 0 ? "fadd" :
            (std::strcmp(op, "-") == 0 ? "fsub" : (std::strcmp(op, "*") == 0 ? "fmul" :
            (std::strcmp(op, "/") == 0 ? "fdiv" : nullptr)));
        if (instruction) {
            const std::string left = resultName + ".left";
            const std::string right = resultName + ".right";
            return csec_emit_f32_expression(tokens, start, operation, left) +
                csec_emit_f32_expression(tokens, operation + 1, end, right) +
                "  " + resultName + " = " + instruction + " float " + left + ", " + right + "\n";
        }
    }
    const char kind = csec_token_kind_at(tokens, start);
    const std::string indexedArray = csec_emit_f32_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (kind == 'F' || kind == 'N') {
        const std::string literal = csec_llvm_float_literal(csec_token_text_at(tokens, start));
        return "  " + resultName + " = fadd float 0.000000e+00, " + literal + "\n";
    }
    if (kind == 'I' && start + 1 == end) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        return "  " + resultName + " = load float, ptr %" + std::string(storage) + "\n";
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        if (close == end - 1 && callee >= 0 && std::strcmp(csec_function_return_type_at(tokens, callee), "Float") == 0) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments;
            int itemStart = start + 2, index = 0;
            for (int cursor = itemStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > itemStart) {
                        const std::string value = resultName + ".arg" + std::to_string(index);
                        const bool floating = index < static_cast<int>(parameters.size()) && parameters[index].second == "Float";
                        const bool pointer = index < static_cast<int>(parameters.size()) &&
                            (parameters[index].second == "Vector" || parameters[index].second == "Array");
                        output += pointer ? csec_emit_ptr_expression(tokens, itemStart, cursor, value)
                            : (floating ? csec_emit_f32_expression(tokens, itemStart, cursor, value)
                                        : csec_emit_i32_expression(tokens, itemStart, cursor, value));
                        if (!arguments.empty()) arguments += ", ";
                        arguments += std::string(pointer ? "ptr " : (floating ? "float " : "i32 ")) + value;
                        ++index;
                    }
                    itemStart = cursor + 1;
                }
            }
            return output + "  " + resultName + " = call float @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    if (kind == 'I' && start + 3 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' && csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (close == end -1 && initializer >= declaration && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
            csec_token_kind_at(tokens, initializer + 2) == 'I') {
            const std::string className = csec_token_text_at(tokens, initializer + 2);
            const int classStart = csec_find_class_declaration(tokens, className.c_str());
            const int method = classStart >= 0 ? csec_find_class_method_declaration(tokens, classStart, csec_token_text_at(tokens, start + 2)) : -1;
            if (method >= 0 && std::strcmp(csec_function_return_type_at(tokens, method), "Float") == 0) {
                const auto parameters = csec_function_params(tokens, method);
                std::string output, arguments;
                if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string receiver = resultName + ".this";
                    output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                    arguments = "ptr " + receiver;
                }
                int itemStart = start + 4, index = 0;
                for (int cursor = itemStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > itemStart) {
                            const std::string value = resultName + ".arg" + std::to_string(index);
                            const bool floating = index < static_cast<int>(parameters.size()) && parameters[index].second == "Float";
                            output += floating ? csec_emit_f32_expression(tokens, itemStart, cursor, value)
                                               : csec_emit_i32_expression(tokens, itemStart, cursor, value);
                            if (!arguments.empty()) arguments += ", ";
                            arguments += std::string(floating ? "float " : "i32 ") + value;
                            ++index;
                        }
                        itemStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call float @" + className + "_" + csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
            }
        }
    }
    return "  " + resultName + " = fadd float 0.000000e+00, 0.000000e+00\n";
}

static std::string csec_emit_f64_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = fadd double 0.000000e+00, 0.000000e+00\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "double", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_is_await_identity(tokens, start, end)) return csec_emit_f64_expression(tokens, start + 1, end, resultName);
    if (csec_is_await_expression(tokens, start, end)) {
        return csec_emit_await_payload(tokens, start, end, resultName + ".raw") +
            "  " + resultName + " = bitcast i64 " + resultName + ".raw to double\n";
    }
    {
        const std::string external = csec_emit_external_call(tokens, start, end, resultName, "double");
        if (!external.empty()) return external;
    }
    if (csec_native_token_is(tokens, start, 'O', "(") && csec_find_closing_token(tokens, start, end, "(", ")") == end - 1)
        return csec_emit_f64_expression(tokens, start + 1, end - 1, resultName);
    if (csec_native_token_is(tokens, start, 'O', "+") && start + 1 < end)
        return csec_emit_f64_expression(tokens, start + 1, end, resultName);
    if (csec_native_token_is(tokens, start, 'O', "-") && start + 1 < end) {
        const std::string value = resultName + ".value";
        return csec_emit_f64_expression(tokens, start + 1, end, value) + "  " + resultName + " = fsub double 0.000000e+00, " + value + "\n";
    }
    const int operation = csec_i32_top_level_operator(tokens, start, end);
    if (operation >= start) {
        const char* op = csec_token_text_at(tokens, operation);
        const char* instruction = std::strcmp(op, "+") == 0 ? "fadd" : (std::strcmp(op, "-") == 0 ? "fsub" :
            (std::strcmp(op, "*") == 0 ? "fmul" : (std::strcmp(op, "/") == 0 ? "fdiv" : nullptr)));
        if (instruction) {
            const std::string left = resultName + ".left";
            const std::string right = resultName + ".right";
            return csec_emit_f64_expression(tokens, start, operation, left) +
                csec_emit_f64_expression(tokens, operation + 1, end, right) +
                "  " + resultName + " = " + instruction + " double " + left + ", " + right + "\n";
        }
    }
    const char kind = csec_token_kind_at(tokens, start);
    const std::string indexedArray = csec_emit_f64_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (kind == 'F' || kind == 'N') return "  " + resultName + " = fadd double 0.000000e+00, " + csec_llvm_double_literal(csec_token_text_at(tokens, start)) + "\n";
    if (kind == 'I' && start + 1 == end) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        return "  " + resultName + " = load double, ptr %" + std::string(storage) + "\n";
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        if (close == end - 1 && callee >= 0 && std::strcmp(csec_function_return_type_at(tokens, callee), "Double") == 0) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments;
            int itemStart = start + 2, index = 0;
            for (int cursor = itemStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > itemStart) {
                        const std::string value = resultName + ".arg" + std::to_string(index);
                        const bool floating = index < static_cast<int>(parameters.size()) && parameters[index].second == "Double";
                        const bool pointer = index < static_cast<int>(parameters.size()) &&
                            (parameters[index].second == "Vector" || parameters[index].second == "Array");
                        output += pointer ? csec_emit_ptr_expression(tokens, itemStart, cursor, value)
                            : (floating ? csec_emit_f64_expression(tokens, itemStart, cursor, value)
                                        : csec_emit_i32_expression(tokens, itemStart, cursor, value));
                        if (!arguments.empty()) arguments += ", ";
                        arguments += std::string(pointer ? "ptr " : (floating ? "double " : "i32 ")) + value;
                        ++index;
                    }
                    itemStart = cursor + 1;
                }
            }
            return output + "  " + resultName + " = call double @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    if (kind == 'I' && start + 3 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' && csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (close == end - 1 && initializer >= declaration && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
            csec_token_kind_at(tokens, initializer + 2) == 'I') {
            const std::string className = csec_token_text_at(tokens, initializer + 2);
            const int classStart = csec_find_class_declaration(tokens, className.c_str());
            const int method = classStart >= 0 ? csec_find_class_method_declaration(tokens, classStart, csec_token_text_at(tokens, start + 2)) : -1;
            if (method >= 0 && std::strcmp(csec_function_return_type_at(tokens, method), "Double") == 0) {
                const auto parameters = csec_function_params(tokens, method);
                std::string output, arguments;
                if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string receiver = resultName + ".this";
                    output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                    arguments = "ptr " + receiver;
                }
                int itemStart = start + 4, index = 0;
                for (int cursor = itemStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > itemStart) {
                        const std::string value = resultName + ".arg" + std::to_string(index);
                        const bool doubleArgument = index < static_cast<int>(parameters.size()) && parameters[index].second == "Double";
                        const bool pointer = index < static_cast<int>(parameters.size()) &&
                            (parameters[index].second == "Vector" || parameters[index].second == "Array");
                        output += pointer ? csec_emit_ptr_expression(tokens, itemStart, cursor, value)
                            : (doubleArgument ? csec_emit_f64_expression(tokens, itemStart, cursor, value)
                                              : csec_emit_i32_expression(tokens, itemStart, cursor, value));
                        if (!arguments.empty()) arguments += ", ";
                        arguments += std::string(pointer ? "ptr " : (doubleArgument ? "double " : "i32 ")) + value;
                            ++index;
                        }
                        itemStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call double @" + className + "_" + csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
            }
        }
    }
    return "  " + resultName + " = fadd double 0.000000e+00, 0.000000e+00\n";
}

static std::string csec_emit_i64_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = add i64 0, 0\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "i64", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_is_await_identity(tokens, start, end)) return csec_emit_i64_expression(tokens, start + 1, end, resultName);
    if (csec_is_await_expression(tokens, start, end)) {
        return csec_emit_await_payload(tokens, start, end, resultName + ".raw") +
            "  " + resultName + " = add i64 " + resultName + ".raw, 0\n";
    }
    if (csec_is_intrinsic_call(tokens, start, end, "threadStart")) {
        const auto args = csec_call_argument_ranges(tokens, start + 1, end - 1);
        if (args.size() == 1 && csec_token_kind_at(tokens, args[0].first) == 'I' && args[0].first + 1 == args[0].second) {
            return "  " + resultName + " = call i64 @csec_thread_start(ptr @" +
                csec_task_callee_symbol(csec_token_text_at(tokens, args[0].first)) + ", ptr null, i64 0)\n";
        }
        return "  " + resultName + " = add i64 0, 0\n";
    }
    {
        const std::string external = csec_emit_external_call(tokens, start, end, resultName, "i64");
        if (!external.empty()) return external;
    }
    const std::string indexedArray = csec_emit_i64_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (csec_native_token_is(tokens, start, 'O', "(") && csec_find_closing_token(tokens, start, end, "(", ")") == end - 1)
        return csec_emit_i64_expression(tokens, start + 1, end - 1, resultName);
    const int operation = csec_i32_top_level_operator(tokens, start, end);
    if (operation >= start) {
        const char* op = csec_token_text_at(tokens, operation);
        const char* instruction = std::strcmp(op, "+") == 0 ? "add" : std::strcmp(op, "-") == 0 ? "sub" :
            std::strcmp(op, "*") == 0 ? "mul" : std::strcmp(op, "/") == 0 ? "sdiv" : std::strcmp(op, "%") == 0 ? "srem" : nullptr;
        if (instruction) {
            const std::string left = resultName + ".left", right = resultName + ".right";
            return csec_emit_i64_expression(tokens, start, operation, left) + csec_emit_i64_expression(tokens, operation + 1, end, right) +
                "  " + resultName + " = " + instruction + " i64 " + left + ", " + right + "\n";
        }
    }
    if (csec_token_kind_at(tokens, start) == 'N') return "  " + resultName + " = add i64 0, " + std::string(csec_token_text_at(tokens, start)) + "\n";
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 == end) {
        const std::string name = csec_token_text_at(tokens, start);
        const int declaration = csec_find_visible_local(tokens, start, name.c_str());
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (declaration >= 0 && csec_is_i64_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer)))
            return "  " + resultName + " = load i64, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
        const int function = csec_enclosing_function_declaration(tokens, start);
        if (function >= 0) for (const auto& parameter : csec_function_params(tokens, function)) if (parameter.first == name && csec_is_i64_type_name(parameter.second))
            return "  " + resultName + " = load i64, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
        // A narrower Int/Boolean local or parameter read in a 64-bit context: load it from its slot and
        // widen it (previously this referenced the bare name, which is not an SSA value).
        std::string narrowType;
        if (declaration >= 0) narrowType = csec_c_local_type(tokens, declaration, declarationEnd, initializer);
        else if (function >= 0) for (const auto& parameter : csec_function_params(tokens, function)) if (parameter.first == name) narrowType = parameter.second;
        if (narrowType == "Int" || narrowType == "Boolean") {
            const char* storage = csec_lookup_visible_storage_name(tokens, start, name.c_str());
            const bool boolean = narrowType == "Boolean";
            return "  " + resultName + ".narrow = load " + std::string(boolean ? "i1" : "i32") + ", ptr %" + storage + "\n" +
                "  " + resultName + " = " + std::string(boolean ? "zext i1 " : "sext i32 ") + resultName + ".narrow to i64\n";
        }
        return "  " + resultName + " = load i64, ptr %" + name + "\n";
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (callee >= 0 && close == end - 1 && csec_is_i64_type_name(csec_function_return_type_at(tokens, callee))) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments; int itemStart = start + 2; int index = 0;
            for (int cursor = itemStart; cursor <= close; ++cursor) if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                if (cursor > itemStart) {
                    const std::string value = resultName + ".arg" + std::to_string(itemStart);
                    const bool pointer = index < static_cast<int>(parameters.size()) &&
                        (parameters[index].second == "Vector" || parameters[index].second == "Array");
                    if (pointer) {
                        const char* storage = csec_lookup_visible_storage_name(tokens, itemStart, csec_token_text_at(tokens, itemStart));
                        output += "  " + value + " = load ptr, ptr %" + std::string(storage) + "\n";
                        arguments += (arguments.empty() ? "" : ", ") + std::string("ptr ") + value;
                    } else {
                        output += csec_emit_i64_expression(tokens, itemStart, cursor, value);
                        arguments += (arguments.empty() ? "" : ", ") + std::string("i64 ") + value;
                    }
                    ++index;
                }
                itemStart = cursor + 1;
            }
            return output + "  " + resultName + " = call i64 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 3 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' && csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (close == end - 1 && initializer >= declaration && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
            csec_token_kind_at(tokens, initializer + 2) == 'I') {
            const std::string className = csec_token_text_at(tokens, initializer + 2);
            const int classStart = csec_find_class_declaration(tokens, className.c_str());
            const int method = classStart >= 0 ? csec_find_class_method_declaration(tokens, classStart, csec_token_text_at(tokens, start + 2)) : -1;
            if (method >= 0 && csec_is_i64_type_name(csec_function_return_type_at(tokens, method))) {
                const auto parameters = csec_function_params(tokens, method);
                std::string output, arguments;
                if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string receiver = resultName + ".this";
                    output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                    arguments = "ptr " + receiver;
                }
                int itemStart = start + 4, index = 0;
                for (int cursor = itemStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > itemStart) {
                            const std::string value = resultName + ".arg" + std::to_string(index);
                            const bool wide = index < static_cast<int>(parameters.size()) && csec_is_i64_type_name(parameters[index].second);
                            output += wide ? csec_emit_i64_expression(tokens, itemStart, cursor, value)
                                           : csec_emit_i32_expression(tokens, itemStart, cursor, value);
                            if (!arguments.empty()) arguments += ", ";
                            arguments += std::string(wide ? "i64 " : "i32 ") + value;
                            ++index;
                        }
                        itemStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call i64 @" + className + "_" + csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
            }
        }
    }
    return "  " + resultName + " = add i64 0, 0\n";
}

static std::string csec_emit_i8_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = add i8 0, 0\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "i8", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_native_token_is(tokens, start, 'O', "(") &&
        csec_find_closing_token(tokens, start, end, "(", ")") == end - 1) {
        return csec_emit_i8_expression(tokens, start + 1, end - 1, resultName);
    }
    if (csec_token_kind_at(tokens, start) == 'N') {
        return "  " + resultName + " = trunc i32 " + std::string(csec_token_text_at(tokens, start)) + " to i8\n";
    }
    if (csec_token_kind_at(tokens, start) == 'C') {
        const std::string literal = csec_token_text_at(tokens, start);
        int value = 0;
        if (literal.size() == 1) value = static_cast<unsigned char>(literal[0]);
        else if (literal.size() == 2 && literal[0] == '\\') {
            // The lexer keeps the text between the quotes, so '\n' arrives as backslash + 'n'.
            const char escaped = literal[1];
            value = escaped == 'n' ? '\n' : escaped == 'r' ? '\r' : escaped == 't' ? '\t' : escaped == '0' ? 0 : static_cast<unsigned char>(escaped);
        }
        else if (literal.size() >= 3 && literal[0] == '\'' && literal[1] != '\\') value = static_cast<unsigned char>(literal[1]);
        else if (literal.size() >= 4 && literal[0] == '\'' && literal[1] == '\\') {
            const char escaped = literal[2];
            value = escaped == 'n' ? '\n' : escaped == 'r' ? '\r' : escaped == 't' ? '\t' : escaped == '0' ? 0 : static_cast<unsigned char>(escaped);
        }
        return "  " + resultName + " = add i8 0, " + std::to_string(value) + "\n";
    }
    {
        const std::string stringByte = csec_emit_string_index(tokens, start, end, resultName);
        if (!stringByte.empty()) return stringByte;
    }
    const std::string indexedArray = csec_emit_i8_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 == end) {
        const std::string name = csec_token_text_at(tokens, start);
        const int declaration = csec_find_visible_local(tokens, start, name.c_str());
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (declaration >= 0 && csec_is_i8_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer))) {
            return "  " + resultName + " = load i8, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
        }
        const int function = csec_enclosing_function_declaration(tokens, start);
        if (function >= 0) {
            for (const auto& parameter : csec_function_params(tokens, function)) {
                if (parameter.first == name && csec_is_i8_type_name(parameter.second)) {
                    return "  " + resultName + " = load i8, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
                }
            }
        }
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (callee >= 0 && close == end - 1 && csec_is_i8_type_name(csec_function_return_type_at(tokens, callee))) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments;
            int argumentStart = start + 2;
            int index = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string value = resultName + ".arg" + std::to_string(index);
                        const bool pointer = index < static_cast<int>(parameters.size()) &&
                            (parameters[index].second == "Vector" || parameters[index].second == "Array");
                        if (pointer) {
                            const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                            output += "  " + value + " = load ptr, ptr %" + std::string(storage) + "\n";
                            arguments += (arguments.empty() ? "" : ", ") + std::string("ptr ") + value;
                        } else {
                            output += csec_emit_i8_expression(tokens, argumentStart, cursor, value);
                            arguments += (arguments.empty() ? "" : ", ") + std::string("i8 ") + value;
                        }
                        ++index;
                    }
                    argumentStart = cursor + 1;
                }
            }
            return output + "  " + resultName + " = call i8 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    const std::string wide = resultName + ".i32";
    return csec_emit_i32_expression(tokens, start, end, wide) +
        "  " + resultName + " = trunc i32 " + wide + " to i8\n";
}

static std::string csec_emit_i16_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = add i16 0, 0\n";
    const std::string indexedArray = csec_emit_i16_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (csec_native_token_is(tokens, start, 'O', "(") && csec_find_closing_token(tokens, start, end, "(", ")") == end - 1)
        return csec_emit_i16_expression(tokens, start + 1, end - 1, resultName);
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 == end) {
        const std::string name = csec_token_text_at(tokens, start);
        const int declaration = csec_find_visible_local(tokens, start, name.c_str());
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (declaration >= 0 && csec_is_i16_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer)))
            return "  " + resultName + " = load i16, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
        const int function = csec_enclosing_function_declaration(tokens, start);
        if (function >= 0) for (const auto& parameter : csec_function_params(tokens, function)) if (parameter.first == name && csec_is_i16_type_name(parameter.second))
            return "  " + resultName + " = load i16, ptr %" + csec_lookup_visible_storage_name(tokens, start, name.c_str()) + "\n";
    }
    if (csec_token_kind_at(tokens, start) == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (callee >= 0 && close == end - 1 && csec_is_i16_type_name(csec_function_return_type_at(tokens, callee))) {
            const auto parameters = csec_function_params(tokens, callee);
            std::string output, arguments;
            int argumentStart = start + 2;
            int index = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                if (cursor > argumentStart) {
                    const std::string value = resultName + ".arg" + std::to_string(index);
                    const bool pointer = index < static_cast<int>(parameters.size()) &&
                        (parameters[index].second == "Vector" || parameters[index].second == "Array");
                    if (pointer) {
                        const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                        output += "  " + value + " = load ptr, ptr %" + std::string(storage) + "\n";
                        arguments += (arguments.empty() ? "" : ", ") + std::string("ptr ") + value;
                    } else {
                        output += csec_emit_i16_expression(tokens, argumentStart, cursor, value);
                        arguments += (arguments.empty() ? "" : ", ") + std::string("i16 ") + value;
                    }
                    ++index;
                }
                argumentStart = cursor + 1;
            }
            return output + "  " + resultName + " = call i16 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
        }
    }
    const std::string wide = resultName + ".i32";
    return csec_emit_i32_expression(tokens, start, end, wide) + "  " + resultName + " = trunc i32 " + wide + " to i16\n";
}

static std::string csec_emit_i1_expression(const char* tokens, int start, int end, const std::string& resultName) {
    end = csec_i32_expression_end(tokens, start, end);
    if (end <= start) return "  " + resultName + " = icmp eq i32 0, 1\n";
    {
        const std::string dictValue = csec_emit_dict_expression(tokens, start, end, "i1", resultName);
        if (!dictValue.empty()) return dictValue;
    }
    if (csec_is_await_identity(tokens, start, end)) return csec_emit_i1_expression(tokens, start + 1, end, resultName);
    if (csec_is_await_expression(tokens, start, end)) {
        return csec_emit_await_payload(tokens, start, end, resultName + ".raw") +
            "  " + resultName + " = icmp ne i64 " + resultName + ".raw, 0\n";
    }
    if (csec_is_intrinsic_call(tokens, start, end, "isAlive")) {
        return csec_emit_i32_intrinsic(tokens, start, end, resultName + ".i32") +
            "  " + resultName + " = icmp ne i32 " + resultName + ".i32, 0\n";
    }
    {
        const std::string external = csec_emit_external_call(tokens, start, end, resultName, "i1");
        if (!external.empty()) return external;
    }
    if (csec_native_token_is(tokens, start, 'O', "(") &&
        csec_find_closing_token(tokens, start, end, "(", ")") == end - 1) {
        return csec_emit_i1_expression(tokens, start + 1, end - 1, resultName);
    }
    if (csec_native_token_is(tokens, start, 'O', "!") && start + 1 < end) {
        const std::string value = resultName + ".value";
        return csec_emit_i1_expression(tokens, start + 1, end, value) +
            "  " + resultName + " = xor i1 " + value + ", true\n";
    }
    if (csec_token_kind_at(tokens, start) == 'B') {
        return "  " + resultName + " = icmp eq i32 0, " +
            std::string(std::strcmp(csec_token_text_at(tokens, start), "true") == 0 ? "0\n" : "1\n");
    }
    const int floatComparison = csec_i32_top_level_operator(tokens, start, end);
    if (floatComparison >= start) {
        const char* op = csec_token_text_at(tokens, floatComparison);
        if ((std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0 || std::strcmp(op, "<") == 0 ||
             std::strcmp(op, "<=") == 0 || std::strcmp(op, ">") == 0 || std::strcmp(op, ">=") == 0) &&
            csec_token_kind_at(tokens, start) == 'I') {
            const std::string name = csec_token_text_at(tokens, start);
            const int declaration = csec_find_visible_local(tokens, start, name.c_str());
            const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
            const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
            if (declaration >= 0 && csec_is_i16_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer))) {
                const std::string left = resultName + ".left", right = resultName + ".right";
                const char* predicate = std::strcmp(op, "==") == 0 ? "eq" : std::strcmp(op, "!=") == 0 ? "ne" : std::strcmp(op, "<") == 0 ? "slt" : std::strcmp(op, "<=") == 0 ? "sle" : std::strcmp(op, ">") == 0 ? "sgt" : "sge";
                return csec_emit_i16_expression(tokens, start, floatComparison, left) + csec_emit_i16_expression(tokens, floatComparison + 1, end, right) +
                    "  " + resultName + " = icmp " + predicate + " i16 " + left + ", " + right + "\n";
            }
            if (declaration >= 0 && csec_is_i8_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer))) {
                const std::string left = resultName + ".left", right = resultName + ".right";
                const char* predicate = std::strcmp(op, "==") == 0 ? "eq" : std::strcmp(op, "!=") == 0 ? "ne" :
                    std::strcmp(op, "<") == 0 ? "slt" : std::strcmp(op, "<=") == 0 ? "sle" : std::strcmp(op, ">") == 0 ? "sgt" : "sge";
                return csec_emit_i8_expression(tokens, start, floatComparison, left) +
                    csec_emit_i8_expression(tokens, floatComparison + 1, end, right) +
                    "  " + resultName + " = icmp " + predicate + " i8 " + left + ", " + right + "\n";
            }
            if (declaration >= 0 && csec_is_i64_type_name(csec_c_local_type(tokens, declaration, declarationEnd, initializer))) {
                const std::string left = resultName + ".left", right = resultName + ".right";
                const char* predicate = std::strcmp(op, "==") == 0 ? "eq" : std::strcmp(op, "!=") == 0 ? "ne" :
                    std::strcmp(op, "<") == 0 ? "slt" : std::strcmp(op, "<=") == 0 ? "sle" : std::strcmp(op, ">") == 0 ? "sgt" : "sge";
                return csec_emit_i64_expression(tokens, start, floatComparison, left) +
                    csec_emit_i64_expression(tokens, floatComparison + 1, end, right) +
                    "  " + resultName + " = icmp " + predicate + " i64 " + left + ", " + right + "\n";
            }
        }
        if (std::strcmp(op, "and") == 0 || std::strcmp(op, "&&") == 0 ||
            std::strcmp(op, "or") == 0 || std::strcmp(op, "||") == 0) {
            const std::string left = resultName + ".left";
            const std::string right = resultName + ".right";
            const char* instruction = (std::strcmp(op, "and") == 0 || std::strcmp(op, "&&") == 0) ? "and" : "or";
            return csec_emit_i1_expression(tokens, start, floatComparison, left) +
                csec_emit_i1_expression(tokens, floatComparison + 1, end, right) +
                "  " + resultName + " = " + instruction + " i1 " + left + ", " + right + "\n";
        }
        if ((std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0 || std::strcmp(op, "<") == 0 ||
             std::strcmp(op, "<=") == 0 || std::strcmp(op, ">") == 0 || std::strcmp(op, ">=") == 0) &&
            (csec_expression_is_double(tokens, start, floatComparison) || csec_expression_is_double(tokens, floatComparison + 1, end))) {
            const std::string left = resultName + ".left";
            const std::string right = resultName + ".right";
            const char* predicate = std::strcmp(op, "==") == 0 ? "oeq" : (std::strcmp(op, "!=") == 0 ? "one" :
                (std::strcmp(op, "<") == 0 ? "olt" : (std::strcmp(op, "<=") == 0 ? "ole" : (std::strcmp(op, ">") == 0 ? "ogt" : "oge"))));
            return csec_emit_f64_expression(tokens, start, floatComparison, left) +
                csec_emit_f64_expression(tokens, floatComparison + 1, end, right) +
                "  " + resultName + " = fcmp " + predicate + " double " + left + ", " + right + "\n";
        }
        if ((std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0 || std::strcmp(op, "<") == 0 ||
             std::strcmp(op, "<=") == 0 || std::strcmp(op, ">") == 0 || std::strcmp(op, ">=") == 0) &&
            (csec_expression_is_float(tokens, start, floatComparison) ||
             csec_expression_is_float(tokens, floatComparison + 1, end))) {
            const std::string left = resultName + ".left";
            const std::string right = resultName + ".right";
            const char* predicate = std::strcmp(op, "==") == 0 ? "oeq" :
                (std::strcmp(op, "!=") == 0 ? "one" : (std::strcmp(op, "<") == 0 ? "olt" :
                (std::strcmp(op, "<=") == 0 ? "ole" : (std::strcmp(op, ">") == 0 ? "ogt" : "oge"))));
            return csec_emit_f32_expression(tokens, start, floatComparison, left) +
                csec_emit_f32_expression(tokens, floatComparison + 1, end, right) +
                "  " + resultName + " = fcmp " + predicate + " float " + left + ", " + right + "\n";
        }
    }
    const char kind = csec_token_kind_at(tokens, start);
    if (csec_native_token_is(tokens, start, 'K', "super") && start + 4 < end &&
        csec_native_token_is(tokens, start + 1, 'O', ".") && csec_token_kind_at(tokens, start + 2) == 'I' &&
        csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int classStart = csec_enclosing_class_declaration(tokens, start);
        const std::string parent = classStart >= 0 ? csec_class_parent_name(tokens, classStart) : "";
        const int parentStart = parent.empty() ? -1 : csec_find_class_declaration(tokens, parent.c_str());
        if (close == end - 1 && parentStart >= 0 && csec_class_uses_pointer_receiver(tokens, parentStart)) {
            return "  " + resultName + " = call i1 @" + parent + "_" +
                csec_token_text_at(tokens, start + 2) + "(ptr %arg.this)\n";
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(") &&
        csec_is_function_parameter(tokens, start, csec_token_text_at(tokens, start)) &&
        csec_function_parameter_returns_boolean(tokens, start, csec_token_text_at(tokens, start))) {
        const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
        if (close == end - 1) {
            std::string output, arguments;
            int argumentStart = start + 2, index = 0;
            for (int cursor = argumentStart; cursor <= close; ++cursor) {
                if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                    if (cursor > argumentStart) {
                        const std::string value = resultName + ".arg" + std::to_string(index++);
                        output += csec_emit_i32_expression(tokens, argumentStart, cursor, value);
                        if (!arguments.empty()) arguments += ", ";
                        arguments += "i32 " + value;
                    }
                    argumentStart = cursor + 1;
                }
            }
            const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
            const std::string closure = resultName + ".closure";
            const std::string codeField = resultName + ".code.field";
            const std::string environmentField = resultName + ".environment.field";
            const std::string functionValue = resultName + ".fn";
            const std::string environment = resultName + ".environment";
            return output + "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n" +
                "  " + codeField + " = getelementptr inbounds { ptr, ptr }, ptr " + closure + ", i32 0, i32 0\n" +
                "  " + functionValue + " = load ptr, ptr " + codeField + "\n" +
                "  " + environmentField + " = getelementptr inbounds { ptr, ptr }, ptr " + closure + ", i32 0, i32 1\n" +
                "  " + environment + " = load ptr, ptr " + environmentField + "\n" +
                "  " + resultName + " = call i1 " + functionValue + "(ptr " + environment + (arguments.empty() ? "" : ", " + arguments) + ")\n";
        }
    }
    if (kind == 'I' && start + 1 < end && csec_native_token_is(tokens, start + 1, 'O', "(")) {
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        int lambdaReturn = -1;
        if (declaration >= 0 && csec_lambda_declaration(tokens, declaration, declarationEnd)) {
            int bodyStart = -1, bodyEnd = -1;
            csec_lambda_declaration(tokens, declaration, declarationEnd, &bodyStart, &bodyEnd);
            for (int cursor = bodyStart; cursor < bodyEnd;) {
                const int next = csec_advance_statement(tokens, cursor, bodyEnd);
                if (csec_native_token_is(tokens, cursor, 'K', "return")) { lambdaReturn = cursor + 1; break; }
                if (next <= cursor) break;
                cursor = next;
            }
            if (lambdaReturn >= 0 && csec_expression_is_boolean(tokens, lambdaReturn, bodyEnd)) {
                const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
                if (close == end - 1) {
                    std::string output, arguments;
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string closure = resultName + ".closure";
                    const std::string codeField = resultName + ".code.field";
                    const std::string environmentField = resultName + ".environment.field";
                    const std::string code = resultName + ".code";
                    const std::string environment = resultName + ".environment";
                    const std::string closureType = csec_lambda_closure_type_name(declaration);
                    output += "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n";
                    output += "  " + codeField + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 0\n";
                    output += "  " + code + " = load ptr, ptr " + codeField + "\n";
                    output += "  " + environmentField + " = getelementptr inbounds " + closureType + ", ptr " + closure + ", i32 0, i32 1\n";
                    output += "  " + environment + " = load ptr, ptr " + environmentField + "\n";
                    arguments = "ptr " + environment;
                    int argumentStart = start + 2, index = 0;
                    for (int cursor = argumentStart; cursor <= close; ++cursor) {
                        if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                            if (cursor > argumentStart) {
                                const std::string value = resultName + ".arg" + std::to_string(index++);
                                output += csec_emit_i32_expression(tokens, argumentStart, cursor, value);
                                if (!arguments.empty()) arguments += ", ";
                                arguments += "i32 " + value;
                            }
                            argumentStart = cursor + 1;
                        }
                    }
                    return output + "  " + resultName + " = call i1 " + code + "(" + arguments + ")\n";
                }
            }
        }
        const int callee = csec_top_level_function_declaration(tokens, csec_token_text_at(tokens, start));
        if (callee >= 0 && std::strcmp(csec_function_return_type_at(tokens, callee), "Boolean") == 0) {
            const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
            if (close == end - 1) {
                const auto parameters = csec_function_params(tokens, callee);
                std::string output, arguments;
                int argumentStart = start + 2, index = 0;
                for (int cursor = argumentStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > argumentStart) {
                            if (!arguments.empty()) arguments += ", ";
                            const bool functionArgument = index < static_cast<int>(parameters.size()) && parameters[index].second == "(";
                            if (functionArgument && csec_token_kind_at(tokens, argumentStart) == 'I' && argumentStart + 1 == cursor) {
                                const int lambdaDeclaration = csec_find_visible_local(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                                const int lambdaEnd = lambdaDeclaration >= 0 ? csec_advance_statement(tokens, lambdaDeclaration, 1000000000) : -1;
                                if (lambdaDeclaration >= 0 && csec_lambda_declaration(tokens, lambdaDeclaration, lambdaEnd)) {
                                    const std::string closure = resultName + ".closure.arg" + std::to_string(index);
                                    const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                                    output += "  " + closure + " = load ptr, ptr %" + std::string(storage) + "\n";
                                    arguments += "ptr " + closure;
                                } else arguments += "ptr null";
                            } else {
                                const std::string value = resultName + ".arg" + std::to_string(index);
                                const bool booleanArgument = index < static_cast<int>(parameters.size()) && parameters[index].second == "Boolean";
                                const bool arrayArgument = index < static_cast<int>(parameters.size()) &&
                                    csec_token_kind_at(tokens, argumentStart) == 'I' && argumentStart + 1 == cursor &&
                                    (csec_visible_i32_array(tokens, argumentStart, csec_token_text_at(tokens, argumentStart)) ||
                                     csec_visible_i1_array(tokens, argumentStart, csec_token_text_at(tokens, argumentStart)));
                                const bool pointerArgument = index < static_cast<int>(parameters.size()) &&
                                    std::strcmp(csec_llvm_type_for_name(parameters[index].second), "ptr") == 0;
                                if (arrayArgument) {
                                    const char* storage = csec_lookup_visible_storage_name(tokens, argumentStart, csec_token_text_at(tokens, argumentStart));
                                    output += "  " + value + " = load ptr, ptr %" + std::string(storage) + "\n";
                                    arguments += "ptr " + value;
                                } else if (pointerArgument) {
                                    output += csec_emit_ptr_expression(tokens, argumentStart, cursor, value);
                                    arguments += "ptr " + value;
                                } else {
                                    output += booleanArgument
                                        ? csec_emit_i1_expression(tokens, argumentStart, cursor, value)
                                        : csec_emit_i32_expression(tokens, argumentStart, cursor, value);
                                    arguments += std::string(booleanArgument ? "i1 " : "i32 ") + value;
                                }
                            }
                            ++index;
                        }
                        argumentStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call i1 @" + std::string(csec_token_text_at(tokens, start)) + "(" + arguments + ")\n";
            }
        }
    }
    if (kind == 'I' && start + 3 < end && csec_native_token_is(tokens, start + 1, 'O', ".") &&
        csec_token_kind_at(tokens, start + 2) == 'I' && csec_native_token_is(tokens, start + 3, 'O', "(")) {
        const int close = csec_find_closing_token(tokens, start + 3, end, "(", ")");
        const int declaration = csec_find_visible_local(tokens, start, csec_token_text_at(tokens, start));
        const int declarationEnd = declaration >= 0 ? csec_advance_statement(tokens, declaration, 1000000000) : -1;
        const int initializer = declaration >= 0 ? csec_find_top_level_operator(tokens, declaration, declarationEnd, 1) : -1;
        if (close == end - 1 && initializer >= declaration && csec_native_token_is(tokens, initializer + 1, 'K', "new") &&
            csec_token_kind_at(tokens, initializer + 2) == 'I') {
            const std::string className = csec_token_text_at(tokens, initializer + 2);
            const int classStart = csec_find_class_declaration(tokens, className.c_str());
            const int method = classStart >= 0 ? csec_find_class_method_declaration(tokens, classStart, csec_token_text_at(tokens, start + 2)) : -1;
            if (method >= 0 && std::strcmp(csec_function_return_type_at(tokens, method), "Boolean") == 0) {
                const auto parameters = csec_function_params(tokens, method);
                std::string output, arguments;
                if (csec_class_uses_pointer_receiver(tokens, classStart)) {
                    const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
                    const std::string receiver = resultName + ".this";
                    output += "  " + receiver + " = load ptr, ptr %" + std::string(storage) + "\n";
                    arguments = "ptr " + receiver;
                }
                int itemStart = start + 4, index = 0;
                for (int cursor = itemStart; cursor <= close; ++cursor) {
                    if (cursor == close || csec_is_argument_separator(tokens, close, cursor)) {
                        if (cursor > itemStart) {
                            const std::string value = resultName + ".arg" + std::to_string(index);
                            const bool floating = index < static_cast<int>(parameters.size()) && parameters[index].second == "Float";
                            output += floating ? csec_emit_f32_expression(tokens, itemStart, cursor, value)
                                               : csec_emit_i32_expression(tokens, itemStart, cursor, value);
                            if (!arguments.empty()) arguments += ", ";
                            arguments += std::string(floating ? "float " : "i32 ") + value;
                            ++index;
                        }
                        itemStart = cursor + 1;
                    }
                }
                return output + "  " + resultName + " = call i1 @" + className + "_" + csec_token_text_at(tokens, start + 2) + "(" + arguments + ")\n";
            }
        }
    }
    const std::string indexedArray = csec_emit_i1_array_index(tokens, start, end, resultName);
    if (!indexedArray.empty()) return indexedArray;
    if (kind == 'I' && start + 1 == end &&
        (csec_is_boolean_local(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_boolean_parameter(tokens, start, csec_token_text_at(tokens, start)) ||
         csec_is_boolean_class_constructor_param(tokens, start, csec_token_text_at(tokens, start)))) {
        const char* storage = csec_lookup_visible_storage_name(tokens, start, csec_token_text_at(tokens, start));
        return "  " + resultName + " = load i1, ptr %" + std::string(storage) + "\n";
    }
    const std::string value = resultName + ".i32";
    return csec_emit_i32_expression(tokens, start, end, value) +
        "  " + resultName + " = icmp ne i32 " + value + ", 0\n";
}

static std::string csec_emit_println_statement(const char* tokens, int start, int rawEnd) {
    const int end = csec_i32_expression_end(tokens, start, rawEnd);
    if (!csec_native_token_is(tokens, start, 'I', "println") || !csec_native_token_is(tokens, start + 1, 'O', "(")) return "";
    const int close = csec_find_closing_token(tokens, start + 1, end, "(", ")");
    if (close != end - 1) return "";
    const std::string seed = std::to_string(start);
    if (close == start + 2) return "  call void @csec_print_newline()\n";
    if (csec_is_string_expression(tokens, start + 2, close)) {
        const std::string value = "%println." + seed + ".string";
        return csec_emit_ptr_expression(tokens, start + 2, close, value) +
            "  call void @csec_print_string(ptr " + value + ")\n" +
            "  call void @csec_print_newline()\n";
    }
    const std::string value = "%println." + seed + ".value";
    const std::string wide = "%println." + seed + ".wide";
    return csec_emit_i32_expression(tokens, start + 2, close, value) +
        "  " + wide + " = sext i32 " + value + " to i64\n" +
        "  call void @csec_print_i64(i64 " + wide + ")\n" +
        "  call void @csec_print_newline()\n";
}

// Defined in llvm_module.inc, which owns the shared statement lowering.
static std::string csec_generate_llvm_flat_body_i1(const char* tokens, int bodyStart, int bodyEnd);
