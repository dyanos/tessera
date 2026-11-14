char* csec_llvm_lexer_helper_definition(const char* name) {
    if (name) {
        if (FILE* trace = std::fopen("selfhost\\last_llvm_helper.txt", "w")) {
            std::fputs(name, trace);
            std::fclose(trace);
        }
    }
    const char* definition = "";
    char tokenKind = '\0';
    if (name && std::strcmp(name, "kindIdentifier") == 0) tokenKind = 'I';
    else if (name && std::strcmp(name, "kindKeyword") == 0) tokenKind = 'K';
    else if (name && std::strcmp(name, "kindInteger") == 0) tokenKind = 'N';
    else if (name && std::strcmp(name, "kindFloat") == 0) tokenKind = 'F';
    else if (name && std::strcmp(name, "kindString") == 0) tokenKind = 'S';
    else if (name && std::strcmp(name, "kindRegex") == 0) tokenKind = 'R';
    else if (name && std::strcmp(name, "kindChar") == 0) tokenKind = 'C';
    else if (name && std::strcmp(name, "kindBool") == 0) tokenKind = 'B';
    else if (name && std::strcmp(name, "kindOperator") == 0) tokenKind = 'O';
    else if (name && std::strcmp(name, "kindComment") == 0) tokenKind = 'M';
    else if (name && std::strcmp(name, "kindEof") == 0) tokenKind = 'E';
    if (tokenKind != '\0') {
        std::string generated = "define i8 @" + std::string(name) + "() {\nentry:\n  ret i8 " + std::to_string(static_cast<unsigned char>(tokenKind)) + "\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    }
    if (name && std::strcmp(name, "tokenIs") == 0) {
        definition = "define i1 @tokenIs(ptr %arg.tokens, i32 %arg.ordinal, i8 %arg.kind, ptr %arg.text) {\nentry:\n  %token.is = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 %arg.kind, ptr %arg.text)\n  %token.is.bool = icmp ne i32 %token.is, 0\n  ret i1 %token.is.bool\n}\n\n";
    } else if (name && std::strcmp(name, "repairLeadingRTokenText") == 0) {
        definition = "@.str.repair.r.eturn = private unnamed_addr constant [6 x i8] c\"eturn\\00\"\n@.str.repair.r.return = private unnamed_addr constant [7 x i8] c\"return\\00\"\n@.str.repair.r.atio = private unnamed_addr constant [5 x i8] c\"atio\\00\"\n@.str.repair.r.ratio = private unnamed_addr constant [6 x i8] c\"ratio\\00\"\n@.str.repair.r.educe = private unnamed_addr constant [6 x i8] c\"educe\\00\"\n@.str.repair.r.reduce = private unnamed_addr constant [7 x i8] c\"reduce\\00\"\n@.str.repair.r.ange = private unnamed_addr constant [5 x i8] c\"ange\\00\"\n@.str.repair.r.range = private unnamed_addr constant [6 x i8] c\"range\\00\"\n@.str.repair.r.egex = private unnamed_addr constant [5 x i8] c\"egex\\00\"\n@.str.repair.r.regex = private unnamed_addr constant [6 x i8] c\"regex\\00\"\n@.str.repair.r.Regex = private unnamed_addr constant [6 x i8] c\"Regex\\00\"\n\ndefine ptr @repairLeadingRTokenText(ptr %arg.text) {\nentry:\n  %case0 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.eturn)\n  %is0 = icmp ne i32 %case0, 0\n  br i1 %is0, label %return0, label %check1\nreturn0:\n  ret ptr @.str.repair.r.return\ncheck1:\n  %case1 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.atio)\n  %is1 = icmp ne i32 %case1, 0\n  br i1 %is1, label %return1, label %check2\nreturn1:\n  ret ptr @.str.repair.r.ratio\ncheck2:\n  %case2 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.educe)\n  %is2 = icmp ne i32 %case2, 0\n  br i1 %is2, label %return2, label %check3\nreturn2:\n  ret ptr @.str.repair.r.reduce\ncheck3:\n  %case3 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.ange)\n  %is3 = icmp ne i32 %case3, 0\n  br i1 %is3, label %return3, label %check4\nreturn3:\n  ret ptr @.str.repair.r.range\ncheck4:\n  %case4 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.egex)\n  %is4 = icmp ne i32 %case4, 0\n  br i1 %is4, label %return4, label %check5\nreturn4:\n  ret ptr @.str.repair.r.regex\ncheck5:\n  %case5 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.Regex)\n  %is5 = icmp ne i32 %case5, 0\n  br i1 %is5, label %return5, label %fallback\nreturn5:\n  ret ptr @.str.repair.r.Regex\nfallback:\n  ret ptr %arg.text\n}\n\n";
    } else if (name && std::strcmp(name, "isWhitespace") == 0) {
        definition = "define i1 @isWhitespace(i8 %arg.ch) {\nentry:\n  %space = icmp eq i8 %arg.ch, 32\n  %newline = icmp eq i8 %arg.ch, 10\n  %carriage = icmp eq i8 %arg.ch, 13\n  %tab = icmp eq i8 %arg.ch, 9\n  %space.or.newline = or i1 %space, %newline\n  %carriage.or.tab = or i1 %carriage, %tab\n  %ret = or i1 %space.or.newline, %carriage.or.tab\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "isDigit") == 0) {
        definition = "define i1 @isDigit(i8 %arg.ch) {\nentry:\n  %lower = icmp sge i8 %arg.ch, 48\n  %upper = icmp sle i8 %arg.ch, 57\n  %ret = and i1 %lower, %upper\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "strEq") == 0) {
        definition = "define i1 @strEq(ptr %arg.left, ptr %arg.right) {\nentry:\n  %left.length = call i64 @csec_string_length(ptr %arg.left)\n  %right.length = call i64 @csec_string_length(ptr %arg.right)\n  %same.length = icmp eq i64 %left.length, %right.length\n  %prefix = call i32 @csec_string_starts_with(ptr %arg.left, ptr %arg.right)\n  %same.text = icmp ne i32 %prefix, 0\n  %ret = and i1 %same.length, %same.text\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "isKeyword") == 0) {
        definition = "define i1 @isKeyword(ptr %arg.text) {\nentry:\n  %keyword = call i32 @csec_is_keyword(ptr %arg.text)\n  %ret = icmp ne i32 %keyword, 0\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "appendToken") == 0) {
        definition = "define ptr @appendToken(ptr %arg.tokens, i8 %arg.kind, ptr %arg.text) {\nentry:\n  %ret = call ptr @csec_token_append_owned(ptr %arg.tokens, i8 %arg.kind, ptr %arg.text)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "appendTokenTo") == 0) {
        definition = "define i32 @appendTokenTo(i64 %arg.builder, i8 %arg.kind, ptr %arg.text) {\nentry:\n  %ret = call i32 @csec_token_builder_append(i64 %arg.builder, i8 %arg.kind, ptr %arg.text)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "twoChars") == 0) {
        definition = "define i1 @twoChars(ptr %arg.source, i32 %arg.index, ptr %arg.text) {\nentry:\n  %length = call i64 @csec_string_length(ptr %arg.source)\n  %last = add i32 %arg.index, 1\n  %last64 = sext i32 %last to i64\n  %in.range = icmp slt i64 %last64, %length\n  br i1 %in.range, label %compare, label %false\ncompare:\n  %source0 = call i8 @csec_string_char_at(ptr %arg.source, i32 %arg.index)\n  %text0 = call i8 @csec_string_char_at(ptr %arg.text, i32 0)\n  %first = icmp eq i8 %source0, %text0\n  %source1 = call i8 @csec_string_char_at(ptr %arg.source, i32 %last)\n  %text1 = call i8 @csec_string_char_at(ptr %arg.text, i32 1)\n  %second = icmp eq i8 %source1, %text1\n  %ret = and i1 %first, %second\n  ret i1 %ret\nfalse:\n  ret i1 false\n}\n\n";
    } else if (name && std::strcmp(name, "threeChars") == 0) {
        definition = "define i1 @threeChars(ptr %arg.source, i32 %arg.index, ptr %arg.text) {\nentry:\n  %length = call i64 @csec_string_length(ptr %arg.source)\n  %last = add i32 %arg.index, 2\n  %last64 = sext i32 %last to i64\n  %in.range = icmp slt i64 %last64, %length\n  br i1 %in.range, label %compare, label %false\ncompare:\n  %source0 = call i8 @csec_string_char_at(ptr %arg.source, i32 %arg.index)\n  %text0 = call i8 @csec_string_char_at(ptr %arg.text, i32 0)\n  %first = icmp eq i8 %source0, %text0\n  %index1 = add i32 %arg.index, 1\n  %source1 = call i8 @csec_string_char_at(ptr %arg.source, i32 %index1)\n  %text1 = call i8 @csec_string_char_at(ptr %arg.text, i32 1)\n  %second = icmp eq i8 %source1, %text1\n  %source2 = call i8 @csec_string_char_at(ptr %arg.source, i32 %last)\n  %text2 = call i8 @csec_string_char_at(ptr %arg.text, i32 2)\n  %third = icmp eq i8 %source2, %text2\n  %first.two = and i1 %first, %second\n  %ret = and i1 %first.two, %third\n  ret i1 %ret\nfalse:\n  ret i1 false\n}\n\n";
    } else if (name && (std::strcmp(name, "operatorLength") == 0 || std::strcmp(name, "lexIdentifier") == 0 || std::strcmp(name, "classMemberKind") == 0 || std::strcmp(name, "classMemberName") == 0 || std::strcmp(name, "parseReturnIntegerInRange") == 0 || std::strcmp(name, "cTypeName") == 0 || std::strcmp(name, "llvmStringLiteralByteLength") == 0 || std::strcmp(name, "generateLLVMStringLiteralGlobals") == 0 || std::strcmp(name, "generateLLVMFunctionDefinition") == 0 || std::strcmp(name, "generateLLVMFlatBodyI32") == 0 || std::strcmp(name, "generateLLVMMainBodyFromRange") == 0 || std::strcmp(name, "generateLLVMBooleanBodyFromRange") == 0 || std::strcmp(name, "generateLLVMCharBodyFromRange") == 0 || std::strcmp(name, "generateLLVMDoubleBodyFromRange") == 0 || std::strcmp(name, "generateLLVMLongBodyFromRange") == 0 || std::strcmp(name, "generateLLVMPointerBodyFromRange") == 0)) {
        definition = std::strcmp(name, "generateLLVMBooleanBodyFromRange") == 0 || std::strcmp(name, "generateLLVMCharBodyFromRange") == 0 || std::strcmp(name, "generateLLVMDoubleBodyFromRange") == 0 || std::strcmp(name, "generateLLVMLongBodyFromRange") == 0 || std::strcmp(name, "generateLLVMPointerBodyFromRange") == 0
            ? (std::strcmp(name, "generateLLVMBooleanBodyFromRange") == 0
                ? "declare ptr @csec_llvm_body_fallback(i32)\n\ndefine ptr @generateLLVMBooleanBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_body_fallback(i32 1)\n  ret ptr %ret\n}\n\n"
                : std::strcmp(name, "generateLLVMCharBodyFromRange") == 0
                ? "define ptr @generateLLVMCharBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_body_fallback(i32 2)\n  ret ptr %ret\n}\n\n"
                : std::strcmp(name, "generateLLVMDoubleBodyFromRange") == 0
                ? "define ptr @generateLLVMDoubleBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_body_fallback(i32 3)\n  ret ptr %ret\n}\n\n"
                : std::strcmp(name, "generateLLVMLongBodyFromRange") == 0
                ? "define ptr @generateLLVMLongBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_body_fallback(i32 4)\n  ret ptr %ret\n}\n\n"
                : "define ptr @generateLLVMPointerBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_body_fallback(i32 5)\n  ret ptr %ret\n}\n\n")
            : std::strcmp(name, "generateLLVMMainBodyFromRange") == 0
            ? "declare ptr @csec_llvm_main_body_fallback()\n\ndefine ptr @generateLLVMMainBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_llvm_main_body_fallback()\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "generateLLVMFlatBodyI32") == 0
            ? "declare ptr @csec_generate_llvm_flat_body_i32(ptr, i32, i32)\n\ndefine ptr @generateLLVMFlatBodyI32(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_generate_llvm_flat_body_i32(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "generateLLVMStringLiteralGlobals") == 0
            ? "declare ptr @csec_generate_llvm_string_literal_globals(ptr)\n\ndefine ptr @generateLLVMStringLiteralGlobals(ptr %arg.tokens) {\nentry:\n  %ret = call ptr @csec_generate_llvm_string_literal_globals(ptr %arg.tokens)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "generateLLVMFunctionDefinition") == 0
            ? "declare ptr @csec_generate_llvm_function_definition(ptr, i32)\n\ndefine ptr @generateLLVMFunctionDefinition(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call ptr @csec_generate_llvm_function_definition(ptr %arg.tokens, i32 %arg.declStart)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "llvmStringLiteralByteLength") == 0
            ? "define i32 @llvmStringLiteralByteLength(ptr %arg.text) {\nentry:\n  %ret = call i32 @csec_llvm_string_literal_byte_length(ptr %arg.text)\n  ret i32 %ret\n}\n\n"
            : std::strcmp(name, "cTypeName") == 0
            ? "declare ptr @csec_c_type_name(ptr)\n\ndefine ptr @cTypeName(ptr %arg.typeName) {\nentry:\n  %ret = call ptr @csec_c_type_name(ptr %arg.typeName)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "parseReturnIntegerInRange") == 0
            ? "declare i32 @csec_parse_return_integer_in_range(ptr, i32, i32)\n\ndefine i32 @parseReturnIntegerInRange(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_parse_return_integer_in_range(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n"
            : std::strcmp(name, "classMemberKind") == 0
            ? "declare ptr @csec_class_member_kind(ptr, i32)\n\ndefine ptr @classMemberKind(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_class_member_kind(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "classMemberName") == 0
            ? "declare ptr @csec_class_member_name(ptr, i32)\n\ndefine ptr @classMemberName(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_class_member_name(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "operatorLength") == 0
            ? "declare i32 @csec_operator_length(ptr, i32)\n\ndefine i32 @operatorLength(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_operator_length(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n"
            : "define i32 @lexIdentifier(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_lex_identifier(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lexNumber") == 0) {
        definition = "define i32 @lexNumber(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_lex_number(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lexQuoted") == 0) {
        definition = "define i32 @lexQuoted(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_lex_quoted(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lexLineComment") == 0) {
        definition = "define i32 @lexLineComment(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_lex_line_comment(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lexBlockComment") == 0) {
        definition = "define i32 @lexBlockComment(ptr %arg.source, i32 %arg.index) {\nentry:\n  %ret = call i32 @csec_lex_block_comment(ptr %arg.source, i32 %arg.index)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "slice") == 0) {
        definition = "define ptr @slice(ptr %arg.source, i32 %arg.start, i32 %arg.end) {\nentry:\n  %valid = icmp sgt i32 %arg.end, %arg.start\n  br i1 %valid, label %slice, label %empty\nslice:\n  %length = sub i32 %arg.end, %arg.start\n  %ret = call ptr @csec_string_substring(ptr %arg.source, i32 %arg.start, i32 %length)\n  ret ptr %ret\nempty:\n  ret ptr null\n}\n\n";
    } else if (name && (std::strcmp(name, "tokenize") == 0 || std::strcmp(name, "tokenizeSlow") == 0)) {
        const char* functionName = std::strcmp(name, "tokenize") == 0 ? "tokenize" : "tokenizeSlow";
        std::string generated = "define ptr @" + std::string(functionName) + "(ptr %arg.source) {\nentry:\n  %ret = call ptr @csec_tokenize_source(ptr %arg.source)\n  ret ptr %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "countCommaSeparated") == 0) {
        definition = "define i32 @countCommaSeparated(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_count_comma_separated(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findLastTopLevelToken") == 0) {
        definition = "define i32 @findLastTopLevelToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text) {\nentry:\n  %ret = call i32 @csec_find_last_top_level_token(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text)\n  ret i32 %ret\n}\n\n";
    } else if (name && (std::strcmp(name, "advanceStatement") == 0 || std::strcmp(name, "advanceStatementSlow") == 0)) {
        const char* functionName = std::strcmp(name, "advanceStatement") == 0 ? "advanceStatement" : "advanceStatementSlow";
        std::string generated = "define i32 @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd) {\nentry:\n  %ret = call i32 @csec_advance_statement(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd)\n  ret i32 %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "countMatchCases") == 0) {
        definition = "@.str.count.match.case = private unnamed_addr constant [5 x i8] c\"case\\00\"\n\ndefine i32 @countMatchCases(ptr %arg.tokens, i32 %arg.matchOrdinal, i32 %arg.end) {\nentry:\n  %open = call i32 @findTokenTextInRange(ptr %arg.tokens, i32 %arg.matchOrdinal, i32 %arg.end, ptr @.str.open.brace)\n  %has.open = icmp sge i32 %open, 0\n  br i1 %has.open, label %find.close, label %empty\nfind.close:\n  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open, i32 %arg.end, ptr @.str.open.brace, ptr @.str.close.brace)\n  %first = add i32 %open, 1\n  br label %loop\nloop:\n  %cursor = phi i32 [ %first, %find.close ], [ %next, %body ]\n  %count = phi i32 [ 0, %find.close ], [ %updated, %body ]\n  %before.close = icmp slt i32 %cursor, %close\n  br i1 %before.close, label %inspect, label %done\ninspect:\n  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %cursor)\n  %is.eof = icmp eq i8 %kind, 69\n  br i1 %is.eof, label %done, label %body\nbody:\n  %case.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %cursor, i8 75, ptr @.str.count.match.case)\n  %is.case = icmp ne i32 %case.raw, 0\n  %increment = zext i1 %is.case to i32\n  %updated = add i32 %count, %increment\n  %next = add i32 %cursor, 1\n  br label %loop\ndone:\n  ret i32 %count\nempty:\n  ret i32 0\n}\n\n@.str.open.brace = private unnamed_addr constant [2 x i8] c\"{\\00\"\n@.str.close.brace = private unnamed_addr constant [2 x i8] c\"}\\00\"\n\n";
    } else if (name && std::strcmp(name, "digitValue") == 0) {
        definition = "define i32 @digitValue(i8 %arg.ch) {\nentry:\n  %ret = call i32 @csec_digit_value(i8 %arg.ch)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "toInt") == 0) {
        definition = "define i32 @toInt(ptr %arg.text) {\nentry:\n  %ret = call i32 @csec_to_int(ptr %arg.text)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lineStart") == 0) {
        definition = "define i32 @lineStart(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call i32 @csec_line_start(ptr %arg.tokens, i32 %arg.ordinal)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lineEnd") == 0) {
        definition = "define i32 @lineEnd(ptr %arg.tokens, i32 %arg.start) {\nentry:\n  %ret = call i32 @csec_line_end(ptr %arg.tokens, i32 %arg.start)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "tokenKindAt") == 0) {
        definition = "define i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %arg.ordinal)\n  ret i8 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "tokenTextAt") == 0) {
        definition = "define ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "skipTrivia") == 0) {
        definition = "define i32 @skipTrivia(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  br label %loop\nloop:\n  %cursor = phi i32 [ %arg.ordinal, %entry ], [ %next, %body ]\n  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %cursor)\n  %is.comment = icmp eq i8 %kind, 77\n  br i1 %is.comment, label %body, label %exit\nbody:\n  %next = add i32 %cursor, 1\n  br label %loop\nexit:\n  ret i32 %cursor\n}\n\n";
    } else if (name && std::strcmp(name, "validateBalanced") == 0) {
        definition = "define i1 @validateBalanced(ptr %arg.tokens) {\nentry:\n  %valid = call i32 @csec_validate_balanced(ptr %arg.tokens)\n  %ret = icmp ne i32 %valid, 0\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "startsTopLevelDeclaration") == 0) {
        definition = "define i1 @startsTopLevelDeclaration(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %valid = call i32 @csec_starts_top_level_declaration(ptr %arg.tokens, i32 %arg.ordinal)\n  %ret = icmp ne i32 %valid, 0\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "validateTopLevel") == 0) {
        definition = "define i1 @validateTopLevel(ptr %arg.tokens) {\nentry:\n  %valid = call i32 @csec_validate_top_level(ptr %arg.tokens)\n  %ret = icmp ne i32 %valid, 0\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "parseProgram") == 0) {
        definition = "define i1 @parseProgram(ptr %arg.tokens) {\nentry:\n  %balanced = call i1 @validateBalanced(ptr %arg.tokens)\n  br i1 %balanced, label %validate, label %invalid\nvalidate:\n  %ret = call i1 @validateTopLevel(ptr %arg.tokens)\n  ret i1 %ret\ninvalid:\n  ret i1 false\n}\n\n";
    } else if (name && std::strcmp(name, "topLevelDeclKind") == 0) {
        definition = "define ptr @topLevelDeclKind(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_top_level_decl_kind(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "topLevelDeclName") == 0) {
        definition = "define ptr @topLevelDeclName(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_top_level_decl_name(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n";
    } else if (name && (std::strcmp(name, "functionReturnType") == 0 || std::strcmp(name, "functionReturnTypeSlow") == 0)) {
        const char* functionName = std::strcmp(name, "functionReturnType") == 0 ? "functionReturnType" : "functionReturnTypeSlow";
        std::string generated = "define ptr @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call ptr @csec_function_return_type_at(ptr %arg.tokens, i32 %arg.declStart)\n  ret ptr %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "countFunctionParams") == 0) {
        definition = "define i32 @countFunctionParams(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call i32 @csec_count_function_params(ptr %arg.tokens, i32 %arg.declStart)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "tokenIsTopLevelOperator") == 0) {
        definition = "define i1 @tokenIsTopLevelOperator(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.group) {\nentry:\n  %valid = call i32 @csec_token_is_top_level_operator(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.group)\n  %ret = icmp ne i32 %valid, 0\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findTopLevelOperator") == 0) {
        definition = "define i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 %arg.group) {\nentry:\n  %ret = call i32 @csec_find_top_level_operator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 %arg.group)\n  ret i32 %ret\n}\n\n";
    } else if (name && (std::strcmp(name, "generateLLVMParamList") == 0 || std::strcmp(name, "generateLLVMParamListSlow") == 0)) {
        const char* functionName = std::strcmp(name, "generateLLVMParamList") == 0 ? "generateLLVMParamList" : "generateLLVMParamListSlow";
        std::string generated = "define ptr @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call ptr @csec_function_llvm_param_list(ptr %arg.tokens, i32 %arg.declStart)\n  ret ptr %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && (std::strcmp(name, "generateLLVMParamAllocas") == 0 || std::strcmp(name, "generateLLVMParamAllocasSlow") == 0)) {
        const char* functionName = std::strcmp(name, "generateLLVMParamAllocas") == 0 ? "generateLLVMParamAllocas" : "generateLLVMParamAllocasSlow";
        std::string generated = "define ptr @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call ptr @csec_function_llvm_param_allocas(ptr %arg.tokens, i32 %arg.declStart)\n  ret ptr %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && (std::strcmp(name, "generateLLVMModule") == 0 || std::strcmp(name, "generateLLVMModuleInto") == 0)) {
        definition = std::strcmp(name, "generateLLVMModuleInto") == 0
            ? "declare i32 @csec_generate_llvm_module_into(ptr, i64)\n\ndefine i32 @generateLLVMModuleInto(ptr %arg.tokens, i64 %arg.builder) {\nentry:\n  %ret = call i32 @csec_generate_llvm_module_into(ptr %arg.tokens, i64 %arg.builder)\n  ret i32 %ret\n}\n\n"
            : "define ptr @generateLLVMModule(ptr %arg.tokens) {\nentry:\n  %valid = call i1 @parseProgram(ptr %arg.tokens)\n  br i1 %valid, label %build, label %empty\nbuild:\n  %builder = call i64 @csec_string_builder_new()\n  %written = call i32 @generateLLVMModuleInto(ptr %arg.tokens, i64 %builder)\n  %result = call ptr @csec_string_builder_finish(i64 %builder)\n  ret ptr %result\nempty:\n  ret ptr null\n}\n\n";
    } else if (name && std::strcmp(name, "compileFile") == 0) {
        definition = "define i32 @compileFile(ptr %arg.inputPath, ptr %arg.outputPath, ptr %arg.mode) {\nentry:\n  %llvm.raw = call i32 @csec_string_starts_with(ptr %arg.mode, ptr @.str.driver.llvm)\n  %llvm = icmp ne i32 %llvm.raw, 0\n  br i1 %llvm, label %build, label %unsupported\nbuild:\n  %source = call ptr @csec_expand_imports(ptr %arg.inputPath)\n  %builder = call i64 @csec_string_builder_new_file(ptr %arg.outputPath)\n  %ready = icmp ne i64 %builder, 0\n  br i1 %ready, label %generate, label %failed\ngenerate:\n  %tokens = call ptr @tokenize(ptr %source)\n  %generated = call i32 @generateLLVMModuleInto(ptr %tokens, i64 %builder)\n  %success = icmp eq i32 %generated, 0\n  br i1 %success, label %write, label %return.generated\nwrite:\n  %written = call i32 @csec_string_builder_write_to_file(i64 %builder, ptr %arg.outputPath)\n  ret i32 %written\nreturn.generated:\n  ret i32 %generated\nfailed:\n  ret i32 1\nunsupported:\n  ret i32 1\n}\n\n@.str.driver.llvm = private unnamed_addr constant [5 x i8] c\"llvm\\00\"\n\n";
    } else if (name && (std::strcmp(name, "replaceDotsWithSlash") == 0 || std::strcmp(name, "importTargetFromLine") == 0 || std::strcmp(name, "importCandidate") == 0 || std::strcmp(name, "resolveImportPath") == 0 || std::strcmp(name, "expandImportsFromFile") == 0)) {
        const bool resolve = std::strcmp(name, "resolveImportPath") == 0;
        const bool expand = std::strcmp(name, "expandImportsFromFile") == 0;
        const char* argumentName = resolve ? "currentPath, ptr %arg.target" : (expand ? "path, ptr %arg.seen" : (std::strcmp(name, "replaceDotsWithSlash") == 0 ? "text" : (std::strcmp(name, "importTargetFromLine") == 0 ? "line" : "target")));
        const char* nativeName = resolve ? "csec_resolve_import_path" : (expand ? "csec_expand_imports" : (std::strcmp(name, "replaceDotsWithSlash") == 0 ? "csec_replace_dots_with_slash" : (std::strcmp(name, "importTargetFromLine") == 0 ? "csec_import_target_from_line" : "csec_import_candidate")));
        const char* nativeSignature = resolve ? "(ptr, ptr)" : "(ptr)";
        const char* callArguments = resolve ? "ptr %arg.currentPath, ptr %arg.target" : (expand ? "ptr %arg.path" : (std::strcmp(name, "replaceDotsWithSlash") == 0 ? "ptr %arg.text" : (std::strcmp(name, "importTargetFromLine") == 0 ? "ptr %arg.line" : "ptr %arg.target")));
        const std::string declaration = expand ? "" : "declare ptr @" + std::string(nativeName) + nativeSignature + "\n\n";
        std::string generated = declaration + "define ptr @" + std::string(name) + "(ptr %arg." + argumentName + ") {\nentry:\n  %ret = call ptr @" + nativeName + "(" + callArguments + ")\n  ret ptr %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "findTopLevelMatch") == 0) {
        definition = "define i32 @findTopLevelMatch(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_find_top_level_match(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findLambdaArrow") == 0) {
        definition = "define i32 @findLambdaArrow(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  ret i32 -1\n}\n\n";
    } else if (name && std::strcmp(name, "findLambdaArrowLegacy") == 0) {
        definition = "@.str.lambda.arrow.open.capture = private unnamed_addr constant [2 x i8] c\"[\\00\"\n@.str.lambda.arrow.close.capture = private unnamed_addr constant [2 x i8] c\"]\\00\"\n@.str.lambda.arrow.open.params = private unnamed_addr constant [2 x i8] c\"(\\00\"\n@.str.lambda.arrow.close.params = private unnamed_addr constant [2 x i8] c\")\\00\"\n@.str.lambda.arrow.symbol = private unnamed_addr constant [3 x i8] c\"->\\00\"\n\ndefine i32 @findLambdaArrow(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %close.capture = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.lambda.arrow.open.capture, ptr @.str.lambda.arrow.close.capture)\n  %has.capture = icmp sge i32 %close.capture, 0\n  br i1 %has.capture, label %params.start, label %none\nparams.start:\n  %after.capture = add i32 %close.capture, 1\n  %open.params = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.capture)\n  %operator.kind = call i8 @kindOperator()\n  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.params, i8 %operator.kind, ptr @.str.lambda.arrow.open.params)\n  %open = icmp ne i32 %open.raw, 0\n  br i1 %open, label %params.close, label %none\nparams.close:\n  %close.params = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.params, i32 %arg.end, ptr @.str.lambda.arrow.open.params, ptr @.str.lambda.arrow.close.params)\n  %has.params = icmp sge i32 %close.params, 0\n  br i1 %has.params, label %arrow.start, label %none\narrow.start:\n  %after.params = add i32 %close.params, 1\n  %arrow = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.params)\n  %arrow.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %arrow, i8 %operator.kind, ptr @.str.lambda.arrow.symbol)\n  %is.arrow = icmp ne i32 %arrow.raw, 0\n  br i1 %is.arrow, label %found, label %none\nfound:\n  ret i32 %arrow\nnone:\n  ret i32 -1\n}\n\n";
    } else if (name && std::strcmp(name, "lambdaCaptureSummary") == 0) {
        definition = "define ptr @lambdaCaptureSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.lambda.capture.open, ptr @.str.lambda.capture.close)\n  %minimum = add i32 %arg.start, 1\n  %has.capture = icmp sgt i32 %close, %minimum\n  br i1 %has.capture, label %capture, label %none\ncapture:\n  %first = add i32 %arg.start, 1\n  %ret.capture = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %first, i32 %close)\n  ret ptr %ret.capture\nnone:\n  %ret.none = getelementptr inbounds [5 x i8], ptr @.str.lambda.none, i32 0, i32 0\n  ret ptr %ret.none\n}\n\n@.str.lambda.capture.open = private unnamed_addr constant [2 x i8] c\"[\\00\"\n@.str.lambda.capture.close = private unnamed_addr constant [2 x i8] c\"]\\00\"\n@.str.lambda.none = private unnamed_addr constant [5 x i8] c\"none\\00\"\n\n";
    } else if (name && std::strcmp(name, "lambdaParameterCount") == 0) {
        definition = "define i32 @lambdaParameterCount(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %capture.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.lambda.params.capture.open, ptr @.str.lambda.params.capture.close)\n  %capture.valid = icmp sge i32 %capture.close, 0\n  br i1 %capture.valid, label %params.start, label %empty\nparams.start:\n  %after.capture = add i32 %capture.close, 1\n  %params.open = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.capture)\n  %is.open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %params.open, i8 79, ptr @.str.lambda.params.open)\n  %is.open = icmp ne i32 %is.open.raw, 0\n  br i1 %is.open, label %params.end, label %empty\nparams.end:\n  %params.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %params.open, i32 %arg.end, ptr @.str.lambda.params.open, ptr @.str.lambda.params.close)\n  %first = add i32 %params.open, 1\n  %ret = call i32 @countCommaSeparated(ptr %arg.tokens, i32 %first, i32 %params.close)\n  ret i32 %ret\nempty:\n  ret i32 0\n}\n\n@.str.lambda.params.capture.open = private unnamed_addr constant [2 x i8] c\"[\\00\"\n@.str.lambda.params.capture.close = private unnamed_addr constant [2 x i8] c\"]\\00\"\n@.str.lambda.params.open = private unnamed_addr constant [2 x i8] c\"(\\00\"\n@.str.lambda.params.close = private unnamed_addr constant [2 x i8] c\")\\00\"\n\n";
    } else if (name && std::strcmp(name, "summarizeLambdaExpression") == 0) {
        definition = "define ptr @summarizeLambdaExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.start, i8 79, ptr @.str.lambda.summary.open)\n  %open = icmp ne i32 %open.raw, 0\n  br i1 %open, label %arrow.check, label %empty\narrow.check:\n  %arrow = call i32 @findLambdaArrow(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %valid = icmp sge i32 %arrow, 0\n  br i1 %valid, label %summary, label %empty\nsummary:\n  %capture = call ptr @lambdaCaptureSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %params = call i32 @lambdaParameterCount(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %params.i64 = sext i32 %params to i64\n  %params.text = call ptr @csec_to_string_i64(i64 %params.i64)\n  %s1 = call ptr @csec_string_concat(ptr @.str.lambda.summary.prefix, ptr %capture)\n  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.lambda.summary.params)\n  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %params.text)\n  ret ptr %s3\nempty:\n  ret ptr @.str.lambda.summary.empty\n}\n\n@.str.lambda.summary.open = private unnamed_addr constant [2 x i8] c\"[\\00\"\n@.str.lambda.summary.prefix = private unnamed_addr constant [21 x i8] c\"Expr lambda capture=\\00\"\n@.str.lambda.summary.params = private unnamed_addr constant [9 x i8] c\" params=\\00\"\n@.str.lambda.summary.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\n";
    } else if (name && std::strcmp(name, "generateExpressionAST") == 0) {
        definition = "define ptr @generateExpressionAST(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {\nentry:\n  %ret = call ptr @csec_generate_expression_ast(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateStatementAST") == 0) {
        definition = "define ptr @generateStatementAST(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call ptr @csec_generate_statement_ast(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateBodyAST") == 0) {
        definition = "define ptr @generateBodyAST(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  %ret = call ptr @csec_generate_body_ast(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findFunctionParamStart") == 0) {
        definition = "define i32 @findFunctionParamStart(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call i32 @csec_find_function_param_start(ptr %arg.tokens, i32 %arg.declStart)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findFunctionReturnTypeStart") == 0) {
        definition = "define i32 @findFunctionReturnTypeStart(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %param.start = call i32 @findFunctionParamStart(ptr %arg.tokens, i32 %arg.declStart)\n  %valid.start = icmp sge i32 %param.start, 0\n  br i1 %valid.start, label %find.end, label %missing\nfind.end:\n  %decl.end = call i32 @advanceTopLevelDecl(ptr %arg.tokens, i32 %arg.declStart)\n  %param.end = call i32 @findClosingToken(ptr %arg.tokens, i32 %param.start, i32 %decl.end, ptr @.str.fn.return.open, ptr @.str.fn.return.close)\n  %valid.end = icmp sge i32 %param.end, 0\n  br i1 %valid.end, label %colon.check, label %missing\ncolon.check:\n  %after = add i32 %param.end, 1\n  %colon.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %after, i8 79, ptr @.str.fn.return.colon)\n  %colon = icmp ne i32 %colon.raw, 0\n  br i1 %colon, label %found, label %missing\nfound:\n  %ret = add i32 %param.end, 2\n  ret i32 %ret\nmissing:\n  ret i32 -1\n}\n\n@.str.fn.return.open = private unnamed_addr constant [2 x i8] c\"(\\00\"\n@.str.fn.return.close = private unnamed_addr constant [2 x i8] c\")\\00\"\n@.str.fn.return.colon = private unnamed_addr constant [2 x i8] c\":\\00\"\n\n";
    } else if (name && std::strcmp(name, "typeSummary") == 0) {
        definition = "define ptr @typeSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.start, i8 79, ptr @.str.type.summary.open)\n  %open = icmp ne i32 %open.raw, 0\n  br i1 %open, label %find.close, label %fallback\nfind.close:\n  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.type.summary.open, ptr @.str.type.summary.close)\n  %valid.close = icmp sgt i32 %close, %arg.start\n  br i1 %valid.close, label %arrow.check, label %fallback\narrow.check:\n  %after.close = add i32 %close, 1\n  %arrow.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %after.close, i8 79, ptr @.str.type.summary.arrow)\n  %arrow = icmp ne i32 %arrow.raw, 0\n  br i1 %arrow, label %function, label %fallback\nfunction:\n  %params.start = add i32 %arg.start, 1\n  %params = call i32 @countCommaSeparated(ptr %arg.tokens, i32 %params.start, i32 %close)\n  %params.i64 = sext i32 %params to i64\n  %params.text = call ptr @csec_to_string_i64(i64 %params.i64)\n  %returns.start = add i32 %close, 2\n  %returns = call ptr @collectTypeName(ptr %arg.tokens, i32 %returns.start, i32 %arg.end)\n  %s1 = call ptr @csec_string_concat(ptr @.str.type.summary.prefix, ptr %params.text)\n  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.type.summary.returns)\n  %ret = call ptr @csec_string_concat(ptr %s2, ptr %returns)\n  ret ptr %ret\nfallback:\n  %fallback.ret = call ptr @collectTypeName(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %fallback.ret\n}\n\n@.str.type.summary.open = private unnamed_addr constant [2 x i8] c\"(\\00\"\n@.str.type.summary.close = private unnamed_addr constant [2 x i8] c\")\\00\"\n@.str.type.summary.arrow = private unnamed_addr constant [3 x i8] c\"=>\\00\"\n@.str.type.summary.prefix = private unnamed_addr constant [21 x i8] c\"FunctionType params=\\00\"\n@.str.type.summary.returns = private unnamed_addr constant [10 x i8] c\" returns=\\00\"\n\n";
    } else if (name && std::strcmp(name, "generateCBody") == 0) {
        definition = "define ptr @generateCBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd, ptr %arg.indent) {\nentry:\n  %ret = call ptr @csec_generate_c_body(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd, ptr %arg.indent)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "appendCExpression") == 0) {
        definition = "define ptr @appendCExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {\nentry:\n  %ret = call ptr @csec_generate_c_expression(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateCStatement") == 0) {
        definition = "@.str.c.statement.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateCStatement(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.indent) {\nentry:\n  ret ptr @.str.c.statement.empty\n}\n\n";
    } else if (name && std::strcmp(name, "collectTypeName") == 0) {
        definition = "define ptr @collectTypeName(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call ptr @csec_collect_type_name(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "irTypeName") == 0) {
        definition = "define ptr @irTypeName(ptr %arg.typeName) {\nentry:\n  %ret = call ptr @csec_ir_type_name(ptr %arg.typeName)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lookupVisibleValueType") == 0) {
        definition = "define ptr @lookupVisibleValueType(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name) {\nentry:\n  %ret = call ptr @csec_lookup_visible_value_type(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "lookupVisibleStorageName") == 0) {
        definition = "define ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name) {\nentry:\n  %ret = call ptr @csec_lookup_visible_storage_name(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateIR") == 0) {
        definition = "@.str.ir.module.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateIR(ptr %arg.tokens) {\nentry:\n  ret ptr @.str.ir.module.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRParamList") == 0) {
        definition = "define ptr @generateIRParamList(ptr %arg.tokens, i32 %arg.declStart) {\nentry:\n  %ret = call ptr @csec_function_llvm_param_list(ptr %arg.tokens, i32 %arg.declStart)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRFlatBody") == 0) {
        definition = "@.str.ir.flat.body.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateIRFlatBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  ret ptr @.str.ir.flat.body.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateFunctionScopeSymbols") == 0) {
        definition = "@.str.function.scope.symbols.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateFunctionScopeSymbols(ptr %arg.tokens, ptr %arg.functionName, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  ret ptr @.str.function.scope.symbols.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateFunctionParamSymbols") == 0) {
        definition = "@.str.function.param.symbols.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateFunctionParamSymbols(ptr %arg.tokens, ptr %arg.functionName, i32 %arg.declStart) {\nentry:\n  ret ptr @.str.function.param.symbols.empty\n}\n\n";
    } else if (name && std::strcmp(name, "classMemberStart") == 0) {
        definition = "@.str.class.member.override = private unnamed_addr constant [9 x i8] c\"override\\00\"\n@.str.class.member.unsafe = private unnamed_addr constant [7 x i8] c\"unsafe\\00\"\n@.str.class.member.constexpr = private unnamed_addr constant [10 x i8] c\"constexpr\\00\"\n\ndefine i32 @classMemberStart(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %override.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.override)\n  %unsafe.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.unsafe)\n  %constexpr.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.constexpr)\n  %override = icmp ne i32 %override.raw, 0\n  %unsafe = icmp ne i32 %unsafe.raw, 0\n  %constexpr = icmp ne i32 %constexpr.raw, 0\n  %modifier = or i1 %override, %unsafe\n  %skip = or i1 %modifier, %constexpr\n  br i1 %skip, label %advanced, label %plain\nadvanced:\n  %next = add i32 %arg.ordinal, 1\n  ret i32 %next\nplain:\n  ret i32 %arg.ordinal\n}\n\n";
    } else if (name && std::strcmp(name, "generateClassMemberSymbols") == 0) {
        definition = "@.str.class.member.symbols.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateClassMemberSymbols(ptr %arg.tokens, ptr %arg.className, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  ret ptr @.str.class.member.symbols.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateClassMemberAST") == 0) {
        definition = "@.str.class.member.ast.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateClassMemberAST(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  ret ptr @.str.class.member.ast.empty\n}\n\n";
    } else if (name && (std::strcmp(name, "templateParameterSummary") == 0 ||
                         std::strcmp(name, "templateTargetKind") == 0 ||
                         std::strcmp(name, "attributeSummary") == 0 ||
                         std::strcmp(name, "externalSymbolKind") == 0)) {
        const std::string functionName(name);
        const std::string globalName = ".str.summary." + functionName + ".empty";
        const std::string generated = "@" + globalName + " = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @" + functionName + "(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  ret ptr @" + globalName + "\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateAST") == 0) {
        definition = "@.str.ast.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateAST(ptr %arg.tokens) {\nentry:\n  ret ptr @.str.ast.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateLLVMMainBody") == 0) {
        definition = "@.str.llvm.main.body.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateLLVMMainBody(ptr %arg.tokens) {\nentry:\n  ret ptr @.str.llvm.main.body.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRBody") == 0) {
        definition = "@.str.ir.body.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateIRBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {\nentry:\n  ret ptr @.str.ir.body.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRDeclarations") == 0) {
        definition = "@.str.ir.declarations.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @generateIRDeclarations(ptr %arg.tokens) {\nentry:\n  ret ptr @.str.ir.declarations.empty\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRFor") == 0) {
        const std::string generated = llvmGenerateIRForDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "llvmLoadForValueType") == 0) {
        const std::string generated = llvmLoadForValueTypeDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMCallArgumentListI32") == 0) {
        const std::string generated = llvmGenerateCallArgumentListI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "llvmI32CallArgumentValue") == 0) {
        const std::string generated = llvmI32CallArgumentValueDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "llvmCallArgumentValue") == 0) {
        const std::string generated = llvmCallArgumentValueDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMCallArgumentLoadI32") == 0) {
        const std::string generated = llvmGenerateCallArgumentLoadI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMCallArgumentLoadsI32") == 0) {
        const std::string generated = llvmGenerateCallArgumentLoadsI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMAssignmentI32") == 0) {
        const std::string generated = llvmGenerateAssignmentI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMAssignmentI64") == 0) {
        const std::string generated = llvmGenerateAssignmentI64Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "llvmRuntimeCallName") == 0) {
        const std::string generated = llvmRuntimeCallNameDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalI32") == 0) {
        const std::string generated = llvmGenerateLocalI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalI64") == 0) {
        const std::string generated = llvmGenerateLocalI64Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalF64") == 0) {
        const std::string generated = llvmGenerateLocalF64Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalI8") == 0) {
        const std::string generated = llvmGenerateLocalI8Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalI1") == 0) {
        const std::string generated = llvmGenerateLocalI1Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMLocalPtr") == 0) {
        const std::string generated = llvmGenerateLocalPtrDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && (std::strcmp(name, "generateLLVMIfI32") == 0 || std::strcmp(name, "generateLLVMWhileI32") == 0 || std::strcmp(name, "generateLLVMForI32") == 0)) {
        const std::string generated = llvmControlFlowDefinition(name);
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMExpressionI32") == 0) {
        const std::string generated = llvmGenerateExpressionI32Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMExpressionI64") == 0) {
        const std::string generated = llvmGenerateExpressionI64Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "llvmBlockEndsWithTopLevelReturn") == 0) {
        const std::string generated = llvmBlockEndsWithTopLevelReturnDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMExpressionF64") == 0) {
        const std::string generated = llvmGenerateExpressionF64Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMConditionI1") == 0) {
        const std::string generated = llvmGenerateConditionI1Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMExpressionI1") == 0) {
        const std::string generated = llvmGenerateExpressionI1Definition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateLLVMExpressionPtr") == 0) {
        const std::string generated = llvmGenerateExpressionPtrDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateIRWhile") == 0) {
        const std::string generated = llvmGenerateIRWhileDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateIRIf") == 0) {
        const std::string generated = llvmGenerateIRIfDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateIRElseFlatBody") == 0) {
        definition = "define ptr @generateIRElseFlatBody(ptr %arg.tokens, i32 %arg.possibleElse, i32 %arg.end, i1 %arg.hasElse) {\nentry:\n  br i1 %arg.hasElse, label %else.entry, label %no.else\nno.else:\n  ret ptr @.str.ir.else.none\nelse.entry:\n  %after.else = add i32 %arg.possibleElse, 1\n  %else.body.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.else)\n  %brace.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.body.start, i8 79, ptr @.str.ir.else.open)\n  %brace = icmp ne i32 %brace.raw, 0\n  br i1 %brace, label %brace.body, label %if.check\nbrace.body:\n  %else.end = call i32 @findClosingToken(ptr %arg.tokens, i32 %else.body.start, i32 %arg.end, ptr @.str.ir.else.open, ptr @.str.ir.else.close)\n  %valid = icmp sgt i32 %else.end, %else.body.start\n  br i1 %valid, label %flat, label %malformed\nflat:\n  %body.start = add i32 %else.body.start, 1\n  %ret.flat = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %body.start, i32 %else.end)\n  ret ptr %ret.flat\nif.check:\n  %if.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.body.start, i8 75, ptr @.str.ir.else.if)\n  %is.if = icmp ne i32 %if.raw, 0\n  br i1 %is.if, label %else.if, label %malformed\nelse.if:\n  ret ptr @.str.ir.else.if.body\nmalformed:\n  ret ptr @.str.ir.else.malformed\n}\n\n@.str.ir.else.none = private unnamed_addr constant [20 x i8] c\"    ; no else body\\0A\\00\"\n@.str.ir.else.open = private unnamed_addr constant [2 x i8] c\"{\\00\"\n@.str.ir.else.close = private unnamed_addr constant [2 x i8] c\"}\\00\"\n@.str.ir.else.if = private unnamed_addr constant [3 x i8] c\"if\\00\"\n@.str.ir.else.if.body = private unnamed_addr constant [20 x i8] c\"    ; else-if body\\0A\\00\"\n@.str.ir.else.malformed = private unnamed_addr constant [27 x i8] c\"    ; malformed else body\\0A\\00\"\n\n";
    } else if (name && std::strcmp(name, "generateIRElseFlatBody") == 0) {
        definition = "define ptr @generateIRElseFlatBody(ptr %arg.tokens, i32 %arg.possibleElse, i32 %arg.end, i1 %arg.hasElse) {\nentry:\n  br i1 %arg.hasElse, label %else.start, label %no.else\nno.else:\n  ret ptr @.str.ir.else.none\nelse.start:\n  %after.else = add i32 %arg.possibleElse, 1\n  %else.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.else)\n  %brace.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.start, i8 79, ptr @.str.ir.else.open)\n  %brace = icmp ne i32 %brace.raw, 0\n  br i1 %brace, label %brace.body, label %if.check\nbrace.body:\n  %else.end = call i32 @findClosingToken(ptr %arg.tokens, i32 %else.start, i32 %arg.end, ptr @.str.ir.else.open, ptr @.str.ir.else.close)\n  %valid = icmp sgt i32 %else.end, %else.start\n  br i1 %valid, label %flat, label %malformed\nflat:\n  %body.start = add i32 %else.start, 1\n  %ret.flat = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %body.start, i32 %else.end)\n  ret ptr %ret.flat\nif.check:\n  %if.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.start, i8 75, ptr @.str.ir.else.if)\n  %is.if = icmp ne i32 %if.raw, 0\n  br i1 %is.if, label %else.if, label %malformed\nelse.if:\n  ret ptr @.str.ir.else.if.body\nmalformed:\n  ret ptr @.str.ir.else.malformed\n}\n\n@.str.ir.else.none = private unnamed_addr constant [20 x i8] c\"    ; no else body\\0A\\00\"\n@.str.ir.else.open = private unnamed_addr constant [2 x i8] c\"{\\00\"\n@.str.ir.else.close = private unnamed_addr constant [2 x i8] c\"}\\00\"\n@.str.ir.else.if = private unnamed_addr constant [3 x i8] c\"if\\00\"\n@.str.ir.else.if.body = private unnamed_addr constant [20 x i8] c\"    ; else-if body\\0A\\00\"\n@.str.ir.else.malformed = private unnamed_addr constant [27 x i8] c\"    ; malformed else body\\0A\\00\"\n\n";
    } else if (name && (std::strcmp(name, "generateSymbolTable") == 0 || std::strcmp(name, "expressionLeafKind") == 0 || std::strcmp(name, "declaredValueType") == 0 || std::strcmp(name, "lookupFunctionReturnType") == 0 || std::strcmp(name, "inferExpressionType") == 0 || std::strcmp(name, "declaredLocalType") == 0 || std::strcmp(name, "localDeclarationType") == 0)) {
        definition = std::strcmp(name, "expressionLeafKind") == 0
            ? "declare ptr @csec_expression_leaf_kind(ptr, i32, i32)\n\ndefine ptr @expressionLeafKind(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call ptr @csec_expression_leaf_kind(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "declaredValueType") == 0
                ? "declare ptr @csec_declared_value_type(ptr, i32, i32)\n\ndefine ptr @declaredValueType(ptr %arg.tokens, i32 %arg.declStart, i32 %arg.declEnd) {\nentry:\n  %ret = call ptr @csec_declared_value_type(ptr %arg.tokens, i32 %arg.declStart, i32 %arg.declEnd)\n  ret ptr %ret\n}\n\n"
                : std::strcmp(name, "declaredLocalType") == 0
                    ? "declare ptr @csec_declared_local_type(ptr, i32, i32)\n\ndefine ptr @declaredLocalType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call ptr @csec_declared_local_type(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %ret\n}\n\n"
                    : std::strcmp(name, "localDeclarationType") == 0
                        ? "@.str.local.type.unknown = private unnamed_addr constant [8 x i8] c\"unknown\\00\"\n\ndefine ptr @localDeclarationType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %explicit = call ptr @declaredLocalType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %has.type = call i32 @csec_string_length(ptr %explicit)\n  %present = icmp sgt i32 %has.type, 0\n  br i1 %present, label %typed, label %fallback\ntyped:\n  ret ptr %explicit\nfallback:\n  ret ptr @.str.local.type.unknown\n}\n\n"
                    : std::strcmp(name, "inferExpressionType") == 0
                    ? "declare ptr @csec_infer_expression_type(ptr, i32, i32)\n\ndefine ptr @inferExpressionType(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {\nentry:\n  %ret = call ptr @csec_infer_expression_type(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd)\n  ret ptr %ret\n}\n\n"
                    : std::strcmp(name, "classMemberKind") == 0
                    ? "@.str.class.member.override = private unnamed_addr constant [9 x i8] c\"override\\00\"\n@.str.class.member.unsafe = private unnamed_addr constant [7 x i8] c\"unsafe\\00\"\n@.str.class.member.constexpr = private unnamed_addr constant [10 x i8] c\"constexpr\\00\"\n@.str.class.member.def = private unnamed_addr constant [4 x i8] c\"def\\00\"\n@.str.class.member.val = private unnamed_addr constant [4 x i8] c\"val\\00\"\n@.str.class.member.var = private unnamed_addr constant [4 x i8] c\"var\\00\"\n@.str.class.member.method = private unnamed_addr constant [7 x i8] c\"method\\00\"\n@.str.class.member.field = private unnamed_addr constant [6 x i8] c\"field\\00\"\n@.str.class.member.mutable = private unnamed_addr constant [14 x i8] c\"mutable-field\\00\"\n@.str.class.member.default = private unnamed_addr constant [7 x i8] c\"member\\00\"\n\ndefine ptr @classMemberKind(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %override = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.override)\n  %unsafe = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.unsafe)\n  %constexpr = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.constexpr)\n  %modifier.1 = icmp ne i32 %override, 0\n  %modifier.2 = icmp ne i32 %unsafe, 0\n  %modifier.3 = icmp ne i32 %constexpr, 0\n  %modifier.any = or i1 %modifier.1, %modifier.2\n  %modifier = or i1 %modifier.any, %modifier.3\n  %next = add i32 %arg.ordinal, 1\n  %start = select i1 %modifier, i32 %next, i32 %arg.ordinal\n  %def = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.member.def)\n  %is.def = icmp ne i32 %def, 0\n  br i1 %is.def, label %method, label %val.check\nmethod:\n  ret ptr @.str.class.member.method\nval.check:\n  %val = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.member.val)\n  %is.val = icmp ne i32 %val, 0\n  br i1 %is.val, label %field, label %var.check\nfield:\n  ret ptr @.str.class.member.field\nvar.check:\n  %var = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.member.var)\n  %is.var = icmp ne i32 %var, 0\n  br i1 %is.var, label %mutable, label %default\nmutable:\n  ret ptr @.str.class.member.mutable\ndefault:\n  ret ptr @.str.class.member.default\n}\n\n"
                    : std::strcmp(name, "classMemberName") == 0
                    ? "@.str.class.name.override = private unnamed_addr constant [9 x i8] c\"override\\00\"\n@.str.class.name.unsafe = private unnamed_addr constant [7 x i8] c\"unsafe\\00\"\n@.str.class.name.constexpr = private unnamed_addr constant [10 x i8] c\"constexpr\\00\"\n@.str.class.name.def = private unnamed_addr constant [4 x i8] c\"def\\00\"\n@.str.class.name.val = private unnamed_addr constant [4 x i8] c\"val\\00\"\n@.str.class.name.var = private unnamed_addr constant [4 x i8] c\"var\\00\"\n@.str.class.name.operator = private unnamed_addr constant [9 x i8] c\"operator\\00\"\n@.str.class.name.invalid = private unnamed_addr constant [9 x i8] c\"<member>\\00\"\n\ndefine ptr @classMemberName(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %override = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.name.override)\n  %unsafe = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.name.unsafe)\n  %constexpr = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.name.constexpr)\n  %modifier.1 = icmp ne i32 %override, 0\n  %modifier.2 = icmp ne i32 %unsafe, 0\n  %modifier.3 = icmp ne i32 %constexpr, 0\n  %modifier.any = or i1 %modifier.1, %modifier.2\n  %modifier = or i1 %modifier.any, %modifier.3\n  %next = add i32 %arg.ordinal, 1\n  %start = select i1 %modifier, i32 %next, i32 %arg.ordinal\n  %def = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.name.def)\n  %is.def = icmp ne i32 %def, 0\n  br i1 %is.def, label %def.name, label %field.check\ndef.name:\n  %after.def = add i32 %start, 1\n  %operator = call i32 @csec_token_is(ptr %arg.tokens, i32 %after.def, i8 75, ptr @.str.class.name.operator)\n  %is.operator = icmp ne i32 %operator, 0\n  br i1 %is.operator, label %operator.name, label %plain.name\noperator.name:\n  %operator.token = add i32 %after.def, 1\n  %operator.text = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %operator.token)\n  %operator.result = call ptr @csec_string_concat(ptr @.str.class.name.operator, ptr %operator.text)\n  ret ptr %operator.result\nplain.name:\n  %plain = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %after.def)\n  ret ptr %plain\nfield.check:\n  %val = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.name.val)\n  %var = call i32 @csec_token_is(ptr %arg.tokens, i32 %start, i8 75, ptr @.str.class.name.var)\n  %is.val = icmp ne i32 %val, 0\n  %is.var = icmp ne i32 %var, 0\n  %is.field = or i1 %is.val, %is.var\n  br i1 %is.field, label %field.name, label %invalid\nfield.name:\n  %after.field = add i32 %start, 1\n  %field = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %after.field)\n  ret ptr %field\ninvalid:\n  ret ptr @.str.class.name.invalid\n}\n\n"
                    : std::strcmp(name, "lookupFunctionReturnType") == 0
                    ? "define ptr @lookupFunctionReturnType(ptr %arg.tokens, ptr %arg.name) {\nentry:\n  %ret = call ptr @csec_lookup_function_return_type(ptr %arg.tokens, ptr %arg.name)\n  ret ptr %ret\n}\n\n"
                    : "define ptr @generateSymbolTable(ptr %arg.tokens) {\nentry:\n  %ret = call ptr @csec_generate_symbol_table(ptr %arg.tokens)\n  ret ptr %ret\n}\n\n";
    } else if (name && (std::strcmp(name, "appendExpressionUntil") == 0 || std::strcmp(name, "statementKind") == 0 || std::strcmp(name, "expressionLeafKind") == 0 || std::strcmp(name, "statementHeaderExpression") == 0 || std::strcmp(name, "summarizePostfixExpression") == 0)) {
        definition = std::strcmp(name, "statementKind") == 0
            ? "declare ptr @csec_statement_kind(ptr, i32)\n\ndefine ptr @statementKind(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call ptr @csec_statement_kind(ptr %arg.tokens, i32 %arg.ordinal)\n  ret ptr %ret\n}\n\n"
            : std::strcmp(name, "expressionLeafKind") == 0
                ? "@.str.leaf.empty = private unnamed_addr constant [6 x i8] c\"empty\\00\"\n@.str.leaf.integer = private unnamed_addr constant [8 x i8] c\"integer\\00\"\n@.str.leaf.float = private unnamed_addr constant [6 x i8] c\"float\\00\"\n@.str.leaf.string = private unnamed_addr constant [7 x i8] c\"string\\00\"\n@.str.leaf.regex = private unnamed_addr constant [6 x i8] c\"regex\\00\"\n@.str.leaf.char = private unnamed_addr constant [5 x i8] c\"char\\00\"\n@.str.leaf.bool = private unnamed_addr constant [5 x i8] c\"bool\\00\"\n@.str.leaf.identifier = private unnamed_addr constant [11 x i8] c\"identifier\\00\"\n@.str.leaf.unknown = private unnamed_addr constant [8 x i8] c\"unknown\\00\"\n\ndefine ptr @expressionLeafKind(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %nonempty = icmp sgt i32 %arg.end, %arg.start\n  br i1 %nonempty, label %kind, label %empty\nkind:\n  %value = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %arg.start)\n  %is.integer = icmp eq i8 %value, 78\n  br i1 %is.integer, label %integer, label %float.check\nfloat.check:\n  %is.float = icmp eq i8 %value, 70\n  br i1 %is.float, label %float, label %string.check\nstring.check:\n  %is.string = icmp eq i8 %value, 83\n  br i1 %is.string, label %string, label %regex.check\nregex.check:\n  %is.regex = icmp eq i8 %value, 82\n  br i1 %is.regex, label %regex, label %char.check\nchar.check:\n  %is.char = icmp eq i8 %value, 67\n  br i1 %is.char, label %char, label %bool.check\nbool.check:\n  %is.bool = icmp eq i8 %value, 66\n  br i1 %is.bool, label %bool, label %identifier.check\nidentifier.check:\n  %is.identifier = icmp eq i8 %value, 73\n  br i1 %is.identifier, label %identifier, label %unknown\nempty:\n  ret ptr @.str.leaf.empty\ninteger:\n  ret ptr @.str.leaf.integer\nfloat:\n  ret ptr @.str.leaf.float\nstring:\n  ret ptr @.str.leaf.string\nregex:\n  ret ptr @.str.leaf.regex\nchar:\n  ret ptr @.str.leaf.char\nbool:\n  ret ptr @.str.leaf.bool\nidentifier:\n  ret ptr @.str.leaf.identifier\nunknown:\n  ret ptr @.str.leaf.unknown\n}\n\n"
                : std::strcmp(name, "statementHeaderExpression") == 0
                    ? "@.str.statement.header.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @statementHeaderExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  %has.open = icmp sge i32 %open, 0\n  %after.open = icmp sgt i32 %close, %open\n  %valid = and i1 %has.open, %after.open\n  br i1 %valid, label %body, label %empty\nbody:\n  %body.start = add i32 %open, 1\n  %ret = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %body.start, i32 %close)\n  ret ptr %ret\nempty:\n  ret ptr @.str.statement.header.empty\n}\n\n"
                    : std::strcmp(name, "summarizePostfixExpression") == 0
                        ? "@.str.postfix.summary.empty = private unnamed_addr constant [1 x i8] c\"\\00\"\n\ndefine ptr @summarizePostfixExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  ret ptr @.str.postfix.summary.empty\n}\n\n"
                        : "define ptr @appendExpressionUntil(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call ptr @csec_append_expression_until(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "generateIRAssignment") == 0) {
        const std::string generated = llvmGenerateIRAssignmentDefinition();
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "generateIRExpression") == 0) {
        definition = "define ptr @generateIRExpression(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {\nentry:\n  %start = call i32 @skipTrivia(ptr %arg.tokens, i32 %arg.rawStart)\n  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %start, i32 %arg.rawEnd)\n  %nonempty = icmp sgt i32 %end, %start\n  br i1 %nonempty, label %classify, label %empty\nempty:\n  %empty.value = getelementptr inbounds [5 x i8], ptr @.str.ir.expr.void, i32 0, i32 0\n  ret ptr %empty.value\nclassify:\n  %type.name = call ptr @inferExpressionType(ptr %arg.tokens, i32 %start, i32 %end)\n  %ir.type = call ptr @irTypeName(ptr %type.name)\n  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %start, i32 %end)\n  %has.op = icmp sgt i32 %op, %start\n  br i1 %has.op, label %binary, label %identifier.check\nbinary:\n  %op.text = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %op)\n  %op.name = call ptr @irOperatorName(ptr %op.text)\n  %b1 = call ptr @csec_string_concat(ptr %op.name, ptr @.str.ir.expr.space)\n  %b2 = call ptr @csec_string_concat(ptr %b1, ptr %ir.type)\n  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.ir.expr.open)\n  %left = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %start, i32 %op)\n  %b4 = call ptr @csec_string_concat(ptr %b3, ptr %left)\n  %b5 = call ptr @csec_string_concat(ptr %b4, ptr @.str.ir.expr.middle)\n  %right.start = add i32 %op, 1\n  %right = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %right.start, i32 %end)\n  %b6 = call ptr @csec_string_concat(ptr %b5, ptr %right)\n  %b7 = call ptr @csec_string_concat(ptr %b6, ptr @.str.ir.expr.close)\n  ret ptr %b7\nidentifier.check:\n  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %start)\n  %is.identifier = icmp eq i8 %kind, 73\n  br i1 %is.identifier, label %call.check, label %fallback\ncall.check:\n  %next = add i32 %start, 1\n  %before.end = icmp slt i32 %next, %end\n  br i1 %before.end, label %open.check, label %single.check\nopen.check:\n  %is.open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %next, i8 79, ptr @.str.ir.expr.paren.open)\n  %is.open = icmp ne i32 %is.open.raw, 0\n  br i1 %is.open, label %close.check, label %single.check\nclose.check:\n  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %next, i32 %end, ptr @.str.ir.expr.paren.open, ptr @.str.ir.expr.paren.close)\n  %last = sub i32 %end, 1\n  %is.call = icmp eq i32 %close, %last\n  br i1 %is.call, label %call, label %single.check\ncall:\n  %c1 = call ptr @csec_string_concat(ptr @.str.ir.expr.call, ptr %ir.type)\n  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.ir.expr.at)\n  %name = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %start)\n  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %name)\n  %c4 = call ptr @csec_string_concat(ptr %c3, ptr @.str.ir.expr.paren.open)\n  %args.start = add i32 %start, 2\n  %args = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %args.start, i32 %last)\n  %c5 = call ptr @csec_string_concat(ptr %c4, ptr %args)\n  %c6 = call ptr @csec_string_concat(ptr %c5, ptr @.str.ir.expr.paren.close)\n  ret ptr %c6\nsingle.check:\n  %single.end = add i32 %start, 1\n  %is.single = icmp eq i32 %end, %single.end\n  br i1 %is.single, label %single, label %fallback\nsingle:\n  %s1 = call ptr @csec_string_concat(ptr @.str.ir.expr.load, ptr %ir.type)\n  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.ir.expr.load.middle)\n  %single.name = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %start)\n  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %single.name)\n  ret ptr %s3\nfallback:\n  %f1 = call ptr @csec_string_concat(ptr %ir.type, ptr @.str.ir.expr.space)\n  %f2 = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %start, i32 %end)\n  %f3 = call ptr @csec_string_concat(ptr %f1, ptr %f2)\n  ret ptr %f3\n}\n\n@.str.ir.expr.void = private unnamed_addr constant [5 x i8] c\"void\\00\"\n@.str.ir.expr.space = private unnamed_addr constant [2 x i8] c\" \\00\"\n@.str.ir.expr.open = private unnamed_addr constant [3 x i8] c\" (\\00\"\n@.str.ir.expr.middle = private unnamed_addr constant [5 x i8] c\"), (\\00\"\n@.str.ir.expr.close = private unnamed_addr constant [2 x i8] c\")\\00\"\n@.str.ir.expr.paren.open = private unnamed_addr constant [2 x i8] c\"(\\00\"\n@.str.ir.expr.paren.close = private unnamed_addr constant [2 x i8] c\")\\00\"\n@.str.ir.expr.call = private unnamed_addr constant [6 x i8] c\"call \\00\"\n@.str.ir.expr.at = private unnamed_addr constant [2 x i8] c\"@\\00\"\n@.str.ir.expr.load = private unnamed_addr constant [6 x i8] c\"load \\00\"\n@.str.ir.expr.load.middle = private unnamed_addr constant [8 x i8] c\", ptr %\\00\"\n\n";
    } else if (name && std::strcmp(name, "irOperatorName") == 0) {
        definition = "define ptr @irOperatorName(ptr %arg.text) {\nentry:\n  %ret = call ptr @csec_ir_operator_name(ptr %arg.text)\n  ret ptr %ret\n}\n\n";
    } else if (name && std::strcmp(name, "expressionTopLevelOperator") == 0) {
        definition = "define i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_expression_top_level_operator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "trimExpressionEnd") == 0) {
        definition = "define i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  br label %trim.loop\ntrim.loop:\n  %last = phi i32 [ %arg.end, %entry ], [ %previous, %trim.comment ]\n  %has.previous = icmp sgt i32 %last, %arg.start\n  br i1 %has.previous, label %trim.inspect, label %trim.check\ntrim.inspect:\n  %index = sub i32 %last, 1\n  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %index)\n  %is.comment = icmp eq i8 %kind, 77\n  br i1 %is.comment, label %trim.comment, label %trim.check\ntrim.comment:\n  %previous = sub i32 %last, 1\n  br label %trim.loop\ntrim.check:\n  %has.token = icmp sgt i32 %last, %arg.start\n  br i1 %has.token, label %trim.semicolon, label %trim.return\ntrim.semicolon:\n  %candidate = sub i32 %last, 1\n  %is.semicolon = call i32 @csec_token_is(ptr %arg.tokens, i32 %candidate, i8 79, ptr @.str.trim.semicolon)\n  %has.semicolon = icmp ne i32 %is.semicolon, 0\n  br i1 %has.semicolon, label %trim.without.semicolon, label %trim.return\ntrim.without.semicolon:\n  %result = sub i32 %last, 1\n  ret i32 %result\ntrim.return:\n  ret i32 %last\n}\n\n@.str.trim.semicolon = private unnamed_addr constant [2 x i8] c\";\\00\"\n\n";
    } else if (name && std::strcmp(name, "findTokenTextInRange") == 0) {
        definition = "define i32 @findTokenTextInRange(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text) {\nentry:\n  %ret = call i32 @csec_find_token_text_in_range(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findClosingToken") == 0) {
        definition = "define i32 @findClosingToken(ptr %arg.tokens, i32 %arg.openOrdinal, i32 %arg.end, ptr %arg.openText, ptr %arg.closeText) {\nentry:\n  %ret = call i32 @csec_find_closing_token(ptr %arg.tokens, i32 %arg.openOrdinal, i32 %arg.end, ptr %arg.openText, ptr %arg.closeText)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findStatementParenStart") == 0) {
        definition = "define i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_find_statement_paren_start(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findStatementParenEnd") == 0) {
        definition = "define i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_find_statement_paren_end(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findStatementBlockStart") == 0) {
        definition = "define i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_find_statement_block_start(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "findStatementBlockEnd") == 0) {
        definition = "define i32 @findStatementBlockEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {\nentry:\n  %ret = call i32 @csec_find_statement_block_end(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)\n  ret i32 %ret\n}\n\n";
    } else if (name && (std::strcmp(name, "advanceTopLevelDecl") == 0 || std::strcmp(name, "advanceTopLevelDeclSlow") == 0)) {
        const char* functionName = std::strcmp(name, "advanceTopLevelDecl") == 0 ? "advanceTopLevelDecl" : "advanceTopLevelDeclSlow";
        std::string generated = "define i32 @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call i32 @csec_advance_top_level_decl(ptr %arg.tokens, i32 %arg.ordinal)\n  ret i32 %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && (std::strcmp(name, "findDeclBodyStart") == 0 || std::strcmp(name, "findDeclBodyStartSlow") == 0)) {
        const char* functionName = std::strcmp(name, "findDeclBodyStart") == 0 ? "findDeclBodyStart" : "findDeclBodyStartSlow";
        std::string generated = "define i32 @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.ordinal) {\nentry:\n  %ret = call i32 @csec_find_decl_body_start(ptr %arg.tokens, i32 %arg.ordinal)\n  ret i32 %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && (std::strcmp(name, "findDeclBodyEnd") == 0 || std::strcmp(name, "findDeclBodyEndSlow") == 0)) {
        const char* functionName = std::strcmp(name, "findDeclBodyEnd") == 0 ? "findDeclBodyEnd" : "findDeclBodyEndSlow";
        std::string generated = "define i32 @" + std::string(functionName) + "(ptr %arg.tokens, i32 %arg.bodyStart) {\nentry:\n  %ret = call i32 @csec_find_decl_body_end(ptr %arg.tokens, i32 %arg.bodyStart)\n  ret i32 %ret\n}\n\n";
        char* result = static_cast<char*>(std::malloc(generated.size() + 1));
        if (!result) return nullptr;
        std::memcpy(result, generated.c_str(), generated.size() + 1);
        return result;
    } else if (name && std::strcmp(name, "isAlpha") == 0) {
        definition = "define i1 @isAlpha(i8 %arg.ch) {\nentry:\n  %lower = icmp sge i8 %arg.ch, 97\n  %lower.end = icmp sle i8 %arg.ch, 122\n  %lower.match = and i1 %lower, %lower.end\n  %upper = icmp sge i8 %arg.ch, 65\n  %upper.end = icmp sle i8 %arg.ch, 90\n  %upper.match = and i1 %upper, %upper.end\n  %ret = or i1 %lower.match, %upper.match\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "isIdentifierStart") == 0) {
        definition = "define i1 @isIdentifierStart(i8 %arg.ch) {\nentry:\n  %alpha = call i1 @isAlpha(i8 %arg.ch)\n  %underscore = icmp eq i8 %arg.ch, 95\n  %ret = or i1 %alpha, %underscore\n  ret i1 %ret\n}\n\n";
    } else if (name && std::strcmp(name, "isIdentifierPart") == 0) {
        definition = "define i1 @isIdentifierPart(i8 %arg.ch) {\nentry:\n  %start = call i1 @isIdentifierStart(i8 %arg.ch)\n  %digit = call i1 @isDigit(i8 %arg.ch)\n  %ret = or i1 %start, %digit\n  ret i1 %ret\n}\n\n";
    }
    const size_t length = std::strlen(definition);
    char* result = static_cast<char*>(std::malloc(length + 1));
    if (!result) return nullptr;
    std::memcpy(result, definition, length + 1);
    return result;
}

int csec_lex_line_comment(const char* source, int index) {
    if (!source || index < 0) return index;
    int cursor = index + 2;
    while (source[cursor] && source[cursor] != '\n') ++cursor;
    return cursor;
}

int csec_lex_block_comment(const char* source, int index) {
    if (!source || index < 0) return index;
    int cursor = index + 2, depth = 1;
    while (source[cursor] && source[cursor + 1] && depth > 0) {
        if (source[cursor] == '/' && source[cursor + 1] == '*') { ++depth; cursor += 2; }
        else if (source[cursor] == '*' && source[cursor + 1] == '/') { --depth; cursor += 2; }
        else ++cursor;
    }
    return cursor;
}

long long csec_string_builder_new(void) {
    auto* builder = new StringBuilder();
    builder->capacity = kStringBuilderGrowQuantum;
    builder->data = static_cast<char*>(std::malloc(builder->capacity));
    if (!builder->data) {
        delete builder;
        return 0;
    }
    builder->data[0] = '\0';
    return reinterpret_cast<long long>(builder);
}

long long csec_string_builder_new_file(const char* path) {
    if (!path) return 0;
    auto* builder = new StringBuilder();
    builder->file = std::fopen(path, "wb");
    if (!builder->file) {
        delete builder;
        return 0;
    }
    std::setvbuf(builder->file, nullptr, _IOFBF, kStringBuilderGrowQuantum);
    return reinterpret_cast<long long>(builder);
}

int csec_string_builder_append(long long handle, const char* text) {
    auto* builder = reinterpret_cast<StringBuilder*>(handle);
    if (!builder) return -1;

    const char* value = text ? text : "";
    size_t valueLen = std::strlen(value);
    if (builder->file) {
        if (std::fwrite(value, 1, valueLen, builder->file) != valueLen) return -1;
        return 0;
    }
    if (!builder->data) return -1;
    size_t needed = builder->length + valueLen + 1;
    if (needed > builder->capacity) {
        size_t nextCapacity = roundStringBuilderCapacity(needed);
        char* next = static_cast<char*>(std::realloc(builder->data, nextCapacity));
        if (!next) return -1;
        builder->data = next;
        builder->capacity = nextCapacity;
    }

    std::memcpy(builder->data + builder->length, value, valueLen);
    builder->length += valueLen;
    builder->data[builder->length] = '\0';
    return 0;
}

char* csec_string_builder_finish(long long handle) {
    auto* builder = reinterpret_cast<StringBuilder*>(handle);
    if (!builder) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    char* result = builder->data;
    builder->data = nullptr;
    delete builder;
    if (!result) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }
    return result;
}

int csec_string_builder_write_to_file(long long handle, const char* path) {
    auto* builder = reinterpret_cast<StringBuilder*>(handle);
    if (!builder || !path) {
        return -1;
    }

    if (builder->file) {
        int closeResult = std::fclose(builder->file);
        builder->file = nullptr;
        delete builder;
        return closeResult == 0 ? 0 : -1;
    }

    FILE* file = std::fopen(path, "wb");
    if (!file) {
        if (builder->data) {
            std::free(builder->data);
            builder->data = nullptr;
        }
        delete builder;
        return -1;
    }

    size_t written = 0;
    if (builder->data && builder->length > 0) {
        written = std::fwrite(builder->data, 1, builder->length, file);
    }
    int closeResult = std::fclose(file);
    int ok = (written == builder->length && closeResult == 0) ? 0 : -1;

    if (builder->data) {
        std::free(builder->data);
        builder->data = nullptr;
    }
    delete builder;
    return ok;
}

char* csec_token_append_owned(const char* tokens, char kind, const char* text) {
    const char* lhs = tokens ? tokens : "";
    const char* tokenText = text ? text : "";
    size_t lhsLen = std::strlen(lhs);
    size_t textLen = std::strlen(tokenText);
    char* result = static_cast<char*>(std::malloc(lhsLen + textLen + 4));
    if (!result) return nullptr;

    std::memcpy(result, lhs, lhsLen);
    result[lhsLen] = kind;
    result[lhsLen + 1] = ':';
    std::memcpy(result + lhsLen + 2, tokenText, textLen);
    result[lhsLen + textLen + 2] = '\n';
    result[lhsLen + textLen + 3] = '\0';

    if (tokens && lhsLen > 0) {
        clearTokenCachesFor(tokens);
    }
    return result;
}

long long csec_token_builder_new(void) {
    auto* builder = new TokenBuilder();
    builder->capacity = 4096;
    builder->data = static_cast<char*>(std::malloc(builder->capacity));
    if (!builder->data) {
        delete builder;
        return 0;
    }
    builder->data[0] = '\0';
    return reinterpret_cast<long long>(builder);
}

int csec_token_builder_append(long long handle, char kind, const char* text) {
    auto* builder = reinterpret_cast<TokenBuilder*>(handle);
    if (!builder || !builder->data) return -1;

    const char* tokenText = text ? text : "";
    size_t textLen = std::strlen(tokenText);
    size_t needed = builder->length + textLen + 4;
    if (needed > builder->capacity) {
        size_t nextCapacity = builder->capacity;
        while (nextCapacity < needed) {
            nextCapacity *= 2;
        }
        char* next = static_cast<char*>(std::realloc(builder->data, nextCapacity));
        if (!next) return -1;
        builder->data = next;
        builder->capacity = nextCapacity;
    }

    builder->data[builder->length] = kind;
    builder->data[builder->length + 1] = ':';
    std::memcpy(builder->data + builder->length + 2, tokenText, textLen);
    builder->length += textLen + 2;
    builder->data[builder->length] = '\n';
    builder->length += 1;
    builder->data[builder->length] = '\0';
    return 0;
}

char* csec_token_builder_finish(long long handle) {
    auto* builder = reinterpret_cast<TokenBuilder*>(handle);
    if (!builder) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    char* result = builder->data;
    builder->data = nullptr;
    delete builder;
    if (!result) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }
    return result;
}

}

static bool csec_lex_starts_with(const std::string& source, size_t index, const char* text) {
    size_t len = std::strlen(text);
    return index + len <= source.size() && source.compare(index, len, text) == 0;
}

static bool csec_lex_is_alpha(char ch) {
    unsigned char value = static_cast<unsigned char>(ch);
    return std::isalpha(value) != 0;
}

static bool csec_lex_is_digit(char ch) {
    unsigned char value = static_cast<unsigned char>(ch);
    return std::isdigit(value) != 0;
}

static bool csec_lex_is_identifier_start(char ch) {
    return csec_lex_is_alpha(ch) || ch == '_';
}

static bool csec_lex_is_identifier_part(char ch) {
    return csec_lex_is_identifier_start(ch) || csec_lex_is_digit(ch);
}

static bool csec_lex_is_keyword(const std::string& text) {
    static const char* const keywords[] = {
        "as", "break", "case", "class", "constexpr", "continue", "def", "else",
        "external", "false", "filter", "for", "if", "import", "in", "map",
        "match", "new", "object", "operator", "override", "pmap", "preduce",
        "reduce", "return", "super", "template", "this", "true", "typename",
        "unatomic", "unsafe", "val", "var", "while", "xor", "and", "or",
        "to", "until", "inner", "outer", "tensor", "box", "extends", "mut",
        "null", "ode", "molecule", "cfd", "protein", "cpu", "openmp", "gpu",
        "simd", "async", "await", "go"
    };
    for (const char* keyword : keywords) {
        if (text == keyword) return true;
    }
    return false;
}

static size_t csec_lex_line_comment_end(const std::string& source, size_t index) {
    size_t cursor = index + 2;
    while (cursor < source.size() && source[cursor] != '\n') {
        ++cursor;
    }
    return cursor;
}

static size_t csec_lex_block_comment_end(const std::string& source, size_t index) {
    size_t cursor = index + 2;
    int depth = 1;
    while (cursor + 1 < source.size() && depth > 0) {
        if (csec_lex_starts_with(source, cursor, "/*")) {
            ++depth;
            cursor += 2;
        } else if (csec_lex_starts_with(source, cursor, "*/")) {
            --depth;
            cursor += 2;
        } else {
            ++cursor;
        }
    }
    return cursor;
}

static size_t csec_lex_quoted_end(const std::string& source, size_t index) {
    char quote = source[index];
    size_t cursor = index + 1;
    while (cursor < source.size() && source[cursor] != quote) {
        if (source[cursor] == '\\' && cursor + 1 < source.size()) {
            cursor += 2;
        } else {
            ++cursor;
        }
    }
    if (cursor < source.size()) return cursor + 1;
    return cursor;
}

static size_t csec_lex_identifier_end(const std::string& source, size_t index) {
    size_t cursor = index + 1;
    while (cursor < source.size() && csec_lex_is_identifier_part(source[cursor])) {
        ++cursor;
    }
    return cursor;
}

static size_t csec_lex_number_end(const std::string& source, size_t index) {
    size_t cursor = index;
    if (csec_lex_starts_with(source, cursor, "0x") || csec_lex_starts_with(source, cursor, "0X") ||
        csec_lex_starts_with(source, cursor, "0b") || csec_lex_starts_with(source, cursor, "0B") ||
        csec_lex_starts_with(source, cursor, "0o") || csec_lex_starts_with(source, cursor, "0O")) {
        cursor += 2;
    }
    while (cursor < source.size() &&
           (csec_lex_is_digit(source[cursor]) || csec_lex_is_alpha(source[cursor]) ||
            source[cursor] == '_' || source[cursor] == '.')) {
        ++cursor;
    }
    return cursor;
}

static size_t csec_lex_operator_width(const std::string& source, size_t index) {
    // Multi-character operators for the self-host lexer. The canonical set lives in docs/grammar.md and
    // the csec++ lexer's initializeOperators() (src/lexer.cpp); keep the three in sync. Single-character
    // operators are handled separately by the caller.
    static const char* const operators[] = {
        "[@", "=>", "<-", "->", "<=", ">=", "==", "&&", "||", "++", "--",
        "!=", "+=", "-=", "*=", "/=", "%=", "<<", ">>", "..", "$$", "**"
    };
    for (const char* op : operators) {
        if (csec_lex_starts_with(source, index, op)) return 2;
    }
    return 1;
}

static std::string csec_lex_slice(const std::string& source, size_t start, size_t end) {
    if (end <= start || start >= source.size()) return std::string();
    if (end > source.size()) end = source.size();
    return source.substr(start, end - start);
}

static std::string csec_lex_repair_leading_r_token(const std::string& text) {
    if (text == "eturn") return "return";
    if (text == "atio") return "ratio";
    if (text == "educe") return "reduce";
    if (text == "ange") return "range";
    if (text == "egex") return "regex";
    if (text == "Regex") return "Regex";
    return text;
}

extern "C" {

// Contract clauses (`requires`/`ensures` after a function signature, `invariant`/`decreases` after a
// while/for header) are specifications for `csec++ --verify`; they are never compiled. Remove their
// tokens so the self-host emitter sees an ordinary signature or loop header. A token stream without
// clauses is returned unchanged.
static char* csec_strip_contract_clauses(char* tokens) {
    if (!tokens) return tokens;
    std::vector<std::pair<char, std::string>> items;
    {
        const char* line = tokens;
        while (*line) {
            const char* end = std::strchr(line, '\n');
            const size_t length = end ? static_cast<size_t>(end - line) : std::strlen(line);
            if (length >= 2) items.emplace_back(line[0], std::string(line + 2, length - 2));
            if (!end) break;
            line = end + 1;
        }
    }
    auto is = [&](size_t i, char kind, const char* text) {
        return i < items.size() && items[i].first == kind && items[i].second == text;
    };
    auto closing = [&](size_t open) -> size_t {
        int depth = 0;
        for (size_t i = open; i < items.size(); ++i) {
            if (is(i, 'O', "(")) ++depth;
            else if (is(i, 'O', ")") && --depth == 0) return i;
        }
        return items.size();
    };
    auto clauseWord = [&](size_t i, bool function) {
        if (i >= items.size() || items[i].first != 'I') return false;
        const std::string& t = items[i].second;
        return function ? (t == "requires" || t == "ensures") : (t == "invariant" || t == "decreases");
    };
    // Drops the clauses starting at `i`; returns the index of the body ('{' or, for functions, '=').
    std::vector<bool> keep(items.size(), true);
    bool stripped = false;
    auto dropClauses = [&](size_t i, bool function) {
        while (clauseWord(i, function)) {
            int depth = 0;
            keep[i++] = false;
            stripped = true;
            for (; i < items.size(); ++i) {
                if (depth == 0 && (clauseWord(i, function) || is(i, 'O', "{") || (function && is(i, 'O', "=")))) break;
                if (is(i, 'O', "(") || is(i, 'O', "[")) ++depth;
                else if ((is(i, 'O', ")") || is(i, 'O', "]")) && depth > 0) --depth;
                keep[i] = false;
            }
        }
    };
    for (size_t i = 0; i < items.size(); ++i) {
        if (is(i, 'K', "def")) {
            size_t open = i + 1;
            while (open < items.size() && !is(open, 'O', "(") && !is(open, 'O', "{") && !is(open, 'O', ";")) ++open;
            if (!is(open, 'O', "(")) continue;
            size_t j = closing(open) + 1;
            if (is(j, 'O', ":")) {
                // Skip the return type: up to the body or a clause word, outside brackets.
                int depth = 0;
                for (++j; j < items.size(); ++j) {
                    if (depth == 0 && (is(j, 'O', "{") || is(j, 'O', "=") || is(j, 'O', ";") || clauseWord(j, true))) break;
                    if (is(j, 'O', "(") || is(j, 'O', "[") || is(j, 'O', "<")) ++depth;
                    else if ((is(j, 'O', ")") || is(j, 'O', "]") || is(j, 'O', ">")) && depth > 0) --depth;
                    else if (is(j, 'O', ">>")) depth = depth > 1 ? depth - 2 : 0;
                }
            }
            dropClauses(j, true);
        }
        else if ((is(i, 'K', "while") || is(i, 'K', "for")) && is(i + 1, 'O', "(")) {
            dropClauses(closing(i + 1) + 1, false);
        }
    }
    if (!stripped) return tokens;
    std::string rebuilt;
    for (size_t i = 0; i < items.size(); ++i) {
        if (!keep[i]) continue;
        rebuilt += items[i].first;
        rebuilt += ':';
        rebuilt += items[i].second;
        rebuilt += '\n';
    }
    char* result = static_cast<char*>(std::malloc(rebuilt.size() + 1));
    if (!result) return tokens;
    std::memcpy(result, rebuilt.c_str(), rebuilt.size() + 1);
    std::free(tokens);
    return result;
}

char* csec_tokenize_source(const char* sourceText) {
    std::string source = sourceText ? sourceText : "";
    long long builder = csec_token_builder_new();
    if (!builder) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t cursor = 0;
    while (cursor < source.size()) {
        char ch = source[cursor];
        if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
            ++cursor;
        } else if (csec_lex_starts_with(source, cursor, "//")) {
            cursor = csec_lex_line_comment_end(source, cursor);
        } else if (csec_lex_starts_with(source, cursor, "/*")) {
            cursor = csec_lex_block_comment_end(source, cursor);
        } else if ((ch == 'r' || ch == 'R') && cursor + 1 < source.size() &&
                   csec_lex_is_identifier_part(source[cursor + 1])) {
            size_t end = csec_lex_identifier_end(source, cursor);
            std::string text(1, ch);
            text += csec_lex_slice(source, cursor + 1, end);
            text = csec_lex_repair_leading_r_token(text);
            char kind = (text == "true" || text == "false") ? 'B' : (csec_lex_is_keyword(text) ? 'K' : 'I');
            csec_token_builder_append(builder, kind, text.c_str());
            cursor = end;
        } else if ((ch == 'r' || ch == 'R') && cursor + 1 < source.size() &&
                   !csec_lex_is_identifier_part(source[cursor + 1]) && source[cursor + 1] == '"') {
            size_t end = csec_lex_quoted_end(source, cursor + 1);
            std::string text = csec_lex_slice(source, cursor + 2, end > 0 ? end - 1 : end);
            csec_token_builder_append(builder, 'R', text.c_str());
            cursor = end;
        } else if ((ch == 'u' || ch == 'U') && cursor + 1 < source.size() && source[cursor + 1] == '"') {
            size_t end = csec_lex_quoted_end(source, cursor + 1);
            std::string text = csec_lex_slice(source, cursor + 2, end > 0 ? end - 1 : end);
            csec_token_builder_append(builder, 'S', text.c_str());
            cursor = end;
        } else if (ch == '"') {
            size_t end = csec_lex_quoted_end(source, cursor);
            std::string text = csec_lex_slice(source, cursor + 1, end > 0 ? end - 1 : end);
            csec_token_builder_append(builder, 'S', text.c_str());
            cursor = end;
        } else if (ch == '\'') {
            size_t end = csec_lex_quoted_end(source, cursor);
            std::string text = csec_lex_slice(source, cursor + 1, end > 0 ? end - 1 : end);
            csec_token_builder_append(builder, 'C', text.c_str());
            cursor = end;
        } else if (csec_lex_is_digit(ch)) {
            size_t end = csec_lex_number_end(source, cursor);
            std::string text = csec_lex_slice(source, cursor, end);
            char kind = (text.find('.') != std::string::npos || text.find('e') != std::string::npos ||
                         text.find('E') != std::string::npos) ? 'F' : 'N';
            csec_token_builder_append(builder, kind, text.c_str());
            cursor = end;
        } else if (csec_lex_is_identifier_start(ch)) {
            size_t end = csec_lex_identifier_end(source, cursor);
            std::string text = csec_lex_slice(source, cursor, end);
            if (ch == 'r' || ch == 'R') {
                text = std::string(1, ch) + csec_lex_slice(source, cursor + 1, end);
            }
            text = csec_lex_repair_leading_r_token(text);
            char kind = (text == "true" || text == "false") ? 'B' : (csec_lex_is_keyword(text) ? 'K' : 'I');
            csec_token_builder_append(builder, kind, text.c_str());
            cursor = end;
        } else {
            size_t width = csec_lex_operator_width(source, cursor);
            std::string text = csec_lex_slice(source, cursor, cursor + width);
            csec_token_builder_append(builder, 'O', text.c_str());
            cursor += width;
        }
    }

    csec_token_builder_append(builder, 'E', "<eof>");
    return csec_strip_contract_clauses(csec_token_builder_finish(builder));
}

char csec_token_kind_at(const char* tokens, int ordinal) {
    const char* value = tokens ? tokens : "";
    if (ordinal < 0) return 'E';
    const auto& starts = tokenLineStarts(value);
    if (static_cast<size_t>(ordinal) >= starts.size()) {
        return 'E';
    }
    int start = starts[static_cast<size_t>(ordinal)];
    return value[start] == '\0' ? 'E' : value[start];
}
