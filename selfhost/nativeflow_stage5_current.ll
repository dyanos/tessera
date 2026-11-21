; ModuleID = 'csec.selfhost'
source_filename = "csec.selfhost"

declare ptr @csec_append_expression_until(ptr, i32, i32)
declare ptr @csec_generate_symbol_table(ptr)
declare ptr @csec_ir_type_name(ptr)
declare ptr @csec_collect_type_name(ptr, i32, i32)
declare ptr @csec_lookup_visible_value_type(ptr, i32, ptr)
declare ptr @csec_lookup_visible_storage_name(ptr, i32, ptr)
declare i32 @csec_find_function_param_start(ptr, i32)
declare i32 @csec_count_comma_separated(ptr, i32, i32)
declare i32 @csec_count_function_params(ptr, i32)
declare i32 @csec_expression_top_level_operator(ptr, i32, i32)
declare i32 @csec_find_closing_token(ptr, i32, i32, ptr, ptr)
declare i32 @csec_find_last_top_level_token(ptr, i32, i32, ptr)
declare i32 @csec_find_statement_block_end(ptr, i32, i32)
declare i32 @csec_find_statement_block_start(ptr, i32, i32)
declare i32 @csec_find_statement_paren_end(ptr, i32, i32)
declare i32 @csec_find_statement_paren_start(ptr, i32, i32)
declare i32 @csec_find_token_text_in_range(ptr, i32, i32, ptr)
declare i32 @csec_find_top_level_match(ptr, i32, i32)
declare i32 @csec_find_top_level_operator(ptr, i32, i32, i32)
declare ptr @csec_generate_body_ast(ptr, i32, i32)
declare ptr @csec_generate_c_body(ptr, i32, i32, ptr)
declare ptr @csec_generate_c_expression(ptr, i32, i32)
declare ptr @csec_generate_expression_ast(ptr, i32, i32)
declare ptr @csec_generate_statement_ast(ptr, i32, i32)
declare ptr @csec_ir_operator_name(ptr)
declare i32 @csec_is_keyword(ptr)
declare i32 @csec_line_end(ptr, i32)
declare i32 @csec_line_start(ptr, i32)
declare i32 @csec_starts_top_level_declaration(ptr, i32)
declare i8 @csec_string_char_at(ptr, i32)
declare ptr @csec_string_concat(ptr, ptr)
declare i32 @csec_string_contains(ptr, ptr)
declare i32 @csec_string_ends_with(ptr, ptr)
declare i32 @csec_string_equals(ptr, ptr)
declare i64 @csec_string_index_of(ptr, ptr)
declare i32 @csec_string_is_empty(ptr)
declare i64 @csec_string_length(ptr)
declare i32 @csec_string_starts_with(ptr, ptr)
declare ptr @csec_string_substring(ptr, i32, i32)
declare ptr @csec_string_to_lower(ptr)
declare ptr @csec_string_to_upper(ptr)
declare ptr @csec_string_trim(ptr)
declare void @csec_print_i64(i64)
declare void @csec_print_newline()
declare void @csec_print_string(ptr)
declare i32 @csec_command_line_arg_count()
declare ptr @csec_command_line_arg(i32)
declare i32 @csec_to_int(ptr)
declare ptr @csec_to_string_char(i8)
declare ptr @csec_to_string_i64(i64)
declare i32 @csec_token_is_top_level_operator(ptr, i32, i32)
declare ptr @csec_top_level_decl_kind(ptr, i32)
declare ptr @csec_top_level_decl_name(ptr, i32)
declare i32 @csec_validate_balanced(ptr)
declare i32 @csec_validate_top_level(ptr)

@.str.332 = private unnamed_addr constant [2 x i8] c"\0D\00"
@.str.585 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.592 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.599 = private unnamed_addr constant [8 x i8] c"extends\00"
@.str.606 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.613 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.620 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.627 = private unnamed_addr constant [9 x i8] c"operator\00"
@.str.634 = private unnamed_addr constant [9 x i8] c"override\00"
@.str.641 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.648 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.655 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.662 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.669 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.676 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.683 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.690 = private unnamed_addr constant [6 x i8] c"match\00"
@.str.697 = private unnamed_addr constant [5 x i8] c"case\00"
@.str.704 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.711 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.718 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.725 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.732 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.739 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.746 = private unnamed_addr constant [9 x i8] c"typename\00"
@.str.753 = private unnamed_addr constant [4 x i8] c"new\00"
@.str.760 = private unnamed_addr constant [5 x i8] c"this\00"
@.str.767 = private unnamed_addr constant [6 x i8] c"super\00"
@.str.774 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.781 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.788 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.795 = private unnamed_addr constant [4 x i8] c"box\00"
@.str.802 = private unnamed_addr constant [4 x i8] c"mut\00"
@.str.809 = private unnamed_addr constant [5 x i8] c"null\00"
@.str.816 = private unnamed_addr constant [6 x i8] c"inner\00"
@.str.823 = private unnamed_addr constant [6 x i8] c"outer\00"
@.str.830 = private unnamed_addr constant [7 x i8] c"tensor\00"
@.str.837 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.844 = private unnamed_addr constant [6 x i8] c"until\00"
@.str.851 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.858 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.865 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.872 = private unnamed_addr constant [4 x i8] c"ode\00"
@.str.879 = private unnamed_addr constant [9 x i8] c"molecule\00"
@.str.886 = private unnamed_addr constant [4 x i8] c"cfd\00"
@.str.893 = private unnamed_addr constant [8 x i8] c"protein\00"
@.str.900 = private unnamed_addr constant [4 x i8] c"cpu\00"
@.str.907 = private unnamed_addr constant [7 x i8] c"openmp\00"
@.str.914 = private unnamed_addr constant [4 x i8] c"gpu\00"
@.str.921 = private unnamed_addr constant [5 x i8] c"simd\00"
@.str.1266 = private unnamed_addr constant [3 x i8] c"[@\00"
@.str.1282 = private unnamed_addr constant [3 x i8] c"=>\00"
@.str.1291 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.1300 = private unnamed_addr constant [3 x i8] c"->\00"
@.str.1309 = private unnamed_addr constant [3 x i8] c"<=\00"
@.str.1318 = private unnamed_addr constant [3 x i8] c">=\00"
@.str.1327 = private unnamed_addr constant [3 x i8] c"==\00"
@.str.1336 = private unnamed_addr constant [3 x i8] c"&&\00"
@.str.1345 = private unnamed_addr constant [3 x i8] c"||\00"
@.str.1354 = private unnamed_addr constant [3 x i8] c"++\00"
@.str.1363 = private unnamed_addr constant [3 x i8] c"--\00"
@.str.1372 = private unnamed_addr constant [3 x i8] c"!=\00"
@.str.1381 = private unnamed_addr constant [3 x i8] c"+=\00"
@.str.1390 = private unnamed_addr constant [3 x i8] c"-=\00"
@.str.1399 = private unnamed_addr constant [3 x i8] c"*=\00"
@.str.1408 = private unnamed_addr constant [3 x i8] c"/=\00"
@.str.1417 = private unnamed_addr constant [3 x i8] c"%=\00"
@.str.1426 = private unnamed_addr constant [3 x i8] c"<<\00"
@.str.1435 = private unnamed_addr constant [3 x i8] c">>\00"
@.str.1444 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.1453 = private unnamed_addr constant [3 x i8] c"$$\00"
@.str.1462 = private unnamed_addr constant [3 x i8] c"**\00"
@.str.1490 = private unnamed_addr constant [6 x i8] c"eturn\00"
@.str.1495 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.1504 = private unnamed_addr constant [5 x i8] c"atio\00"
@.str.1509 = private unnamed_addr constant [6 x i8] c"ratio\00"
@.str.1518 = private unnamed_addr constant [6 x i8] c"educe\00"
@.str.1523 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.1532 = private unnamed_addr constant [5 x i8] c"ange\00"
@.str.1537 = private unnamed_addr constant [6 x i8] c"range\00"
@.str.1546 = private unnamed_addr constant [5 x i8] c"egex\00"
@.str.1551 = private unnamed_addr constant [6 x i8] c"regex\00"
@.str.1560 = private unnamed_addr constant [6 x i8] c"Regex\00"
@.str.1565 = private unnamed_addr constant [6 x i8] c"Regex\00"
@.str.1598 = private unnamed_addr constant [1 x i8] c"\00"
@.str.1676 = private unnamed_addr constant [1 x i8] c"\00"
@.str.1727 = private unnamed_addr constant [3 x i8] c"//\00"
@.str.1754 = private unnamed_addr constant [3 x i8] c"/*\00"
@.str.1817 = private unnamed_addr constant [2 x i8] c"r\00"
@.str.1839 = private unnamed_addr constant [2 x i8] c"R\00"
@.str.1866 = private unnamed_addr constant [5 x i8] c"true\00"
@.str.1873 = private unnamed_addr constant [6 x i8] c"false\00"
@.str.2207 = private unnamed_addr constant [2 x i8] c".\00"
@.str.2214 = private unnamed_addr constant [2 x i8] c"e\00"
@.str.2221 = private unnamed_addr constant [2 x i8] c"E\00"
@.str.2294 = private unnamed_addr constant [2 x i8] c"r\00"
@.str.2317 = private unnamed_addr constant [2 x i8] c"R\00"
@.str.2344 = private unnamed_addr constant [5 x i8] c"true\00"
@.str.2351 = private unnamed_addr constant [6 x i8] c"false\00"
@.str.2454 = private unnamed_addr constant [6 x i8] c"<eof>\00"
@.str.2959 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.2976 = private unnamed_addr constant [2 x i8] c")\00"
@.str.2993 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.3010 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.3027 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.3044 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.3159 = private unnamed_addr constant [3 x i8] c"[@\00"
@.str.3179 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.3186 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.3193 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.3200 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.3207 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.3214 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.3221 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.3228 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.3235 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.3242 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.3249 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.3394 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.3411 = private unnamed_addr constant [2 x i8] c")\00"
@.str.3428 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.3445 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.3482 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.3499 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.3516 = private unnamed_addr constant [2 x i8] c";\00"
@.str.3585 = private unnamed_addr constant [33 x i8] c"selfhost/selfhost_llvm_trace.txt\00"
@.str.3587 = private unnamed_addr constant [9 x i8] c"balanced\00"
@.str.3640 = private unnamed_addr constant [3 x i8] c"[@\00"
@.str.3645 = private unnamed_addr constant [10 x i8] c"attribute\00"
@.str.3663 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.3672 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.3696 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.3701 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.3710 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.3715 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.3724 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.3729 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.3733 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.3742 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.3749 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.3756 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.3777 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.3782 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.3791 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.3796 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.3805 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.3810 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.3819 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.3824 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.3833 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.3838 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.3847 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.3852 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.3861 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.3866 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.3870 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.3905 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.3912 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.3919 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.3940 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.3992 = private unnamed_addr constant [2 x i8] c"<\00"
@.str.4009 = private unnamed_addr constant [2 x i8] c">\00"
@.str.4046 = private unnamed_addr constant [11 x i8] c"<template>\00"
@.str.4055 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.4076 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.4093 = private unnamed_addr constant [9 x i8] c"operator\00"
@.str.4098 = private unnamed_addr constant [9 x i8] c"operator\00"
@.str.4127 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.4134 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.4141 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.4148 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.4164 = private unnamed_addr constant [12 x i8] c"<anonymous>\00"
@.str.4308 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.4326 = private unnamed_addr constant [2 x i8] c")\00"
@.str.4344 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.4362 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.4401 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.4419 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.4437 = private unnamed_addr constant [2 x i8] c";\00"
@.str.4560 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.4582 = private unnamed_addr constant [2 x i8] c";\00"
@.str.4712 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.4730 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.4818 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.4823 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.4832 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.4837 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.4846 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.4851 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.4860 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.4865 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.4874 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.4879 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.4888 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.4893 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.4902 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.4907 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.4916 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.4921 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.4930 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.4935 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.4944 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.4949 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.4958 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.4963 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.4972 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.4977 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.4986 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.4991 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.5000 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.5005 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.5014 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.5019 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.5028 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.5033 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.5047 = private unnamed_addr constant [4 x i8] c"eof\00"
@.str.5051 = private unnamed_addr constant [11 x i8] c"expression\00"
@.str.5084 = private unnamed_addr constant [1 x i8] c"\00"
@.str.5128 = private unnamed_addr constant [2 x i8] c" \00"
@.str.5202 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.5209 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.5216 = private unnamed_addr constant [3 x i8] c"+=\00"
@.str.5223 = private unnamed_addr constant [3 x i8] c"-=\00"
@.str.5230 = private unnamed_addr constant [3 x i8] c"*=\00"
@.str.5237 = private unnamed_addr constant [3 x i8] c"/=\00"
@.str.5244 = private unnamed_addr constant [3 x i8] c"%=\00"
@.str.5267 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.5283 = private unnamed_addr constant [3 x i8] c"||\00"
@.str.5290 = private unnamed_addr constant [2 x i8] c"|\00"
@.str.5315 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.5330 = private unnamed_addr constant [3 x i8] c"&&\00"
@.str.5354 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.5369 = private unnamed_addr constant [2 x i8] c"^\00"
@.str.5392 = private unnamed_addr constant [2 x i8] c"&\00"
@.str.5414 = private unnamed_addr constant [3 x i8] c"==\00"
@.str.5421 = private unnamed_addr constant [3 x i8] c"!=\00"
@.str.5428 = private unnamed_addr constant [2 x i8] c"<\00"
@.str.5435 = private unnamed_addr constant [2 x i8] c">\00"
@.str.5442 = private unnamed_addr constant [3 x i8] c"<=\00"
@.str.5449 = private unnamed_addr constant [3 x i8] c">=\00"
@.str.5471 = private unnamed_addr constant [3 x i8] c"<<\00"
@.str.5478 = private unnamed_addr constant [3 x i8] c">>\00"
@.str.5501 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.5517 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.5524 = private unnamed_addr constant [6 x i8] c"until\00"
@.str.5548 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.5555 = private unnamed_addr constant [2 x i8] c"-\00"
@.str.5577 = private unnamed_addr constant [2 x i8] c"*\00"
@.str.5584 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.5591 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.5598 = private unnamed_addr constant [2 x i8] c"@\00"
@.str.5620 = private unnamed_addr constant [6 x i8] c"inner\00"
@.str.5627 = private unnamed_addr constant [6 x i8] c"outer\00"
@.str.5634 = private unnamed_addr constant [7 x i8] c"tensor\00"
@.str.5656 = private unnamed_addr constant [2 x i8] c",\00"
@.str.5755 = private unnamed_addr constant [2 x i8] c")\00"
@.str.5773 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.5791 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.5809 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.5827 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.5845 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.5993 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.6011 = private unnamed_addr constant [2 x i8] c")\00"
@.str.6029 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.6047 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.6065 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.6083 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.6119 = private unnamed_addr constant [6 x i8] c"match\00"
@.str.6352 = private unnamed_addr constant [2 x i8] c";\00"
@.str.6392 = private unnamed_addr constant [6 x i8] c"empty\00"
@.str.6417 = private unnamed_addr constant [8 x i8] c"integer\00"
@.str.6430 = private unnamed_addr constant [6 x i8] c"float\00"
@.str.6443 = private unnamed_addr constant [7 x i8] c"string\00"
@.str.6456 = private unnamed_addr constant [6 x i8] c"regex\00"
@.str.6469 = private unnamed_addr constant [5 x i8] c"char\00"
@.str.6482 = private unnamed_addr constant [5 x i8] c"bool\00"
@.str.6495 = private unnamed_addr constant [11 x i8] c"identifier\00"
@.str.6525 = private unnamed_addr constant [5 x i8] c"this\00"
@.str.6530 = private unnamed_addr constant [5 x i8] c"this\00"
@.str.6539 = private unnamed_addr constant [6 x i8] c"super\00"
@.str.6544 = private unnamed_addr constant [6 x i8] c"super\00"
@.str.6553 = private unnamed_addr constant [4 x i8] c"new\00"
@.str.6558 = private unnamed_addr constant [4 x i8] c"new\00"
@.str.6567 = private unnamed_addr constant [6 x i8] c"match\00"
@.str.6572 = private unnamed_addr constant [6 x i8] c"match\00"
@.str.6577 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.6689 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.6716 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.6718 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.6767 = private unnamed_addr constant [5 x i8] c"case\00"
@.str.6816 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.6873 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.6875 = private unnamed_addr constant [2 x i8] c")\00"
@.str.6929 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.6942 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.6999 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.7001 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.7076 = private unnamed_addr constant [1 x i8] c"\00"
@.str.7193 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.7211 = private unnamed_addr constant [2 x i8] c")\00"
@.str.7229 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.7247 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.7265 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.7283 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.7301 = private unnamed_addr constant [2 x i8] c",\00"
@.str.7427 = private unnamed_addr constant [2 x i8] c")\00"
@.str.7445 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.7463 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.7481 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.7499 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.7517 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.7593 = private unnamed_addr constant [1 x i8] c"\00"
@.str.7610 = private unnamed_addr constant [2 x i8] c")\00"
@.str.7627 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.7638 = private unnamed_addr constant [18 x i8] c"Expr call target=\00"
@.str.7649 = private unnamed_addr constant [7 x i8] c" argc=\00"
@.str.7680 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.7697 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.7708 = private unnamed_addr constant [19 x i8] c"Expr index target=\00"
@.str.7719 = private unnamed_addr constant [7 x i8] c" argc=\00"
@.str.7749 = private unnamed_addr constant [2 x i8] c".\00"
@.str.7760 = private unnamed_addr constant [13 x i8] c"Expr member \00"
@.str.7771 = private unnamed_addr constant [2 x i8] c".\00"
@.str.7786 = private unnamed_addr constant [1 x i8] c"\00"
@.str.7820 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.7822 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.7864 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.7886 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.7888 = private unnamed_addr constant [2 x i8] c")\00"
@.str.7929 = private unnamed_addr constant [3 x i8] c"->\00"
@.str.7973 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.7975 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.7988 = private unnamed_addr constant [5 x i8] c"none\00"
@.str.8035 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.8037 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8078 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.8099 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.8101 = private unnamed_addr constant [2 x i8] c")\00"
@.str.8148 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.8153 = private unnamed_addr constant [1 x i8] c"\00"
@.str.8178 = private unnamed_addr constant [1 x i8] c"\00"
@.str.8182 = private unnamed_addr constant [21 x i8] c"Expr lambda capture=\00"
@.str.8193 = private unnamed_addr constant [9 x i8] c" params=\00"
@.str.8257 = private unnamed_addr constant [11 x i8] c"Expr empty\00"
@.str.8270 = private unnamed_addr constant [6 x i8] c"Expr \00"
@.str.8281 = private unnamed_addr constant [2 x i8] c" \00"
@.str.8342 = private unnamed_addr constant [18 x i8] c"Expr match cases=\00"
@.str.8353 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8364 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8379 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.8396 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.8398 = private unnamed_addr constant [2 x i8] c")\00"
@.str.8411 = private unnamed_addr constant [13 x i8] c"Expr group (\00"
@.str.8426 = private unnamed_addr constant [2 x i8] c")\00"
@.str.8454 = private unnamed_addr constant [13 x i8] c"Expr assign \00"
@.str.8463 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8474 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8487 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8511 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8520 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8531 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8544 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8568 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8577 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8588 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8601 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8625 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8634 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8645 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8658 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8682 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8691 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8702 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8715 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8739 = private unnamed_addr constant [14 x i8] c"Expr compare \00"
@.str.8748 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8759 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8772 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8796 = private unnamed_addr constant [12 x i8] c"Expr shift \00"
@.str.8805 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8816 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8829 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8853 = private unnamed_addr constant [12 x i8] c"Expr range \00"
@.str.8862 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8873 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8886 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8910 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8919 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8930 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.8943 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.8967 = private unnamed_addr constant [13 x i8] c"Expr binary \00"
@.str.8976 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.8987 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.9000 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9024 = private unnamed_addr constant [13 x i8] c"Expr tensor \00"
@.str.9033 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.9044 = private unnamed_addr constant [4 x i8] c"] [\00"
@.str.9057 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9083 = private unnamed_addr constant [2 x i8] c"!\00"
@.str.9095 = private unnamed_addr constant [2 x i8] c"-\00"
@.str.9107 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.9119 = private unnamed_addr constant [2 x i8] c"~\00"
@.str.9131 = private unnamed_addr constant [2 x i8] c"*\00"
@.str.9143 = private unnamed_addr constant [2 x i8] c"&\00"
@.str.9155 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.9167 = private unnamed_addr constant [3 x i8] c"++\00"
@.str.9179 = private unnamed_addr constant [3 x i8] c"--\00"
@.str.9185 = private unnamed_addr constant [12 x i8] c"Expr unary \00"
@.str.9194 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.9207 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9243 = private unnamed_addr constant [3 x i8] c"++\00"
@.str.9257 = private unnamed_addr constant [3 x i8] c"--\00"
@.str.9263 = private unnamed_addr constant [14 x i8] c"Expr postfix \00"
@.str.9274 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.9287 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9348 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.9361 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.9363 = private unnamed_addr constant [2 x i8] c")\00"
@.str.9372 = private unnamed_addr constant [11 x i8] c"Expr call \00"
@.str.9381 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.9396 = private unnamed_addr constant [2 x i8] c")\00"
@.str.9411 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.9416 = private unnamed_addr constant [18 x i8] c"Expr array count=\00"
@.str.9431 = private unnamed_addr constant [3 x i8] c" [\00"
@.str.9446 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9471 = private unnamed_addr constant [4 x i8] c"new\00"
@.str.9476 = private unnamed_addr constant [10 x i8] c"Expr new \00"
@.str.9512 = private unnamed_addr constant [6 x i8] c"match\00"
@.str.9517 = private unnamed_addr constant [11 x i8] c"Expr match\00"
@.str.9521 = private unnamed_addr constant [6 x i8] c"Expr \00"
@.str.9532 = private unnamed_addr constant [2 x i8] c" \00"
@.str.9669 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.9686 = private unnamed_addr constant [2 x i8] c")\00"
@.str.9703 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.9720 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.9790 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.9803 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.9833 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.9860 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.9877 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.9879 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.9910 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.9927 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.9944 = private unnamed_addr constant [2 x i8] c";\00"
@.str.10014 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.10019 = private unnamed_addr constant [15 x i8] c"  Stmt return \00"
@.str.10032 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10041 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.10048 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.10076 = private unnamed_addr constant [8 x i8] c"  Stmt \00"
@.str.10080 = private unnamed_addr constant [2 x i8] c" \00"
@.str.10091 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.10104 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10108 = private unnamed_addr constant [8 x i8] c"  Stmt \00"
@.str.10112 = private unnamed_addr constant [2 x i8] c" \00"
@.str.10123 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10132 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.10139 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.10146 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.10153 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.10160 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.10167 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.10174 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.10181 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.10186 = private unnamed_addr constant [8 x i8] c"  Stmt \00"
@.str.10190 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.10201 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.10210 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.10217 = private unnamed_addr constant [9 x i8] c"unatomic\00"
@.str.10224 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.10229 = private unnamed_addr constant [8 x i8] c"  Stmt \00"
@.str.10233 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10242 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.10249 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.10254 = private unnamed_addr constant [8 x i8] c"  Stmt \00"
@.str.10258 = private unnamed_addr constant [2 x i8] c" \00"
@.str.10269 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10273 = private unnamed_addr constant [19 x i8] c"  Stmt expression \00"
@.str.10284 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.10310 = private unnamed_addr constant [1 x i8] c"\00"
@.str.10370 = private unnamed_addr constant [14 x i8] c"  Stmt error\0A\00"
@.str.10447 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.10467 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.10480 = private unnamed_addr constant [2 x i8] c";\00"
@.str.10557 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.10559 = private unnamed_addr constant [2 x i8] c")\00"
@.str.10588 = private unnamed_addr constant [2 x i8] c":\00"
@.str.10633 = private unnamed_addr constant [1 x i8] c"\00"
@.str.10671 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.10678 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.10685 = private unnamed_addr constant [2 x i8] c";\00"
@.str.10692 = private unnamed_addr constant [2 x i8] c",\00"
@.str.10774 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.10813 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.10875 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.10877 = private unnamed_addr constant [2 x i8] c")\00"
@.str.10928 = private unnamed_addr constant [2 x i8] c",\00"
@.str.10980 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.10997 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.10999 = private unnamed_addr constant [2 x i8] c")\00"
@.str.11020 = private unnamed_addr constant [3 x i8] c"=>\00"
@.str.11025 = private unnamed_addr constant [21 x i8] c"FunctionType params=\00"
@.str.11038 = private unnamed_addr constant [10 x i8] c" returns=\00"
@.str.11119 = private unnamed_addr constant [2 x i8] c":\00"
@.str.11148 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.11161 = private unnamed_addr constant [2 x i8] c";\00"
@.str.11166 = private unnamed_addr constant [6 x i8] c"infer\00"
@.str.11177 = private unnamed_addr constant [6 x i8] c"infer\00"
@.str.11213 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11266 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.11273 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.11315 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11370 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.11395 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.11408 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.11421 = private unnamed_addr constant [7 x i8] c"String\00"
@.str.11434 = private unnamed_addr constant [6 x i8] c"Regex\00"
@.str.11447 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.11460 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.11487 = private unnamed_addr constant [9 x i8] c"Function\00"
@.str.11512 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11539 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.11563 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.11587 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.11704 = private unnamed_addr constant [6 x i8] c"Range\00"
@.str.11822 = private unnamed_addr constant [4 x i8] c"new\00"
@.str.11851 = private unnamed_addr constant [2 x i8] c"[\00"
@.str.11856 = private unnamed_addr constant [6 x i8] c"Array\00"
@.str.11879 = private unnamed_addr constant [10 x i8] c"Expr call\00"
@.str.11903 = private unnamed_addr constant [11 x i8] c"Expr index\00"
@.str.11908 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11917 = private unnamed_addr constant [12 x i8] c"Expr member\00"
@.str.11922 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11940 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.11944 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.12002 = private unnamed_addr constant [2 x i8] c":\00"
@.str.12031 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.12044 = private unnamed_addr constant [2 x i8] c";\00"
@.str.12049 = private unnamed_addr constant [1 x i8] c"\00"
@.str.12060 = private unnamed_addr constant [1 x i8] c"\00"
@.str.12145 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.12175 = private unnamed_addr constant [1 x i8] c"\00"
@.str.12243 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.12250 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.12258 = private unnamed_addr constant [9 x i8] c"  Local \00"
@.str.12262 = private unnamed_addr constant [2 x i8] c".\00"
@.str.12273 = private unnamed_addr constant [6 x i8] c" mut=\00"
@.str.12277 = private unnamed_addr constant [7 x i8] c" type=\00"
@.str.12288 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.12298 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.12305 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.12312 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.12319 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.12326 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.12333 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.12340 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.12347 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.12355 = private unnamed_addr constant [9 x i8] c"  Block \00"
@.str.12359 = private unnamed_addr constant [2 x i8] c".\00"
@.str.12363 = private unnamed_addr constant [9 x i8] c" header=\00"
@.str.12374 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.12417 = private unnamed_addr constant [10 x i8] c"  Assign \00"
@.str.12421 = private unnamed_addr constant [2 x i8] c".\00"
@.str.12430 = private unnamed_addr constant [5 x i8] c" op=\00"
@.str.12439 = private unnamed_addr constant [7 x i8] c" type=\00"
@.str.12452 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.12508 = private unnamed_addr constant [1 x i8] c"\00"
@.str.12529 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.12531 = private unnamed_addr constant [2 x i8] c")\00"
@.str.12548 = private unnamed_addr constant [1 x i8] c"\00"
@.str.12587 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.12603 = private unnamed_addr constant [2 x i8] c":\00"
@.str.12625 = private unnamed_addr constant [9 x i8] c"  Param \00"
@.str.12629 = private unnamed_addr constant [2 x i8] c".\00"
@.str.12638 = private unnamed_addr constant [7 x i8] c" type=\00"
@.str.12642 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.12662 = private unnamed_addr constant [2 x i8] c",\00"
@.str.12685 = private unnamed_addr constant [2 x i8] c",\00"
@.str.12727 = private unnamed_addr constant [9 x i8] c"override\00"
@.str.12740 = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.12753 = private unnamed_addr constant [10 x i8] c"constexpr\00"
@.str.12805 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.12810 = private unnamed_addr constant [7 x i8] c"method\00"
@.str.12825 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.12830 = private unnamed_addr constant [6 x i8] c"field\00"
@.str.12845 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.12850 = private unnamed_addr constant [14 x i8] c"mutable-field\00"
@.str.12854 = private unnamed_addr constant [7 x i8] c"member\00"
@.str.12895 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.12913 = private unnamed_addr constant [9 x i8] c"operator\00"
@.str.12918 = private unnamed_addr constant [9 x i8] c"operator\00"
@.str.12953 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.12966 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.12982 = private unnamed_addr constant [9 x i8] c"<member>\00"
@.str.13012 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13104 = private unnamed_addr constant [7 x i8] c"method\00"
@.str.13112 = private unnamed_addr constant [10 x i8] c"  Method \00"
@.str.13116 = private unnamed_addr constant [2 x i8] c".\00"
@.str.13120 = private unnamed_addr constant [9 x i8] c" params=\00"
@.str.13129 = private unnamed_addr constant [10 x i8] c" returns=\00"
@.str.13138 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.13148 = private unnamed_addr constant [6 x i8] c"field\00"
@.str.13155 = private unnamed_addr constant [14 x i8] c"mutable-field\00"
@.str.13163 = private unnamed_addr constant [9 x i8] c"  Field \00"
@.str.13167 = private unnamed_addr constant [2 x i8] c".\00"
@.str.13171 = private unnamed_addr constant [6 x i8] c" mut=\00"
@.str.13175 = private unnamed_addr constant [7 x i8] c" type=\00"
@.str.13186 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.13226 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13280 = private unnamed_addr constant [10 x i8] c"  Member \00"
@.str.13289 = private unnamed_addr constant [2 x i8] c" \00"
@.str.13298 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.13341 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.13346 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13367 = private unnamed_addr constant [2 x i8] c"<\00"
@.str.13378 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13395 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13421 = private unnamed_addr constant [2 x i8] c">\00"
@.str.13437 = private unnamed_addr constant [9 x i8] c"typename\00"
@.str.13450 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.13458 = private unnamed_addr constant [7 x i8] c" type \00"
@.str.13495 = private unnamed_addr constant [8 x i8] c" value \00"
@.str.13552 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.13557 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13608 = private unnamed_addr constant [2 x i8] c"<\00"
@.str.13626 = private unnamed_addr constant [2 x i8] c">\00"
@.str.13663 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.13668 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.13677 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.13682 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.13698 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13728 = private unnamed_addr constant [3 x i8] c"[@\00"
@.str.13733 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13750 = private unnamed_addr constant [1 x i8] c"\00"
@.str.13776 = private unnamed_addr constant [2 x i8] c"]\00"
@.str.13793 = private unnamed_addr constant [2 x i8] c" \00"
@.str.13850 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.13855 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.13864 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.13869 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.13878 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.13883 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.13910 = private unnamed_addr constant [21 x i8] c"error: parse failed\0A\00"
@.str.13918 = private unnamed_addr constant [9 x i8] c"Symbols\0A\00"
@.str.13988 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.13995 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.14003 = private unnamed_addr constant [17 x i8] c"Symbol function \00"
@.str.14007 = private unnamed_addr constant [9 x i8] c" params=\00"
@.str.14016 = private unnamed_addr constant [10 x i8] c" returns=\00"
@.str.14025 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14033 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.14041 = private unnamed_addr constant [21 x i8] c"  External function \00"
@.str.14045 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14120 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.14127 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.14135 = private unnamed_addr constant [8 x i8] c"Symbol \00"
@.str.14139 = private unnamed_addr constant [2 x i8] c" \00"
@.str.14143 = private unnamed_addr constant [7 x i8] c" type=\00"
@.str.14154 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14164 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.14172 = private unnamed_addr constant [17 x i8] c"Symbol template \00"
@.str.14176 = private unnamed_addr constant [9 x i8] c" target=\00"
@.str.14185 = private unnamed_addr constant [9 x i8] c" params=\00"
@.str.14194 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14204 = private unnamed_addr constant [10 x i8] c"attribute\00"
@.str.14212 = private unnamed_addr constant [18 x i8] c"Symbol attribute \00"
@.str.14221 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14231 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.14238 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.14245 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.14252 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.14260 = private unnamed_addr constant [17 x i8] c"Symbol external \00"
@.str.14269 = private unnamed_addr constant [2 x i8] c" \00"
@.str.14273 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14283 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.14290 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.14297 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.14304 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.14312 = private unnamed_addr constant [13 x i8] c"Symbol type \00"
@.str.14316 = private unnamed_addr constant [7 x i8] c" kind=\00"
@.str.14320 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14328 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.14335 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.14343 = private unnamed_addr constant [12 x i8] c"  External \00"
@.str.14352 = private unnamed_addr constant [2 x i8] c" \00"
@.str.14356 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14418 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.14426 = private unnamed_addr constant [15 x i8] c"Symbol import \00"
@.str.14430 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14467 = private unnamed_addr constant [21 x i8] c"error: parse failed\0A\00"
@.str.14475 = private unnamed_addr constant [9 x i8] c"Program\0A\00"
@.str.14531 = private unnamed_addr constant [6 x i8] c"Decl \00"
@.str.14535 = private unnamed_addr constant [2 x i8] c" \00"
@.str.14539 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14547 = private unnamed_addr constant [10 x i8] c"attribute\00"
@.str.14555 = private unnamed_addr constant [11 x i8] c"Attribute \00"
@.str.14564 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14573 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.14581 = private unnamed_addr constant [17 x i8] c"Template target \00"
@.str.14590 = private unnamed_addr constant [8 x i8] c" params\00"
@.str.14599 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14608 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.14615 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.14622 = private unnamed_addr constant [9 x i8] c"template\00"
@.str.14674 = private unnamed_addr constant [15 x i8] c"  Body tokens=\00"
@.str.14682 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.14710 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.14717 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.14724 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.14731 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.14783 = private unnamed_addr constant [15 x i8] c"  Body tokens=\00"
@.str.14791 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.15013 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.15099 = private unnamed_addr constant [1 x i8] c"\00"
@.str.15148 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.15156 = private unnamed_addr constant [3 x i8] c"&&\00"
@.str.15177 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.15185 = private unnamed_addr constant [3 x i8] c"||\00"
@.str.15206 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.15214 = private unnamed_addr constant [2 x i8] c"^\00"
@.str.15239 = private unnamed_addr constant [2 x i8] c" \00"
@.str.15269 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.15274 = private unnamed_addr constant [4 x i8] c"int\00"
@.str.15283 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.15288 = private unnamed_addr constant [5 x i8] c"char\00"
@.str.15297 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.15302 = private unnamed_addr constant [4 x i8] c"int\00"
@.str.15311 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.15316 = private unnamed_addr constant [10 x i8] c"long long\00"
@.str.15325 = private unnamed_addr constant [6 x i8] c"Float\00"
@.str.15330 = private unnamed_addr constant [6 x i8] c"float\00"
@.str.15339 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.15344 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.15353 = private unnamed_addr constant [7 x i8] c"String\00"
@.str.15358 = private unnamed_addr constant [12 x i8] c"const char*\00"
@.str.15367 = private unnamed_addr constant [6 x i8] c"Regex\00"
@.str.15372 = private unnamed_addr constant [12 x i8] c"const char*\00"
@.str.15381 = private unnamed_addr constant [6 x i8] c"Array\00"
@.str.15386 = private unnamed_addr constant [6 x i8] c"void*\00"
@.str.15395 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.15400 = private unnamed_addr constant [5 x i8] c"void\00"
@.str.15404 = private unnamed_addr constant [6 x i8] c"void*\00"
@.str.15434 = private unnamed_addr constant [1 x i8] c"\00"
@.str.15502 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.15512 = private unnamed_addr constant [8 x i8] c"return \00"
@.str.15525 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.15535 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.15542 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.15617 = private unnamed_addr constant [2 x i8] c" \00"
@.str.15628 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.15641 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.15657 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.15671 = private unnamed_addr constant [2 x i8] c" \00"
@.str.15682 = private unnamed_addr constant [7 x i8] c" = 0;\0A\00"
@.str.15693 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.15700 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.15717 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.15733 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.15735 = private unnamed_addr constant [2 x i8] c")\00"
@.str.15753 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.15769 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.15771 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.15801 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.15814 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.15828 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.15833 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.15841 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.15875 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.15905 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.15922 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.15924 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.15940 = private unnamed_addr constant [8 x i8] c"else {\0A\00"
@.str.15954 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.15959 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.15974 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.16049 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.16115 = private unnamed_addr constant [4 x i8] c" < \00"
@.str.16128 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.16140 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.16146 = private unnamed_addr constant [5 x i8] c" <= \00"
@.str.16169 = private unnamed_addr constant [10 x i8] c"for (int \00"
@.str.16173 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.16186 = private unnamed_addr constant [3 x i8] c"; \00"
@.str.16203 = private unnamed_addr constant [3 x i8] c"; \00"
@.str.16207 = private unnamed_addr constant [7 x i8] c"++) {\0A\00"
@.str.16221 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.16226 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.16237 = private unnamed_addr constant [9 x i8] c"/* for (\00"
@.str.16248 = private unnamed_addr constant [6 x i8] c") */\0A\00"
@.str.16288 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.16295 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.16302 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.16309 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.16316 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.16354 = private unnamed_addr constant [4 x i8] c"/* \00"
@.str.16358 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.16369 = private unnamed_addr constant [6 x i8] c") */\0A\00"
@.str.16418 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.16475 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.16482 = private unnamed_addr constant [8 x i8] c"return \00"
@.str.16495 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.16504 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.16511 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.16583 = private unnamed_addr constant [2 x i8] c" \00"
@.str.16594 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.16607 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.16621 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.16632 = private unnamed_addr constant [2 x i8] c" \00"
@.str.16643 = private unnamed_addr constant [7 x i8] c" = 0;\0A\00"
@.str.16652 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.16659 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.16666 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.16673 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.16680 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.16687 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.16694 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.16701 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.16708 = private unnamed_addr constant [32 x i8] c"/* unsupported nested block */\0A\00"
@.str.16723 = private unnamed_addr constant [3 x i8] c";\0A\00"
@.str.16769 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.16784 = private unnamed_addr constant [5 x i8] c"main\00"
@.str.16799 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.16814 = private unnamed_addr constant [2 x i8] c")\00"
@.str.16829 = private unnamed_addr constant [2 x i8] c":\00"
@.str.16843 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.16858 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.16976 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.16991 = private unnamed_addr constant [5 x i8] c"main\00"
@.str.17006 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.17021 = private unnamed_addr constant [2 x i8] c")\00"
@.str.17036 = private unnamed_addr constant [2 x i8] c":\00"
@.str.17050 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.17090 = private unnamed_addr constant [35 x i8] c"int main(int argc, char** argv) {\0A\00"
@.str.17092 = private unnamed_addr constant [17 x i8] c"    (void)argc;\0A\00"
@.str.17094 = private unnamed_addr constant [17 x i8] c"    (void)argv;\0A\00"
@.str.17104 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.17107 = private unnamed_addr constant [15 x i8] c"    return 0;\0A\00"
@.str.17109 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.17121 = private unnamed_addr constant [1 x i8] c"\00"
@.str.17140 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.17145 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.17154 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.17159 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.17168 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.17173 = private unnamed_addr constant [4 x i8] c"i32\00"
@.str.17182 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.17187 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.17196 = private unnamed_addr constant [6 x i8] c"Float\00"
@.str.17201 = private unnamed_addr constant [6 x i8] c"float\00"
@.str.17210 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.17215 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.17224 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.17229 = private unnamed_addr constant [5 x i8] c"void\00"
@.str.17238 = private unnamed_addr constant [7 x i8] c"String\00"
@.str.17243 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17252 = private unnamed_addr constant [6 x i8] c"Regex\00"
@.str.17257 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17266 = private unnamed_addr constant [6 x i8] c"Range\00"
@.str.17271 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17280 = private unnamed_addr constant [6 x i8] c"Array\00"
@.str.17285 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17298 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17302 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.17321 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.17326 = private unnamed_addr constant [4 x i8] c"add\00"
@.str.17335 = private unnamed_addr constant [2 x i8] c"-\00"
@.str.17340 = private unnamed_addr constant [4 x i8] c"sub\00"
@.str.17349 = private unnamed_addr constant [2 x i8] c"*\00"
@.str.17354 = private unnamed_addr constant [4 x i8] c"mul\00"
@.str.17363 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.17368 = private unnamed_addr constant [5 x i8] c"sdiv\00"
@.str.17377 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.17382 = private unnamed_addr constant [5 x i8] c"srem\00"
@.str.17391 = private unnamed_addr constant [3 x i8] c"<<\00"
@.str.17396 = private unnamed_addr constant [4 x i8] c"shl\00"
@.str.17405 = private unnamed_addr constant [3 x i8] c">>\00"
@.str.17410 = private unnamed_addr constant [5 x i8] c"ashr\00"
@.str.17419 = private unnamed_addr constant [2 x i8] c"&\00"
@.str.17424 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.17433 = private unnamed_addr constant [2 x i8] c"|\00"
@.str.17438 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.17447 = private unnamed_addr constant [2 x i8] c"^\00"
@.str.17454 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.17459 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.17468 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.17475 = private unnamed_addr constant [3 x i8] c"&&\00"
@.str.17480 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.17489 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.17496 = private unnamed_addr constant [3 x i8] c"||\00"
@.str.17501 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.17510 = private unnamed_addr constant [3 x i8] c"==\00"
@.str.17515 = private unnamed_addr constant [8 x i8] c"icmp_eq\00"
@.str.17524 = private unnamed_addr constant [3 x i8] c"!=\00"
@.str.17529 = private unnamed_addr constant [8 x i8] c"icmp_ne\00"
@.str.17538 = private unnamed_addr constant [2 x i8] c"<\00"
@.str.17543 = private unnamed_addr constant [9 x i8] c"icmp_slt\00"
@.str.17552 = private unnamed_addr constant [3 x i8] c"<=\00"
@.str.17557 = private unnamed_addr constant [9 x i8] c"icmp_sle\00"
@.str.17566 = private unnamed_addr constant [2 x i8] c">\00"
@.str.17571 = private unnamed_addr constant [9 x i8] c"icmp_sgt\00"
@.str.17580 = private unnamed_addr constant [3 x i8] c">=\00"
@.str.17585 = private unnamed_addr constant [9 x i8] c"icmp_sge\00"
@.str.17594 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.17599 = private unnamed_addr constant [6 x i8] c"range\00"
@.str.17608 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.17613 = private unnamed_addr constant [9 x i8] c"range_to\00"
@.str.17622 = private unnamed_addr constant [6 x i8] c"until\00"
@.str.17627 = private unnamed_addr constant [12 x i8] c"range_until\00"
@.str.17636 = private unnamed_addr constant [2 x i8] c"@\00"
@.str.17641 = private unnamed_addr constant [7 x i8] c"matmul\00"
@.str.17650 = private unnamed_addr constant [6 x i8] c"inner\00"
@.str.17655 = private unnamed_addr constant [6 x i8] c"inner\00"
@.str.17664 = private unnamed_addr constant [6 x i8] c"outer\00"
@.str.17669 = private unnamed_addr constant [6 x i8] c"outer\00"
@.str.17678 = private unnamed_addr constant [7 x i8] c"tensor\00"
@.str.17683 = private unnamed_addr constant [7 x i8] c"tensor\00"
@.str.18032 = private unnamed_addr constant [5 x i8] c"void\00"
@.str.18091 = private unnamed_addr constant [2 x i8] c" \00"
@.str.18095 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.18106 = private unnamed_addr constant [5 x i8] c"), (\00"
@.str.18119 = private unnamed_addr constant [2 x i8] c")\00"
@.str.18153 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.18166 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.18168 = private unnamed_addr constant [2 x i8] c")\00"
@.str.18177 = private unnamed_addr constant [6 x i8] c"call \00"
@.str.18181 = private unnamed_addr constant [3 x i8] c" @\00"
@.str.18190 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.18205 = private unnamed_addr constant [2 x i8] c")\00"
@.str.18229 = private unnamed_addr constant [6 x i8] c"load \00"
@.str.18233 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.18246 = private unnamed_addr constant [2 x i8] c" \00"
@.str.18311 = private unnamed_addr constant [1 x i8] c"\00"
@.str.18360 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.18367 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.18372 = private unnamed_addr constant [11 x i8] c"    store \00"
@.str.18376 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.18380 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18429 = private unnamed_addr constant [3 x i8] c"%=\00"
@.str.18435 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.18439 = private unnamed_addr constant [10 x i8] c"    %old.\00"
@.str.18443 = private unnamed_addr constant [9 x i8] c" = load \00"
@.str.18447 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.18451 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18453 = private unnamed_addr constant [10 x i8] c"    %new.\00"
@.str.18457 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.18464 = private unnamed_addr constant [2 x i8] c" \00"
@.str.18468 = private unnamed_addr constant [7 x i8] c" %old.\00"
@.str.18472 = private unnamed_addr constant [4 x i8] c", (\00"
@.str.18494 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.18496 = private unnamed_addr constant [11 x i8] c"    store \00"
@.str.18500 = private unnamed_addr constant [7 x i8] c" %new.\00"
@.str.18504 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.18508 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18545 = private unnamed_addr constant [1 x i8] c"\00"
@.str.18574 = private unnamed_addr constant [1 x i8] c"\00"
@.str.18620 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.18636 = private unnamed_addr constant [2 x i8] c":\00"
@.str.18664 = private unnamed_addr constant [3 x i8] c", \00"
@.str.18676 = private unnamed_addr constant [3 x i8] c" %\00"
@.str.18707 = private unnamed_addr constant [2 x i8] c",\00"
@.str.18730 = private unnamed_addr constant [2 x i8] c",\00"
@.str.18769 = private unnamed_addr constant [1 x i8] c"\00"
@.str.18837 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.18845 = private unnamed_addr constant [9 x i8] c"    ret \00"
@.str.18858 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18868 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.18875 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.18922 = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.18933 = private unnamed_addr constant [11 x i8] c" = alloca \00"
@.str.18940 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18942 = private unnamed_addr constant [11 x i8] c"    store \00"
@.str.18955 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.18966 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.18975 = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.18986 = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.18997 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.19004 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.19011 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.19018 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.19025 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.19032 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.19039 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.19046 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.19054 = private unnamed_addr constant [20 x i8] c"    ; nested block \00"
@.str.19058 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.19069 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.19110 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.19121 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19168 = private unnamed_addr constant [20 x i8] c"    ; no else body\0A\00"
@.str.19197 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.19214 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.19216 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.19252 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.19257 = private unnamed_addr constant [20 x i8] c"    ; else-if body\0A\00"
@.str.19261 = private unnamed_addr constant [27 x i8] c"    ; malformed else body\0A\00"
@.str.19337 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.19339 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.19362 = private unnamed_addr constant [20 x i8] c"    ; malformed if\0A\00"
@.str.19370 = private unnamed_addr constant [1 x i8] c"\00"
@.str.19411 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.19421 = private unnamed_addr constant [11 x i8] c"    %cond.\00"
@.str.19425 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.19438 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19440 = private unnamed_addr constant [17 x i8] c"    br i1 %cond.\00"
@.str.19444 = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.19448 = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.19452 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19454 = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.19458 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19471 = private unnamed_addr constant [22 x i8] c"    br label %if.end.\00"
@.str.19475 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19477 = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.19481 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19494 = private unnamed_addr constant [22 x i8] c"    br label %if.end.\00"
@.str.19498 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19500 = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.19504 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19580 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.19582 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.19605 = private unnamed_addr constant [23 x i8] c"    ; malformed while\0A\00"
@.str.19613 = private unnamed_addr constant [1 x i8] c"\00"
@.str.19618 = private unnamed_addr constant [26 x i8] c"    br label %while.cond.\00"
@.str.19622 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19624 = private unnamed_addr constant [12 x i8] c"while.cond.\00"
@.str.19628 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19630 = private unnamed_addr constant [16 x i8] c"    %whilecond.\00"
@.str.19634 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.19647 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19649 = private unnamed_addr constant [22 x i8] c"    br i1 %whilecond.\00"
@.str.19653 = private unnamed_addr constant [21 x i8] c", label %while.body.\00"
@.str.19657 = private unnamed_addr constant [20 x i8] c", label %while.end.\00"
@.str.19661 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19663 = private unnamed_addr constant [12 x i8] c"while.body.\00"
@.str.19667 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19680 = private unnamed_addr constant [26 x i8] c"    br label %while.cond.\00"
@.str.19684 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19686 = private unnamed_addr constant [11 x i8] c"while.end.\00"
@.str.19690 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19766 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.19768 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.19791 = private unnamed_addr constant [21 x i8] c"    ; malformed for\0A\00"
@.str.19823 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.19834 = private unnamed_addr constant [28 x i8] c"    ; malformed for source\0A\00"
@.str.19842 = private unnamed_addr constant [1 x i8] c"\00"
@.str.19847 = private unnamed_addr constant [18 x i8] c"    ; for source \00"
@.str.19860 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19862 = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.19866 = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.19868 = private unnamed_addr constant [24 x i8] c"    br label %for.cond.\00"
@.str.19872 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19874 = private unnamed_addr constant [10 x i8] c"for.cond.\00"
@.str.19878 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19880 = private unnamed_addr constant [21 x i8] c"    ; iterator next \00"
@.str.19884 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19886 = private unnamed_addr constant [24 x i8] c"    br i1 %for.hasnext.\00"
@.str.19890 = private unnamed_addr constant [19 x i8] c", label %for.body.\00"
@.str.19894 = private unnamed_addr constant [18 x i8] c", label %for.end.\00"
@.str.19898 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19900 = private unnamed_addr constant [10 x i8] c"for.body.\00"
@.str.19904 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19917 = private unnamed_addr constant [24 x i8] c"    br label %for.cond.\00"
@.str.19921 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.19923 = private unnamed_addr constant [9 x i8] c"for.end.\00"
@.str.19927 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.19953 = private unnamed_addr constant [1 x i8] c"\00"
@.str.20021 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.20029 = private unnamed_addr constant [9 x i8] c"    ret \00"
@.str.20042 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20052 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.20059 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.20106 = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.20117 = private unnamed_addr constant [11 x i8] c" = alloca \00"
@.str.20124 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20126 = private unnamed_addr constant [11 x i8] c"    store \00"
@.str.20139 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.20150 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20159 = private unnamed_addr constant [6 x i8] c"    %\00"
@.str.20170 = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.20181 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.20206 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.20231 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.20256 = private unnamed_addr constant [4 x i8] c"map\00"
@.str.20263 = private unnamed_addr constant [5 x i8] c"pmap\00"
@.str.20270 = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.20277 = private unnamed_addr constant [8 x i8] c"preduce\00"
@.str.20284 = private unnamed_addr constant [7 x i8] c"filter\00"
@.str.20292 = private unnamed_addr constant [13 x i8] c"    ; block \00"
@.str.20296 = private unnamed_addr constant [3 x i8] c" (\00"
@.str.20307 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.20348 = private unnamed_addr constant [5 x i8] c"    \00"
@.str.20359 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20392 = private unnamed_addr constant [1 x i8] c"\00"
@.str.20450 = private unnamed_addr constant [9 x i8] c"function\00"
@.str.20457 = private unnamed_addr constant [18 x i8] c"external-function\00"
@.str.20465 = private unnamed_addr constant [8 x i8] c"define \00"
@.str.20477 = private unnamed_addr constant [3 x i8] c" @\00"
@.str.20481 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.20490 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.20545 = private unnamed_addr constant [12 x i8] c"end define\0A\00"
@.str.20555 = private unnamed_addr constant [6 x i8] c"class\00"
@.str.20562 = private unnamed_addr constant [7 x i8] c"object\00"
@.str.20569 = private unnamed_addr constant [15 x i8] c"external-class\00"
@.str.20576 = private unnamed_addr constant [16 x i8] c"external-object\00"
@.str.20584 = private unnamed_addr constant [7 x i8] c"%type.\00"
@.str.20588 = private unnamed_addr constant [18 x i8] c" = type opaque ; \00"
@.str.20592 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20602 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.20609 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.20617 = private unnamed_addr constant [2 x i8] c"@\00"
@.str.20621 = private unnamed_addr constant [22 x i8] c" = global ptr null ; \00"
@.str.20625 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20667 = private unnamed_addr constant [21 x i8] c"error: parse failed\0A\00"
@.str.20681 = private unnamed_addr constant [22 x i8] c"module csec.selfhost\0A\00"
@.str.20688 = private unnamed_addr constant [12 x i8] c"entry main\0A\00"
@.str.20690 = private unnamed_addr constant [20 x i8] c"lowering llvm-text\0A\00"
@.str.20697 = private unnamed_addr constant [13 x i8] c"main.return \00"
@.str.20701 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20725 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.20727 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.20731 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.20754 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.20777 = private unnamed_addr constant [3 x i8] c"73\00"
@.str.20788 = private unnamed_addr constant [3 x i8] c"75\00"
@.str.20799 = private unnamed_addr constant [3 x i8] c"78\00"
@.str.20810 = private unnamed_addr constant [3 x i8] c"70\00"
@.str.20821 = private unnamed_addr constant [3 x i8] c"83\00"
@.str.20832 = private unnamed_addr constant [3 x i8] c"82\00"
@.str.20843 = private unnamed_addr constant [3 x i8] c"67\00"
@.str.20854 = private unnamed_addr constant [3 x i8] c"66\00"
@.str.20865 = private unnamed_addr constant [3 x i8] c"79\00"
@.str.20876 = private unnamed_addr constant [3 x i8] c"77\00"
@.str.20887 = private unnamed_addr constant [3 x i8] c"69\00"
@.str.20898 = private unnamed_addr constant [4 x i8] c"120\00"
@.str.20909 = private unnamed_addr constant [4 x i8] c"121\00"
@.str.20920 = private unnamed_addr constant [4 x i8] c"122\00"
@.str.20931 = private unnamed_addr constant [3 x i8] c"97\00"
@.str.20942 = private unnamed_addr constant [4 x i8] c"116\00"
@.str.20953 = private unnamed_addr constant [4 x i8] c"117\00"
@.str.20964 = private unnamed_addr constant [3 x i8] c"48\00"
@.str.20975 = private unnamed_addr constant [3 x i8] c"49\00"
@.str.20986 = private unnamed_addr constant [3 x i8] c"50\00"
@.str.20997 = private unnamed_addr constant [3 x i8] c"51\00"
@.str.21008 = private unnamed_addr constant [3 x i8] c"52\00"
@.str.21019 = private unnamed_addr constant [3 x i8] c"53\00"
@.str.21030 = private unnamed_addr constant [3 x i8] c"54\00"
@.str.21041 = private unnamed_addr constant [3 x i8] c"55\00"
@.str.21052 = private unnamed_addr constant [3 x i8] c"56\00"
@.str.21063 = private unnamed_addr constant [3 x i8] c"57\00"
@.str.21074 = private unnamed_addr constant [3 x i8] c"95\00"
@.str.21085 = private unnamed_addr constant [3 x i8] c"32\00"
@.str.21096 = private unnamed_addr constant [3 x i8] c"34\00"
@.str.21122 = private unnamed_addr constant [3 x i8] c"92\00"
@.str.21148 = private unnamed_addr constant [3 x i8] c"10\00"
@.str.21174 = private unnamed_addr constant [2 x i8] c"9\00"
@.str.21178 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.21221 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.21306 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.21315 = private unnamed_addr constant [6 x i8] c".val.\00"
@.str.21321 = private unnamed_addr constant [5 x i8] c"%tmp\00"
@.str.21362 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21385 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21406 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.21408 = private unnamed_addr constant [2 x i8] c")\00"
@.str.21419 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21489 = private unnamed_addr constant [2 x i8] c":\00"
@.str.21507 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21527 = private unnamed_addr constant [2 x i8] c",\00"
@.str.21546 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21588 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21644 = private unnamed_addr constant [1 x i8] c"\00"
@.str.21687 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.21700 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.21799 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.21884 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.21897 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.21932 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.21960 = private unnamed_addr constant [1 x i8] c"\00"
@.str.21997 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.22010 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.22046 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.22110 = private unnamed_addr constant [4 x i8] c"val\00"
@.str.22123 = private unnamed_addr constant [4 x i8] c"var\00"
@.str.22158 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.22224 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.22229 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22233 = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.22237 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22246 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.22251 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22255 = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.22259 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22268 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.22273 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22277 = private unnamed_addr constant [22 x i8] c" = load double, ptr %\00"
@.str.22281 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22290 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.22295 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22299 = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.22303 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22315 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.22320 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22324 = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.22328 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22332 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.22336 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.22340 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22441 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.22446 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.22450 = private unnamed_addr constant [7 x i8] c".cval.\00"
@.str.22454 = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.22458 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22460 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.22464 = private unnamed_addr constant [6 x i8] c".val.\00"
@.str.22468 = private unnamed_addr constant [13 x i8] c" = zext i8 %\00"
@.str.22472 = private unnamed_addr constant [7 x i8] c".cval.\00"
@.str.22476 = private unnamed_addr constant [9 x i8] c" to i32\0A\00"
@.str.22480 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.22484 = private unnamed_addr constant [6 x i8] c".val.\00"
@.str.22488 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.22492 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22496 = private unnamed_addr constant [1 x i8] c"\00"
@.str.22539 = private unnamed_addr constant [6 x i8] c"false\00"
@.str.22592 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.22601 = private unnamed_addr constant [6 x i8] c".bval\00"
@.str.22605 = private unnamed_addr constant [6 x i8] c"%btmp\00"
@.str.22687 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.22691 = private unnamed_addr constant [23 x i8] c".bval = load i1, ptr %\00"
@.str.22695 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.22699 = private unnamed_addr constant [1 x i8] c"\00"
@.str.22750 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.22785 = private unnamed_addr constant [5 x i8] c".arg\00"
@.str.22830 = private unnamed_addr constant [4 x i8] c"i32\00"
@.str.22854 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.22878 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.22902 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.22960 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.22967 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.22974 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.22981 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.22988 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.23028 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.23041 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.23043 = private unnamed_addr constant [2 x i8] c")\00"
@.str.23077 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23084 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.23091 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.23098 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.23105 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.23157 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.23172 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23188 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23194 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23199 = private unnamed_addr constant [4 x i8] c"i32\00"
@.str.23250 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.23346 = private unnamed_addr constant [5 x i8] c".arg\00"
@.str.23395 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23399 = private unnamed_addr constant [17 x i8] c" = add i32 0, 0\0A\00"
@.str.23446 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23450 = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.23454 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.23458 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.23482 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23486 = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.23495 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.23519 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23523 = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.23535 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.23614 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23621 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.23628 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.23635 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.23642 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.23647 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23651 = private unnamed_addr constant [9 x i8] c" = load \00"
@.str.23655 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.23659 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.23663 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23667 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.23671 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.23705 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.23718 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.23720 = private unnamed_addr constant [2 x i8] c")\00"
@.str.23761 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23768 = private unnamed_addr constant [15 x i8] c"commandLineArg\00"
@.str.23787 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.23790 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23794 = private unnamed_addr constant [40 x i8] c" = call ptr @csec_command_line_arg(i32 \00"
@.str.23798 = private unnamed_addr constant [8 x i8] c".arg0)\0A\00"
@.str.23807 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23814 = private unnamed_addr constant [12 x i8] c"tokenTextAt\00"
@.str.23835 = private unnamed_addr constant [2 x i8] c",\00"
@.str.23860 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.23877 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.23880 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23884 = private unnamed_addr constant [37 x i8] c" = call ptr @csec_token_text_at(ptr \00"
@.str.23888 = private unnamed_addr constant [12 x i8] c".arg0, i32 \00"
@.str.23892 = private unnamed_addr constant [8 x i8] c".arg1)\0A\00"
@.str.23902 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.23915 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23919 = private unnamed_addr constant [13 x i8] c" = call i8 @\00"
@.str.23923 = private unnamed_addr constant [4 x i8] c"()\0A\00"
@.str.23932 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.23967 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.23971 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.23975 = private unnamed_addr constant [4 x i8] c"()\0A\00"
@.str.23991 = private unnamed_addr constant [2 x i8] c",\00"
@.str.24012 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.24015 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24019 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.24023 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24034 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24038 = private unnamed_addr constant [8 x i8] c".arg0)\0A\00"
@.str.24068 = private unnamed_addr constant [2 x i8] c",\00"
@.str.24089 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.24102 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.24105 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24109 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.24113 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24124 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24128 = private unnamed_addr constant [8 x i8] c".arg0, \00"
@.str.24139 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24143 = private unnamed_addr constant [8 x i8] c".arg1)\0A\00"
@.str.24173 = private unnamed_addr constant [2 x i8] c",\00"
@.str.24194 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.24207 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.24220 = private unnamed_addr constant [6 x i8] c".arg2\00"
@.str.24223 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24227 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.24231 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24242 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24246 = private unnamed_addr constant [8 x i8] c".arg0, \00"
@.str.24257 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24261 = private unnamed_addr constant [8 x i8] c".arg1, \00"
@.str.24272 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24276 = private unnamed_addr constant [8 x i8] c".arg2)\0A\00"
@.str.24304 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.24317 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.24330 = private unnamed_addr constant [6 x i8] c".arg2\00"
@.str.24343 = private unnamed_addr constant [6 x i8] c".arg3\00"
@.str.24346 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24350 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.24354 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24365 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24369 = private unnamed_addr constant [8 x i8] c".arg0, \00"
@.str.24380 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24384 = private unnamed_addr constant [8 x i8] c".arg1, \00"
@.str.24395 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24399 = private unnamed_addr constant [8 x i8] c".arg2, \00"
@.str.24410 = private unnamed_addr constant [2 x i8] c" \00"
@.str.24414 = private unnamed_addr constant [8 x i8] c".arg3)\0A\00"
@.str.24449 = private unnamed_addr constant [2 x i8] c".\00"
@.str.24464 = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.24479 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24492 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.24494 = private unnamed_addr constant [2 x i8] c")\00"
@.str.24563 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24567 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.24571 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.24597 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24601 = private unnamed_addr constant [21 x i8] c".index = add i32 0, \00"
@.str.24610 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.24638 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24642 = private unnamed_addr constant [25 x i8] c".index = load i32, ptr %\00"
@.str.24658 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.24667 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24671 = private unnamed_addr constant [23 x i8] c".index = add i32 0, 0\0A\00"
@.str.24677 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24681 = private unnamed_addr constant [40 x i8] c".i8 = call i8 @csec_string_char_at(ptr \00"
@.str.24685 = private unnamed_addr constant [11 x i8] c".obj, i32 \00"
@.str.24689 = private unnamed_addr constant [9 x i8] c".index)\0A\00"
@.str.24691 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24695 = private unnamed_addr constant [12 x i8] c" = zext i8 \00"
@.str.24699 = private unnamed_addr constant [12 x i8] c".i8 to i32\0A\00"
@.str.24745 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.24760 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.24776 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.24800 = private unnamed_addr constant [1 x i8] c"\00"
@.str.24807 = private unnamed_addr constant [1 x i8] c"\00"
@.str.24822 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.24838 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.24872 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24876 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_char(i8 \00"
@.str.24888 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.24934 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.24940 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24944 = private unnamed_addr constant [26 x i8] c".left.i8 = load i8, ptr %\00"
@.str.24960 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.24962 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24966 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_char(i8 \00"
@.str.24970 = private unnamed_addr constant [11 x i8] c".left.i8)\0A\00"
@.str.24987 = private unnamed_addr constant [10 x i8] c".left.i32\00"
@.str.24990 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.24994 = private unnamed_addr constant [22 x i8] c".left.i64 = sext i32 \00"
@.str.24998 = private unnamed_addr constant [18 x i8] c".left.i32 to i64\0A\00"
@.str.25000 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25004 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_i64(i64 \00"
@.str.25008 = private unnamed_addr constant [12 x i8] c".left.i64)\0A\00"
@.str.25024 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.25040 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.25074 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25078 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_char(i8 \00"
@.str.25090 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.25136 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.25142 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25146 = private unnamed_addr constant [27 x i8] c".right.i8 = load i8, ptr %\00"
@.str.25162 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.25164 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25168 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_char(i8 \00"
@.str.25172 = private unnamed_addr constant [12 x i8] c".right.i8)\0A\00"
@.str.25189 = private unnamed_addr constant [11 x i8] c".right.i32\00"
@.str.25192 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25196 = private unnamed_addr constant [23 x i8] c".right.i64 = sext i32 \00"
@.str.25200 = private unnamed_addr constant [19 x i8] c".right.i32 to i64\0A\00"
@.str.25202 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25206 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_i64(i64 \00"
@.str.25210 = private unnamed_addr constant [13 x i8] c".right.i64)\0A\00"
@.str.25218 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25222 = private unnamed_addr constant [37 x i8] c" = call ptr @csec_string_concat(ptr \00"
@.str.25226 = private unnamed_addr constant [12 x i8] c".left, ptr \00"
@.str.25230 = private unnamed_addr constant [9 x i8] c".right)\0A\00"
@.str.25254 = private unnamed_addr constant [4 x i8] c"add\00"
@.str.25261 = private unnamed_addr constant [4 x i8] c"sub\00"
@.str.25268 = private unnamed_addr constant [4 x i8] c"mul\00"
@.str.25275 = private unnamed_addr constant [5 x i8] c"sdiv\00"
@.str.25282 = private unnamed_addr constant [5 x i8] c"srem\00"
@.str.25289 = private unnamed_addr constant [4 x i8] c"shl\00"
@.str.25296 = private unnamed_addr constant [5 x i8] c"ashr\00"
@.str.25303 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.25310 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.25317 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.25332 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.25347 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.25350 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25354 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.25358 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.25362 = private unnamed_addr constant [8 x i8] c".left, \00"
@.str.25366 = private unnamed_addr constant [8 x i8] c".right\0A\00"
@.str.25371 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.25375 = private unnamed_addr constant [17 x i8] c" = add i32 0, 0\0A\00"
@.str.25434 = private unnamed_addr constant [1 x i8] c"\00"
@.str.25471 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.25509 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.25529 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.25567 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.25587 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.25607 = private unnamed_addr constant [6 x i8] c".arg2\00"
@.str.25622 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.25642 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.25662 = private unnamed_addr constant [6 x i8] c".arg2\00"
@.str.25682 = private unnamed_addr constant [6 x i8] c".arg3\00"
@.str.25742 = private unnamed_addr constant [1 x i8] c"\00"
@.str.25778 = private unnamed_addr constant [2 x i8] c" \00"
@.str.25841 = private unnamed_addr constant [2 x i8] c" \00"
@.str.25856 = private unnamed_addr constant [3 x i8] c", \00"
@.str.25867 = private unnamed_addr constant [2 x i8] c" \00"
@.str.25944 = private unnamed_addr constant [2 x i8] c" \00"
@.str.25959 = private unnamed_addr constant [3 x i8] c", \00"
@.str.25970 = private unnamed_addr constant [2 x i8] c" \00"
@.str.25985 = private unnamed_addr constant [3 x i8] c", \00"
@.str.25996 = private unnamed_addr constant [2 x i8] c" \00"
@.str.26064 = private unnamed_addr constant [2 x i8] c" \00"
@.str.26079 = private unnamed_addr constant [3 x i8] c", \00"
@.str.26090 = private unnamed_addr constant [2 x i8] c" \00"
@.str.26105 = private unnamed_addr constant [3 x i8] c", \00"
@.str.26116 = private unnamed_addr constant [2 x i8] c" \00"
@.str.26131 = private unnamed_addr constant [3 x i8] c", \00"
@.str.26142 = private unnamed_addr constant [2 x i8] c" \00"
@.str.26174 = private unnamed_addr constant [20 x i8] c"commandLineArgCount\00"
@.str.26179 = private unnamed_addr constant [28 x i8] c"csec_command_line_arg_count\00"
@.str.26188 = private unnamed_addr constant [15 x i8] c"commandLineArg\00"
@.str.26193 = private unnamed_addr constant [22 x i8] c"csec_command_line_arg\00"
@.str.26220 = private unnamed_addr constant [5 x i8] c"kind\00"
@.str.26234 = private unnamed_addr constant [12 x i8] c"tokenKindAt\00"
@.str.26248 = private unnamed_addr constant [19 x i8] c"csec_token_kind_at\00"
@.str.26267 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.26323 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26327 = private unnamed_addr constant [17 x i8] c" = add i32 0, 0\0A\00"
@.str.26351 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26355 = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.26364 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26388 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26392 = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.26404 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26473 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.26478 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26482 = private unnamed_addr constant [20 x i8] c".c = load i8, ptr %\00"
@.str.26486 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26488 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26492 = private unnamed_addr constant [12 x i8] c" = zext i8 \00"
@.str.26496 = private unnamed_addr constant [11 x i8] c".c to i32\0A\00"
@.str.26500 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26504 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.26508 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26542 = private unnamed_addr constant [2 x i8] c".\00"
@.str.26557 = private unnamed_addr constant [7 x i8] c"length\00"
@.str.26588 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26592 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.26596 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26598 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26602 = private unnamed_addr constant [41 x i8] c".i64 = call i64 @csec_string_length(ptr \00"
@.str.26606 = private unnamed_addr constant [7 x i8] c".obj)\0A\00"
@.str.26608 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26612 = private unnamed_addr constant [14 x i8] c" = trunc i64 \00"
@.str.26616 = private unnamed_addr constant [13 x i8] c".i64 to i32\0A\00"
@.str.26650 = private unnamed_addr constant [2 x i8] c".\00"
@.str.26665 = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.26680 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26693 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26695 = private unnamed_addr constant [2 x i8] c")\00"
@.str.26730 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26734 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.26738 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.26754 = private unnamed_addr constant [7 x i8] c".index\00"
@.str.26757 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26761 = private unnamed_addr constant [40 x i8] c".i8 = call i8 @csec_string_char_at(ptr \00"
@.str.26765 = private unnamed_addr constant [11 x i8] c".obj, i32 \00"
@.str.26769 = private unnamed_addr constant [9 x i8] c".index)\0A\00"
@.str.26771 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26775 = private unnamed_addr constant [12 x i8] c" = zext i8 \00"
@.str.26779 = private unnamed_addr constant [12 x i8] c".i8 to i32\0A\00"
@.str.26813 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26826 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26828 = private unnamed_addr constant [2 x i8] c")\00"
@.str.26874 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26878 = private unnamed_addr constant [16 x i8] c".i8 = call i8 @\00"
@.str.26885 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26902 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.26904 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26908 = private unnamed_addr constant [12 x i8] c" = zext i8 \00"
@.str.26912 = private unnamed_addr constant [12 x i8] c".i8 to i32\0A\00"
@.str.26931 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.26935 = private unnamed_addr constant [14 x i8] c" = call i32 @\00"
@.str.26947 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.26964 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.27020 = private unnamed_addr constant [4 x i8] c"add\00"
@.str.27027 = private unnamed_addr constant [4 x i8] c"sub\00"
@.str.27034 = private unnamed_addr constant [4 x i8] c"mul\00"
@.str.27041 = private unnamed_addr constant [5 x i8] c"sdiv\00"
@.str.27048 = private unnamed_addr constant [5 x i8] c"srem\00"
@.str.27055 = private unnamed_addr constant [4 x i8] c"shl\00"
@.str.27062 = private unnamed_addr constant [5 x i8] c"ashr\00"
@.str.27069 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.27076 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.27083 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.27098 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.27113 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.27116 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27120 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.27124 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.27128 = private unnamed_addr constant [8 x i8] c".left, \00"
@.str.27132 = private unnamed_addr constant [8 x i8] c".right\0A\00"
@.str.27137 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27141 = private unnamed_addr constant [17 x i8] c" = add i32 0, 0\0A\00"
@.str.27199 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.27208 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.27212 = private unnamed_addr constant [7 x i8] c".init.\00"
@.str.27224 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.27228 = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.27243 = private unnamed_addr constant [13 x i8] c"  store i32 \00"
@.str.27247 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.27251 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27255 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.27259 = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.27261 = private unnamed_addr constant [21 x i8] c"  store i32 0, ptr %\00"
@.str.27265 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27308 = private unnamed_addr constant [2 x i8] c"0\00"
@.str.27361 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.27370 = private unnamed_addr constant [7 x i8] c".lval.\00"
@.str.27376 = private unnamed_addr constant [6 x i8] c"%ltmp\00"
@.str.27458 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.27462 = private unnamed_addr constant [7 x i8] c".lval.\00"
@.str.27466 = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.27470 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27474 = private unnamed_addr constant [1 x i8] c"\00"
@.str.27521 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27525 = private unnamed_addr constant [17 x i8] c" = add i64 0, 0\0A\00"
@.str.27549 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27553 = private unnamed_addr constant [15 x i8] c" = add i64 0, \00"
@.str.27562 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27586 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27590 = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.27606 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27640 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.27653 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.27655 = private unnamed_addr constant [2 x i8] c")\00"
@.str.27679 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27683 = private unnamed_addr constant [14 x i8] c" = call i64 @\00"
@.str.27695 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.27712 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.27768 = private unnamed_addr constant [4 x i8] c"add\00"
@.str.27775 = private unnamed_addr constant [4 x i8] c"sub\00"
@.str.27782 = private unnamed_addr constant [4 x i8] c"mul\00"
@.str.27789 = private unnamed_addr constant [5 x i8] c"sdiv\00"
@.str.27796 = private unnamed_addr constant [5 x i8] c"srem\00"
@.str.27803 = private unnamed_addr constant [4 x i8] c"shl\00"
@.str.27810 = private unnamed_addr constant [5 x i8] c"ashr\00"
@.str.27817 = private unnamed_addr constant [4 x i8] c"and\00"
@.str.27824 = private unnamed_addr constant [3 x i8] c"or\00"
@.str.27831 = private unnamed_addr constant [4 x i8] c"xor\00"
@.str.27856 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27860 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.27864 = private unnamed_addr constant [6 x i8] c" i64 \00"
@.str.27875 = private unnamed_addr constant [3 x i8] c", \00"
@.str.27888 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.27893 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.27897 = private unnamed_addr constant [17 x i8] c" = add i64 0, 0\0A\00"
@.str.27955 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.27964 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.27968 = private unnamed_addr constant [8 x i8] c".linit.\00"
@.str.27980 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.27984 = private unnamed_addr constant [15 x i8] c" = alloca i64\0A\00"
@.str.27999 = private unnamed_addr constant [13 x i8] c"  store i64 \00"
@.str.28003 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.28007 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28011 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.28015 = private unnamed_addr constant [15 x i8] c" = alloca i64\0A\00"
@.str.28017 = private unnamed_addr constant [21 x i8] c"  store i64 0, ptr %\00"
@.str.28021 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28064 = private unnamed_addr constant [13 x i8] c"0.000000e+00\00"
@.str.28130 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.28139 = private unnamed_addr constant [7 x i8] c".fval.\00"
@.str.28145 = private unnamed_addr constant [6 x i8] c"%ftmp\00"
@.str.28227 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.28231 = private unnamed_addr constant [7 x i8] c".fval.\00"
@.str.28235 = private unnamed_addr constant [22 x i8] c" = load double, ptr %\00"
@.str.28239 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28243 = private unnamed_addr constant [1 x i8] c"\00"
@.str.28290 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28294 = private unnamed_addr constant [43 x i8] c" = fadd double 0.000000e+00, 0.000000e+00\0A\00"
@.str.28331 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28335 = private unnamed_addr constant [30 x i8] c" = fadd double 0.000000e+00, \00"
@.str.28344 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28368 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28372 = private unnamed_addr constant [22 x i8] c" = load double, ptr %\00"
@.str.28388 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28422 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.28435 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.28437 = private unnamed_addr constant [2 x i8] c")\00"
@.str.28461 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28465 = private unnamed_addr constant [17 x i8] c" = call double @\00"
@.str.28477 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.28494 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.28546 = private unnamed_addr constant [1 x i8] c"\00"
@.str.28554 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.28560 = private unnamed_addr constant [5 x i8] c"fadd\00"
@.str.28569 = private unnamed_addr constant [2 x i8] c"-\00"
@.str.28575 = private unnamed_addr constant [5 x i8] c"fsub\00"
@.str.28584 = private unnamed_addr constant [2 x i8] c"*\00"
@.str.28590 = private unnamed_addr constant [5 x i8] c"fmul\00"
@.str.28599 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.28605 = private unnamed_addr constant [5 x i8] c"fdiv\00"
@.str.28638 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28642 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.28646 = private unnamed_addr constant [9 x i8] c" double \00"
@.str.28657 = private unnamed_addr constant [3 x i8] c", \00"
@.str.28670 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28675 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.28679 = private unnamed_addr constant [43 x i8] c" = fadd double 0.000000e+00, 0.000000e+00\0A\00"
@.str.28737 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.28746 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.28750 = private unnamed_addr constant [8 x i8] c".finit.\00"
@.str.28762 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.28766 = private unnamed_addr constant [18 x i8] c" = alloca double\0A\00"
@.str.28781 = private unnamed_addr constant [16 x i8] c"  store double \00"
@.str.28785 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.28789 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28793 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.28797 = private unnamed_addr constant [18 x i8] c" = alloca double\0A\00"
@.str.28799 = private unnamed_addr constant [35 x i8] c"  store double 0.000000e+00, ptr %\00"
@.str.28803 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28861 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.28870 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.28874 = private unnamed_addr constant [8 x i8] c".cinit.\00"
@.str.28934 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.28938 = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.28940 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.28952 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.28956 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.28999 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29003 = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.29005 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29009 = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.29013 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29015 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.29019 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29023 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29057 = private unnamed_addr constant [2 x i8] c".\00"
@.str.29072 = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.29087 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29100 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29102 = private unnamed_addr constant [2 x i8] c")\00"
@.str.29130 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29134 = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.29136 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29140 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.29144 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29160 = private unnamed_addr constant [7 x i8] c".index\00"
@.str.29163 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29167 = private unnamed_addr constant [37 x i8] c" = call i8 @csec_string_char_at(ptr \00"
@.str.29171 = private unnamed_addr constant [11 x i8] c".obj, i32 \00"
@.str.29175 = private unnamed_addr constant [9 x i8] c".index)\0A\00"
@.str.29177 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.29181 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29185 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29219 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29232 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29234 = private unnamed_addr constant [2 x i8] c")\00"
@.str.29243 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29247 = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.29264 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29268 = private unnamed_addr constant [13 x i8] c" = call i8 @\00"
@.str.29280 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29297 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.29299 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.29303 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29307 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29312 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29316 = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.29318 = private unnamed_addr constant [20 x i8] c"  store i8 0, ptr %\00"
@.str.29322 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29378 = private unnamed_addr constant [1 x i8] c"\00"
@.str.29425 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.29432 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.29441 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.29445 = private unnamed_addr constant [10 x i8] c".fassign.\00"
@.str.29463 = private unnamed_addr constant [16 x i8] c"  store double \00"
@.str.29467 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29471 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29475 = private unnamed_addr constant [1 x i8] c"\00"
@.str.29531 = private unnamed_addr constant [1 x i8] c"\00"
@.str.29632 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29636 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29640 = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.29644 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.29648 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.29650 = private unnamed_addr constant [14 x i8] c"  store ptr %\00"
@.str.29654 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29658 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29662 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29705 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.29709 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29713 = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.29717 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29719 = private unnamed_addr constant [14 x i8] c"  store ptr %\00"
@.str.29723 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29727 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29731 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29765 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29778 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29780 = private unnamed_addr constant [2 x i8] c")\00"
@.str.29793 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.29797 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29817 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29821 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.29833 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.29850 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.29852 = private unnamed_addr constant [13 x i8] c"  store ptr \00"
@.str.29856 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.29860 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.29893 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.29913 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.29917 = private unnamed_addr constant [10 x i8] c".passign.\00"
@.str.29959 = private unnamed_addr constant [1 x i8] c"\00"
@.str.29995 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.29999 = private unnamed_addr constant [34 x i8] c".right = getelementptr inbounds [\00"
@.str.30009 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.30013 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.30039 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30043 = private unnamed_addr constant [25 x i8] c".right = load ptr, ptr %\00"
@.str.30059 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30094 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30107 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30109 = private unnamed_addr constant [2 x i8] c")\00"
@.str.30134 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30138 = private unnamed_addr constant [20 x i8] c".right = call ptr @\00"
@.str.30150 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30167 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.30180 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30184 = private unnamed_addr constant [24 x i8] c".left = load ptr, ptr %\00"
@.str.30188 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30192 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30196 = private unnamed_addr constant [37 x i8] c" = call ptr @csec_string_concat(ptr \00"
@.str.30200 = private unnamed_addr constant [12 x i8] c".left, ptr \00"
@.str.30204 = private unnamed_addr constant [9 x i8] c".right)\0A\00"
@.str.30206 = private unnamed_addr constant [13 x i8] c"  store ptr \00"
@.str.30210 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30214 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30219 = private unnamed_addr constant [1 x i8] c"\00"
@.str.30275 = private unnamed_addr constant [1 x i8] c"\00"
@.str.30353 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.30365 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30369 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30412 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.30416 = private unnamed_addr constant [10 x i8] c".cassign.\00"
@.str.30420 = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.30424 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30426 = private unnamed_addr constant [13 x i8] c"  store i8 %\00"
@.str.30430 = private unnamed_addr constant [10 x i8] c".cassign.\00"
@.str.30434 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30438 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30472 = private unnamed_addr constant [2 x i8] c".\00"
@.str.30487 = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.30502 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30515 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30517 = private unnamed_addr constant [2 x i8] c")\00"
@.str.30549 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.30553 = private unnamed_addr constant [10 x i8] c".cassign.\00"
@.str.30558 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30562 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.30566 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30582 = private unnamed_addr constant [7 x i8] c".index\00"
@.str.30585 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30589 = private unnamed_addr constant [37 x i8] c" = call i8 @csec_string_char_at(ptr \00"
@.str.30593 = private unnamed_addr constant [11 x i8] c".obj, i32 \00"
@.str.30597 = private unnamed_addr constant [9 x i8] c".index)\0A\00"
@.str.30599 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.30603 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30607 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30641 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30654 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30656 = private unnamed_addr constant [2 x i8] c")\00"
@.str.30669 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.30673 = private unnamed_addr constant [10 x i8] c".cassign.\00"
@.str.30693 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.30697 = private unnamed_addr constant [13 x i8] c" = call i8 @\00"
@.str.30709 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.30726 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.30728 = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.30732 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30736 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30740 = private unnamed_addr constant [1 x i8] c"\00"
@.str.30796 = private unnamed_addr constant [1 x i8] c"\00"
@.str.30843 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.30850 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.30859 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.30863 = private unnamed_addr constant [9 x i8] c".assign.\00"
@.str.30881 = private unnamed_addr constant [13 x i8] c"  store i32 \00"
@.str.30885 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.30889 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30912 = private unnamed_addr constant [3 x i8] c"%=\00"
@.str.30918 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.30922 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.30926 = private unnamed_addr constant [6 x i8] c".old.\00"
@.str.30930 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.30934 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30946 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.30950 = private unnamed_addr constant [6 x i8] c".rhs.\00"
@.str.30955 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.30959 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.30963 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.30970 = private unnamed_addr constant [7 x i8] c" i32 %\00"
@.str.30974 = private unnamed_addr constant [6 x i8] c".old.\00"
@.str.30978 = private unnamed_addr constant [4 x i8] c", %\00"
@.str.30982 = private unnamed_addr constant [6 x i8] c".rhs.\00"
@.str.30986 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.30988 = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.30992 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.30996 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.31000 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31056 = private unnamed_addr constant [1 x i8] c"\00"
@.str.31103 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.31110 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.31119 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.31123 = private unnamed_addr constant [10 x i8] c".lassign.\00"
@.str.31141 = private unnamed_addr constant [13 x i8] c"  store i64 \00"
@.str.31145 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.31149 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31172 = private unnamed_addr constant [3 x i8] c"%=\00"
@.str.31178 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.31182 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.31186 = private unnamed_addr constant [7 x i8] c".lold.\00"
@.str.31190 = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.31194 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31206 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.31210 = private unnamed_addr constant [7 x i8] c".lrhs.\00"
@.str.31215 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.31219 = private unnamed_addr constant [8 x i8] c".lnext.\00"
@.str.31223 = private unnamed_addr constant [4 x i8] c" = \00"
@.str.31230 = private unnamed_addr constant [7 x i8] c" i64 %\00"
@.str.31234 = private unnamed_addr constant [7 x i8] c".lold.\00"
@.str.31238 = private unnamed_addr constant [4 x i8] c", %\00"
@.str.31242 = private unnamed_addr constant [7 x i8] c".lrhs.\00"
@.str.31246 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31248 = private unnamed_addr constant [14 x i8] c"  store i64 %\00"
@.str.31252 = private unnamed_addr constant [8 x i8] c".lnext.\00"
@.str.31256 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.31260 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31311 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.31322 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.31324 = private unnamed_addr constant [2 x i8] c")\00"
@.str.31361 = private unnamed_addr constant [2 x i8] c"!\00"
@.str.31372 = private unnamed_addr constant [5 x i8] c".not\00"
@.str.31388 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31392 = private unnamed_addr constant [11 x i8] c" = xor i1 \00"
@.str.31396 = private unnamed_addr constant [12 x i8] c".not, true\0A\00"
@.str.31443 = private unnamed_addr constant [6 x i8] c"icmp_\00"
@.str.31518 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31522 = private unnamed_addr constant [26 x i8] c".left.i8 = load i8, ptr %\00"
@.str.31526 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31528 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31532 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.31536 = private unnamed_addr constant [5 x i8] c" i8 \00"
@.str.31540 = private unnamed_addr constant [11 x i8] c".left.i8, \00"
@.str.31554 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31620 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31624 = private unnamed_addr constant [27 x i8] c".right.i8 = load i8, ptr %\00"
@.str.31628 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31630 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31634 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.31638 = private unnamed_addr constant [5 x i8] c" i8 \00"
@.str.31650 = private unnamed_addr constant [3 x i8] c", \00"
@.str.31654 = private unnamed_addr constant [11 x i8] c".right.i8\0A\00"
@.str.31664 = private unnamed_addr constant [10 x i8] c".left.i32\00"
@.str.31673 = private unnamed_addr constant [11 x i8] c".right.i32\00"
@.str.31700 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31704 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.31708 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.31712 = private unnamed_addr constant [12 x i8] c".left.i32, \00"
@.str.31716 = private unnamed_addr constant [12 x i8] c".right.i32\0A\00"
@.str.31751 = private unnamed_addr constant [5 x i8] c"true\00"
@.str.31756 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31760 = private unnamed_addr constant [26 x i8] c" = icmp eq i1 true, true\0A\00"
@.str.31764 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31768 = private unnamed_addr constant [27 x i8] c" = icmp eq i1 false, true\0A\00"
@.str.31792 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31796 = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.31812 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31846 = private unnamed_addr constant [2 x i8] c".\00"
@.str.31861 = private unnamed_addr constant [11 x i8] c"startsWith\00"
@.str.31876 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.31889 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.31891 = private unnamed_addr constant [2 x i8] c")\00"
@.str.31925 = private unnamed_addr constant [8 x i8] c".needle\00"
@.str.31928 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31932 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.31936 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.31953 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31957 = private unnamed_addr constant [46 x i8] c".i32 = call i32 @csec_string_starts_with(ptr \00"
@.str.31961 = private unnamed_addr constant [11 x i8] c".obj, ptr \00"
@.str.31965 = private unnamed_addr constant [10 x i8] c".needle)\0A\00"
@.str.31967 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.31971 = private unnamed_addr constant [16 x i8] c" = icmp ne i32 \00"
@.str.31975 = private unnamed_addr constant [9 x i8] c".i32, 0\0A\00"
@.str.32009 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32022 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32024 = private unnamed_addr constant [2 x i8] c")\00"
@.str.32048 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32052 = private unnamed_addr constant [13 x i8] c" = call i1 @\00"
@.str.32064 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32081 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.32085 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32089 = private unnamed_addr constant [21 x i8] c" = icmp eq i32 0, 0\0A\00"
@.str.32136 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32140 = private unnamed_addr constant [27 x i8] c" = icmp eq i1 false, true\0A\00"
@.str.32155 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32166 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32168 = private unnamed_addr constant [2 x i8] c")\00"
@.str.32205 = private unnamed_addr constant [2 x i8] c"!\00"
@.str.32216 = private unnamed_addr constant [5 x i8] c".not\00"
@.str.32232 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32236 = private unnamed_addr constant [11 x i8] c" = xor i1 \00"
@.str.32240 = private unnamed_addr constant [12 x i8] c".not, true\0A\00"
@.str.32274 = private unnamed_addr constant [5 x i8] c"true\00"
@.str.32279 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32283 = private unnamed_addr constant [26 x i8] c" = icmp eq i1 true, true\0A\00"
@.str.32287 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32291 = private unnamed_addr constant [27 x i8] c" = icmp eq i1 false, true\0A\00"
@.str.32315 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32319 = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.32335 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32380 = private unnamed_addr constant [14 x i8] c"csec_token_is\00"
@.str.32395 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32408 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32410 = private unnamed_addr constant [2 x i8] c")\00"
@.str.32454 = private unnamed_addr constant [2 x i8] c",\00"
@.str.32474 = private unnamed_addr constant [2 x i8] c",\00"
@.str.32494 = private unnamed_addr constant [2 x i8] c",\00"
@.str.32617 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32621 = private unnamed_addr constant [29 x i8] c".call.arg0 = load ptr, ptr %\00"
@.str.32625 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32639 = private unnamed_addr constant [11 x i8] c".call.arg1\00"
@.str.32642 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32646 = private unnamed_addr constant [28 x i8] c".call.arg2 = load i8, ptr %\00"
@.str.32650 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32652 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32656 = private unnamed_addr constant [29 x i8] c".call.arg3 = load ptr, ptr %\00"
@.str.32660 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32662 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32666 = private unnamed_addr constant [37 x i8] c".call = call i32 @csec_token_is(ptr \00"
@.str.32670 = private unnamed_addr constant [17 x i8] c".call.arg0, i32 \00"
@.str.32674 = private unnamed_addr constant [16 x i8] c".call.arg1, i8 \00"
@.str.32678 = private unnamed_addr constant [17 x i8] c".call.arg2, ptr \00"
@.str.32682 = private unnamed_addr constant [13 x i8] c".call.arg3)\0A\00"
@.str.32684 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32688 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.32692 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.32696 = private unnamed_addr constant [8 x i8] c".call, \00"
@.str.32707 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32740 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32753 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32755 = private unnamed_addr constant [2 x i8] c")\00"
@.str.32828 = private unnamed_addr constant [6 x i8] c".call\00"
@.str.32831 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32835 = private unnamed_addr constant [19 x i8] c".call = call i32 @\00"
@.str.32847 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.32861 = private unnamed_addr constant [6 x i8] c".call\00"
@.str.32864 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.32866 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32870 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.32874 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.32878 = private unnamed_addr constant [8 x i8] c".call, \00"
@.str.32889 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.32922 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.32931 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.32958 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.32962 = private unnamed_addr constant [10 x i8] c" = or i1 \00"
@.str.32966 = private unnamed_addr constant [8 x i8] c".left, \00"
@.str.32970 = private unnamed_addr constant [8 x i8] c".right\0A\00"
@.str.32972 = private unnamed_addr constant [19 x i8] c"  ; bool operands \00"
@.str.32983 = private unnamed_addr constant [3 x i8] c", \00"
@.str.32996 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33026 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.33035 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.33062 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33066 = private unnamed_addr constant [11 x i8] c" = and i1 \00"
@.str.33070 = private unnamed_addr constant [8 x i8] c".left, \00"
@.str.33074 = private unnamed_addr constant [8 x i8] c".right\0A\00"
@.str.33076 = private unnamed_addr constant [19 x i8] c"  ; bool operands \00"
@.str.33087 = private unnamed_addr constant [3 x i8] c", \00"
@.str.33100 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33171 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.33180 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.33184 = private unnamed_addr constant [8 x i8] c".binit.\00"
@.str.33196 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.33200 = private unnamed_addr constant [14 x i8] c" = alloca i1\0A\00"
@.str.33215 = private unnamed_addr constant [12 x i8] c"  store i1 \00"
@.str.33219 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.33223 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33227 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.33231 = private unnamed_addr constant [14 x i8] c" = alloca i1\0A\00"
@.str.33233 = private unnamed_addr constant [24 x i8] c"  store i1 false, ptr %\00"
@.str.33237 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33293 = private unnamed_addr constant [1 x i8] c"\00"
@.str.33340 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.33347 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.33356 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.33360 = private unnamed_addr constant [10 x i8] c".bassign.\00"
@.str.33378 = private unnamed_addr constant [12 x i8] c"  store i1 \00"
@.str.33382 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.33386 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33390 = private unnamed_addr constant [1 x i8] c"\00"
@.str.33473 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.33547 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33551 = private unnamed_addr constant [26 x i8] c" = inttoptr i64 0 to ptr\0A\00"
@.str.33598 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33602 = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.33606 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.33610 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.33634 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33638 = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.33654 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33688 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.33701 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.33703 = private unnamed_addr constant [2 x i8] c")\00"
@.str.33727 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33731 = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.33743 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.33760 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.33794 = private unnamed_addr constant [2 x i8] c".\00"
@.str.33809 = private unnamed_addr constant [10 x i8] c"substring\00"
@.str.33824 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.33837 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.33839 = private unnamed_addr constant [2 x i8] c")\00"
@.str.33864 = private unnamed_addr constant [2 x i8] c",\00"
@.str.33903 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33907 = private unnamed_addr constant [23 x i8] c".obj = load ptr, ptr %\00"
@.str.33911 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.33925 = private unnamed_addr constant [7 x i8] c".start\00"
@.str.33942 = private unnamed_addr constant [8 x i8] c".length\00"
@.str.33945 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.33949 = private unnamed_addr constant [40 x i8] c" = call ptr @csec_string_substring(ptr \00"
@.str.33953 = private unnamed_addr constant [11 x i8] c".obj, i32 \00"
@.str.33957 = private unnamed_addr constant [13 x i8] c".start, i32 \00"
@.str.33961 = private unnamed_addr constant [10 x i8] c".length)\0A\00"
@.str.34005 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.34084 = private unnamed_addr constant [1 x i8] c"\00"
@.str.34091 = private unnamed_addr constant [1 x i8] c"\00"
@.str.34151 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.34185 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34198 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34200 = private unnamed_addr constant [2 x i8] c")\00"
@.str.34224 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.34258 = private unnamed_addr constant [2 x i8] c".\00"
@.str.34273 = private unnamed_addr constant [10 x i8] c"substring\00"
@.str.34288 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34301 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34303 = private unnamed_addr constant [2 x i8] c")\00"
@.str.34336 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.34353 = private unnamed_addr constant [6 x i8] c".left\00"
@.str.34380 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34384 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_char(i8 \00"
@.str.34396 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.34435 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.34441 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34445 = private unnamed_addr constant [26 x i8] c".left.i8 = load i8, ptr %\00"
@.str.34461 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.34463 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34467 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_char(i8 \00"
@.str.34471 = private unnamed_addr constant [11 x i8] c".left.i8)\0A\00"
@.str.34488 = private unnamed_addr constant [10 x i8] c".left.i32\00"
@.str.34491 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34495 = private unnamed_addr constant [22 x i8] c".left.i64 = sext i32 \00"
@.str.34499 = private unnamed_addr constant [18 x i8] c".left.i32 to i64\0A\00"
@.str.34501 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34505 = private unnamed_addr constant [42 x i8] c".left = call ptr @csec_to_string_i64(i64 \00"
@.str.34509 = private unnamed_addr constant [12 x i8] c".left.i64)\0A\00"
@.str.34570 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.34604 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34617 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34619 = private unnamed_addr constant [2 x i8] c")\00"
@.str.34643 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.34677 = private unnamed_addr constant [2 x i8] c".\00"
@.str.34692 = private unnamed_addr constant [10 x i8] c"substring\00"
@.str.34707 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34720 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.34722 = private unnamed_addr constant [2 x i8] c")\00"
@.str.34755 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.34772 = private unnamed_addr constant [7 x i8] c".right\00"
@.str.34799 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34803 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_char(i8 \00"
@.str.34815 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.34854 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.34860 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34864 = private unnamed_addr constant [27 x i8] c".right.i8 = load i8, ptr %\00"
@.str.34880 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.34882 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34886 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_char(i8 \00"
@.str.34890 = private unnamed_addr constant [12 x i8] c".right.i8)\0A\00"
@.str.34907 = private unnamed_addr constant [11 x i8] c".right.i32\00"
@.str.34910 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34914 = private unnamed_addr constant [23 x i8] c".right.i64 = sext i32 \00"
@.str.34918 = private unnamed_addr constant [19 x i8] c".right.i32 to i64\0A\00"
@.str.34920 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34924 = private unnamed_addr constant [43 x i8] c".right = call ptr @csec_to_string_i64(i64 \00"
@.str.34928 = private unnamed_addr constant [13 x i8] c".right.i64)\0A\00"
@.str.34936 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34940 = private unnamed_addr constant [37 x i8] c" = call ptr @csec_string_concat(ptr \00"
@.str.34944 = private unnamed_addr constant [12 x i8] c".left, ptr \00"
@.str.34948 = private unnamed_addr constant [9 x i8] c".right)\0A\00"
@.str.34952 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.34956 = private unnamed_addr constant [26 x i8] c" = inttoptr i64 0 to ptr\0A\00"
@.str.35014 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.35023 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.35027 = private unnamed_addr constant [8 x i8] c".pinit.\00"
@.str.35067 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.35071 = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.35084 = private unnamed_addr constant [13 x i8] c"  store ptr \00"
@.str.35088 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.35092 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.35096 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.35100 = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.35102 = private unnamed_addr constant [24 x i8] c"  store ptr null, ptr %\00"
@.str.35106 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.35216 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.35251 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.35264 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.35266 = private unnamed_addr constant [2 x i8] c")\00"
@.str.35292 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.35327 = private unnamed_addr constant [2 x i8] c".\00"
@.str.35342 = private unnamed_addr constant [10 x i8] c"substring\00"
@.str.35357 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.35370 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.35372 = private unnamed_addr constant [2 x i8] c")\00"
@.str.35424 = private unnamed_addr constant [2 x i8] c"+\00"
@.str.35543 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.35550 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.35567 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.35581 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.35614 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.35647 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.35680 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.35713 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.35748 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.35763 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.35778 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.35793 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.35808 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.35823 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.35850 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.35881 = private unnamed_addr constant [2 x i8] c";\00"
@.str.35889 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.35900 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.35932 = private unnamed_addr constant [2 x i8] c";\00"
@.str.35966 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.35974 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.35978 = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.35982 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.35990 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.35992 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.35996 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36028 = private unnamed_addr constant [2 x i8] c";\00"
@.str.36075 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.36085 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.36092 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.36099 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.36106 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.36113 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.36121 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.36125 = private unnamed_addr constant [9 x i8] c" = load \00"
@.str.36129 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.36142 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36144 = private unnamed_addr constant [7 x i8] c"  ret \00"
@.str.36148 = private unnamed_addr constant [2 x i8] c" \00"
@.str.36152 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36175 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.36179 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36218 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.36237 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.36239 = private unnamed_addr constant [2 x i8] c")\00"
@.str.36276 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.36293 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.36300 = private unnamed_addr constant [3 x i8] c"i8\00"
@.str.36307 = private unnamed_addr constant [3 x i8] c"i1\00"
@.str.36314 = private unnamed_addr constant [4 x i8] c"i64\00"
@.str.36321 = private unnamed_addr constant [7 x i8] c"double\00"
@.str.36343 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.36347 = private unnamed_addr constant [9 x i8] c" = call \00"
@.str.36351 = private unnamed_addr constant [3 x i8] c" @\00"
@.str.36365 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.36380 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.36382 = private unnamed_addr constant [7 x i8] c"  ret \00"
@.str.36386 = private unnamed_addr constant [2 x i8] c" \00"
@.str.36390 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36413 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.36417 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36442 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.36463 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.36467 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36478 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.36499 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.36503 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36515 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.36574 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.36576 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.36602 = private unnamed_addr constant [18 x i8] c"  ; malformed if\0A\00"
@.str.36613 = private unnamed_addr constant [1 x i8] c"\00"
@.str.36652 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.36656 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36674 = private unnamed_addr constant [1 x i8] c"\00"
@.str.36682 = private unnamed_addr constant [1 x i8] c"\00"
@.str.36689 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.36693 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36711 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.36741 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.36758 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.36760 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.36799 = private unnamed_addr constant [1 x i8] c"\00"
@.str.36819 = private unnamed_addr constant [7 x i8] c"%cond.\00"
@.str.36824 = private unnamed_addr constant [15 x i8] c"  br i1 %cond.\00"
@.str.36828 = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.36832 = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.36836 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.36838 = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.36842 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.36848 = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.36852 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.36858 = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.36862 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.36874 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.36933 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.36935 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.36961 = private unnamed_addr constant [21 x i8] c"  ; malformed while\0A\00"
@.str.36972 = private unnamed_addr constant [1 x i8] c"\00"
@.str.36997 = private unnamed_addr constant [24 x i8] c"  br label %while.cond.\00"
@.str.37001 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37019 = private unnamed_addr constant [1 x i8] c"\00"
@.str.37026 = private unnamed_addr constant [24 x i8] c"  br label %while.cond.\00"
@.str.37030 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37032 = private unnamed_addr constant [12 x i8] c"while.cond.\00"
@.str.37036 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37048 = private unnamed_addr constant [12 x i8] c"%whilecond.\00"
@.str.37053 = private unnamed_addr constant [20 x i8] c"  br i1 %whilecond.\00"
@.str.37057 = private unnamed_addr constant [21 x i8] c", label %while.body.\00"
@.str.37061 = private unnamed_addr constant [20 x i8] c", label %while.end.\00"
@.str.37065 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37067 = private unnamed_addr constant [12 x i8] c"while.body.\00"
@.str.37071 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37077 = private unnamed_addr constant [11 x i8] c"while.end.\00"
@.str.37081 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37093 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.37152 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.37154 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.37180 = private unnamed_addr constant [19 x i8] c"  ; malformed for\0A\00"
@.str.37205 = private unnamed_addr constant [28 x i8] c"  ; malformed for iterator\0A\00"
@.str.37240 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.37254 = private unnamed_addr constant [26 x i8] c"  ; malformed for source\0A\00"
@.str.37289 = private unnamed_addr constant [28 x i8] c"  ; unsupported for source\0A\00"
@.str.37300 = private unnamed_addr constant [1 x i8] c"\00"
@.str.37309 = private unnamed_addr constant [4 x i8] c"slt\00"
@.str.37322 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.37334 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.37340 = private unnamed_addr constant [4 x i8] c"sle\00"
@.str.37364 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.37368 = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.37372 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.37376 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37378 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.37382 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.37386 = private unnamed_addr constant [13 x i8] c" = add i32 %\00"
@.str.37390 = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.37394 = private unnamed_addr constant [5 x i8] c", 1\0A\00"
@.str.37396 = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.37400 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.37404 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.37408 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37410 = private unnamed_addr constant [22 x i8] c"  br label %for.cond.\00"
@.str.37414 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37432 = private unnamed_addr constant [1 x i8] c"\00"
@.str.37439 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.37443 = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.37455 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.37459 = private unnamed_addr constant [7 x i8] c".start\00"
@.str.37462 = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.37466 = private unnamed_addr constant [14 x i8] c".start, ptr %\00"
@.str.37470 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37472 = private unnamed_addr constant [22 x i8] c"  br label %for.cond.\00"
@.str.37476 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37478 = private unnamed_addr constant [10 x i8] c"for.cond.\00"
@.str.37482 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37484 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.37488 = private unnamed_addr constant [23 x i8] c".val = load i32, ptr %\00"
@.str.37492 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37504 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.37508 = private unnamed_addr constant [5 x i8] c".end\00"
@.str.37511 = private unnamed_addr constant [12 x i8] c"  %forcond.\00"
@.str.37515 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.37519 = private unnamed_addr constant [7 x i8] c" i32 %\00"
@.str.37523 = private unnamed_addr constant [8 x i8] c".val, %\00"
@.str.37527 = private unnamed_addr constant [6 x i8] c".end\0A\00"
@.str.37529 = private unnamed_addr constant [18 x i8] c"  br i1 %forcond.\00"
@.str.37533 = private unnamed_addr constant [19 x i8] c", label %for.body.\00"
@.str.37537 = private unnamed_addr constant [18 x i8] c", label %for.end.\00"
@.str.37541 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.37543 = private unnamed_addr constant [10 x i8] c"for.body.\00"
@.str.37547 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37553 = private unnamed_addr constant [9 x i8] c"for.end.\00"
@.str.37557 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.37597 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.37609 = private unnamed_addr constant [27 x i8] c"csec_string_builder_append\00"
@.str.37628 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.37630 = private unnamed_addr constant [2 x i8] c")\00"
@.str.37647 = private unnamed_addr constant [9 x i8] c"%append.\00"
@.str.37668 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.37672 = private unnamed_addr constant [41 x i8] c" = call i32 @csec_string_builder_append(\00"
@.str.37687 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.37732 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.37758 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.37784 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.37810 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.37839 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.37984 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.37986 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.38009 = private unnamed_addr constant [18 x i8] c"  ; malformed if\0A\00"
@.str.38017 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38056 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.38060 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38078 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38086 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38093 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.38097 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38115 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.38145 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.38174 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.38191 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.38193 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.38232 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38249 = private unnamed_addr constant [7 x i8] c"%cond.\00"
@.str.38254 = private unnamed_addr constant [15 x i8] c"  br i1 %cond.\00"
@.str.38258 = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.38262 = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.38266 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38268 = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.38272 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38278 = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.38282 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38288 = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.38292 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38368 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.38370 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.38393 = private unnamed_addr constant [21 x i8] c"  ; malformed while\0A\00"
@.str.38401 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38426 = private unnamed_addr constant [24 x i8] c"  br label %while.cond.\00"
@.str.38430 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38448 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38452 = private unnamed_addr constant [24 x i8] c"  br label %while.cond.\00"
@.str.38456 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38458 = private unnamed_addr constant [12 x i8] c"while.cond.\00"
@.str.38462 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38474 = private unnamed_addr constant [12 x i8] c"%whilecond.\00"
@.str.38479 = private unnamed_addr constant [20 x i8] c"  br i1 %whilecond.\00"
@.str.38483 = private unnamed_addr constant [21 x i8] c", label %while.body.\00"
@.str.38487 = private unnamed_addr constant [20 x i8] c", label %while.end.\00"
@.str.38491 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38493 = private unnamed_addr constant [12 x i8] c"while.body.\00"
@.str.38497 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38503 = private unnamed_addr constant [11 x i8] c"while.end.\00"
@.str.38507 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38583 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.38585 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.38608 = private unnamed_addr constant [19 x i8] c"  ; malformed for\0A\00"
@.str.38628 = private unnamed_addr constant [28 x i8] c"  ; malformed for iterator\0A\00"
@.str.38660 = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.38671 = private unnamed_addr constant [26 x i8] c"  ; malformed for source\0A\00"
@.str.38700 = private unnamed_addr constant [28 x i8] c"  ; unsupported for source\0A\00"
@.str.38708 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38717 = private unnamed_addr constant [4 x i8] c"slt\00"
@.str.38730 = private unnamed_addr constant [3 x i8] c"to\00"
@.str.38742 = private unnamed_addr constant [3 x i8] c"..\00"
@.str.38748 = private unnamed_addr constant [4 x i8] c"sle\00"
@.str.38772 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.38776 = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.38780 = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.38784 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38786 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.38790 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.38794 = private unnamed_addr constant [13 x i8] c" = add i32 %\00"
@.str.38798 = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.38802 = private unnamed_addr constant [5 x i8] c", 1\0A\00"
@.str.38804 = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.38808 = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.38812 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.38816 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38818 = private unnamed_addr constant [22 x i8] c"  br label %for.cond.\00"
@.str.38822 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38840 = private unnamed_addr constant [1 x i8] c"\00"
@.str.38844 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.38848 = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.38860 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.38864 = private unnamed_addr constant [7 x i8] c".start\00"
@.str.38867 = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.38871 = private unnamed_addr constant [14 x i8] c".start, ptr %\00"
@.str.38875 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38877 = private unnamed_addr constant [22 x i8] c"  br label %for.cond.\00"
@.str.38881 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38883 = private unnamed_addr constant [10 x i8] c"for.cond.\00"
@.str.38887 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38889 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.38893 = private unnamed_addr constant [23 x i8] c".val = load i32, ptr %\00"
@.str.38897 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38909 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.38913 = private unnamed_addr constant [5 x i8] c".end\00"
@.str.38916 = private unnamed_addr constant [12 x i8] c"  %forcond.\00"
@.str.38920 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.38924 = private unnamed_addr constant [7 x i8] c" i32 %\00"
@.str.38928 = private unnamed_addr constant [8 x i8] c".val, %\00"
@.str.38932 = private unnamed_addr constant [6 x i8] c".end\0A\00"
@.str.38934 = private unnamed_addr constant [18 x i8] c"  br i1 %forcond.\00"
@.str.38938 = private unnamed_addr constant [19 x i8] c", label %for.body.\00"
@.str.38942 = private unnamed_addr constant [18 x i8] c", label %for.end.\00"
@.str.38946 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.38948 = private unnamed_addr constant [10 x i8] c"for.body.\00"
@.str.38952 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.38958 = private unnamed_addr constant [9 x i8] c"for.end.\00"
@.str.38962 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.39020 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.39051 = private unnamed_addr constant [2 x i8] c";\00"
@.str.39056 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.39067 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.39088 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.39091 = private unnamed_addr constant [16 x i8] c"  ret i32 %ret\0A\00"
@.str.39102 = private unnamed_addr constant [13 x i8] c"  ret i32 0\0A\00"
@.str.39135 = private unnamed_addr constant [7 x i8] c"System\00"
@.str.39150 = private unnamed_addr constant [2 x i8] c".\00"
@.str.39165 = private unnamed_addr constant [3 x i8] c"IO\00"
@.str.39180 = private unnamed_addr constant [2 x i8] c".\00"
@.str.39195 = private unnamed_addr constant [5 x i8] c"File\00"
@.str.39210 = private unnamed_addr constant [2 x i8] c".\00"
@.str.39225 = private unnamed_addr constant [13 x i8] c"WriteAllText\00"
@.str.39240 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.39259 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.39261 = private unnamed_addr constant [2 x i8] c")\00"
@.str.39278 = private unnamed_addr constant [13 x i8] c"%file.write.\00"
@.str.39296 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.39300 = private unnamed_addr constant [39 x i8] c" = call i32 @csec_file_write_all_text(\00"
@.str.39315 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.39331 = private unnamed_addr constant [7 x i8] c"System\00"
@.str.39346 = private unnamed_addr constant [2 x i8] c".\00"
@.str.39361 = private unnamed_addr constant [8 x i8] c"Console\00"
@.str.39376 = private unnamed_addr constant [2 x i8] c".\00"
@.str.39391 = private unnamed_addr constant [10 x i8] c"WriteLine\00"
@.str.39406 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.39425 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.39427 = private unnamed_addr constant [2 x i8] c")\00"
@.str.39444 = private unnamed_addr constant [16 x i8] c"%console.write.\00"
@.str.39462 = private unnamed_addr constant [32 x i8] c"  call void @csec_print_string(\00"
@.str.39477 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.39479 = private unnamed_addr constant [35 x i8] c"  call void @csec_print_newline()\0A\00"
@.str.39484 = private unnamed_addr constant [1 x i8] c"\00"
@.str.39576 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.39583 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.39600 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.39614 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.39646 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.39678 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.39710 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.39742 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.39778 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.39804 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.39835 = private unnamed_addr constant [2 x i8] c";\00"
@.str.39843 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.39854 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.39891 = private unnamed_addr constant [2 x i8] c";\00"
@.str.39929 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.39947 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.39951 = private unnamed_addr constant [5 x i8] c".ret\00"
@.str.39954 = private unnamed_addr constant [24 x i8] c"  %ret.bool = zext i1 %\00"
@.str.39958 = private unnamed_addr constant [13 x i8] c".ret to i32\0A\00"
@.str.39960 = private unnamed_addr constant [21 x i8] c"  ret i32 %ret.bool\0A\00"
@.str.39976 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.39994 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.39998 = private unnamed_addr constant [5 x i8] c".ret\00"
@.str.40001 = private unnamed_addr constant [32 x i8] c"  %ret.double = fptosi double %\00"
@.str.40005 = private unnamed_addr constant [13 x i8] c".ret to i32\0A\00"
@.str.40007 = private unnamed_addr constant [23 x i8] c"  ret i32 %ret.double\0A\00"
@.str.40031 = private unnamed_addr constant [2 x i8] c"%\00"
@.str.40035 = private unnamed_addr constant [5 x i8] c".ret\00"
@.str.40038 = private unnamed_addr constant [12 x i8] c"  ret i32 %\00"
@.str.40042 = private unnamed_addr constant [6 x i8] c".ret\0A\00"
@.str.40066 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.40069 = private unnamed_addr constant [16 x i8] c"  ret i32 %ret\0A\00"
@.str.40085 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.40110 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.40135 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.40161 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.40169 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.40177 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.40185 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.40193 = private unnamed_addr constant [6 x i8] c"while\00"
@.str.40201 = private unnamed_addr constant [4 x i8] c"for\00"
@.str.40281 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.40306 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.40331 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.40356 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.40384 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.40410 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.40418 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.40426 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.40434 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.40445 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.40515 = private unnamed_addr constant [13 x i8] c"  ret i32 0\0A\00"
@.str.40582 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40605 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40622 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40668 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.40684 = private unnamed_addr constant [2 x i8] c":\00"
@.str.40712 = private unnamed_addr constant [3 x i8] c", \00"
@.str.40724 = private unnamed_addr constant [7 x i8] c" %arg.\00"
@.str.40755 = private unnamed_addr constant [2 x i8] c",\00"
@.str.40834 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40855 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.40857 = private unnamed_addr constant [2 x i8] c")\00"
@.str.40868 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40885 = private unnamed_addr constant [1 x i8] c"\00"
@.str.40936 = private unnamed_addr constant [8 x i8] c"unknown\00"
@.str.40952 = private unnamed_addr constant [2 x i8] c":\00"
@.str.40984 = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.40988 = private unnamed_addr constant [11 x i8] c" = alloca \00"
@.str.40992 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.40994 = private unnamed_addr constant [9 x i8] c"  store \00"
@.str.40998 = private unnamed_addr constant [7 x i8] c" %arg.\00"
@.str.41002 = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.41006 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41026 = private unnamed_addr constant [2 x i8] c",\00"
@.str.41137 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.41148 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.41273 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.41277 = private unnamed_addr constant [28 x i8] c".left.i32 = load i32, ptr %\00"
@.str.41281 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41283 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.41287 = private unnamed_addr constant [25 x i8] c".right.i32 = add i32 0, \00"
@.str.41298 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41300 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.41304 = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.41308 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.41312 = private unnamed_addr constant [12 x i8] c".left.i32, \00"
@.str.41316 = private unnamed_addr constant [12 x i8] c".right.i32\0A\00"
@.str.41318 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.41322 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41359 = private unnamed_addr constant [2 x i8] c";\00"
@.str.41380 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.41384 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41421 = private unnamed_addr constant [2 x i8] c";\00"
@.str.41429 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.41433 = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.41453 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41455 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.41459 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41486 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.41490 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41507 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.41566 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.41568 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.41594 = private unnamed_addr constant [18 x i8] c"  ; malformed if\0A\00"
@.str.41605 = private unnamed_addr constant [1 x i8] c"\00"
@.str.41628 = private unnamed_addr constant [1 x i8] c"\00"
@.str.41635 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.41639 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41670 = private unnamed_addr constant [1 x i8] c"\00"
@.str.41694 = private unnamed_addr constant [1 x i8] c"\00"
@.str.41701 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.41705 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41723 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.41753 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.41782 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.41799 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.41801 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.41840 = private unnamed_addr constant [1 x i8] c"\00"
@.str.41876 = private unnamed_addr constant [7 x i8] c"%cond.\00"
@.str.41881 = private unnamed_addr constant [15 x i8] c"  br i1 %cond.\00"
@.str.41885 = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.41889 = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.41893 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.41895 = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.41899 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.41905 = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.41909 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.41915 = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.41919 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.41955 = private unnamed_addr constant [16 x i8] c"  ret i1 false\0A\00"
@.str.42047 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.42078 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42086 = private unnamed_addr constant [10 x i8] c"  ret i8 \00"
@.str.42100 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.42137 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42145 = private unnamed_addr constant [24 x i8] c"  %ret = load i8, ptr %\00"
@.str.42165 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.42167 = private unnamed_addr constant [15 x i8] c"  ret i8 %ret\0A\00"
@.str.42210 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.42229 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.42231 = private unnamed_addr constant [2 x i8] c")\00"
@.str.42248 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.42269 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.42273 = private unnamed_addr constant [13 x i8] c" = call i8 @\00"
@.str.42287 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.42302 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.42304 = private unnamed_addr constant [10 x i8] c"  ret i8 \00"
@.str.42308 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.42348 = private unnamed_addr constant [12 x i8] c"  ret i8 0\0A\00"
@.str.42440 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.42486 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42504 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.42507 = private unnamed_addr constant [19 x i8] c"  ret double %ret\0A\00"
@.str.42544 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42552 = private unnamed_addr constant [28 x i8] c"  %ret = load double, ptr %\00"
@.str.42572 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.42574 = private unnamed_addr constant [19 x i8] c"  ret double %ret\0A\00"
@.str.42598 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.42601 = private unnamed_addr constant [19 x i8] c"  ret double %ret\0A\00"
@.str.42639 = private unnamed_addr constant [27 x i8] c"  ret double 0.000000e+00\0A\00"
@.str.42731 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.42762 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42780 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.42783 = private unnamed_addr constant [16 x i8] c"  ret i64 %ret\0A\00"
@.str.42820 = private unnamed_addr constant [2 x i8] c";\00"
@.str.42828 = private unnamed_addr constant [25 x i8] c"  %ret = load i64, ptr %\00"
@.str.42848 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.42850 = private unnamed_addr constant [16 x i8] c"  ret i64 %ret\0A\00"
@.str.42874 = private unnamed_addr constant [5 x i8] c"%ret\00"
@.str.42877 = private unnamed_addr constant [16 x i8] c"  ret i64 %ret\0A\00"
@.str.42915 = private unnamed_addr constant [13 x i8] c"  ret i64 0\0A\00"
@.str.43031 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.43040 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.43071 = private unnamed_addr constant [2 x i8] c";\00"
@.str.43103 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.43107 = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.43111 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.43119 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.43121 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.43125 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43162 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.43181 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.43183 = private unnamed_addr constant [2 x i8] c")\00"
@.str.43212 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.43216 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43254 = private unnamed_addr constant [2 x i8] c";\00"
@.str.43262 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.43266 = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.43286 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43288 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.43292 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43333 = private unnamed_addr constant [11 x i8] c"  ret ptr \00"
@.str.43337 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43351 = private unnamed_addr constant [16 x i8] c"  ret ptr null\0A\00"
@.str.43368 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.43375 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.43401 = private unnamed_addr constant [3 x i8] c"if\00"
@.str.43460 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.43462 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.43488 = private unnamed_addr constant [18 x i8] c"  ; malformed if\0A\00"
@.str.43499 = private unnamed_addr constant [1 x i8] c"\00"
@.str.43522 = private unnamed_addr constant [1 x i8] c"\00"
@.str.43529 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.43533 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43564 = private unnamed_addr constant [1 x i8] c"\00"
@.str.43588 = private unnamed_addr constant [1 x i8] c"\00"
@.str.43595 = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
@.str.43599 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43617 = private unnamed_addr constant [5 x i8] c"else\00"
@.str.43647 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.43664 = private unnamed_addr constant [2 x i8] c"{\00"
@.str.43666 = private unnamed_addr constant [2 x i8] c"}\00"
@.str.43705 = private unnamed_addr constant [1 x i8] c"\00"
@.str.43741 = private unnamed_addr constant [7 x i8] c"%cond.\00"
@.str.43746 = private unnamed_addr constant [15 x i8] c"  br i1 %cond.\00"
@.str.43750 = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.43754 = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.43758 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.43760 = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.43764 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.43770 = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.43774 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.43780 = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.43784 = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.43850 = private unnamed_addr constant [16 x i8] c"  ret ptr null\0A\00"
@.str.43942 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.43950 = private unnamed_addr constant [12 x i8] c"  ret void\0A\00"
@.str.43988 = private unnamed_addr constant [12 x i8] c"  ret void\0A\00"
@.str.44102 = private unnamed_addr constant [7 x i8] c"@.str.\00"
@.str.44106 = private unnamed_addr constant [35 x i8] c" = private unnamed_addr constant [\00"
@.str.44110 = private unnamed_addr constant [10 x i8] c" x i8] c\22\00"
@.str.44117 = private unnamed_addr constant [3 x i8] c"\22\0A\00"
@.str.44141 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.44191 = private unnamed_addr constant [5 x i8] c"main\00"
@.str.44197 = private unnamed_addr constant [15 x i8] c"csec_user_main\00"
@.str.44338 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.44360 = private unnamed_addr constant [12 x i8] c"define i8 @\00"
@.str.44364 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.44366 = private unnamed_addr constant [10 x i8] c"  ret i8 \00"
@.str.44378 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.44387 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.44409 = private unnamed_addr constant [13 x i8] c"define i32 @\00"
@.str.44413 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.44415 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.44424 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.44433 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.44455 = private unnamed_addr constant [12 x i8] c"define i1 @\00"
@.str.44459 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.44461 = private unnamed_addr constant [10 x i8] c"  ret i1 \00"
@.str.44470 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.44482 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.44526 = private unnamed_addr constant [13 x i8] c"define ptr @\00"
@.str.44530 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.44532 = private unnamed_addr constant [34 x i8] c"  %ret = getelementptr inbounds [\00"
@.str.44536 = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.44540 = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.44542 = private unnamed_addr constant [19 x i8] c"  ret ptr %ret\0A}\0A\0A\00"
@.str.44558 = private unnamed_addr constant [1 x i8] c"\00"
@.str.44570 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.44650 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.44662 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.44675 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.44703 = private unnamed_addr constant [2 x i8] c";\00"
@.str.44742 = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.44747 = private unnamed_addr constant [13 x i8] c"define ptr @\00"
@.str.44751 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.44760 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.44762 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.44780 = private unnamed_addr constant [25 x i8] c"  %ret = load ptr, ptr %\00"
@.str.44784 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.44786 = private unnamed_addr constant [19 x i8] c"  ret ptr %ret\0A}\0A\0A\00"
@.str.44891 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.44898 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.44905 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.44919 = private unnamed_addr constant [6 x i8] c"value\00"
@.str.44931 = private unnamed_addr constant [9 x i8] c"variable\00"
@.str.44958 = private unnamed_addr constant [2 x i8] c"=\00"
@.str.44970 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.44994 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.44999 = private unnamed_addr constant [12 x i8] c"define i8 @\00"
@.str.45003 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.45005 = private unnamed_addr constant [10 x i8] c"  ret i8 \00"
@.str.45019 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.45028 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.45033 = private unnamed_addr constant [16 x i8] c"define double @\00"
@.str.45037 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.45039 = private unnamed_addr constant [14 x i8] c"  ret double \00"
@.str.45050 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.45054 = private unnamed_addr constant [13 x i8] c"define i64 @\00"
@.str.45058 = private unnamed_addr constant [13 x i8] c"() {\0Aentry:\0A\00"
@.str.45060 = private unnamed_addr constant [11 x i8] c"  ret i64 \00"
@.str.45071 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.45080 = private unnamed_addr constant [4 x i8] c"Int\00"
@.str.45121 = private unnamed_addr constant [7 x i8] c"return\00"
@.str.45149 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45168 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45170 = private unnamed_addr constant [2 x i8] c")\00"
@.str.45187 = private unnamed_addr constant [6 x i8] c"%ret.\00"
@.str.45192 = private unnamed_addr constant [13 x i8] c"define i32 @\00"
@.str.45196 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45205 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45207 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45229 = private unnamed_addr constant [3 x i8] c"  \00"
@.str.45233 = private unnamed_addr constant [14 x i8] c" = call i32 @\00"
@.str.45247 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45262 = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.45264 = private unnamed_addr constant [11 x i8] c"  ret i32 \00"
@.str.45268 = private unnamed_addr constant [5 x i8] c"\0A}\0A\0A\00"
@.str.45287 = private unnamed_addr constant [13 x i8] c"define i32 @\00"
@.str.45291 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45300 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45302 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45313 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45322 = private unnamed_addr constant [8 x i8] c"Boolean\00"
@.str.45327 = private unnamed_addr constant [12 x i8] c"define i1 @\00"
@.str.45331 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45340 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45342 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45360 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45369 = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.45374 = private unnamed_addr constant [12 x i8] c"define i8 @\00"
@.str.45378 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45387 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45389 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45407 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45416 = private unnamed_addr constant [7 x i8] c"Double\00"
@.str.45421 = private unnamed_addr constant [16 x i8] c"define double @\00"
@.str.45425 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45434 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45436 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45454 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45463 = private unnamed_addr constant [5 x i8] c"Long\00"
@.str.45468 = private unnamed_addr constant [13 x i8] c"define i64 @\00"
@.str.45472 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45481 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45483 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45501 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45510 = private unnamed_addr constant [5 x i8] c"Unit\00"
@.str.45515 = private unnamed_addr constant [14 x i8] c"define void @\00"
@.str.45519 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45528 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45530 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45548 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45560 = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.45565 = private unnamed_addr constant [13 x i8] c"define ptr @\00"
@.str.45569 = private unnamed_addr constant [2 x i8] c"(\00"
@.str.45578 = private unnamed_addr constant [5 x i8] c") {\0A\00"
@.str.45580 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.45598 = private unnamed_addr constant [4 x i8] c"}\0A\0A\00"
@.str.45602 = private unnamed_addr constant [1 x i8] c"\00"
@.str.45628 = private unnamed_addr constant [30 x i8] c"; ModuleID = 'csec.selfhost'\0A\00"
@.str.45640 = private unnamed_addr constant [36 x i8] c"source_filename = \22csec.selfhost\22\0A\0A\00"
@.str.45652 = private unnamed_addr constant [38 x i8] c"declare i64 @csec_string_length(ptr)\0A\00"
@.str.45664 = private unnamed_addr constant [44 x i8] c"declare i8 @csec_string_char_at(ptr, i32)\0A\0A\00"
@.str.45676 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_lex_quoted(ptr, i32)\0A\00"
@.str.45688 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_lex_number(ptr, i32)\0A\00"
@.str.45700 = private unnamed_addr constant [44 x i8] c"declare i32 @csec_lex_identifier(ptr, i32)\0A\00"
@.str.45712 = private unnamed_addr constant [46 x i8] c"declare i32 @csec_lex_line_comment(ptr, i32)\0A\00"
@.str.45724 = private unnamed_addr constant [47 x i8] c"declare i32 @csec_lex_block_comment(ptr, i32)\0A\00"
@.str.45736 = private unnamed_addr constant [35 x i8] c"declare i32 @csec_digit_value(i8)\0A\00"
@.str.45748 = private unnamed_addr constant [35 x i8] c"declare i32 @csec_is_keyword(ptr)\0A\00"
@.str.45760 = private unnamed_addr constant [31 x i8] c"declare i32 @csec_to_int(ptr)\0A\00"
@.str.45772 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_line_start(ptr, i32)\0A\00"
@.str.45784 = private unnamed_addr constant [38 x i8] c"declare i32 @csec_line_end(ptr, i32)\0A\00"
@.str.45796 = private unnamed_addr constant [42 x i8] c"declare i32 @csec_validate_balanced(ptr)\0A\00"
@.str.45808 = private unnamed_addr constant [58 x i8] c"declare i32 @csec_starts_top_level_declaration(ptr, i32)\0A\00"
@.str.45820 = private unnamed_addr constant [43 x i8] c"declare i32 @csec_validate_top_level(ptr)\0A\00"
@.str.45832 = private unnamed_addr constant [49 x i8] c"declare ptr @csec_top_level_decl_kind(ptr, i32)\0A\00"
@.str.45844 = private unnamed_addr constant [49 x i8] c"declare ptr @csec_top_level_decl_name(ptr, i32)\0A\00"
@.str.45856 = private unnamed_addr constant [51 x i8] c"declare i32 @csec_count_function_params(ptr, i32)\0A\00"
@.str.45868 = private unnamed_addr constant [62 x i8] c"declare i32 @csec_token_is_top_level_operator(ptr, i32, i32)\0A\00"
@.str.45880 = private unnamed_addr constant [63 x i8] c"declare i32 @csec_find_top_level_operator(ptr, i32, i32, i32)\0A\00"
@.str.45892 = private unnamed_addr constant [55 x i8] c"declare i32 @csec_find_top_level_match(ptr, i32, i32)\0A\00"
@.str.45904 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_find_lambda_arrow(ptr, i32, i32)\0A\00"
@.str.45916 = private unnamed_addr constant [46 x i8] c"declare ptr @csec_generate_symbol_table(ptr)\0A\00"
@.str.45928 = private unnamed_addr constant [58 x i8] c"declare ptr @csec_append_expression_until(ptr, i32, i32)\0A\00"
@.str.45940 = private unnamed_addr constant [58 x i8] c"declare ptr @csec_generate_expression_ast(ptr, i32, i32)\0A\00"
@.str.45952 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_generate_statement_ast(ptr, i32, i32)\0A\00"
@.str.45964 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_generate_body_ast(ptr, i32, i32)\0A\00"
@.str.45976 = private unnamed_addr constant [55 x i8] c"declare i32 @csec_find_function_param_start(ptr, i32)\0A\00"
@.str.45988 = private unnamed_addr constant [55 x i8] c"declare ptr @csec_generate_c_body(ptr, i32, i32, ptr)\0A\00"
@.str.46000 = private unnamed_addr constant [56 x i8] c"declare ptr @csec_generate_c_expression(ptr, i32, i32)\0A\00"
@.str.46012 = private unnamed_addr constant [41 x i8] c"declare ptr @csec_ir_operator_name(ptr)\0A\00"
@.str.46024 = private unnamed_addr constant [37 x i8] c"declare ptr @csec_ir_type_name(ptr)\0A\00"
@.str.46036 = private unnamed_addr constant [64 x i8] c"declare i32 @csec_expression_top_level_operator(ptr, i32, i32)\0A\00"
@.str.46048 = private unnamed_addr constant [64 x i8] c"declare i32 @csec_find_token_text_in_range(ptr, i32, i32, ptr)\0A\00"
@.str.46060 = private unnamed_addr constant [63 x i8] c"declare i32 @csec_find_closing_token(ptr, i32, i32, ptr, ptr)\0A\00"
@.str.46072 = private unnamed_addr constant [61 x i8] c"declare i32 @csec_find_statement_paren_start(ptr, i32, i32)\0A\00"
@.str.46084 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_find_statement_paren_end(ptr, i32, i32)\0A\00"
@.str.46096 = private unnamed_addr constant [61 x i8] c"declare i32 @csec_find_statement_block_start(ptr, i32, i32)\0A\00"
@.str.46108 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_find_statement_block_end(ptr, i32, i32)\0A\00"
@.str.46120 = private unnamed_addr constant [56 x i8] c"declare i32 @csec_count_comma_separated(ptr, i32, i32)\0A\00"
@.str.46132 = private unnamed_addr constant [65 x i8] c"declare i32 @csec_find_last_top_level_token(ptr, i32, i32, ptr)\0A\00"
@.str.46144 = private unnamed_addr constant [53 x i8] c"declare ptr @csec_llvm_lexer_helper_definition(ptr)\0A\00"
@.str.46156 = private unnamed_addr constant [40 x i8] c"declare i64 @csec_string_builder_new()\0A\00"
@.str.46168 = private unnamed_addr constant [48 x i8] c"declare i64 @csec_string_builder_new_file(ptr)\0A\00"
@.str.46180 = private unnamed_addr constant [51 x i8] c"declare i32 @csec_string_builder_append(i64, ptr)\0A\00"
@.str.46192 = private unnamed_addr constant [46 x i8] c"declare ptr @csec_string_builder_finish(i64)\0A\00"
@.str.46204 = private unnamed_addr constant [58 x i8] c"declare i32 @csec_string_builder_write_to_file(i64, ptr)\0A\00"
@.str.46216 = private unnamed_addr constant [52 x i8] c"declare void @csec_set_command_line_args(i32, ptr)\0A\00"
@.str.46228 = private unnamed_addr constant [44 x i8] c"declare i32 @csec_command_line_arg_count()\0A\00"
@.str.46240 = private unnamed_addr constant [41 x i8] c"declare ptr @csec_command_line_arg(i32)\0A\00"
@.str.46252 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_file_read_all_text(ptr)\0A\00"
@.str.46264 = private unnamed_addr constant [49 x i8] c"declare i32 @csec_file_write_all_text(ptr, ptr)\0A\00"
@.str.46276 = private unnamed_addr constant [38 x i8] c"declare void @csec_print_string(ptr)\0A\00"
@.str.46288 = private unnamed_addr constant [36 x i8] c"declare void @csec_print_newline()\0A\00"
@.str.46300 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_string_concat(ptr, ptr)\0A\00"
@.str.46312 = private unnamed_addr constant [43 x i8] c"declare i32 @csec_string_equals(ptr, ptr)\0A\00"
@.str.46324 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_string_starts_with(ptr, ptr)\0A\00"
@.str.46336 = private unnamed_addr constant [51 x i8] c"declare ptr @csec_string_substring(ptr, i32, i32)\0A\00"
@.str.46348 = private unnamed_addr constant [38 x i8] c"declare ptr @csec_to_string_i64(i64)\0A\00"
@.str.46360 = private unnamed_addr constant [38 x i8] c"declare ptr @csec_to_string_char(i8)\0A\00"
@.str.46372 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_token_append_owned(ptr, i8, ptr)\0A\00"
@.str.46384 = private unnamed_addr constant [39 x i8] c"declare i64 @csec_token_builder_new()\0A\00"
@.str.46396 = private unnamed_addr constant [54 x i8] c"declare i32 @csec_token_builder_append(i64, i8, ptr)\0A\00"
@.str.46408 = private unnamed_addr constant [45 x i8] c"declare ptr @csec_token_builder_finish(i64)\0A\00"
@.str.46420 = private unnamed_addr constant [40 x i8] c"declare ptr @csec_tokenize_source(ptr)\0A\00"
@.str.46432 = private unnamed_addr constant [42 x i8] c"declare i8 @csec_token_kind_at(ptr, i32)\0A\00"
@.str.46444 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_token_text_at(ptr, i32)\0A\00"
@.str.46456 = private unnamed_addr constant [47 x i8] c"declare i32 @csec_token_is(ptr, i32, i8, ptr)\0A\00"
@.str.46468 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_advance_statement(ptr, i32, i32)\0A\00"
@.str.46480 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_advance_top_level_decl(ptr, i32)\0A\00"
@.str.46492 = private unnamed_addr constant [50 x i8] c"declare i32 @csec_find_decl_body_start(ptr, i32)\0A\00"
@.str.46504 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_find_decl_body_end(ptr, i32)\0A\00"
@.str.46516 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_function_param_end(ptr, i32)\0A\00"
@.str.46528 = private unnamed_addr constant [54 x i8] c"declare ptr @csec_function_llvm_param_list(ptr, i32)\0A\00"
@.str.46540 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_function_llvm_param_allocas(ptr, i32)\0A\00"
@.str.46552 = private unnamed_addr constant [51 x i8] c"declare ptr @csec_llvm_name_with_number(ptr, i32)\0A\00"
@.str.46564 = private unnamed_addr constant [50 x i8] c"declare ptr @csec_llvm_string_literal_bytes(ptr)\0A\00"
@.str.46576 = private unnamed_addr constant [56 x i8] c"declare i32 @csec_llvm_string_literal_byte_length(ptr)\0A\00"
@.str.46588 = private unnamed_addr constant [53 x i8] c"declare ptr @csec_function_return_type_at(ptr, i32)\0A\00"
@.str.46600 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_collect_type_name(ptr, i32, i32)\0A\00"
@.str.46612 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_enclosing_function_decl_start(ptr, i32)\0A\00"
@.str.46624 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_enclosing_function_body_start(ptr, i32)\0A\00"
@.str.46636 = private unnamed_addr constant [57 x i8] c"declare i32 @csec_enclosing_function_body_end(ptr, i32)\0A\00"
@.str.46648 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_lookup_function_return_type(ptr, ptr)\0A\00"
@.str.46660 = private unnamed_addr constant [60 x i8] c"declare ptr @csec_lookup_visible_value_type(ptr, i32, ptr)\0A\00"
@.str.46672 = private unnamed_addr constant [62 x i8] c"declare ptr @csec_lookup_visible_storage_name(ptr, i32, ptr)\0A\00"
@.str.46684 = private unnamed_addr constant [40 x i8] c"declare ptr @csec_expand_imports(ptr)\0A\0A\00"
@.str.46749 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.46770 = private unnamed_addr constant [9 x i8] c"external\00"
@.str.46786 = private unnamed_addr constant [5 x i8] c"main\00"
@.str.46803 = private unnamed_addr constant [42 x i8] c"define i32 @main(i32 %argc, ptr %argv) {\0A\00"
@.str.46805 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.46807 = private unnamed_addr constant [63 x i8] c"  call void @csec_set_command_line_args(i32 %argc, ptr %argv)\0A\00"
@.str.46809 = private unnamed_addr constant [37 x i8] c"  %ret = call i32 @csec_user_main()\0A\00"
@.str.46811 = private unnamed_addr constant [16 x i8] c"  ret i32 %ret\0A\00"
@.str.46813 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.46906 = private unnamed_addr constant [22 x i8] c"define i32 @main() {\0A\00"
@.str.46913 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.46952 = private unnamed_addr constant [1 x i8] c"\00"
@.str.46960 = private unnamed_addr constant [30 x i8] c"; ModuleID = 'csec.selfhost'\0A\00"
@.str.46962 = private unnamed_addr constant [36 x i8] c"source_filename = \22csec.selfhost\22\0A\0A\00"
@.str.46964 = private unnamed_addr constant [38 x i8] c"declare i64 @csec_string_length(ptr)\0A\00"
@.str.46966 = private unnamed_addr constant [44 x i8] c"declare i8 @csec_string_char_at(ptr, i32)\0A\0A\00"
@.str.46968 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_lex_quoted(ptr, i32)\0A\00"
@.str.46970 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_lex_number(ptr, i32)\0A\00"
@.str.46972 = private unnamed_addr constant [44 x i8] c"declare i32 @csec_lex_identifier(ptr, i32)\0A\00"
@.str.46974 = private unnamed_addr constant [46 x i8] c"declare i32 @csec_lex_line_comment(ptr, i32)\0A\00"
@.str.46976 = private unnamed_addr constant [47 x i8] c"declare i32 @csec_lex_block_comment(ptr, i32)\0A\00"
@.str.46978 = private unnamed_addr constant [35 x i8] c"declare i32 @csec_digit_value(i8)\0A\00"
@.str.46980 = private unnamed_addr constant [35 x i8] c"declare i32 @csec_is_keyword(ptr)\0A\00"
@.str.46982 = private unnamed_addr constant [31 x i8] c"declare i32 @csec_to_int(ptr)\0A\00"
@.str.46984 = private unnamed_addr constant [40 x i8] c"declare i32 @csec_line_start(ptr, i32)\0A\00"
@.str.46986 = private unnamed_addr constant [38 x i8] c"declare i32 @csec_line_end(ptr, i32)\0A\00"
@.str.46988 = private unnamed_addr constant [42 x i8] c"declare i32 @csec_validate_balanced(ptr)\0A\00"
@.str.46990 = private unnamed_addr constant [58 x i8] c"declare i32 @csec_starts_top_level_declaration(ptr, i32)\0A\00"
@.str.46992 = private unnamed_addr constant [43 x i8] c"declare i32 @csec_validate_top_level(ptr)\0A\00"
@.str.46994 = private unnamed_addr constant [49 x i8] c"declare ptr @csec_top_level_decl_kind(ptr, i32)\0A\00"
@.str.46996 = private unnamed_addr constant [49 x i8] c"declare ptr @csec_top_level_decl_name(ptr, i32)\0A\00"
@.str.46998 = private unnamed_addr constant [51 x i8] c"declare i32 @csec_count_function_params(ptr, i32)\0A\00"
@.str.47000 = private unnamed_addr constant [62 x i8] c"declare i32 @csec_token_is_top_level_operator(ptr, i32, i32)\0A\00"
@.str.47002 = private unnamed_addr constant [63 x i8] c"declare i32 @csec_find_top_level_operator(ptr, i32, i32, i32)\0A\00"
@.str.47004 = private unnamed_addr constant [55 x i8] c"declare i32 @csec_find_top_level_match(ptr, i32, i32)\0A\00"
@.str.47006 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_find_lambda_arrow(ptr, i32, i32)\0A\00"
@.str.47008 = private unnamed_addr constant [46 x i8] c"declare ptr @csec_generate_symbol_table(ptr)\0A\00"
@.str.47010 = private unnamed_addr constant [58 x i8] c"declare ptr @csec_append_expression_until(ptr, i32, i32)\0A\00"
@.str.47012 = private unnamed_addr constant [58 x i8] c"declare ptr @csec_generate_expression_ast(ptr, i32, i32)\0A\00"
@.str.47014 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_generate_statement_ast(ptr, i32, i32)\0A\00"
@.str.47016 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_generate_body_ast(ptr, i32, i32)\0A\00"
@.str.47018 = private unnamed_addr constant [55 x i8] c"declare i32 @csec_find_function_param_start(ptr, i32)\0A\00"
@.str.47020 = private unnamed_addr constant [55 x i8] c"declare ptr @csec_generate_c_body(ptr, i32, i32, ptr)\0A\00"
@.str.47022 = private unnamed_addr constant [56 x i8] c"declare ptr @csec_generate_c_expression(ptr, i32, i32)\0A\00"
@.str.47024 = private unnamed_addr constant [41 x i8] c"declare ptr @csec_ir_operator_name(ptr)\0A\00"
@.str.47026 = private unnamed_addr constant [37 x i8] c"declare ptr @csec_ir_type_name(ptr)\0A\00"
@.str.47028 = private unnamed_addr constant [64 x i8] c"declare i32 @csec_expression_top_level_operator(ptr, i32, i32)\0A\00"
@.str.47030 = private unnamed_addr constant [64 x i8] c"declare i32 @csec_find_token_text_in_range(ptr, i32, i32, ptr)\0A\00"
@.str.47032 = private unnamed_addr constant [63 x i8] c"declare i32 @csec_find_closing_token(ptr, i32, i32, ptr, ptr)\0A\00"
@.str.47034 = private unnamed_addr constant [61 x i8] c"declare i32 @csec_find_statement_paren_start(ptr, i32, i32)\0A\00"
@.str.47036 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_find_statement_paren_end(ptr, i32, i32)\0A\00"
@.str.47038 = private unnamed_addr constant [61 x i8] c"declare i32 @csec_find_statement_block_start(ptr, i32, i32)\0A\00"
@.str.47040 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_find_statement_block_end(ptr, i32, i32)\0A\00"
@.str.47042 = private unnamed_addr constant [56 x i8] c"declare i32 @csec_count_comma_separated(ptr, i32, i32)\0A\00"
@.str.47044 = private unnamed_addr constant [65 x i8] c"declare i32 @csec_find_last_top_level_token(ptr, i32, i32, ptr)\0A\00"
@.str.47046 = private unnamed_addr constant [53 x i8] c"declare ptr @csec_llvm_lexer_helper_definition(ptr)\0A\00"
@.str.47048 = private unnamed_addr constant [40 x i8] c"declare i64 @csec_string_builder_new()\0A\00"
@.str.47050 = private unnamed_addr constant [48 x i8] c"declare i64 @csec_string_builder_new_file(ptr)\0A\00"
@.str.47052 = private unnamed_addr constant [51 x i8] c"declare i32 @csec_string_builder_append(i64, ptr)\0A\00"
@.str.47054 = private unnamed_addr constant [46 x i8] c"declare ptr @csec_string_builder_finish(i64)\0A\00"
@.str.47056 = private unnamed_addr constant [58 x i8] c"declare i32 @csec_string_builder_write_to_file(i64, ptr)\0A\00"
@.str.47058 = private unnamed_addr constant [52 x i8] c"declare void @csec_set_command_line_args(i32, ptr)\0A\00"
@.str.47060 = private unnamed_addr constant [44 x i8] c"declare i32 @csec_command_line_arg_count()\0A\00"
@.str.47062 = private unnamed_addr constant [41 x i8] c"declare ptr @csec_command_line_arg(i32)\0A\00"
@.str.47064 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_file_read_all_text(ptr)\0A\00"
@.str.47066 = private unnamed_addr constant [49 x i8] c"declare i32 @csec_file_write_all_text(ptr, ptr)\0A\00"
@.str.47068 = private unnamed_addr constant [38 x i8] c"declare void @csec_print_string(ptr)\0A\00"
@.str.47070 = private unnamed_addr constant [36 x i8] c"declare void @csec_print_newline()\0A\00"
@.str.47072 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_string_concat(ptr, ptr)\0A\00"
@.str.47074 = private unnamed_addr constant [43 x i8] c"declare i32 @csec_string_equals(ptr, ptr)\0A\00"
@.str.47076 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_string_starts_with(ptr, ptr)\0A\00"
@.str.47078 = private unnamed_addr constant [51 x i8] c"declare ptr @csec_string_substring(ptr, i32, i32)\0A\00"
@.str.47080 = private unnamed_addr constant [38 x i8] c"declare ptr @csec_to_string_i64(i64)\0A\00"
@.str.47082 = private unnamed_addr constant [38 x i8] c"declare ptr @csec_to_string_char(i8)\0A\00"
@.str.47084 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_token_append_owned(ptr, i8, ptr)\0A\00"
@.str.47086 = private unnamed_addr constant [39 x i8] c"declare i64 @csec_token_builder_new()\0A\00"
@.str.47088 = private unnamed_addr constant [54 x i8] c"declare i32 @csec_token_builder_append(i64, i8, ptr)\0A\00"
@.str.47090 = private unnamed_addr constant [45 x i8] c"declare ptr @csec_token_builder_finish(i64)\0A\00"
@.str.47092 = private unnamed_addr constant [40 x i8] c"declare ptr @csec_tokenize_source(ptr)\0A\00"
@.str.47094 = private unnamed_addr constant [42 x i8] c"declare i8 @csec_token_kind_at(ptr, i32)\0A\00"
@.str.47096 = private unnamed_addr constant [43 x i8] c"declare ptr @csec_token_text_at(ptr, i32)\0A\00"
@.str.47098 = private unnamed_addr constant [47 x i8] c"declare i32 @csec_token_is(ptr, i32, i8, ptr)\0A\00"
@.str.47100 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_advance_statement(ptr, i32, i32)\0A\00"
@.str.47102 = private unnamed_addr constant [52 x i8] c"declare i32 @csec_advance_top_level_decl(ptr, i32)\0A\00"
@.str.47104 = private unnamed_addr constant [50 x i8] c"declare i32 @csec_find_decl_body_start(ptr, i32)\0A\00"
@.str.47106 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_find_decl_body_end(ptr, i32)\0A\00"
@.str.47108 = private unnamed_addr constant [48 x i8] c"declare i32 @csec_function_param_end(ptr, i32)\0A\00"
@.str.47110 = private unnamed_addr constant [54 x i8] c"declare ptr @csec_function_llvm_param_list(ptr, i32)\0A\00"
@.str.47112 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_function_llvm_param_allocas(ptr, i32)\0A\00"
@.str.47114 = private unnamed_addr constant [51 x i8] c"declare ptr @csec_llvm_name_with_number(ptr, i32)\0A\00"
@.str.47116 = private unnamed_addr constant [50 x i8] c"declare ptr @csec_llvm_string_literal_bytes(ptr)\0A\00"
@.str.47118 = private unnamed_addr constant [56 x i8] c"declare i32 @csec_llvm_string_literal_byte_length(ptr)\0A\00"
@.str.47120 = private unnamed_addr constant [53 x i8] c"declare ptr @csec_function_return_type_at(ptr, i32)\0A\00"
@.str.47122 = private unnamed_addr constant [52 x i8] c"declare ptr @csec_collect_type_name(ptr, i32, i32)\0A\00"
@.str.47124 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_enclosing_function_decl_start(ptr, i32)\0A\00"
@.str.47126 = private unnamed_addr constant [59 x i8] c"declare i32 @csec_enclosing_function_body_start(ptr, i32)\0A\00"
@.str.47128 = private unnamed_addr constant [57 x i8] c"declare i32 @csec_enclosing_function_body_end(ptr, i32)\0A\00"
@.str.47130 = private unnamed_addr constant [57 x i8] c"declare ptr @csec_lookup_function_return_type(ptr, ptr)\0A\00"
@.str.47132 = private unnamed_addr constant [60 x i8] c"declare ptr @csec_lookup_visible_value_type(ptr, i32, ptr)\0A\00"
@.str.47134 = private unnamed_addr constant [62 x i8] c"declare ptr @csec_lookup_visible_storage_name(ptr, i32, ptr)\0A\00"
@.str.47136 = private unnamed_addr constant [40 x i8] c"declare ptr @csec_expand_imports(ptr)\0A\0A\00"
@.str.47190 = private unnamed_addr constant [4 x i8] c"def\00"
@.str.47205 = private unnamed_addr constant [5 x i8] c"main\00"
@.str.47299 = private unnamed_addr constant [42 x i8] c"define i32 @main(i32 %argc, ptr %argv) {\0A\00"
@.str.47301 = private unnamed_addr constant [8 x i8] c"entry:\0A\00"
@.str.47303 = private unnamed_addr constant [63 x i8] c"  call void @csec_set_command_line_args(i32 %argc, ptr %argv)\0A\00"
@.str.47305 = private unnamed_addr constant [37 x i8] c"  %ret = call i32 @csec_user_main()\0A\00"
@.str.47307 = private unnamed_addr constant [16 x i8] c"  ret i32 %ret\0A\00"
@.str.47309 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.47319 = private unnamed_addr constant [22 x i8] c"define i32 @main() {\0A\00"
@.str.47326 = private unnamed_addr constant [3 x i8] c"}\0A\00"
@.str.47349 = private unnamed_addr constant [1 x i8] c"\00"
@.str.47585 = private unnamed_addr constant [2 x i8] c".\00"
@.str.47610 = private unnamed_addr constant [1 x i8] c"\00"
@.str.47644 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.47653 = private unnamed_addr constant [1 x i8] c"\00"
@.str.47695 = private unnamed_addr constant [3 x i8] c"//\00"
@.str.47703 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.47708 = private unnamed_addr constant [1 x i8] c"\00"
@.str.47755 = private unnamed_addr constant [1 x i8] c"\00"
@.str.47915 = private unnamed_addr constant [6 x i8] c".csec\00"
@.str.47926 = private unnamed_addr constant [6 x i8] c".csec\00"
@.str.47970 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.48030 = private unnamed_addr constant [2 x i8] c"/\00"
@.str.48075 = private unnamed_addr constant [2 x i8] c"|\00"
@.str.48079 = private unnamed_addr constant [2 x i8] c"|\00"
@.str.48092 = private unnamed_addr constant [1 x i8] c"\00"
@.str.48118 = private unnamed_addr constant [7 x i8] c"import\00"
@.str.48147 = private unnamed_addr constant [1 x i8] c"\00"
@.str.48262 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.48286 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.48312 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.48370 = private unnamed_addr constant [9 x i8] c"tokenlen\00"
@.str.48375 = private unnamed_addr constant [8 x i8] c"tokens=\00"
@.str.48381 = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.48390 = private unnamed_addr constant [7 x i8] c"tokens\00"
@.str.48404 = private unnamed_addr constant [4 x i8] c"ast\00"
@.str.48421 = private unnamed_addr constant [8 x i8] c"symbols\00"
@.str.48438 = private unnamed_addr constant [3 x i8] c"ir\00"
@.str.48455 = private unnamed_addr constant [5 x i8] c"llvm\00"
@.str.48507 = private unnamed_addr constant [5 x i8] c"llvm\00"
@.str.48534 = private unnamed_addr constant [14 x i8] c"compile error\00"
@.str.48603 = private unnamed_addr constant [14 x i8] c"compile error\00"
@.str.48673 = private unnamed_addr constant [20 x i8] c"selfhost/input.csec\00"
@.str.48675 = private unnamed_addr constant [15 x i8] c"selfhost/out.c\00"
@.str.48677 = private unnamed_addr constant [2 x i8] c"c\00"

declare ptr @csec_token_append_owned()
declare ptr @csec_token_builder_new()
declare ptr @csec_token_builder_append()
declare ptr @csec_token_builder_finish()
declare ptr @csec_tokenize_source()
declare void @csec_token_buffer_release(ptr)
declare ptr @csec_token_kind_at()
declare ptr @csec_token_text_at()
declare ptr @csec_token_is()
declare ptr @csec_expand_imports()
declare ptr @csec_string_builder_new_file()
declare ptr @csec_lex_quoted()
declare ptr @csec_lex_number()
declare ptr @csec_lex_identifier()
declare ptr @csec_digit_value()
declare ptr @csec_lex_line_comment()
declare ptr @csec_lex_block_comment()
define i8 @kindIdentifier() {
entry:
  ret i8 73
}

define i8 @kindKeyword() {
entry:
  ret i8 75
}

define i8 @kindInteger() {
entry:
  ret i8 78
}

define i8 @kindFloat() {
entry:
  ret i8 70
}

define i8 @kindString() {
entry:
  ret i8 83
}

define i8 @kindRegex() {
entry:
  ret i8 82
}

define i8 @kindChar() {
entry:
  ret i8 67
}

define i8 @kindBool() {
entry:
  ret i8 66
}

define i8 @kindOperator() {
entry:
  ret i8 79
}

define i8 @kindComment() {
entry:
  ret i8 77
}

define i8 @kindEof() {
entry:
  ret i8 69
}

define i1 @isWhitespace(i8 %arg.ch) {
entry:
  %space = icmp eq i8 %arg.ch, 32
  %newline = icmp eq i8 %arg.ch, 10
  %carriage = icmp eq i8 %arg.ch, 13
  %tab = icmp eq i8 %arg.ch, 9
  %space.or.newline = or i1 %space, %newline
  %carriage.or.tab = or i1 %carriage, %tab
  %ret = or i1 %space.or.newline, %carriage.or.tab
  ret i1 %ret
}

define i1 @isDigit(i8 %arg.ch) {
entry:
  %lower = icmp sge i8 %arg.ch, 48
  %upper = icmp sle i8 %arg.ch, 57
  %ret = and i1 %lower, %upper
  ret i1 %ret
}

define i32 @digitValue(i8 %arg.ch) {
entry:
  %ret = call i32 @csec_digit_value(i8 %arg.ch)
  ret i32 %ret
}

define i32 @toInt(ptr %arg.text) {
entry:
  %ret = call i32 @csec_to_int(ptr %arg.text)
  ret i32 %ret
}

define i1 @isAlpha(i8 %arg.ch) {
entry:
  %lower = icmp sge i8 %arg.ch, 97
  %lower.end = icmp sle i8 %arg.ch, 122
  %lower.match = and i1 %lower, %lower.end
  %upper = icmp sge i8 %arg.ch, 65
  %upper.end = icmp sle i8 %arg.ch, 90
  %upper.match = and i1 %upper, %upper.end
  %ret = or i1 %lower.match, %upper.match
  ret i1 %ret
}

define i1 @isIdentifierStart(i8 %arg.ch) {
entry:
  %alpha = call i1 @isAlpha(i8 %arg.ch)
  %underscore = icmp eq i8 %arg.ch, 95
  %ret = or i1 %alpha, %underscore
  ret i1 %ret
}

define i1 @isIdentifierPart(i8 %arg.ch) {
entry:
  %start = call i1 @isIdentifierStart(i8 %arg.ch)
  %digit = call i1 @isDigit(i8 %arg.ch)
  %ret = or i1 %start, %digit
  ret i1 %ret
}

define i1 @strEq(ptr %arg.left, ptr %arg.right) {
entry:
  %left.length = call i64 @csec_string_length(ptr %arg.left)
  %right.length = call i64 @csec_string_length(ptr %arg.right)
  %same.length = icmp eq i64 %left.length, %right.length
  %prefix = call i32 @csec_string_starts_with(ptr %arg.left, ptr %arg.right)
  %same.text = icmp ne i32 %prefix, 0
  %ret = and i1 %same.length, %same.text
  ret i1 %ret
}

define i1 @isKeyword(ptr %arg.text) {
entry:
  %keyword = call i32 @csec_is_keyword(ptr %arg.text)
  %ret = icmp ne i32 %keyword, 0
  ret i1 %ret
}

define ptr @appendToken(ptr %arg.tokens, i8 %arg.kind, ptr %arg.text) {
entry:
  %ret = call ptr @csec_token_append_owned(ptr %arg.tokens, i8 %arg.kind, ptr %arg.text)
  ret ptr %ret
}

define i32 @appendTokenTo(i64 %arg.builder, i8 %arg.kind, ptr %arg.text) {
entry:
  %ret = call i32 @csec_token_builder_append(i64 %arg.builder, i8 %arg.kind, ptr %arg.text)
  ret i32 %ret
}

define i1 @twoChars(ptr %arg.source, i32 %arg.index, ptr %arg.text) {
entry:
  %length = call i64 @csec_string_length(ptr %arg.source)
  %last = add i32 %arg.index, 1
  %last64 = sext i32 %last to i64
  %in.range = icmp slt i64 %last64, %length
  br i1 %in.range, label %compare, label %false
compare:
  %source0 = call i8 @csec_string_char_at(ptr %arg.source, i32 %arg.index)
  %text0 = call i8 @csec_string_char_at(ptr %arg.text, i32 0)
  %first = icmp eq i8 %source0, %text0
  %source1 = call i8 @csec_string_char_at(ptr %arg.source, i32 %last)
  %text1 = call i8 @csec_string_char_at(ptr %arg.text, i32 1)
  %second = icmp eq i8 %source1, %text1
  %ret = and i1 %first, %second
  ret i1 %ret
false:
  ret i1 false
}

define i1 @threeChars(ptr %arg.source, i32 %arg.index, ptr %arg.text) {
entry:
  %length = call i64 @csec_string_length(ptr %arg.source)
  %last = add i32 %arg.index, 2
  %last64 = sext i32 %last to i64
  %in.range = icmp slt i64 %last64, %length
  br i1 %in.range, label %compare, label %false
compare:
  %source0 = call i8 @csec_string_char_at(ptr %arg.source, i32 %arg.index)
  %text0 = call i8 @csec_string_char_at(ptr %arg.text, i32 0)
  %first = icmp eq i8 %source0, %text0
  %index1 = add i32 %arg.index, 1
  %source1 = call i8 @csec_string_char_at(ptr %arg.source, i32 %index1)
  %text1 = call i8 @csec_string_char_at(ptr %arg.text, i32 1)
  %second = icmp eq i8 %source1, %text1
  %source2 = call i8 @csec_string_char_at(ptr %arg.source, i32 %last)
  %text2 = call i8 @csec_string_char_at(ptr %arg.text, i32 2)
  %third = icmp eq i8 %source2, %text2
  %first.two = and i1 %first, %second
  %ret = and i1 %first.two, %third
  ret i1 %ret
false:
  ret i1 false
}

define i32 @lexIdentifier(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_lex_identifier(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

define i32 @lexNumber(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_lex_number(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

define i32 @lexQuoted(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_lex_quoted(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

define i32 @lexLineComment(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_lex_line_comment(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

define i32 @lexBlockComment(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_lex_block_comment(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

declare i32 @csec_operator_length(ptr, i32)

define i32 @operatorLength(ptr %arg.source, i32 %arg.index) {
entry:
  %ret = call i32 @csec_operator_length(ptr %arg.source, i32 %arg.index)
  ret i32 %ret
}

@.str.repair.r.eturn = private unnamed_addr constant [6 x i8] c"eturn\00"
@.str.repair.r.return = private unnamed_addr constant [7 x i8] c"return\00"
@.str.repair.r.atio = private unnamed_addr constant [5 x i8] c"atio\00"
@.str.repair.r.ratio = private unnamed_addr constant [6 x i8] c"ratio\00"
@.str.repair.r.educe = private unnamed_addr constant [6 x i8] c"educe\00"
@.str.repair.r.reduce = private unnamed_addr constant [7 x i8] c"reduce\00"
@.str.repair.r.ange = private unnamed_addr constant [5 x i8] c"ange\00"
@.str.repair.r.range = private unnamed_addr constant [6 x i8] c"range\00"
@.str.repair.r.egex = private unnamed_addr constant [5 x i8] c"egex\00"
@.str.repair.r.regex = private unnamed_addr constant [6 x i8] c"regex\00"
@.str.repair.r.Regex = private unnamed_addr constant [6 x i8] c"Regex\00"

define ptr @repairLeadingRTokenText(ptr %arg.text) {
entry:
  %case0 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.eturn)
  %is0 = icmp ne i32 %case0, 0
  br i1 %is0, label %return0, label %check1
return0:
  ret ptr @.str.repair.r.return
check1:
  %case1 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.atio)
  %is1 = icmp ne i32 %case1, 0
  br i1 %is1, label %return1, label %check2
return1:
  ret ptr @.str.repair.r.ratio
check2:
  %case2 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.educe)
  %is2 = icmp ne i32 %case2, 0
  br i1 %is2, label %return2, label %check3
return2:
  ret ptr @.str.repair.r.reduce
check3:
  %case3 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.ange)
  %is3 = icmp ne i32 %case3, 0
  br i1 %is3, label %return3, label %check4
return3:
  ret ptr @.str.repair.r.range
check4:
  %case4 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.egex)
  %is4 = icmp ne i32 %case4, 0
  br i1 %is4, label %return4, label %check5
return4:
  ret ptr @.str.repair.r.regex
check5:
  %case5 = call i32 @csec_string_equals(ptr %arg.text, ptr @.str.repair.r.Regex)
  %is5 = icmp ne i32 %case5, 0
  br i1 %is5, label %return5, label %fallback
return5:
  ret ptr @.str.repair.r.Regex
fallback:
  ret ptr %arg.text
}

define ptr @slice(ptr %arg.source, i32 %arg.start, i32 %arg.end) {
entry:
  %valid = icmp sgt i32 %arg.end, %arg.start
  br i1 %valid, label %slice, label %empty
slice:
  %length = sub i32 %arg.end, %arg.start
  %ret = call ptr @csec_string_substring(ptr %arg.source, i32 %arg.start, i32 %length)
  ret ptr %ret
empty:
  ret ptr null
}

define ptr @tokenize(ptr %arg.source) {
entry:
  %ret = call ptr @csec_tokenize_source(ptr %arg.source)
  ret ptr %ret
}

define ptr @tokenizeSlow(ptr %arg.source) {
entry:
  %ret = call ptr @csec_tokenize_source(ptr %arg.source)
  ret ptr %ret
}

define i32 @lineStart(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i32 @csec_line_start(ptr %arg.tokens, i32 %arg.ordinal)
  ret i32 %ret
}

define i32 @lineEnd(ptr %arg.tokens, i32 %arg.start) {
entry:
  %ret = call i32 @csec_line_end(ptr %arg.tokens, i32 %arg.start)
  ret i32 %ret
}

define i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %arg.ordinal)
  ret i8 %ret
}

define ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

define i1 @tokenIs(ptr %arg.tokens, i32 %arg.ordinal, i8 %arg.kind, ptr %arg.text) {
entry:
  %token.is = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 %arg.kind, ptr %arg.text)
  %token.is.bool = icmp ne i32 %token.is, 0
  ret i1 %token.is.bool
}

define i32 @skipTrivia(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  br label %loop
loop:
  %cursor = phi i32 [ %arg.ordinal, %entry ], [ %next, %body ]
  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %cursor)
  %is.comment = icmp eq i8 %kind, 77
  br i1 %is.comment, label %body, label %exit
body:
  %next = add i32 %cursor, 1
  br label %loop
exit:
  ret i32 %cursor
}

declare ptr @csec_lookup_function_return_type()
declare ptr @csec_advance_statement()
declare ptr @csec_advance_top_level_decl()
declare ptr @csec_find_decl_body_start()
declare ptr @csec_find_decl_body_end()
declare ptr @csec_function_param_end()
declare ptr @csec_function_llvm_param_list()
declare ptr @csec_function_llvm_param_allocas()
declare ptr @csec_llvm_name_with_number()
declare ptr @csec_function_return_type_at()
define i1 @validateBalanced(ptr %arg.tokens) {
entry:
  %valid = call i32 @csec_validate_balanced(ptr %arg.tokens)
  %ret = icmp ne i32 %valid, 0
  ret i1 %ret
}

define i1 @startsTopLevelDeclaration(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %valid = call i32 @csec_starts_top_level_declaration(ptr %arg.tokens, i32 %arg.ordinal)
  %ret = icmp ne i32 %valid, 0
  ret i1 %ret
}

define i1 @validateTopLevel(ptr %arg.tokens) {
entry:
  %valid = call i32 @csec_validate_top_level(ptr %arg.tokens)
  %ret = icmp ne i32 %valid, 0
  ret i1 %ret
}

define i1 @parseProgram(ptr %arg.tokens) {
entry:
  %balanced = call i1 @validateBalanced(ptr %arg.tokens)
  br i1 %balanced, label %validate, label %invalid
validate:
  %ret = call i1 @validateTopLevel(ptr %arg.tokens)
  ret i1 %ret
invalid:
  ret i1 false
}

define ptr @topLevelDeclKind(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_top_level_decl_kind(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

define ptr @topLevelDeclName(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_top_level_decl_name(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

define i32 @advanceTopLevelDecl(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i32 @csec_advance_top_level_decl(ptr %arg.tokens, i32 %arg.ordinal)
  ret i32 %ret
}

define i32 @advanceTopLevelDeclSlow(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i32 @csec_advance_top_level_decl(ptr %arg.tokens, i32 %arg.ordinal)
  ret i32 %ret
}

define i32 @findDeclBodyStart(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i32 @csec_find_decl_body_start(ptr %arg.tokens, i32 %arg.ordinal)
  ret i32 %ret
}

define i32 @findDeclBodyStartSlow(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call i32 @csec_find_decl_body_start(ptr %arg.tokens, i32 %arg.ordinal)
  ret i32 %ret
}

define i32 @findDeclBodyEnd(ptr %arg.tokens, i32 %arg.bodyStart) {
entry:
  %ret = call i32 @csec_find_decl_body_end(ptr %arg.tokens, i32 %arg.bodyStart)
  ret i32 %ret
}

define i32 @findDeclBodyEndSlow(ptr %arg.tokens, i32 %arg.bodyStart) {
entry:
  %ret = call i32 @csec_find_decl_body_end(ptr %arg.tokens, i32 %arg.bodyStart)
  ret i32 %ret
}

declare ptr @csec_statement_kind(ptr, i32)

define ptr @statementKind(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_statement_kind(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

define ptr @appendExpressionUntil(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call ptr @csec_append_expression_until(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %ret
}

define i1 @tokenIsTopLevelOperator(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.group) {
entry:
  %valid = call i32 @csec_token_is_top_level_operator(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.group)
  %ret = icmp ne i32 %valid, 0
  ret i1 %ret
}

define i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 %arg.group) {
entry:
  %ret = call i32 @csec_find_top_level_operator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 %arg.group)
  ret i32 %ret
}

define i32 @findTopLevelMatch(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_find_top_level_match(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define i32 @findClosingToken(ptr %arg.tokens, i32 %arg.openOrdinal, i32 %arg.end, ptr %arg.openText, ptr %arg.closeText) {
entry:
  %ret = call i32 @csec_find_closing_token(ptr %arg.tokens, i32 %arg.openOrdinal, i32 %arg.end, ptr %arg.openText, ptr %arg.closeText)
  ret i32 %ret
}

define i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  br label %trim.loop
trim.loop:
  %last = phi i32 [ %arg.end, %entry ], [ %previous, %trim.comment ]
  %has.previous = icmp sgt i32 %last, %arg.start
  br i1 %has.previous, label %trim.inspect, label %trim.check
trim.inspect:
  %index = sub i32 %last, 1
  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %index)
  %is.comment = icmp eq i8 %kind, 77
  br i1 %is.comment, label %trim.comment, label %trim.check
trim.comment:
  %previous = sub i32 %last, 1
  br label %trim.loop
trim.check:
  %has.token = icmp sgt i32 %last, %arg.start
  br i1 %has.token, label %trim.semicolon, label %trim.return
trim.semicolon:
  %candidate = sub i32 %last, 1
  %is.semicolon = call i32 @csec_token_is(ptr %arg.tokens, i32 %candidate, i8 79, ptr @.str.trim.semicolon)
  %has.semicolon = icmp ne i32 %is.semicolon, 0
  br i1 %has.semicolon, label %trim.without.semicolon, label %trim.return
trim.without.semicolon:
  %result = sub i32 %last, 1
  ret i32 %result
trim.return:
  ret i32 %last
}

@.str.trim.semicolon = private unnamed_addr constant [2 x i8] c";\00"

declare ptr @csec_expression_leaf_kind(ptr, i32, i32)

define ptr @expressionLeafKind(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call ptr @csec_expression_leaf_kind(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %ret
}

define i32 @findTokenTextInRange(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text) {
entry:
  %ret = call i32 @csec_find_token_text_in_range(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text)
  ret i32 %ret
}

@.str.count.match.case = private unnamed_addr constant [5 x i8] c"case\00"

define i32 @countMatchCases(ptr %arg.tokens, i32 %arg.matchOrdinal, i32 %arg.end) {
entry:
  %open = call i32 @findTokenTextInRange(ptr %arg.tokens, i32 %arg.matchOrdinal, i32 %arg.end, ptr @.str.open.brace)
  %has.open = icmp sge i32 %open, 0
  br i1 %has.open, label %find.close, label %empty
find.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open, i32 %arg.end, ptr @.str.open.brace, ptr @.str.close.brace)
  %first = add i32 %open, 1
  br label %loop
loop:
  %cursor = phi i32 [ %first, %find.close ], [ %next, %body ]
  %count = phi i32 [ 0, %find.close ], [ %updated, %body ]
  %before.close = icmp slt i32 %cursor, %close
  br i1 %before.close, label %inspect, label %done
inspect:
  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %cursor)
  %is.eof = icmp eq i8 %kind, 69
  br i1 %is.eof, label %done, label %body
body:
  %case.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %cursor, i8 75, ptr @.str.count.match.case)
  %is.case = icmp ne i32 %case.raw, 0
  %increment = zext i1 %is.case to i32
  %updated = add i32 %count, %increment
  %next = add i32 %cursor, 1
  br label %loop
done:
  ret i32 %count
empty:
  ret i32 0
}

@.str.open.brace = private unnamed_addr constant [2 x i8] c"{\00"
@.str.close.brace = private unnamed_addr constant [2 x i8] c"}\00"

define i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_find_statement_paren_start(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_find_statement_paren_end(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_find_statement_block_start(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define i32 @findStatementBlockEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_find_statement_block_end(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

@.str.statement.header.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @statementHeaderExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %has.open = icmp sge i32 %open, 0
  %after.open = icmp sgt i32 %close, %open
  %valid = and i1 %has.open, %after.open
  br i1 %valid, label %body, label %empty
body:
  %body.start = add i32 %open, 1
  %ret = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %body.start, i32 %close)
  ret ptr %ret
empty:
  ret ptr @.str.statement.header.empty
}

define i32 @countCommaSeparated(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_count_comma_separated(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define i32 @findLastTopLevelToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text) {
entry:
  %ret = call i32 @csec_find_last_top_level_token(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.text)
  ret i32 %ret
}

@.str.postfix.summary.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @summarizePostfixExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  ret ptr @.str.postfix.summary.empty
}

define i32 @findLambdaArrow(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  ret i32 -1
}

define ptr @lambdaCaptureSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.lambda.capture.open, ptr @.str.lambda.capture.close)
  %minimum = add i32 %arg.start, 1
  %has.capture = icmp sgt i32 %close, %minimum
  br i1 %has.capture, label %capture, label %none
capture:
  %first = add i32 %arg.start, 1
  %ret.capture = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %first, i32 %close)
  ret ptr %ret.capture
none:
  %ret.none = getelementptr inbounds [5 x i8], ptr @.str.lambda.none, i32 0, i32 0
  ret ptr %ret.none
}

@.str.lambda.capture.open = private unnamed_addr constant [2 x i8] c"[\00"
@.str.lambda.capture.close = private unnamed_addr constant [2 x i8] c"]\00"
@.str.lambda.none = private unnamed_addr constant [5 x i8] c"none\00"

define i32 @lambdaParameterCount(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %capture.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.lambda.params.capture.open, ptr @.str.lambda.params.capture.close)
  %capture.valid = icmp sge i32 %capture.close, 0
  br i1 %capture.valid, label %params.start, label %empty
params.start:
  %after.capture = add i32 %capture.close, 1
  %params.open = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.capture)
  %is.open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %params.open, i8 79, ptr @.str.lambda.params.open)
  %is.open = icmp ne i32 %is.open.raw, 0
  br i1 %is.open, label %params.end, label %empty
params.end:
  %params.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %params.open, i32 %arg.end, ptr @.str.lambda.params.open, ptr @.str.lambda.params.close)
  %first = add i32 %params.open, 1
  %ret = call i32 @countCommaSeparated(ptr %arg.tokens, i32 %first, i32 %params.close)
  ret i32 %ret
empty:
  ret i32 0
}

@.str.lambda.params.capture.open = private unnamed_addr constant [2 x i8] c"[\00"
@.str.lambda.params.capture.close = private unnamed_addr constant [2 x i8] c"]\00"
@.str.lambda.params.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.lambda.params.close = private unnamed_addr constant [2 x i8] c")\00"

define ptr @summarizeLambdaExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.start, i8 79, ptr @.str.lambda.summary.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %arrow.check, label %empty
arrow.check:
  %arrow = call i32 @findLambdaArrow(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %valid = icmp sge i32 %arrow, 0
  br i1 %valid, label %summary, label %empty
summary:
  %capture = call ptr @lambdaCaptureSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %params = call i32 @lambdaParameterCount(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %params.i64 = sext i32 %params to i64
  %params.text = call ptr @csec_to_string_i64(i64 %params.i64)
  %s1 = call ptr @csec_string_concat(ptr @.str.lambda.summary.prefix, ptr %capture)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.lambda.summary.params)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %params.text)
  ret ptr %s3
empty:
  ret ptr @.str.lambda.summary.empty
}

@.str.lambda.summary.open = private unnamed_addr constant [2 x i8] c"[\00"
@.str.lambda.summary.prefix = private unnamed_addr constant [21 x i8] c"Expr lambda capture=\00"
@.str.lambda.summary.params = private unnamed_addr constant [9 x i8] c" params=\00"
@.str.lambda.summary.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateExpressionAST(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {
entry:
  %ret = call ptr @csec_generate_expression_ast(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd)
  ret ptr %ret
}

define i32 @advanceStatement(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd) {
entry:
  %ret = call i32 @csec_advance_statement(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd)
  ret i32 %ret
}

define i32 @advanceStatementSlow(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd) {
entry:
  %ret = call i32 @csec_advance_statement(ptr %arg.tokens, i32 %arg.ordinal, i32 %arg.bodyEnd)
  ret i32 %ret
}

define ptr @generateStatementAST(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call ptr @csec_generate_statement_ast(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %ret
}

define ptr @generateBodyAST(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_generate_body_ast(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd)
  ret ptr %ret
}

define i32 @findFunctionParamStart(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call i32 @csec_find_function_param_start(ptr %arg.tokens, i32 %arg.declStart)
  ret i32 %ret
}

define i32 @findFunctionReturnTypeStart(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %param.start = call i32 @findFunctionParamStart(ptr %arg.tokens, i32 %arg.declStart)
  %valid.start = icmp sge i32 %param.start, 0
  br i1 %valid.start, label %find.end, label %missing
find.end:
  %decl.end = call i32 @advanceTopLevelDecl(ptr %arg.tokens, i32 %arg.declStart)
  %param.end = call i32 @findClosingToken(ptr %arg.tokens, i32 %param.start, i32 %decl.end, ptr @.str.fn.return.open, ptr @.str.fn.return.close)
  %valid.end = icmp sge i32 %param.end, 0
  br i1 %valid.end, label %colon.check, label %missing
colon.check:
  %after = add i32 %param.end, 1
  %colon.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %after, i8 79, ptr @.str.fn.return.colon)
  %colon = icmp ne i32 %colon.raw, 0
  br i1 %colon, label %found, label %missing
found:
  %ret = add i32 %param.end, 2
  ret i32 %ret
missing:
  ret i32 -1
}

@.str.fn.return.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.fn.return.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.fn.return.colon = private unnamed_addr constant [2 x i8] c":\00"

define ptr @collectTypeName(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call ptr @csec_collect_type_name(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %ret
}

define ptr @functionReturnType(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_return_type_at(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

define ptr @functionReturnTypeSlow(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_return_type_at(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

define i32 @countFunctionParams(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call i32 @csec_count_function_params(ptr %arg.tokens, i32 %arg.declStart)
  ret i32 %ret
}

define ptr @typeSummary(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.start, i8 79, ptr @.str.type.summary.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %find.close, label %fallback
find.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr @.str.type.summary.open, ptr @.str.type.summary.close)
  %valid.close = icmp sgt i32 %close, %arg.start
  br i1 %valid.close, label %arrow.check, label %fallback
arrow.check:
  %after.close = add i32 %close, 1
  %arrow.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %after.close, i8 79, ptr @.str.type.summary.arrow)
  %arrow = icmp ne i32 %arrow.raw, 0
  br i1 %arrow, label %function, label %fallback
function:
  %params.start = add i32 %arg.start, 1
  %params = call i32 @countCommaSeparated(ptr %arg.tokens, i32 %params.start, i32 %close)
  %params.i64 = sext i32 %params to i64
  %params.text = call ptr @csec_to_string_i64(i64 %params.i64)
  %returns.start = add i32 %close, 2
  %returns = call ptr @collectTypeName(ptr %arg.tokens, i32 %returns.start, i32 %arg.end)
  %s1 = call ptr @csec_string_concat(ptr @.str.type.summary.prefix, ptr %params.text)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.type.summary.returns)
  %ret = call ptr @csec_string_concat(ptr %s2, ptr %returns)
  ret ptr %ret
fallback:
  %fallback.ret = call ptr @collectTypeName(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %fallback.ret
}

@.str.type.summary.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.type.summary.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.type.summary.arrow = private unnamed_addr constant [3 x i8] c"=>\00"
@.str.type.summary.prefix = private unnamed_addr constant [21 x i8] c"FunctionType params=\00"
@.str.type.summary.returns = private unnamed_addr constant [10 x i8] c" returns=\00"

declare ptr @csec_declared_value_type(ptr, i32, i32)

define ptr @declaredValueType(ptr %arg.tokens, i32 %arg.declStart, i32 %arg.declEnd) {
entry:
  %ret = call ptr @csec_declared_value_type(ptr %arg.tokens, i32 %arg.declStart, i32 %arg.declEnd)
  ret ptr %ret
}

define ptr @lookupFunctionReturnType(ptr %arg.tokens, ptr %arg.name) {
entry:
  %ret = call ptr @csec_lookup_function_return_type(ptr %arg.tokens, ptr %arg.name)
  ret ptr %ret
}

declare ptr @csec_infer_expression_type(ptr, i32, i32)

define ptr @inferExpressionType(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {
entry:
  %ret = call ptr @csec_infer_expression_type(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd)
  ret ptr %ret
}

declare ptr @csec_declared_local_type(ptr, i32, i32)

define ptr @declaredLocalType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call ptr @csec_declared_local_type(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret ptr %ret
}

@.str.local.type.unknown = private unnamed_addr constant [8 x i8] c"unknown\00"

define ptr @localDeclarationType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %explicit = call ptr @declaredLocalType(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %has.type = call i32 @csec_string_length(ptr %explicit)
  %present = icmp sgt i32 %has.type, 0
  br i1 %present, label %typed, label %fallback
typed:
  ret ptr %explicit
fallback:
  ret ptr @.str.local.type.unknown
}

@.str.function.scope.symbols.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateFunctionScopeSymbols(ptr %arg.tokens, ptr %arg.functionName, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  ret ptr @.str.function.scope.symbols.empty
}

@.str.function.param.symbols.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateFunctionParamSymbols(ptr %arg.tokens, ptr %arg.functionName, i32 %arg.declStart) {
entry:
  ret ptr @.str.function.param.symbols.empty
}

@.str.class.member.override = private unnamed_addr constant [9 x i8] c"override\00"
@.str.class.member.unsafe = private unnamed_addr constant [7 x i8] c"unsafe\00"
@.str.class.member.constexpr = private unnamed_addr constant [10 x i8] c"constexpr\00"

define i32 @classMemberStart(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %override.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.override)
  %unsafe.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.unsafe)
  %constexpr.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %arg.ordinal, i8 75, ptr @.str.class.member.constexpr)
  %override = icmp ne i32 %override.raw, 0
  %unsafe = icmp ne i32 %unsafe.raw, 0
  %constexpr = icmp ne i32 %constexpr.raw, 0
  %modifier = or i1 %override, %unsafe
  %skip = or i1 %modifier, %constexpr
  br i1 %skip, label %advanced, label %plain
advanced:
  %next = add i32 %arg.ordinal, 1
  ret i32 %next
plain:
  ret i32 %arg.ordinal
}

declare ptr @csec_class_member_kind(ptr, i32)

define ptr @classMemberKind(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_class_member_kind(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

declare ptr @csec_class_member_name(ptr, i32)

define ptr @classMemberName(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  %ret = call ptr @csec_class_member_name(ptr %arg.tokens, i32 %arg.ordinal)
  ret ptr %ret
}

@.str.class.member.symbols.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateClassMemberSymbols(ptr %arg.tokens, ptr %arg.className, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  ret ptr @.str.class.member.symbols.empty
}

@.str.class.member.ast.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateClassMemberAST(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  ret ptr @.str.class.member.ast.empty
}

@.str.summary.templateParameterSummary.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @templateParameterSummary(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  ret ptr @.str.summary.templateParameterSummary.empty
}

@.str.summary.templateTargetKind.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @templateTargetKind(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  ret ptr @.str.summary.templateTargetKind.empty
}

@.str.summary.attributeSummary.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @attributeSummary(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  ret ptr @.str.summary.attributeSummary.empty
}

@.str.summary.externalSymbolKind.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @externalSymbolKind(ptr %arg.tokens, i32 %arg.ordinal) {
entry:
  ret ptr @.str.summary.externalSymbolKind.empty
}

define ptr @generateSymbolTable(ptr %arg.tokens) {
entry:
  %ret = call ptr @csec_generate_symbol_table(ptr %arg.tokens)
  ret ptr %ret
}

@.str.ast.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateAST(ptr %arg.tokens) {
entry:
  ret ptr @.str.ast.empty
}

declare ptr @csec_string_builder_new()
declare ptr @csec_string_builder_append()
declare ptr @csec_string_builder_finish()
declare ptr @csec_string_builder_write_to_file()
declare ptr @csec_llvm_string_literal_bytes()
declare ptr @csec_llvm_string_literal_byte_length()
declare ptr @csec_enclosing_function_decl_start()
declare ptr @csec_enclosing_function_body_start()
declare ptr @csec_enclosing_function_body_end()
declare ptr @csec_llvm_lexer_helper_definition()
declare i32 @csec_parse_return_integer_in_range(ptr, i32, i32)

define i32 @parseReturnIntegerInRange(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_parse_return_integer_in_range(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define ptr @appendCExpression(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %ret = call ptr @csec_generate_c_expression(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  ret ptr %ret
}

declare ptr @csec_c_type_name(ptr)

define ptr @cTypeName(ptr %arg.typeName) {
entry:
  %ret = call ptr @csec_c_type_name(ptr %arg.typeName)
  ret ptr %ret
}

define ptr @generateCBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd, ptr %arg.indent) {
entry:
  %ret = call ptr @csec_generate_c_body(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd, ptr %arg.indent)
  ret ptr %ret
}

@.str.c.statement.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateCStatement(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, ptr %arg.indent) {
entry:
  ret ptr @.str.c.statement.empty
}

define i32 @parseMainReturnInt(ptr %arg.tokens) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %cursor.addr.16736 = alloca i32
  %cursor.init.16736 = add i32 0, 0
  store i32 %cursor.init.16736, ptr %cursor.addr.16736
  %ret.16929 = add i32 0, 0
  ret i32 %ret.16929
}

define ptr @generateMainExecutionC(ptr %arg.tokens) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %cursor.addr.16943 = alloca i32
  %cursor.init.16943 = add i32 0, 0
  store i32 %cursor.init.16943, ptr %cursor.addr.16943
  br label %while.cond.16950
while.cond.16950:
  %whilecond.16950.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %whilecond.16950.i32.left.char.arg0 = trunc i32 %whilecond.16950.i32.left.char.arg0.i32 to i8
  %whilecond.16950.i32.left.char.arg1.i32 = load i32, ptr %cursor.addr.16943
  %whilecond.16950.i32.left.char.arg1 = trunc i32 %whilecond.16950.i32.left.char.arg1.i32 to i8
  %whilecond.16950.i32.left.char = call i8 @tokenKindAt(i8 %whilecond.16950.i32.left.char.arg0, i8 %whilecond.16950.i32.left.char.arg1)
  %whilecond.16950.i32.left = zext i8 %whilecond.16950.i32.left.char to i32
  %whilecond.16950.i32.right.char = call i8 @kindEof()
  %whilecond.16950.i32.right = zext i8 %whilecond.16950.i32.right.char to i32
  %whilecond.16950.i32.comparison = icmp ne i32 %whilecond.16950.i32.left, %whilecond.16950.i32.right
  %whilecond.16950.i32 = zext i1 %whilecond.16950.i32.comparison to i32
  %whilecond.16950 = icmp ne i32 %whilecond.16950.i32, 0
  br i1 %whilecond.16950, label %while.body.16950, label %while.end.16950
while.body.16950:
  %ifcond.16964.left.left.left.left.arg0 = load ptr, ptr %tokens
  %ifcond.16964.left.left.left.left.arg1 = load i32, ptr %cursor.addr.16943
  %ifcond.16964.left.left.left.left.arg2.char = call i8 @kindKeyword()
  %ifcond.16964.left.left.left.left.arg2 = zext i8 %ifcond.16964.left.left.left.left.arg2.char to i32
  %ifcond.16964.left.left.left.left.arg3 = getelementptr inbounds [4 x i8], ptr @.str.16976, i32 0, i32 0
  %ifcond.16964.left.left.left.left = call i1 @tokenIs(ptr %ifcond.16964.left.left.left.left.arg0, i32 %ifcond.16964.left.left.left.left.arg1, i32 %ifcond.16964.left.left.left.left.arg2, ptr %ifcond.16964.left.left.left.left.arg3)
  %ifcond.16964.left.left.left.right.arg0 = load ptr, ptr %tokens
  %ifcond.16964.left.left.left.right.arg1.left = load i32, ptr %cursor.addr.16943
  %ifcond.16964.left.left.left.right.arg1.right = add i32 0, 1
  %ifcond.16964.left.left.left.right.arg1 = add i32 %ifcond.16964.left.left.left.right.arg1.left, %ifcond.16964.left.left.left.right.arg1.right
  %ifcond.16964.left.left.left.right.arg2.char = call i8 @kindIdentifier()
  %ifcond.16964.left.left.left.right.arg2 = zext i8 %ifcond.16964.left.left.left.right.arg2.char to i32
  %ifcond.16964.left.left.left.right.arg3 = getelementptr inbounds [5 x i8], ptr @.str.16991, i32 0, i32 0
  %ifcond.16964.left.left.left.right = call i1 @tokenIs(ptr %ifcond.16964.left.left.left.right.arg0, i32 %ifcond.16964.left.left.left.right.arg1, i32 %ifcond.16964.left.left.left.right.arg2, ptr %ifcond.16964.left.left.left.right.arg3)
  %ifcond.16964.left.left.left = and i1 %ifcond.16964.left.left.left.left, %ifcond.16964.left.left.left.right
  %ifcond.16964.left.left.right.arg0 = getelementptr i8, ptr null, i32 0
  %ifcond.16964.left.left.right.arg1.left = load i32, ptr %cursor.addr.16943
  %ifcond.16964.left.left.right.arg1.right = add i32 0, 3
  %ifcond.16964.left.left.right.arg1 = add i32 %ifcond.16964.left.left.right.arg1.left, %ifcond.16964.left.left.right.arg1.right
  %ifcond.16964.left.left.right.arg2.char = call i8 @kindOperator()
  %ifcond.16964.left.left.right.arg2 = zext i8 %ifcond.16964.left.left.right.arg2.char to i32
  %ifcond.16964.left.left.right.arg3 = getelementptr inbounds [2 x i8], ptr @.str.17021, i32 0, i32 0
  %ifcond.16964.left.left.right = call i1 @tokenIs(ptr %ifcond.16964.left.left.right.arg0, i32 %ifcond.16964.left.left.right.arg1, i32 %ifcond.16964.left.left.right.arg2, ptr %ifcond.16964.left.left.right.arg3)
  %ifcond.16964.left.left = and i1 %ifcond.16964.left.left.left, %ifcond.16964.left.left.right
  %ifcond.16964.left.right.arg0 = load ptr, ptr %tokens
  %ifcond.16964.left.right.arg1.left = load i32, ptr %cursor.addr.16943
  %ifcond.16964.left.right.arg1.right = add i32 0, 4
  %ifcond.16964.left.right.arg1 = add i32 %ifcond.16964.left.right.arg1.left, %ifcond.16964.left.right.arg1.right
  %ifcond.16964.left.right.arg2.char = call i8 @kindOperator()
  %ifcond.16964.left.right.arg2 = zext i8 %ifcond.16964.left.right.arg2.char to i32
  %ifcond.16964.left.right.arg3 = getelementptr inbounds [2 x i8], ptr @.str.17036, i32 0, i32 0
  %ifcond.16964.left.right = call i1 @tokenIs(ptr %ifcond.16964.left.right.arg0, i32 %ifcond.16964.left.right.arg1, i32 %ifcond.16964.left.right.arg2, ptr %ifcond.16964.left.right.arg3)
  %ifcond.16964.left = and i1 %ifcond.16964.left.left, %ifcond.16964.left.right
  %ifcond.16964.right.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.16964.right.arg0.arg1.left = load i32, ptr %cursor.addr.16943
  %ifcond.16964.right.arg0.arg1.right = add i32 0, 5
  %ifcond.16964.right.arg0.arg1 = add i32 %ifcond.16964.right.arg0.arg1.left, %ifcond.16964.right.arg0.arg1.right
  %ifcond.16964.right.arg0 = call ptr @tokenTextAt(ptr %ifcond.16964.right.arg0.arg0, i32 %ifcond.16964.right.arg0.arg1)
  %ifcond.16964.right.arg1 = getelementptr inbounds [4 x i8], ptr @.str.17050, i32 0, i32 0
  %ifcond.16964.right = call i1 @strEq(ptr %ifcond.16964.right.arg0, ptr %ifcond.16964.right.arg1)
  %ifcond.16964 = and i1 %ifcond.16964.left, %ifcond.16964.right
  br i1 %ifcond.16964, label %if.then.16964, label %if.else.16964
if.then.16964:
  %bodyStart.addr.17054 = alloca i32
  %bodyStart.init.17054.arg0 = load ptr, ptr %tokens
  %bodyStart.init.17054.arg1 = load i32, ptr %cursor.addr.16943
  %bodyStart.init.17054 = call i32 @findDeclBodyStart(ptr %bodyStart.init.17054.arg0, i32 %bodyStart.init.17054.arg1)
  store i32 %bodyStart.init.17054, ptr %bodyStart.addr.17054
  %bodyEnd.addr.17066 = alloca i32
  %bodyEnd.init.17066.arg0 = load ptr, ptr %tokens
  %bodyEnd.init.17066.arg1 = load i32, ptr %bodyStart.addr.17054
  %bodyEnd.init.17066 = call i32 @findDeclBodyEnd(ptr %bodyEnd.init.17066.arg0, i32 %bodyEnd.init.17066.arg1)
  store i32 %bodyEnd.init.17066, ptr %bodyEnd.addr.17066
  %ifcond.17078.left.i32.left = load i32, ptr %bodyStart.addr.17054
  %ifcond.17078.left.i32.right = add i32 0, 0
  %ifcond.17078.left.i32.comparison = icmp sge i32 %ifcond.17078.left.i32.left, %ifcond.17078.left.i32.right
  %ifcond.17078.left.i32 = zext i1 %ifcond.17078.left.i32.comparison to i32
  %ifcond.17078.left = icmp ne i32 %ifcond.17078.left.i32, 0
  %ifcond.17078.right.i32.left = load i32, ptr %bodyEnd.addr.17066
  %ifcond.17078.right.i32.right = load i32, ptr %bodyStart.addr.17054
  %ifcond.17078.right.i32.comparison = icmp sge i32 %ifcond.17078.right.i32.left, %ifcond.17078.right.i32.right
  %ifcond.17078.right.i32 = zext i1 %ifcond.17078.right.i32.comparison to i32
  %ifcond.17078.right = icmp ne i32 %ifcond.17078.right.i32, 0
  %ifcond.17078 = and i1 %ifcond.17078.left, %ifcond.17078.right
  br i1 %ifcond.17078, label %if.then.17078, label %if.else.17078
if.then.17078:
  %ret.17089.left.left.left.left.left = getelementptr inbounds [35 x i8], ptr @.str.17090, i32 0, i32 0
  %ret.17089.left.left.left.left.right = getelementptr inbounds [17 x i8], ptr @.str.17092, i32 0, i32 0
  %ret.17089.left.left.left.left = call ptr @csec_string_concat(ptr %ret.17089.left.left.left.left.left, ptr %ret.17089.left.left.left.left.right)
  %ret.17089.left.left.left.right = getelementptr inbounds [17 x i8], ptr @.str.17094, i32 0, i32 0
  %ret.17089.left.left.left = call ptr @csec_string_concat(ptr %ret.17089.left.left.left.left, ptr %ret.17089.left.left.left.right)
  %ret.17089.left.left.right.arg0 = load ptr, ptr %tokens
  %ret.17089.left.left.right.arg1 = load i32, ptr %bodyStart.addr.17054
  %ret.17089.left.left.right.arg2 = load i32, ptr %bodyEnd.addr.17066
  %ret.17089.left.left.right.arg3 = getelementptr inbounds [5 x i8], ptr @.str.17104, i32 0, i32 0
  %ret.17089.left.left.right = call ptr @generateCBody(ptr %ret.17089.left.left.right.arg0, i32 %ret.17089.left.left.right.arg1, i32 %ret.17089.left.left.right.arg2, ptr %ret.17089.left.left.right.arg3)
  %ret.17089.left.left = call ptr @csec_string_concat(ptr %ret.17089.left.left.left, ptr %ret.17089.left.left.right)
  %ret.17089.left.right = getelementptr inbounds [15 x i8], ptr @.str.17107, i32 0, i32 0
  %ret.17089.left = call ptr @csec_string_concat(ptr %ret.17089.left.left, ptr %ret.17089.left.right)
  %ret.17089.right = getelementptr inbounds [3 x i8], ptr @.str.17109, i32 0, i32 0
  %ret.17089 = call ptr @csec_string_concat(ptr %ret.17089.left, ptr %ret.17089.right)
  ret ptr %ret.17089
if.else.17078:
  br label %if.end.17078
if.end.17078:
  br label %if.end.16964
if.else.16964:
  br label %if.end.16964
if.end.16964:
  %cursor.assign.17113.left = load i32, ptr %cursor.addr.16943
  %cursor.assign.17113.right = add i32 0, 1
  %cursor.assign.17113 = add i32 %cursor.assign.17113.left, %cursor.assign.17113.right
  store i32 %cursor.assign.17113, ptr %cursor.addr.16943
  br label %while.cond.16950
while.end.16950:
  %ret.17120 = getelementptr inbounds [1 x i8], ptr @.str.17121, i32 0, i32 0
  ret ptr %ret.17120
}

define ptr @irTypeName(ptr %arg.typeName) {
entry:
  %ret = call ptr @csec_ir_type_name(ptr %arg.typeName)
  ret ptr %ret
}

define ptr @irOperatorName(ptr %arg.text) {
entry:
  %ret = call ptr @csec_ir_operator_name(ptr %arg.text)
  ret ptr %ret
}

define i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %ret = call i32 @csec_expression_top_level_operator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  ret i32 %ret
}

define ptr @generateIRExpression(ptr %arg.tokens, i32 %arg.rawStart, i32 %arg.rawEnd) {
entry:
  %start = call i32 @skipTrivia(ptr %arg.tokens, i32 %arg.rawStart)
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %start, i32 %arg.rawEnd)
  %nonempty = icmp sgt i32 %end, %start
  br i1 %nonempty, label %classify, label %empty
empty:
  %empty.value = getelementptr inbounds [5 x i8], ptr @.str.ir.expr.void, i32 0, i32 0
  ret ptr %empty.value
classify:
  %type.name = call ptr @inferExpressionType(ptr %arg.tokens, i32 %start, i32 %end)
  %ir.type = call ptr @irTypeName(ptr %type.name)
  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %start, i32 %end)
  %has.op = icmp sgt i32 %op, %start
  br i1 %has.op, label %binary, label %identifier.check
binary:
  %op.text = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %op)
  %op.name = call ptr @irOperatorName(ptr %op.text)
  %b1 = call ptr @csec_string_concat(ptr %op.name, ptr @.str.ir.expr.space)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr %ir.type)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.ir.expr.open)
  %left = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %start, i32 %op)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr %left)
  %b5 = call ptr @csec_string_concat(ptr %b4, ptr @.str.ir.expr.middle)
  %right.start = add i32 %op, 1
  %right = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %right.start, i32 %end)
  %b6 = call ptr @csec_string_concat(ptr %b5, ptr %right)
  %b7 = call ptr @csec_string_concat(ptr %b6, ptr @.str.ir.expr.close)
  ret ptr %b7
identifier.check:
  %kind = call i8 @csec_token_kind_at(ptr %arg.tokens, i32 %start)
  %is.identifier = icmp eq i8 %kind, 73
  br i1 %is.identifier, label %call.check, label %fallback
call.check:
  %next = add i32 %start, 1
  %before.end = icmp slt i32 %next, %end
  br i1 %before.end, label %open.check, label %single.check
open.check:
  %is.open.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %next, i8 79, ptr @.str.ir.expr.paren.open)
  %is.open = icmp ne i32 %is.open.raw, 0
  br i1 %is.open, label %close.check, label %single.check
close.check:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %next, i32 %end, ptr @.str.ir.expr.paren.open, ptr @.str.ir.expr.paren.close)
  %last = sub i32 %end, 1
  %is.call = icmp eq i32 %close, %last
  br i1 %is.call, label %call, label %single.check
call:
  %c1 = call ptr @csec_string_concat(ptr @.str.ir.expr.call, ptr %ir.type)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.ir.expr.at)
  %name = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %start)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %name)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr @.str.ir.expr.paren.open)
  %args.start = add i32 %start, 2
  %args = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %args.start, i32 %last)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr %args)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr @.str.ir.expr.paren.close)
  ret ptr %c6
single.check:
  %single.end = add i32 %start, 1
  %is.single = icmp eq i32 %end, %single.end
  br i1 %is.single, label %single, label %fallback
single:
  %s1 = call ptr @csec_string_concat(ptr @.str.ir.expr.load, ptr %ir.type)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.ir.expr.load.middle)
  %single.name = call ptr @csec_token_text_at(ptr %arg.tokens, i32 %start)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %single.name)
  ret ptr %s3
fallback:
  %f1 = call ptr @csec_string_concat(ptr %ir.type, ptr @.str.ir.expr.space)
  %f2 = call ptr @appendExpressionUntil(ptr %arg.tokens, i32 %start, i32 %end)
  %f3 = call ptr @csec_string_concat(ptr %f1, ptr %f2)
  ret ptr %f3
}

@.str.ir.expr.void = private unnamed_addr constant [5 x i8] c"void\00"
@.str.ir.expr.space = private unnamed_addr constant [2 x i8] c" \00"
@.str.ir.expr.open = private unnamed_addr constant [3 x i8] c" (\00"
@.str.ir.expr.middle = private unnamed_addr constant [5 x i8] c"), (\00"
@.str.ir.expr.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.ir.expr.paren.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.ir.expr.paren.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.ir.expr.call = private unnamed_addr constant [6 x i8] c"call \00"
@.str.ir.expr.at = private unnamed_addr constant [2 x i8] c"@\00"
@.str.ir.expr.load = private unnamed_addr constant [6 x i8] c"load \00"
@.str.ir.expr.load.middle = private unnamed_addr constant [8 x i8] c", ptr %\00"

define ptr @generateIRAssignment(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
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

define ptr @generateIRParamList(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_llvm_param_list(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

@.str.ir.flat.body.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateIRFlatBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  ret ptr @.str.ir.flat.body.empty
}

define ptr @generateIRElseFlatBody(ptr %arg.tokens, i32 %arg.possibleElse, i32 %arg.end, i1 %arg.hasElse) {
entry:
  br i1 %arg.hasElse, label %else.entry, label %no.else
no.else:
  ret ptr @.str.ir.else.none
else.entry:
  %after.else = add i32 %arg.possibleElse, 1
  %else.body.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %after.else)
  %brace.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.body.start, i8 79, ptr @.str.ir.else.open)
  %brace = icmp ne i32 %brace.raw, 0
  br i1 %brace, label %brace.body, label %if.check
brace.body:
  %else.end = call i32 @findClosingToken(ptr %arg.tokens, i32 %else.body.start, i32 %arg.end, ptr @.str.ir.else.open, ptr @.str.ir.else.close)
  %valid = icmp sgt i32 %else.end, %else.body.start
  br i1 %valid, label %flat, label %malformed
flat:
  %body.start = add i32 %else.body.start, 1
  %ret.flat = call ptr @generateIRFlatBody(ptr %arg.tokens, i32 %body.start, i32 %else.end)
  ret ptr %ret.flat
if.check:
  %if.raw = call i32 @csec_token_is(ptr %arg.tokens, i32 %else.body.start, i8 75, ptr @.str.ir.else.if)
  %is.if = icmp ne i32 %if.raw, 0
  br i1 %is.if, label %else.if, label %malformed
else.if:
  ret ptr @.str.ir.else.if.body
malformed:
  ret ptr @.str.ir.else.malformed
}

@.str.ir.else.none = private unnamed_addr constant [20 x i8] c"    ; no else body\0A\00"
@.str.ir.else.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.ir.else.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.ir.else.if = private unnamed_addr constant [3 x i8] c"if\00"
@.str.ir.else.if.body = private unnamed_addr constant [20 x i8] c"    ; else-if body\0A\00"
@.str.ir.else.malformed = private unnamed_addr constant [27 x i8] c"    ; malformed else body\0A\00"

define ptr @generateIRIf(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
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
define ptr @generateIRWhile(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
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
define ptr @generateIRFor(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
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
@.str.ir.body.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateIRBody(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  ret ptr @.str.ir.body.empty
}

@.str.ir.declarations.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateIRDeclarations(ptr %arg.tokens) {
entry:
  ret ptr @.str.ir.declarations.empty
}

@.str.ir.module.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateIR(ptr %arg.tokens) {
entry:
  ret ptr @.str.ir.module.empty
}

@.str.llvm.main.body.empty = private unnamed_addr constant [1 x i8] c"\00"

define ptr @generateLLVMMainBody(ptr %arg.tokens) {
entry:
  ret ptr @.str.llvm.main.body.empty
}

define ptr @llvmCharI8Value(ptr %arg.text) {
entry:
  %text = alloca ptr
  store ptr %arg.text, ptr %text
  %ifcond.20744.i32.left.string = load ptr, ptr %text
  %ifcond.20744.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.20744.i32.left.string)
  %ifcond.20744.i32.left = trunc i64 %ifcond.20744.i32.left.length64 to i32
  %ifcond.20744.i32.right = add i32 0, 0
  %ifcond.20744.i32.comparison = icmp eq i32 %ifcond.20744.i32.left, %ifcond.20744.i32.right
  %ifcond.20744.i32 = zext i1 %ifcond.20744.i32.comparison to i32
  %ifcond.20744 = icmp ne i32 %ifcond.20744.i32, 0
  br i1 %ifcond.20744, label %if.then.20744, label %if.else.20744
if.then.20744:
  %ret.20753 = getelementptr inbounds [2 x i8], ptr @.str.20754, i32 0, i32 0
  ret ptr %ret.20753
if.else.20744:
  br label %if.end.20744
if.end.20744:
  %ch.addr.20757 = alloca i8
  %ch.init.20757.i32.string = load ptr, ptr %text
  %ch.init.20757.i32.index = add i32 0, 0
  %ch.init.20757.i32.char = call i8 @csec_string_char_at(ptr %ch.init.20757.i32.string, i32 %ch.init.20757.i32.index)
  %ch.init.20757.i32 = zext i8 %ch.init.20757.i32.char to i32
  %ch.init.20757 = trunc i32 %ch.init.20757.i32 to i8
  store i8 %ch.init.20757, ptr %ch.addr.20757
  %ifcond.20769.left = load i8, ptr %ch.addr.20757
  %ifcond.20769.right = add i8 0, 73
  %ifcond.20769 = icmp eq i8 %ifcond.20769.left, %ifcond.20769.right
  br i1 %ifcond.20769, label %if.then.20769, label %if.else.20769
if.then.20769:
  %ret.20776 = getelementptr inbounds [3 x i8], ptr @.str.20777, i32 0, i32 0
  ret ptr %ret.20776
if.else.20769:
  br label %if.end.20769
if.end.20769:
  %ifcond.20780.left = load i8, ptr %ch.addr.20757
  %ifcond.20780.right = add i8 0, 75
  %ifcond.20780 = icmp eq i8 %ifcond.20780.left, %ifcond.20780.right
  br i1 %ifcond.20780, label %if.then.20780, label %if.else.20780
if.then.20780:
  %ret.20787 = getelementptr inbounds [3 x i8], ptr @.str.20788, i32 0, i32 0
  ret ptr %ret.20787
if.else.20780:
  br label %if.end.20780
if.end.20780:
  %ifcond.20791.left = load i8, ptr %ch.addr.20757
  %ifcond.20791.right = add i8 0, 78
  %ifcond.20791 = icmp eq i8 %ifcond.20791.left, %ifcond.20791.right
  br i1 %ifcond.20791, label %if.then.20791, label %if.else.20791
if.then.20791:
  %ret.20798 = getelementptr inbounds [3 x i8], ptr @.str.20799, i32 0, i32 0
  ret ptr %ret.20798
if.else.20791:
  br label %if.end.20791
if.end.20791:
  %ifcond.20802.left = load i8, ptr %ch.addr.20757
  %ifcond.20802.right = add i8 0, 70
  %ifcond.20802 = icmp eq i8 %ifcond.20802.left, %ifcond.20802.right
  br i1 %ifcond.20802, label %if.then.20802, label %if.else.20802
if.then.20802:
  %ret.20809 = getelementptr inbounds [3 x i8], ptr @.str.20810, i32 0, i32 0
  ret ptr %ret.20809
if.else.20802:
  br label %if.end.20802
if.end.20802:
  %ifcond.20813.left = load i8, ptr %ch.addr.20757
  %ifcond.20813.right = add i8 0, 83
  %ifcond.20813 = icmp eq i8 %ifcond.20813.left, %ifcond.20813.right
  br i1 %ifcond.20813, label %if.then.20813, label %if.else.20813
if.then.20813:
  %ret.20820 = getelementptr inbounds [3 x i8], ptr @.str.20821, i32 0, i32 0
  ret ptr %ret.20820
if.else.20813:
  br label %if.end.20813
if.end.20813:
  %ifcond.20824.left = load i8, ptr %ch.addr.20757
  %ifcond.20824.right = add i8 0, 82
  %ifcond.20824 = icmp eq i8 %ifcond.20824.left, %ifcond.20824.right
  br i1 %ifcond.20824, label %if.then.20824, label %if.else.20824
if.then.20824:
  %ret.20831 = getelementptr inbounds [3 x i8], ptr @.str.20832, i32 0, i32 0
  ret ptr %ret.20831
if.else.20824:
  br label %if.end.20824
if.end.20824:
  %ifcond.20835.left = load i8, ptr %ch.addr.20757
  %ifcond.20835.right = add i8 0, 67
  %ifcond.20835 = icmp eq i8 %ifcond.20835.left, %ifcond.20835.right
  br i1 %ifcond.20835, label %if.then.20835, label %if.else.20835
if.then.20835:
  %ret.20842 = getelementptr inbounds [3 x i8], ptr @.str.20843, i32 0, i32 0
  ret ptr %ret.20842
if.else.20835:
  br label %if.end.20835
if.end.20835:
  %ifcond.20846.left = load i8, ptr %ch.addr.20757
  %ifcond.20846.right = add i8 0, 66
  %ifcond.20846 = icmp eq i8 %ifcond.20846.left, %ifcond.20846.right
  br i1 %ifcond.20846, label %if.then.20846, label %if.else.20846
if.then.20846:
  %ret.20853 = getelementptr inbounds [3 x i8], ptr @.str.20854, i32 0, i32 0
  ret ptr %ret.20853
if.else.20846:
  br label %if.end.20846
if.end.20846:
  %ifcond.20857.left = load i8, ptr %ch.addr.20757
  %ifcond.20857.right = add i8 0, 79
  %ifcond.20857 = icmp eq i8 %ifcond.20857.left, %ifcond.20857.right
  br i1 %ifcond.20857, label %if.then.20857, label %if.else.20857
if.then.20857:
  %ret.20864 = getelementptr inbounds [3 x i8], ptr @.str.20865, i32 0, i32 0
  ret ptr %ret.20864
if.else.20857:
  br label %if.end.20857
if.end.20857:
  %ifcond.20868.left = load i8, ptr %ch.addr.20757
  %ifcond.20868.right = add i8 0, 77
  %ifcond.20868 = icmp eq i8 %ifcond.20868.left, %ifcond.20868.right
  br i1 %ifcond.20868, label %if.then.20868, label %if.else.20868
if.then.20868:
  %ret.20875 = getelementptr inbounds [3 x i8], ptr @.str.20876, i32 0, i32 0
  ret ptr %ret.20875
if.else.20868:
  br label %if.end.20868
if.end.20868:
  %ifcond.20879.left = load i8, ptr %ch.addr.20757
  %ifcond.20879.right = add i8 0, 69
  %ifcond.20879 = icmp eq i8 %ifcond.20879.left, %ifcond.20879.right
  br i1 %ifcond.20879, label %if.then.20879, label %if.else.20879
if.then.20879:
  %ret.20886 = getelementptr inbounds [3 x i8], ptr @.str.20887, i32 0, i32 0
  ret ptr %ret.20886
if.else.20879:
  br label %if.end.20879
if.end.20879:
  %ifcond.20890.left = load i8, ptr %ch.addr.20757
  %ifcond.20890.right = add i8 0, 120
  %ifcond.20890 = icmp eq i8 %ifcond.20890.left, %ifcond.20890.right
  br i1 %ifcond.20890, label %if.then.20890, label %if.else.20890
if.then.20890:
  %ret.20897 = getelementptr inbounds [4 x i8], ptr @.str.20898, i32 0, i32 0
  ret ptr %ret.20897
if.else.20890:
  br label %if.end.20890
if.end.20890:
  %ifcond.20901.left = load i8, ptr %ch.addr.20757
  %ifcond.20901.right = add i8 0, 121
  %ifcond.20901 = icmp eq i8 %ifcond.20901.left, %ifcond.20901.right
  br i1 %ifcond.20901, label %if.then.20901, label %if.else.20901
if.then.20901:
  %ret.20908 = getelementptr inbounds [4 x i8], ptr @.str.20909, i32 0, i32 0
  ret ptr %ret.20908
if.else.20901:
  br label %if.end.20901
if.end.20901:
  %ifcond.20912.left = load i8, ptr %ch.addr.20757
  %ifcond.20912.right = add i8 0, 122
  %ifcond.20912 = icmp eq i8 %ifcond.20912.left, %ifcond.20912.right
  br i1 %ifcond.20912, label %if.then.20912, label %if.else.20912
if.then.20912:
  %ret.20919 = getelementptr inbounds [4 x i8], ptr @.str.20920, i32 0, i32 0
  ret ptr %ret.20919
if.else.20912:
  br label %if.end.20912
if.end.20912:
  %ifcond.20923.left = load i8, ptr %ch.addr.20757
  %ifcond.20923.right = add i8 0, 97
  %ifcond.20923 = icmp eq i8 %ifcond.20923.left, %ifcond.20923.right
  br i1 %ifcond.20923, label %if.then.20923, label %if.else.20923
if.then.20923:
  %ret.20930 = getelementptr inbounds [3 x i8], ptr @.str.20931, i32 0, i32 0
  ret ptr %ret.20930
if.else.20923:
  br label %if.end.20923
if.end.20923:
  %ifcond.20934.left = load i8, ptr %ch.addr.20757
  %ifcond.20934.right = add i8 0, 116
  %ifcond.20934 = icmp eq i8 %ifcond.20934.left, %ifcond.20934.right
  br i1 %ifcond.20934, label %if.then.20934, label %if.else.20934
if.then.20934:
  %ret.20941 = getelementptr inbounds [4 x i8], ptr @.str.20942, i32 0, i32 0
  ret ptr %ret.20941
if.else.20934:
  br label %if.end.20934
if.end.20934:
  %ifcond.20945.left = load i8, ptr %ch.addr.20757
  %ifcond.20945.right = add i8 0, 117
  %ifcond.20945 = icmp eq i8 %ifcond.20945.left, %ifcond.20945.right
  br i1 %ifcond.20945, label %if.then.20945, label %if.else.20945
if.then.20945:
  %ret.20952 = getelementptr inbounds [4 x i8], ptr @.str.20953, i32 0, i32 0
  ret ptr %ret.20952
if.else.20945:
  br label %if.end.20945
if.end.20945:
  %ifcond.20956.left = load i8, ptr %ch.addr.20757
  %ifcond.20956.right = add i8 0, 48
  %ifcond.20956 = icmp eq i8 %ifcond.20956.left, %ifcond.20956.right
  br i1 %ifcond.20956, label %if.then.20956, label %if.else.20956
if.then.20956:
  %ret.20963 = getelementptr inbounds [3 x i8], ptr @.str.20964, i32 0, i32 0
  ret ptr %ret.20963
if.else.20956:
  br label %if.end.20956
if.end.20956:
  %ifcond.20967.left = load i8, ptr %ch.addr.20757
  %ifcond.20967.right = add i8 0, 49
  %ifcond.20967 = icmp eq i8 %ifcond.20967.left, %ifcond.20967.right
  br i1 %ifcond.20967, label %if.then.20967, label %if.else.20967
if.then.20967:
  %ret.20974 = getelementptr inbounds [3 x i8], ptr @.str.20975, i32 0, i32 0
  ret ptr %ret.20974
if.else.20967:
  br label %if.end.20967
if.end.20967:
  %ifcond.20978.left = load i8, ptr %ch.addr.20757
  %ifcond.20978.right = add i8 0, 50
  %ifcond.20978 = icmp eq i8 %ifcond.20978.left, %ifcond.20978.right
  br i1 %ifcond.20978, label %if.then.20978, label %if.else.20978
if.then.20978:
  %ret.20985 = getelementptr inbounds [3 x i8], ptr @.str.20986, i32 0, i32 0
  ret ptr %ret.20985
if.else.20978:
  br label %if.end.20978
if.end.20978:
  %ifcond.20989.left = load i8, ptr %ch.addr.20757
  %ifcond.20989.right = add i8 0, 51
  %ifcond.20989 = icmp eq i8 %ifcond.20989.left, %ifcond.20989.right
  br i1 %ifcond.20989, label %if.then.20989, label %if.else.20989
if.then.20989:
  %ret.20996 = getelementptr inbounds [3 x i8], ptr @.str.20997, i32 0, i32 0
  ret ptr %ret.20996
if.else.20989:
  br label %if.end.20989
if.end.20989:
  %ifcond.21000.left = load i8, ptr %ch.addr.20757
  %ifcond.21000.right = add i8 0, 52
  %ifcond.21000 = icmp eq i8 %ifcond.21000.left, %ifcond.21000.right
  br i1 %ifcond.21000, label %if.then.21000, label %if.else.21000
if.then.21000:
  %ret.21007 = getelementptr inbounds [3 x i8], ptr @.str.21008, i32 0, i32 0
  ret ptr %ret.21007
if.else.21000:
  br label %if.end.21000
if.end.21000:
  %ifcond.21011.left = load i8, ptr %ch.addr.20757
  %ifcond.21011.right = add i8 0, 53
  %ifcond.21011 = icmp eq i8 %ifcond.21011.left, %ifcond.21011.right
  br i1 %ifcond.21011, label %if.then.21011, label %if.else.21011
if.then.21011:
  %ret.21018 = getelementptr inbounds [3 x i8], ptr @.str.21019, i32 0, i32 0
  ret ptr %ret.21018
if.else.21011:
  br label %if.end.21011
if.end.21011:
  %ifcond.21022.left = load i8, ptr %ch.addr.20757
  %ifcond.21022.right = add i8 0, 54
  %ifcond.21022 = icmp eq i8 %ifcond.21022.left, %ifcond.21022.right
  br i1 %ifcond.21022, label %if.then.21022, label %if.else.21022
if.then.21022:
  %ret.21029 = getelementptr inbounds [3 x i8], ptr @.str.21030, i32 0, i32 0
  ret ptr %ret.21029
if.else.21022:
  br label %if.end.21022
if.end.21022:
  %ifcond.21033.left = load i8, ptr %ch.addr.20757
  %ifcond.21033.right = add i8 0, 55
  %ifcond.21033 = icmp eq i8 %ifcond.21033.left, %ifcond.21033.right
  br i1 %ifcond.21033, label %if.then.21033, label %if.else.21033
if.then.21033:
  %ret.21040 = getelementptr inbounds [3 x i8], ptr @.str.21041, i32 0, i32 0
  ret ptr %ret.21040
if.else.21033:
  br label %if.end.21033
if.end.21033:
  %ifcond.21044.left = load i8, ptr %ch.addr.20757
  %ifcond.21044.right = add i8 0, 56
  %ifcond.21044 = icmp eq i8 %ifcond.21044.left, %ifcond.21044.right
  br i1 %ifcond.21044, label %if.then.21044, label %if.else.21044
if.then.21044:
  %ret.21051 = getelementptr inbounds [3 x i8], ptr @.str.21052, i32 0, i32 0
  ret ptr %ret.21051
if.else.21044:
  br label %if.end.21044
if.end.21044:
  %ifcond.21055.left = load i8, ptr %ch.addr.20757
  %ifcond.21055.right = add i8 0, 57
  %ifcond.21055 = icmp eq i8 %ifcond.21055.left, %ifcond.21055.right
  br i1 %ifcond.21055, label %if.then.21055, label %if.else.21055
if.then.21055:
  %ret.21062 = getelementptr inbounds [3 x i8], ptr @.str.21063, i32 0, i32 0
  ret ptr %ret.21062
if.else.21055:
  br label %if.end.21055
if.end.21055:
  %ifcond.21066.left = load i8, ptr %ch.addr.20757
  %ifcond.21066.right = add i8 0, 95
  %ifcond.21066 = icmp eq i8 %ifcond.21066.left, %ifcond.21066.right
  br i1 %ifcond.21066, label %if.then.21066, label %if.else.21066
if.then.21066:
  %ret.21073 = getelementptr inbounds [3 x i8], ptr @.str.21074, i32 0, i32 0
  ret ptr %ret.21073
if.else.21066:
  br label %if.end.21066
if.end.21066:
  %ifcond.21077.left = load i8, ptr %ch.addr.20757
  %ifcond.21077.right = add i8 0, 32
  %ifcond.21077 = icmp eq i8 %ifcond.21077.left, %ifcond.21077.right
  br i1 %ifcond.21077, label %if.then.21077, label %if.else.21077
if.then.21077:
  %ret.21084 = getelementptr inbounds [3 x i8], ptr @.str.21085, i32 0, i32 0
  ret ptr %ret.21084
if.else.21077:
  br label %if.end.21077
if.end.21077:
  %ifcond.21088.left = load i8, ptr %ch.addr.20757
  %ifcond.21088.right = add i8 0, 34
  %ifcond.21088 = icmp eq i8 %ifcond.21088.left, %ifcond.21088.right
  br i1 %ifcond.21088, label %if.then.21088, label %if.else.21088
if.then.21088:
  %ret.21095 = getelementptr inbounds [3 x i8], ptr @.str.21096, i32 0, i32 0
  ret ptr %ret.21095
if.else.21088:
  br label %if.end.21088
if.end.21088:
  %ifcond.21099.left.left.left = load i8, ptr %ch.addr.20757
  %ifcond.21099.left.left.right = add i8 0, 0
  %ifcond.21099.left.left = icmp eq i8 %ifcond.21099.left.left.left, %ifcond.21099.left.left.right
  %ifcond.21099.left.right.i32.left.string = load ptr, ptr %text
  %ifcond.21099.left.right.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.21099.left.right.i32.left.string)
  %ifcond.21099.left.right.i32.left = trunc i64 %ifcond.21099.left.right.i32.left.length64 to i32
  %ifcond.21099.left.right.i32.right = add i32 0, 1
  %ifcond.21099.left.right.i32.comparison = icmp sgt i32 %ifcond.21099.left.right.i32.left, %ifcond.21099.left.right.i32.right
  %ifcond.21099.left.right.i32 = zext i1 %ifcond.21099.left.right.i32.comparison to i32
  %ifcond.21099.left.right = icmp ne i32 %ifcond.21099.left.right.i32, 0
  %ifcond.21099.left = and i1 %ifcond.21099.left.left, %ifcond.21099.left.right
  %ifcond.21099.right.i32.left.string = load ptr, ptr %text
  %ifcond.21099.right.i32.left.index = add i32 0, 1
  %ifcond.21099.right.i32.left.char = call i8 @csec_string_char_at(ptr %ifcond.21099.right.i32.left.string, i32 %ifcond.21099.right.i32.left.index)
  %ifcond.21099.right.i32.left = zext i8 %ifcond.21099.right.i32.left.char to i32
  %ifcond.21099.right.i32.right.char = add i8 0, 0
  %ifcond.21099.right.i32.right = zext i8 %ifcond.21099.right.i32.right.char to i32
  %ifcond.21099.right.i32.comparison = icmp eq i32 %ifcond.21099.right.i32.left, %ifcond.21099.right.i32.right
  %ifcond.21099.right.i32 = zext i1 %ifcond.21099.right.i32.comparison to i32
  %ifcond.21099.right = icmp ne i32 %ifcond.21099.right.i32, 0
  %ifcond.21099 = and i1 %ifcond.21099.left, %ifcond.21099.right
  br i1 %ifcond.21099, label %if.then.21099, label %if.else.21099
if.then.21099:
  %ret.21121 = getelementptr inbounds [3 x i8], ptr @.str.21122, i32 0, i32 0
  ret ptr %ret.21121
if.else.21099:
  br label %if.end.21099
if.end.21099:
  %ifcond.21125.left.left.left = load i8, ptr %ch.addr.20757
  %ifcond.21125.left.left.right = add i8 0, 0
  %ifcond.21125.left.left = icmp eq i8 %ifcond.21125.left.left.left, %ifcond.21125.left.left.right
  %ifcond.21125.left.right.i32.left.string = load ptr, ptr %text
  %ifcond.21125.left.right.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.21125.left.right.i32.left.string)
  %ifcond.21125.left.right.i32.left = trunc i64 %ifcond.21125.left.right.i32.left.length64 to i32
  %ifcond.21125.left.right.i32.right = add i32 0, 1
  %ifcond.21125.left.right.i32.comparison = icmp sgt i32 %ifcond.21125.left.right.i32.left, %ifcond.21125.left.right.i32.right
  %ifcond.21125.left.right.i32 = zext i1 %ifcond.21125.left.right.i32.comparison to i32
  %ifcond.21125.left.right = icmp ne i32 %ifcond.21125.left.right.i32, 0
  %ifcond.21125.left = and i1 %ifcond.21125.left.left, %ifcond.21125.left.right
  %ifcond.21125.right.i32.left.string = load ptr, ptr %text
  %ifcond.21125.right.i32.left.index = add i32 0, 1
  %ifcond.21125.right.i32.left.char = call i8 @csec_string_char_at(ptr %ifcond.21125.right.i32.left.string, i32 %ifcond.21125.right.i32.left.index)
  %ifcond.21125.right.i32.left = zext i8 %ifcond.21125.right.i32.left.char to i32
  %ifcond.21125.right.i32.right.char = add i8 0, 110
  %ifcond.21125.right.i32.right = zext i8 %ifcond.21125.right.i32.right.char to i32
  %ifcond.21125.right.i32.comparison = icmp eq i32 %ifcond.21125.right.i32.left, %ifcond.21125.right.i32.right
  %ifcond.21125.right.i32 = zext i1 %ifcond.21125.right.i32.comparison to i32
  %ifcond.21125.right = icmp ne i32 %ifcond.21125.right.i32, 0
  %ifcond.21125 = and i1 %ifcond.21125.left, %ifcond.21125.right
  br i1 %ifcond.21125, label %if.then.21125, label %if.else.21125
if.then.21125:
  %ret.21147 = getelementptr inbounds [3 x i8], ptr @.str.21148, i32 0, i32 0
  ret ptr %ret.21147
if.else.21125:
  br label %if.end.21125
if.end.21125:
  %ifcond.21151.left.left.left = load i8, ptr %ch.addr.20757
  %ifcond.21151.left.left.right = add i8 0, 0
  %ifcond.21151.left.left = icmp eq i8 %ifcond.21151.left.left.left, %ifcond.21151.left.left.right
  %ifcond.21151.left.right.i32.left.string = load ptr, ptr %text
  %ifcond.21151.left.right.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.21151.left.right.i32.left.string)
  %ifcond.21151.left.right.i32.left = trunc i64 %ifcond.21151.left.right.i32.left.length64 to i32
  %ifcond.21151.left.right.i32.right = add i32 0, 1
  %ifcond.21151.left.right.i32.comparison = icmp sgt i32 %ifcond.21151.left.right.i32.left, %ifcond.21151.left.right.i32.right
  %ifcond.21151.left.right.i32 = zext i1 %ifcond.21151.left.right.i32.comparison to i32
  %ifcond.21151.left.right = icmp ne i32 %ifcond.21151.left.right.i32, 0
  %ifcond.21151.left = and i1 %ifcond.21151.left.left, %ifcond.21151.left.right
  %ifcond.21151.right.i32.left.string = load ptr, ptr %text
  %ifcond.21151.right.i32.left.index = add i32 0, 1
  %ifcond.21151.right.i32.left.char = call i8 @csec_string_char_at(ptr %ifcond.21151.right.i32.left.string, i32 %ifcond.21151.right.i32.left.index)
  %ifcond.21151.right.i32.left = zext i8 %ifcond.21151.right.i32.left.char to i32
  %ifcond.21151.right.i32.right.char = add i8 0, 116
  %ifcond.21151.right.i32.right = zext i8 %ifcond.21151.right.i32.right.char to i32
  %ifcond.21151.right.i32.comparison = icmp eq i32 %ifcond.21151.right.i32.left, %ifcond.21151.right.i32.right
  %ifcond.21151.right.i32 = zext i1 %ifcond.21151.right.i32.comparison to i32
  %ifcond.21151.right = icmp ne i32 %ifcond.21151.right.i32, 0
  %ifcond.21151 = and i1 %ifcond.21151.left, %ifcond.21151.right
  br i1 %ifcond.21151, label %if.then.21151, label %if.else.21151
if.then.21151:
  %ret.21173 = getelementptr inbounds [2 x i8], ptr @.str.21174, i32 0, i32 0
  ret ptr %ret.21173
if.else.21151:
  br label %if.end.21151
if.end.21151:
  %ret.21177 = getelementptr inbounds [2 x i8], ptr @.str.21178, i32 0, i32 0
  ret ptr %ret.21177
}

define ptr @llvmI32Value(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.21199 = alloca i32
  %end.init.21199.arg0 = load ptr, ptr %tokens
  %end.init.21199.arg1 = load i32, ptr %start
  %end.init.21199.arg2 = load i32, ptr %rawEnd
  %end.init.21199 = call i32 @trimExpressionEnd(ptr %end.init.21199.arg0, i32 %end.init.21199.arg1, i32 %end.init.21199.arg2)
  store i32 %end.init.21199, ptr %end.addr.21199
  %ifcond.21213.i32.left = load i32, ptr %end.addr.21199
  %ifcond.21213.i32.right = load i32, ptr %start
  %ifcond.21213.i32.comparison = icmp sle i32 %ifcond.21213.i32.left, %ifcond.21213.i32.right
  %ifcond.21213.i32 = zext i1 %ifcond.21213.i32.comparison to i32
  %ifcond.21213 = icmp ne i32 %ifcond.21213.i32, 0
  br i1 %ifcond.21213, label %if.then.21213, label %if.else.21213
if.then.21213:
  %ret.21220 = getelementptr inbounds [2 x i8], ptr @.str.21221, i32 0, i32 0
  ret ptr %ret.21220
if.else.21213:
  br label %if.end.21213
if.end.21213:
  %ifcond.21224.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.21224.left.i32.left.char.arg0 = trunc i32 %ifcond.21224.left.i32.left.char.arg0.i32 to i8
  %ifcond.21224.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.21224.left.i32.left.char.arg1 = trunc i32 %ifcond.21224.left.i32.left.char.arg1.i32 to i8
  %ifcond.21224.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.21224.left.i32.left.char.arg0, i8 %ifcond.21224.left.i32.left.char.arg1)
  %ifcond.21224.left.i32.left = zext i8 %ifcond.21224.left.i32.left.char to i32
  %ifcond.21224.left.i32.right.char = call i8 @kindInteger()
  %ifcond.21224.left.i32.right = zext i8 %ifcond.21224.left.i32.right.char to i32
  %ifcond.21224.left.i32.comparison = icmp eq i32 %ifcond.21224.left.i32.left, %ifcond.21224.left.i32.right
  %ifcond.21224.left.i32 = zext i1 %ifcond.21224.left.i32.comparison to i32
  %ifcond.21224.left = icmp ne i32 %ifcond.21224.left.i32, 0
  %ifcond.21224.right.i32.left = load i32, ptr %end.addr.21199
  %ifcond.21224.right.i32.right.left = load i32, ptr %start
  %ifcond.21224.right.i32.right.right = add i32 0, 1
  %ifcond.21224.right.i32.right = add i32 %ifcond.21224.right.i32.right.left, %ifcond.21224.right.i32.right.right
  %ifcond.21224.right.i32.comparison = icmp eq i32 %ifcond.21224.right.i32.left, %ifcond.21224.right.i32.right
  %ifcond.21224.right.i32 = zext i1 %ifcond.21224.right.i32.comparison to i32
  %ifcond.21224.right = icmp ne i32 %ifcond.21224.right.i32, 0
  %ifcond.21224 = and i1 %ifcond.21224.left, %ifcond.21224.right
  br i1 %ifcond.21224, label %if.then.21224, label %if.else.21224
if.then.21224:
  %ret.21244.arg0 = load ptr, ptr %tokens
  %ret.21244.arg1 = load i32, ptr %start
  %ret.21244 = call ptr @tokenTextAt(ptr %ret.21244.arg0, i32 %ret.21244.arg1)
  ret ptr %ret.21244
if.else.21224:
  br label %if.end.21224
if.end.21224:
  %ifcond.21253.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.21253.left.i32.left.char.arg0 = trunc i32 %ifcond.21253.left.i32.left.char.arg0.i32 to i8
  %ifcond.21253.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.21253.left.i32.left.char.arg1 = trunc i32 %ifcond.21253.left.i32.left.char.arg1.i32 to i8
  %ifcond.21253.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.21253.left.i32.left.char.arg0, i8 %ifcond.21253.left.i32.left.char.arg1)
  %ifcond.21253.left.i32.left = zext i8 %ifcond.21253.left.i32.left.char to i32
  %ifcond.21253.left.i32.right.char = call i8 @kindChar()
  %ifcond.21253.left.i32.right = zext i8 %ifcond.21253.left.i32.right.char to i32
  %ifcond.21253.left.i32.comparison = icmp eq i32 %ifcond.21253.left.i32.left, %ifcond.21253.left.i32.right
  %ifcond.21253.left.i32 = zext i1 %ifcond.21253.left.i32.comparison to i32
  %ifcond.21253.left = icmp ne i32 %ifcond.21253.left.i32, 0
  %ifcond.21253.right.i32.left = load i32, ptr %end.addr.21199
  %ifcond.21253.right.i32.right.left = load i32, ptr %start
  %ifcond.21253.right.i32.right.right = add i32 0, 1
  %ifcond.21253.right.i32.right = add i32 %ifcond.21253.right.i32.right.left, %ifcond.21253.right.i32.right.right
  %ifcond.21253.right.i32.comparison = icmp eq i32 %ifcond.21253.right.i32.left, %ifcond.21253.right.i32.right
  %ifcond.21253.right.i32 = zext i1 %ifcond.21253.right.i32.comparison to i32
  %ifcond.21253.right = icmp ne i32 %ifcond.21253.right.i32, 0
  %ifcond.21253 = and i1 %ifcond.21253.left, %ifcond.21253.right
  br i1 %ifcond.21253, label %if.then.21253, label %if.else.21253
if.then.21253:
  %ret.21273.arg0.arg0 = load ptr, ptr %tokens
  %ret.21273.arg0.arg1 = load i32, ptr %start
  %ret.21273.arg0 = call ptr @tokenTextAt(ptr %ret.21273.arg0.arg0, i32 %ret.21273.arg0.arg1)
  %ret.21273 = call ptr @llvmCharI8Value(ptr %ret.21273.arg0)
  ret ptr %ret.21273
if.else.21253:
  br label %if.end.21253
if.end.21253:
  %ifcond.21285.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.21285.left.i32.left.char.arg0 = trunc i32 %ifcond.21285.left.i32.left.char.arg0.i32 to i8
  %ifcond.21285.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.21285.left.i32.left.char.arg1 = trunc i32 %ifcond.21285.left.i32.left.char.arg1.i32 to i8
  %ifcond.21285.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.21285.left.i32.left.char.arg0, i8 %ifcond.21285.left.i32.left.char.arg1)
  %ifcond.21285.left.i32.left = zext i8 %ifcond.21285.left.i32.left.char to i32
  %ifcond.21285.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.21285.left.i32.right = zext i8 %ifcond.21285.left.i32.right.char to i32
  %ifcond.21285.left.i32.comparison = icmp eq i32 %ifcond.21285.left.i32.left, %ifcond.21285.left.i32.right
  %ifcond.21285.left.i32 = zext i1 %ifcond.21285.left.i32.comparison to i32
  %ifcond.21285.left = icmp ne i32 %ifcond.21285.left.i32, 0
  %ifcond.21285.right.i32.left = load i32, ptr %end.addr.21199
  %ifcond.21285.right.i32.right.left = load i32, ptr %start
  %ifcond.21285.right.i32.right.right = add i32 0, 1
  %ifcond.21285.right.i32.right = add i32 %ifcond.21285.right.i32.right.left, %ifcond.21285.right.i32.right.right
  %ifcond.21285.right.i32.comparison = icmp eq i32 %ifcond.21285.right.i32.left, %ifcond.21285.right.i32.right
  %ifcond.21285.right.i32 = zext i1 %ifcond.21285.right.i32.comparison to i32
  %ifcond.21285.right = icmp ne i32 %ifcond.21285.right.i32, 0
  %ifcond.21285 = and i1 %ifcond.21285.left, %ifcond.21285.right
  br i1 %ifcond.21285, label %if.then.21285, label %if.else.21285
if.then.21285:
  %ret.21305.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.21306, i32 0, i32 0
  %ret.21305.left.left.right.arg0 = load ptr, ptr %tokens
  %ret.21305.left.left.right.arg1 = load i32, ptr %start
  %ret.21305.left.left.right = call ptr @tokenTextAt(ptr %ret.21305.left.left.right.arg0, i32 %ret.21305.left.left.right.arg1)
  %ret.21305.left.left = call ptr @csec_string_concat(ptr %ret.21305.left.left.left, ptr %ret.21305.left.left.right)
  %ret.21305.left.right = getelementptr inbounds [6 x i8], ptr @.str.21315, i32 0, i32 0
  %ret.21305.left = call ptr @csec_string_concat(ptr %ret.21305.left.left, ptr %ret.21305.left.right)
  %ret.21305.right.i32 = load i32, ptr %start
  %ret.21305.right.i64 = sext i32 %ret.21305.right.i32 to i64
  %ret.21305.right = call ptr @csec_to_string_i64(i64 %ret.21305.right.i64)
  %ret.21305 = call ptr @csec_string_concat(ptr %ret.21305.left, ptr %ret.21305.right)
  ret ptr %ret.21305
if.else.21285:
  br label %if.end.21285
if.end.21285:
  %ret.21320 = getelementptr inbounds [5 x i8], ptr @.str.21321, i32 0, i32 0
  ret ptr %ret.21320
}

define ptr @lookupFunctionParamType(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %limit = alloca i32
  store i32 %arg.limit, ptr %limit
  %name = alloca ptr
  store ptr %arg.name, ptr %name
  %decl.addr.21342 = alloca i32
  %decl.init.21342.arg0 = load ptr, ptr %tokens
  %decl.init.21342.arg1 = load i32, ptr %limit
  %decl.init.21342 = call i32 @csec_enclosing_function_decl_start(ptr %decl.init.21342.arg0, i32 %decl.init.21342.arg1)
  store i32 %decl.init.21342, ptr %decl.addr.21342
  %ifcond.21354.i32.left = load i32, ptr %decl.addr.21342
  %ifcond.21354.i32.right = add i32 0, 0
  %ifcond.21354.i32.comparison = icmp slt i32 %ifcond.21354.i32.left, %ifcond.21354.i32.right
  %ifcond.21354.i32 = zext i1 %ifcond.21354.i32.comparison to i32
  %ifcond.21354 = icmp ne i32 %ifcond.21354.i32, 0
  br i1 %ifcond.21354, label %if.then.21354, label %if.else.21354
if.then.21354:
  %ret.21361 = getelementptr inbounds [8 x i8], ptr @.str.21362, i32 0, i32 0
  ret ptr %ret.21361
if.else.21354:
  br label %if.end.21354
if.end.21354:
  %paramStart.addr.21365 = alloca i32
  %paramStart.init.21365.arg0 = load ptr, ptr %tokens
  %paramStart.init.21365.arg1 = load i32, ptr %decl.addr.21342
  %paramStart.init.21365 = call i32 @findFunctionParamStart(ptr %paramStart.init.21365.arg0, i32 %paramStart.init.21365.arg1)
  store i32 %paramStart.init.21365, ptr %paramStart.addr.21365
  %ifcond.21377.i32.left = load i32, ptr %paramStart.addr.21365
  %ifcond.21377.i32.right = add i32 0, 0
  %ifcond.21377.i32.comparison = icmp slt i32 %ifcond.21377.i32.left, %ifcond.21377.i32.right
  %ifcond.21377.i32 = zext i1 %ifcond.21377.i32.comparison to i32
  %ifcond.21377 = icmp ne i32 %ifcond.21377.i32, 0
  br i1 %ifcond.21377, label %if.then.21377, label %if.else.21377
if.then.21377:
  %ret.21384 = getelementptr inbounds [8 x i8], ptr @.str.21385, i32 0, i32 0
  ret ptr %ret.21384
if.else.21377:
  br label %if.end.21377
if.end.21377:
  %paramEnd.addr.21388 = alloca i32
  %paramEnd.init.21388.arg0 = load ptr, ptr %tokens
  %paramEnd.init.21388.arg1 = load i32, ptr %paramStart.addr.21365
  %paramEnd.init.21388.arg2.arg0 = load ptr, ptr %tokens
  %paramEnd.init.21388.arg2.arg1 = load i32, ptr %decl.addr.21342
  %paramEnd.init.21388.arg2 = call i32 @advanceTopLevelDecl(ptr %paramEnd.init.21388.arg2.arg0, i32 %paramEnd.init.21388.arg2.arg1)
  %paramEnd.init.21388.arg3 = getelementptr inbounds [2 x i8], ptr @.str.21406, i32 0, i32 0
  %paramEnd.init.21388.arg4 = getelementptr inbounds [2 x i8], ptr @.str.21408, i32 0, i32 0
  %paramEnd.init.21388 = call i32 @findClosingToken(ptr %paramEnd.init.21388.arg0, i32 %paramEnd.init.21388.arg1, i32 %paramEnd.init.21388.arg2, ptr %paramEnd.init.21388.arg3, ptr %paramEnd.init.21388.arg4)
  store i32 %paramEnd.init.21388, ptr %paramEnd.addr.21388
  %ifcond.21411.i32.left = load i32, ptr %paramEnd.addr.21388
  %ifcond.21411.i32.right = load i32, ptr %paramStart.addr.21365
  %ifcond.21411.i32.comparison = icmp sle i32 %ifcond.21411.i32.left, %ifcond.21411.i32.right
  %ifcond.21411.i32 = zext i1 %ifcond.21411.i32.comparison to i32
  %ifcond.21411 = icmp ne i32 %ifcond.21411.i32, 0
  br i1 %ifcond.21411, label %if.then.21411, label %if.else.21411
if.then.21411:
  %ret.21418 = getelementptr inbounds [8 x i8], ptr @.str.21419, i32 0, i32 0
  ret ptr %ret.21418
if.else.21411:
  br label %if.end.21411
if.end.21411:
  %cursor.addr.21422 = alloca i32
  %cursor.init.21422.left = load i32, ptr %paramStart.addr.21365
  %cursor.init.21422.right = add i32 0, 1
  %cursor.init.21422 = add i32 %cursor.init.21422.left, %cursor.init.21422.right
  store i32 %cursor.init.21422, ptr %cursor.addr.21422
  br label %while.cond.21431
while.cond.21431:
  %whilecond.21431.left.i32.left = load i32, ptr %cursor.addr.21422
  %whilecond.21431.left.i32.right = load i32, ptr %paramEnd.addr.21388
  %whilecond.21431.left.i32.comparison = icmp slt i32 %whilecond.21431.left.i32.left, %whilecond.21431.left.i32.right
  %whilecond.21431.left.i32 = zext i1 %whilecond.21431.left.i32.comparison to i32
  %whilecond.21431.left = icmp ne i32 %whilecond.21431.left.i32, 0
  %whilecond.21431.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %whilecond.21431.right.i32.left.char.arg0 = trunc i32 %whilecond.21431.right.i32.left.char.arg0.i32 to i8
  %whilecond.21431.right.i32.left.char.arg1.i32 = load i32, ptr %cursor.addr.21422
  %whilecond.21431.right.i32.left.char.arg1 = trunc i32 %whilecond.21431.right.i32.left.char.arg1.i32 to i8
  %whilecond.21431.right.i32.left.char = call i8 @tokenKindAt(i8 %whilecond.21431.right.i32.left.char.arg0, i8 %whilecond.21431.right.i32.left.char.arg1)
  %whilecond.21431.right.i32.left = zext i8 %whilecond.21431.right.i32.left.char to i32
  %whilecond.21431.right.i32.right.char = call i8 @kindEof()
  %whilecond.21431.right.i32.right = zext i8 %whilecond.21431.right.i32.right.char to i32
  %whilecond.21431.right.i32.comparison = icmp ne i32 %whilecond.21431.right.i32.left, %whilecond.21431.right.i32.right
  %whilecond.21431.right.i32 = zext i1 %whilecond.21431.right.i32.comparison to i32
  %whilecond.21431.right = icmp ne i32 %whilecond.21431.right.i32, 0
  %whilecond.21431 = and i1 %whilecond.21431.left, %whilecond.21431.right
  br i1 %whilecond.21431, label %while.body.21431, label %while.end.21431
while.body.21431:
  %ifcond.21449.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.21449.left.i32.left.char.arg0 = trunc i32 %ifcond.21449.left.i32.left.char.arg0.i32 to i8
  %ifcond.21449.left.i32.left.char.arg1.i32 = load i32, ptr %cursor.addr.21422
  %ifcond.21449.left.i32.left.char.arg1 = trunc i32 %ifcond.21449.left.i32.left.char.arg1.i32 to i8
  %ifcond.21449.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.21449.left.i32.left.char.arg0, i8 %ifcond.21449.left.i32.left.char.arg1)
  %ifcond.21449.left.i32.left = zext i8 %ifcond.21449.left.i32.left.char to i32
  %ifcond.21449.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.21449.left.i32.right = zext i8 %ifcond.21449.left.i32.right.char to i32
  %ifcond.21449.left.i32.comparison = icmp eq i32 %ifcond.21449.left.i32.left, %ifcond.21449.left.i32.right
  %ifcond.21449.left.i32 = zext i1 %ifcond.21449.left.i32.comparison to i32
  %ifcond.21449.left = icmp ne i32 %ifcond.21449.left.i32, 0
  %ifcond.21449.right.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.21449.right.arg0.arg1 = load i32, ptr %cursor.addr.21422
  %ifcond.21449.right.arg0 = call ptr @tokenTextAt(ptr %ifcond.21449.right.arg0.arg0, i32 %ifcond.21449.right.arg0.arg1)
  %ifcond.21449.right.arg1 = load ptr, ptr %name
  %ifcond.21449.right = call i1 @strEq(ptr %ifcond.21449.right.arg0, ptr %ifcond.21449.right.arg1)
  %ifcond.21449 = and i1 %ifcond.21449.left, %ifcond.21449.right
  br i1 %ifcond.21449, label %if.then.21449, label %if.else.21449
if.then.21449:
  %ifcond.21475.arg0 = load ptr, ptr %tokens
  %ifcond.21475.arg1.left = load i32, ptr %cursor.addr.21422
  %ifcond.21475.arg1.right = add i32 0, 1
  %ifcond.21475.arg1 = add i32 %ifcond.21475.arg1.left, %ifcond.21475.arg1.right
  %ifcond.21475.arg2.char = call i8 @kindOperator()
  %ifcond.21475.arg2 = zext i8 %ifcond.21475.arg2.char to i32
  %ifcond.21475.arg3 = getelementptr inbounds [2 x i8], ptr @.str.21489, i32 0, i32 0
  %ifcond.21475 = call i1 @tokenIs(ptr %ifcond.21475.arg0, i32 %ifcond.21475.arg1, i32 %ifcond.21475.arg2, ptr %ifcond.21475.arg3)
  br i1 %ifcond.21475, label %if.then.21475, label %if.else.21475
if.then.21475:
  %ret.21493.arg0 = load ptr, ptr %tokens
  %ret.21493.arg1.left = load i32, ptr %cursor.addr.21422
  %ret.21493.arg1.right = add i32 0, 2
  %ret.21493.arg1 = add i32 %ret.21493.arg1.left, %ret.21493.arg1.right
  %ret.21493.arg2 = load i32, ptr %paramEnd.addr.21388
  %ret.21493 = call ptr @typeSummary(ptr %ret.21493.arg0, i32 %ret.21493.arg1, i32 %ret.21493.arg2)
  ret ptr %ret.21493
if.else.21475:
  br label %if.end.21475
if.end.21475:
  %ret.21506 = getelementptr inbounds [8 x i8], ptr @.str.21507, i32 0, i32 0
  ret ptr %ret.21506
if.else.21449:
  br label %if.end.21449
if.end.21449:
  br label %while.cond.21510
while.cond.21510:
  %whilecond.21510.left.i32.left = load i32, ptr %cursor.addr.21422
  %whilecond.21510.left.i32.right = load i32, ptr %paramEnd.addr.21388
  %whilecond.21510.left.i32.comparison = icmp slt i32 %whilecond.21510.left.i32.left, %whilecond.21510.left.i32.right
  %whilecond.21510.left.i32 = zext i1 %whilecond.21510.left.i32.comparison to i32
  %whilecond.21510.left = icmp ne i32 %whilecond.21510.left.i32, 0
  %whilecond.21510.right.value.arg0 = load ptr, ptr %tokens
  %whilecond.21510.right.value.arg1 = load i32, ptr %cursor.addr.21422
  %whilecond.21510.right.value.arg2.char = call i8 @kindOperator()
  %whilecond.21510.right.value.arg2 = zext i8 %whilecond.21510.right.value.arg2.char to i32
  %whilecond.21510.right.value.arg3 = getelementptr inbounds [2 x i8], ptr @.str.21527, i32 0, i32 0
  %whilecond.21510.right.value = call i1 @tokenIs(ptr %whilecond.21510.right.value.arg0, i32 %whilecond.21510.right.value.arg1, i32 %whilecond.21510.right.value.arg2, ptr %whilecond.21510.right.value.arg3)
  %whilecond.21510.right = xor i1 %whilecond.21510.right.value, true
  %whilecond.21510 = and i1 %whilecond.21510.left, %whilecond.21510.right
  br i1 %whilecond.21510, label %while.body.21510, label %while.end.21510
while.body.21510:
  %cursor.assign.21531.left = load i32, ptr %cursor.addr.21422
  %cursor.assign.21531.right = add i32 0, 1
  %cursor.assign.21531 = add i32 %cursor.assign.21531.left, %cursor.assign.21531.right
  store i32 %cursor.assign.21531, ptr %cursor.addr.21422
  br label %while.cond.21510
while.end.21510:
  %cursor.assign.21538.left = load i32, ptr %cursor.addr.21422
  %cursor.assign.21538.right = add i32 0, 1
  %cursor.assign.21538 = add i32 %cursor.assign.21538.left, %cursor.assign.21538.right
  store i32 %cursor.assign.21538, ptr %cursor.addr.21422
  br label %while.cond.21431
while.end.21431:
  %ret.21545 = getelementptr inbounds [8 x i8], ptr @.str.21546, i32 0, i32 0
  ret ptr %ret.21545
}

define ptr @lookupVisibleValueType(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name) {
entry:
  %ret = call ptr @csec_lookup_visible_value_type(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name)
  ret ptr %ret
}

define ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name) {
entry:
  %ret = call ptr @csec_lookup_visible_storage_name(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name)
  ret ptr %ret
}

define ptr @llvmLoadForValueType(ptr %arg.tokens, i32 %arg.limit, ptr %arg.name, ptr %arg.valueType, ptr %arg.resultName) {
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
define ptr @generateLLVMLoadIfIdentifier(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.22361 = alloca i32
  %end.init.22361.arg0 = load ptr, ptr %tokens
  %end.init.22361.arg1 = load i32, ptr %start
  %end.init.22361.arg2 = load i32, ptr %rawEnd
  %end.init.22361 = call i32 @trimExpressionEnd(ptr %end.init.22361.arg0, i32 %end.init.22361.arg1, i32 %end.init.22361.arg2)
  store i32 %end.init.22361, ptr %end.addr.22361
  %ifcond.22375.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22375.left.i32.left.char.arg0 = trunc i32 %ifcond.22375.left.i32.left.char.arg0.i32 to i8
  %ifcond.22375.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22375.left.i32.left.char.arg1 = trunc i32 %ifcond.22375.left.i32.left.char.arg1.i32 to i8
  %ifcond.22375.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22375.left.i32.left.char.arg0, i8 %ifcond.22375.left.i32.left.char.arg1)
  %ifcond.22375.left.i32.left = zext i8 %ifcond.22375.left.i32.left.char to i32
  %ifcond.22375.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.22375.left.i32.right = zext i8 %ifcond.22375.left.i32.right.char to i32
  %ifcond.22375.left.i32.comparison = icmp eq i32 %ifcond.22375.left.i32.left, %ifcond.22375.left.i32.right
  %ifcond.22375.left.i32 = zext i1 %ifcond.22375.left.i32.comparison to i32
  %ifcond.22375.left = icmp ne i32 %ifcond.22375.left.i32, 0
  %ifcond.22375.right.i32.left = load i32, ptr %end.addr.22361
  %ifcond.22375.right.i32.right.left = load i32, ptr %start
  %ifcond.22375.right.i32.right.right = add i32 0, 1
  %ifcond.22375.right.i32.right = add i32 %ifcond.22375.right.i32.right.left, %ifcond.22375.right.i32.right.right
  %ifcond.22375.right.i32.comparison = icmp eq i32 %ifcond.22375.right.i32.left, %ifcond.22375.right.i32.right
  %ifcond.22375.right.i32 = zext i1 %ifcond.22375.right.i32.comparison to i32
  %ifcond.22375.right = icmp ne i32 %ifcond.22375.right.i32, 0
  %ifcond.22375 = and i1 %ifcond.22375.left, %ifcond.22375.right
  br i1 %ifcond.22375, label %if.then.22375, label %if.else.22375
if.then.22375:
  %name.addr.22395 = alloca ptr
  %name.init.22395.arg0 = load ptr, ptr %tokens
  %name.init.22395.arg1 = load i32, ptr %start
  %name.init.22395 = call ptr @tokenTextAt(ptr %name.init.22395.arg0, i32 %name.init.22395.arg1)
  store ptr %name.init.22395, ptr %name.addr.22395
  %storageName.addr.22407 = alloca ptr
  %storageName.init.22407.arg0 = load ptr, ptr %tokens
  %storageName.init.22407.arg1 = load i32, ptr %start
  %storageName.init.22407.arg2 = load ptr, ptr %name.addr.22395
  %storageName.init.22407 = call ptr @lookupVisibleStorageName(ptr %storageName.init.22407.arg0, i32 %storageName.init.22407.arg1, ptr %storageName.init.22407.arg2)
  store ptr %storageName.init.22407, ptr %storageName.addr.22407
  %valueType.addr.22421 = alloca ptr
  %valueType.init.22421.arg0 = load ptr, ptr %tokens
  %valueType.init.22421.arg1 = load i32, ptr %start
  %valueType.init.22421.arg2 = load ptr, ptr %name.addr.22395
  %valueType.init.22421 = call ptr @lookupVisibleValueType(ptr %valueType.init.22421.arg0, i32 %valueType.init.22421.arg1, ptr %valueType.init.22421.arg2)
  store ptr %valueType.init.22421, ptr %valueType.addr.22421
  %ifcond.22435.arg0 = load ptr, ptr %valueType.addr.22421
  %ifcond.22435.arg1 = getelementptr inbounds [5 x i8], ptr @.str.22441, i32 0, i32 0
  %ifcond.22435 = call i1 @strEq(ptr %ifcond.22435.arg0, ptr %ifcond.22435.arg1)
  br i1 %ifcond.22435, label %if.then.22435, label %if.else.22435
if.then.22435:
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.22446, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %name.addr.22395
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [7 x i8], ptr @.str.22450, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %start
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right.i64)
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [18 x i8], ptr @.str.22454, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %storageName.addr.22407
  %ret.22445.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [2 x i8], ptr @.str.22458, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.left.right = getelementptr inbounds [4 x i8], ptr @.str.22460, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.left.right = load ptr, ptr %name.addr.22395
  %ret.22445.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.left.right = getelementptr inbounds [6 x i8], ptr @.str.22464, i32 0, i32 0
  %ret.22445.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.left.right.i32 = load i32, ptr %start
  %ret.22445.left.left.left.left.left.right.i64 = sext i32 %ret.22445.left.left.left.left.left.right.i32 to i64
  %ret.22445.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.22445.left.left.left.left.left.right.i64)
  %ret.22445.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left.left, ptr %ret.22445.left.left.left.left.left.right)
  %ret.22445.left.left.left.left.right = getelementptr inbounds [13 x i8], ptr @.str.22468, i32 0, i32 0
  %ret.22445.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left.left, ptr %ret.22445.left.left.left.left.right)
  %ret.22445.left.left.left.right = load ptr, ptr %name.addr.22395
  %ret.22445.left.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left.left, ptr %ret.22445.left.left.left.right)
  %ret.22445.left.left.right = getelementptr inbounds [7 x i8], ptr @.str.22472, i32 0, i32 0
  %ret.22445.left.left = call ptr @csec_string_concat(ptr %ret.22445.left.left.left, ptr %ret.22445.left.left.right)
  %ret.22445.left.right.i32 = load i32, ptr %start
  %ret.22445.left.right.i64 = sext i32 %ret.22445.left.right.i32 to i64
  %ret.22445.left.right = call ptr @csec_to_string_i64(i64 %ret.22445.left.right.i64)
  %ret.22445.left = call ptr @csec_string_concat(ptr %ret.22445.left.left, ptr %ret.22445.left.right)
  %ret.22445.right = getelementptr inbounds [9 x i8], ptr @.str.22476, i32 0, i32 0
  %ret.22445 = call ptr @csec_string_concat(ptr %ret.22445.left, ptr %ret.22445.right)
  ret ptr %ret.22445
if.else.22435:
  br label %if.end.22435
if.end.22435:
  %ret.22479.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.22480, i32 0, i32 0
  %ret.22479.left.left.left.left.left.right = load ptr, ptr %name.addr.22395
  %ret.22479.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22479.left.left.left.left.left.left, ptr %ret.22479.left.left.left.left.left.right)
  %ret.22479.left.left.left.left.right = getelementptr inbounds [6 x i8], ptr @.str.22484, i32 0, i32 0
  %ret.22479.left.left.left.left = call ptr @csec_string_concat(ptr %ret.22479.left.left.left.left.left, ptr %ret.22479.left.left.left.left.right)
  %ret.22479.left.left.left.right.i32 = load i32, ptr %start
  %ret.22479.left.left.left.right.i64 = sext i32 %ret.22479.left.left.left.right.i32 to i64
  %ret.22479.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.22479.left.left.left.right.i64)
  %ret.22479.left.left.left = call ptr @csec_string_concat(ptr %ret.22479.left.left.left.left, ptr %ret.22479.left.left.left.right)
  %ret.22479.left.left.right = getelementptr inbounds [19 x i8], ptr @.str.22488, i32 0, i32 0
  %ret.22479.left.left = call ptr @csec_string_concat(ptr %ret.22479.left.left.left, ptr %ret.22479.left.left.right)
  %ret.22479.left.right = load ptr, ptr %storageName.addr.22407
  %ret.22479.left = call ptr @csec_string_concat(ptr %ret.22479.left.left, ptr %ret.22479.left.right)
  %ret.22479.right = getelementptr inbounds [2 x i8], ptr @.str.22492, i32 0, i32 0
  %ret.22479 = call ptr @csec_string_concat(ptr %ret.22479.left, ptr %ret.22479.right)
  ret ptr %ret.22479
if.else.22375:
  br label %if.end.22375
if.end.22375:
  %ret.22495 = getelementptr inbounds [1 x i8], ptr @.str.22496, i32 0, i32 0
  ret ptr %ret.22495
}

define ptr @llvmI1Value(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.22517 = alloca i32
  %end.init.22517.arg0 = load ptr, ptr %tokens
  %end.init.22517.arg1 = load i32, ptr %start
  %end.init.22517.arg2 = load i32, ptr %rawEnd
  %end.init.22517 = call i32 @trimExpressionEnd(ptr %end.init.22517.arg0, i32 %end.init.22517.arg1, i32 %end.init.22517.arg2)
  store i32 %end.init.22517, ptr %end.addr.22517
  %ifcond.22531.i32.left = load i32, ptr %end.addr.22517
  %ifcond.22531.i32.right = load i32, ptr %start
  %ifcond.22531.i32.comparison = icmp sle i32 %ifcond.22531.i32.left, %ifcond.22531.i32.right
  %ifcond.22531.i32 = zext i1 %ifcond.22531.i32.comparison to i32
  %ifcond.22531 = icmp ne i32 %ifcond.22531.i32, 0
  br i1 %ifcond.22531, label %if.then.22531, label %if.else.22531
if.then.22531:
  %ret.22538 = getelementptr inbounds [6 x i8], ptr @.str.22539, i32 0, i32 0
  ret ptr %ret.22538
if.else.22531:
  br label %if.end.22531
if.end.22531:
  %ifcond.22542.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22542.left.i32.left.char.arg0 = trunc i32 %ifcond.22542.left.i32.left.char.arg0.i32 to i8
  %ifcond.22542.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22542.left.i32.left.char.arg1 = trunc i32 %ifcond.22542.left.i32.left.char.arg1.i32 to i8
  %ifcond.22542.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22542.left.i32.left.char.arg0, i8 %ifcond.22542.left.i32.left.char.arg1)
  %ifcond.22542.left.i32.left = zext i8 %ifcond.22542.left.i32.left.char to i32
  %ifcond.22542.left.i32.right.char = call i8 @kindBool()
  %ifcond.22542.left.i32.right = zext i8 %ifcond.22542.left.i32.right.char to i32
  %ifcond.22542.left.i32.comparison = icmp eq i32 %ifcond.22542.left.i32.left, %ifcond.22542.left.i32.right
  %ifcond.22542.left.i32 = zext i1 %ifcond.22542.left.i32.comparison to i32
  %ifcond.22542.left = icmp ne i32 %ifcond.22542.left.i32, 0
  %ifcond.22542.right.i32.left = load i32, ptr %end.addr.22517
  %ifcond.22542.right.i32.right.left = load i32, ptr %start
  %ifcond.22542.right.i32.right.right = add i32 0, 1
  %ifcond.22542.right.i32.right = add i32 %ifcond.22542.right.i32.right.left, %ifcond.22542.right.i32.right.right
  %ifcond.22542.right.i32.comparison = icmp eq i32 %ifcond.22542.right.i32.left, %ifcond.22542.right.i32.right
  %ifcond.22542.right.i32 = zext i1 %ifcond.22542.right.i32.comparison to i32
  %ifcond.22542.right = icmp ne i32 %ifcond.22542.right.i32, 0
  %ifcond.22542 = and i1 %ifcond.22542.left, %ifcond.22542.right
  br i1 %ifcond.22542, label %if.then.22542, label %if.else.22542
if.then.22542:
  %ret.22562.arg0 = load ptr, ptr %tokens
  %ret.22562.arg1 = load i32, ptr %start
  %ret.22562 = call ptr @tokenTextAt(ptr %ret.22562.arg0, i32 %ret.22562.arg1)
  ret ptr %ret.22562
if.else.22542:
  br label %if.end.22542
if.end.22542:
  %ifcond.22571.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22571.left.i32.left.char.arg0 = trunc i32 %ifcond.22571.left.i32.left.char.arg0.i32 to i8
  %ifcond.22571.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22571.left.i32.left.char.arg1 = trunc i32 %ifcond.22571.left.i32.left.char.arg1.i32 to i8
  %ifcond.22571.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22571.left.i32.left.char.arg0, i8 %ifcond.22571.left.i32.left.char.arg1)
  %ifcond.22571.left.i32.left = zext i8 %ifcond.22571.left.i32.left.char to i32
  %ifcond.22571.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.22571.left.i32.right = zext i8 %ifcond.22571.left.i32.right.char to i32
  %ifcond.22571.left.i32.comparison = icmp eq i32 %ifcond.22571.left.i32.left, %ifcond.22571.left.i32.right
  %ifcond.22571.left.i32 = zext i1 %ifcond.22571.left.i32.comparison to i32
  %ifcond.22571.left = icmp ne i32 %ifcond.22571.left.i32, 0
  %ifcond.22571.right.i32.left = load i32, ptr %end.addr.22517
  %ifcond.22571.right.i32.right.left = load i32, ptr %start
  %ifcond.22571.right.i32.right.right = add i32 0, 1
  %ifcond.22571.right.i32.right = add i32 %ifcond.22571.right.i32.right.left, %ifcond.22571.right.i32.right.right
  %ifcond.22571.right.i32.comparison = icmp eq i32 %ifcond.22571.right.i32.left, %ifcond.22571.right.i32.right
  %ifcond.22571.right.i32 = zext i1 %ifcond.22571.right.i32.comparison to i32
  %ifcond.22571.right = icmp ne i32 %ifcond.22571.right.i32, 0
  %ifcond.22571 = and i1 %ifcond.22571.left, %ifcond.22571.right
  br i1 %ifcond.22571, label %if.then.22571, label %if.else.22571
if.then.22571:
  %ret.22591.left.left = getelementptr inbounds [2 x i8], ptr @.str.22592, i32 0, i32 0
  %ret.22591.left.right.arg0 = load ptr, ptr %tokens
  %ret.22591.left.right.arg1 = load i32, ptr %start
  %ret.22591.left.right = call ptr @tokenTextAt(ptr %ret.22591.left.right.arg0, i32 %ret.22591.left.right.arg1)
  %ret.22591.left = call ptr @csec_string_concat(ptr %ret.22591.left.left, ptr %ret.22591.left.right)
  %ret.22591.right = getelementptr inbounds [6 x i8], ptr @.str.22601, i32 0, i32 0
  %ret.22591 = call ptr @csec_string_concat(ptr %ret.22591.left, ptr %ret.22591.right)
  ret ptr %ret.22591
if.else.22571:
  br label %if.end.22571
if.end.22571:
  %ret.22604 = getelementptr inbounds [6 x i8], ptr @.str.22605, i32 0, i32 0
  ret ptr %ret.22604
}

define ptr @generateLLVMLoadBoolIfIdentifier(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.22626 = alloca i32
  %end.init.22626.arg0 = load ptr, ptr %tokens
  %end.init.22626.arg1 = load i32, ptr %start
  %end.init.22626.arg2 = load i32, ptr %rawEnd
  %end.init.22626 = call i32 @trimExpressionEnd(ptr %end.init.22626.arg0, i32 %end.init.22626.arg1, i32 %end.init.22626.arg2)
  store i32 %end.init.22626, ptr %end.addr.22626
  %ifcond.22640.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22640.left.i32.left.char.arg0 = trunc i32 %ifcond.22640.left.i32.left.char.arg0.i32 to i8
  %ifcond.22640.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22640.left.i32.left.char.arg1 = trunc i32 %ifcond.22640.left.i32.left.char.arg1.i32 to i8
  %ifcond.22640.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22640.left.i32.left.char.arg0, i8 %ifcond.22640.left.i32.left.char.arg1)
  %ifcond.22640.left.i32.left = zext i8 %ifcond.22640.left.i32.left.char to i32
  %ifcond.22640.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.22640.left.i32.right = zext i8 %ifcond.22640.left.i32.right.char to i32
  %ifcond.22640.left.i32.comparison = icmp eq i32 %ifcond.22640.left.i32.left, %ifcond.22640.left.i32.right
  %ifcond.22640.left.i32 = zext i1 %ifcond.22640.left.i32.comparison to i32
  %ifcond.22640.left = icmp ne i32 %ifcond.22640.left.i32, 0
  %ifcond.22640.right.i32.left = load i32, ptr %end.addr.22626
  %ifcond.22640.right.i32.right.left = load i32, ptr %start
  %ifcond.22640.right.i32.right.right = add i32 0, 1
  %ifcond.22640.right.i32.right = add i32 %ifcond.22640.right.i32.right.left, %ifcond.22640.right.i32.right.right
  %ifcond.22640.right.i32.comparison = icmp eq i32 %ifcond.22640.right.i32.left, %ifcond.22640.right.i32.right
  %ifcond.22640.right.i32 = zext i1 %ifcond.22640.right.i32.comparison to i32
  %ifcond.22640.right = icmp ne i32 %ifcond.22640.right.i32, 0
  %ifcond.22640 = and i1 %ifcond.22640.left, %ifcond.22640.right
  br i1 %ifcond.22640, label %if.then.22640, label %if.else.22640
if.then.22640:
  %name.addr.22660 = alloca ptr
  %name.init.22660.arg0 = load ptr, ptr %tokens
  %name.init.22660.arg1 = load i32, ptr %start
  %name.init.22660 = call ptr @tokenTextAt(ptr %name.init.22660.arg0, i32 %name.init.22660.arg1)
  store ptr %name.init.22660, ptr %name.addr.22660
  %storageName.addr.22672 = alloca ptr
  %storageName.init.22672.arg0 = load ptr, ptr %tokens
  %storageName.init.22672.arg1 = load i32, ptr %start
  %storageName.init.22672.arg2 = load ptr, ptr %name.addr.22660
  %storageName.init.22672 = call ptr @lookupVisibleStorageName(ptr %storageName.init.22672.arg0, i32 %storageName.init.22672.arg1, ptr %storageName.init.22672.arg2)
  store ptr %storageName.init.22672, ptr %storageName.addr.22672
  %ret.22686.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.22687, i32 0, i32 0
  %ret.22686.left.left.left.right = load ptr, ptr %name.addr.22660
  %ret.22686.left.left.left = call ptr @csec_string_concat(ptr %ret.22686.left.left.left.left, ptr %ret.22686.left.left.left.right)
  %ret.22686.left.left.right = getelementptr inbounds [23 x i8], ptr @.str.22691, i32 0, i32 0
  %ret.22686.left.left = call ptr @csec_string_concat(ptr %ret.22686.left.left.left, ptr %ret.22686.left.left.right)
  %ret.22686.left.right = load ptr, ptr %storageName.addr.22672
  %ret.22686.left = call ptr @csec_string_concat(ptr %ret.22686.left.left, ptr %ret.22686.left.right)
  %ret.22686.right = getelementptr inbounds [2 x i8], ptr @.str.22695, i32 0, i32 0
  %ret.22686 = call ptr @csec_string_concat(ptr %ret.22686.left, ptr %ret.22686.right)
  ret ptr %ret.22686
if.else.22640:
  br label %if.end.22640
if.end.22640:
  %ret.22698 = getelementptr inbounds [1 x i8], ptr @.str.22699, i32 0, i32 0
  ret ptr %ret.22698
}

define ptr @llvmI32CallArgumentValue(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName, i32 %arg.index) {
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
define ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.22808 = alloca i32
  %end.init.22808.arg0 = load ptr, ptr %tokens
  %end.init.22808.arg1 = load i32, ptr %start
  %end.init.22808.arg2 = load i32, ptr %rawEnd
  %end.init.22808 = call i32 @trimExpressionEnd(ptr %end.init.22808.arg0, i32 %end.init.22808.arg1, i32 %end.init.22808.arg2)
  store i32 %end.init.22808, ptr %end.addr.22808
  %ifcond.22822.i32.left = load i32, ptr %end.addr.22808
  %ifcond.22822.i32.right = load i32, ptr %start
  %ifcond.22822.i32.comparison = icmp sle i32 %ifcond.22822.i32.left, %ifcond.22822.i32.right
  %ifcond.22822.i32 = zext i1 %ifcond.22822.i32.comparison to i32
  %ifcond.22822 = icmp ne i32 %ifcond.22822.i32, 0
  br i1 %ifcond.22822, label %if.then.22822, label %if.else.22822
if.then.22822:
  %ret.22829 = getelementptr inbounds [4 x i8], ptr @.str.22830, i32 0, i32 0
  ret ptr %ret.22829
if.else.22822:
  br label %if.end.22822
if.end.22822:
  %ifcond.22833.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22833.left.i32.left.char.arg0 = trunc i32 %ifcond.22833.left.i32.left.char.arg0.i32 to i8
  %ifcond.22833.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22833.left.i32.left.char.arg1 = trunc i32 %ifcond.22833.left.i32.left.char.arg1.i32 to i8
  %ifcond.22833.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22833.left.i32.left.char.arg0, i8 %ifcond.22833.left.i32.left.char.arg1)
  %ifcond.22833.left.i32.left = zext i8 %ifcond.22833.left.i32.left.char to i32
  %ifcond.22833.left.i32.right.char = call i8 @kindString()
  %ifcond.22833.left.i32.right = zext i8 %ifcond.22833.left.i32.right.char to i32
  %ifcond.22833.left.i32.comparison = icmp eq i32 %ifcond.22833.left.i32.left, %ifcond.22833.left.i32.right
  %ifcond.22833.left.i32 = zext i1 %ifcond.22833.left.i32.comparison to i32
  %ifcond.22833.left = icmp ne i32 %ifcond.22833.left.i32, 0
  %ifcond.22833.right.i32.left = load i32, ptr %end.addr.22808
  %ifcond.22833.right.i32.right.left = load i32, ptr %start
  %ifcond.22833.right.i32.right.right = add i32 0, 1
  %ifcond.22833.right.i32.right = add i32 %ifcond.22833.right.i32.right.left, %ifcond.22833.right.i32.right.right
  %ifcond.22833.right.i32.comparison = icmp eq i32 %ifcond.22833.right.i32.left, %ifcond.22833.right.i32.right
  %ifcond.22833.right.i32 = zext i1 %ifcond.22833.right.i32.comparison to i32
  %ifcond.22833.right = icmp ne i32 %ifcond.22833.right.i32, 0
  %ifcond.22833 = and i1 %ifcond.22833.left, %ifcond.22833.right
  br i1 %ifcond.22833, label %if.then.22833, label %if.else.22833
if.then.22833:
  %ret.22853 = getelementptr inbounds [4 x i8], ptr @.str.22854, i32 0, i32 0
  ret ptr %ret.22853
if.else.22833:
  br label %if.end.22833
if.end.22833:
  %ifcond.22857.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22857.left.i32.left.char.arg0 = trunc i32 %ifcond.22857.left.i32.left.char.arg0.i32 to i8
  %ifcond.22857.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22857.left.i32.left.char.arg1 = trunc i32 %ifcond.22857.left.i32.left.char.arg1.i32 to i8
  %ifcond.22857.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22857.left.i32.left.char.arg0, i8 %ifcond.22857.left.i32.left.char.arg1)
  %ifcond.22857.left.i32.left = zext i8 %ifcond.22857.left.i32.left.char to i32
  %ifcond.22857.left.i32.right.char = call i8 @kindChar()
  %ifcond.22857.left.i32.right = zext i8 %ifcond.22857.left.i32.right.char to i32
  %ifcond.22857.left.i32.comparison = icmp eq i32 %ifcond.22857.left.i32.left, %ifcond.22857.left.i32.right
  %ifcond.22857.left.i32 = zext i1 %ifcond.22857.left.i32.comparison to i32
  %ifcond.22857.left = icmp ne i32 %ifcond.22857.left.i32, 0
  %ifcond.22857.right.i32.left = load i32, ptr %end.addr.22808
  %ifcond.22857.right.i32.right.left = load i32, ptr %start
  %ifcond.22857.right.i32.right.right = add i32 0, 1
  %ifcond.22857.right.i32.right = add i32 %ifcond.22857.right.i32.right.left, %ifcond.22857.right.i32.right.right
  %ifcond.22857.right.i32.comparison = icmp eq i32 %ifcond.22857.right.i32.left, %ifcond.22857.right.i32.right
  %ifcond.22857.right.i32 = zext i1 %ifcond.22857.right.i32.comparison to i32
  %ifcond.22857.right = icmp ne i32 %ifcond.22857.right.i32, 0
  %ifcond.22857 = and i1 %ifcond.22857.left, %ifcond.22857.right
  br i1 %ifcond.22857, label %if.then.22857, label %if.else.22857
if.then.22857:
  %ret.22877 = getelementptr inbounds [3 x i8], ptr @.str.22878, i32 0, i32 0
  ret ptr %ret.22877
if.else.22857:
  br label %if.end.22857
if.end.22857:
  %ifcond.22881.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22881.left.i32.left.char.arg0 = trunc i32 %ifcond.22881.left.i32.left.char.arg0.i32 to i8
  %ifcond.22881.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22881.left.i32.left.char.arg1 = trunc i32 %ifcond.22881.left.i32.left.char.arg1.i32 to i8
  %ifcond.22881.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22881.left.i32.left.char.arg0, i8 %ifcond.22881.left.i32.left.char.arg1)
  %ifcond.22881.left.i32.left = zext i8 %ifcond.22881.left.i32.left.char to i32
  %ifcond.22881.left.i32.right.char = call i8 @kindBool()
  %ifcond.22881.left.i32.right = zext i8 %ifcond.22881.left.i32.right.char to i32
  %ifcond.22881.left.i32.comparison = icmp eq i32 %ifcond.22881.left.i32.left, %ifcond.22881.left.i32.right
  %ifcond.22881.left.i32 = zext i1 %ifcond.22881.left.i32.comparison to i32
  %ifcond.22881.left = icmp ne i32 %ifcond.22881.left.i32, 0
  %ifcond.22881.right.i32.left = load i32, ptr %end.addr.22808
  %ifcond.22881.right.i32.right.left = load i32, ptr %start
  %ifcond.22881.right.i32.right.right = add i32 0, 1
  %ifcond.22881.right.i32.right = add i32 %ifcond.22881.right.i32.right.left, %ifcond.22881.right.i32.right.right
  %ifcond.22881.right.i32.comparison = icmp eq i32 %ifcond.22881.right.i32.left, %ifcond.22881.right.i32.right
  %ifcond.22881.right.i32 = zext i1 %ifcond.22881.right.i32.comparison to i32
  %ifcond.22881.right = icmp ne i32 %ifcond.22881.right.i32, 0
  %ifcond.22881 = and i1 %ifcond.22881.left, %ifcond.22881.right
  br i1 %ifcond.22881, label %if.then.22881, label %if.else.22881
if.then.22881:
  %ret.22901 = getelementptr inbounds [3 x i8], ptr @.str.22902, i32 0, i32 0
  ret ptr %ret.22901
if.else.22881:
  br label %if.end.22881
if.end.22881:
  %ifcond.22905.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.22905.left.i32.left.char.arg0 = trunc i32 %ifcond.22905.left.i32.left.char.arg0.i32 to i8
  %ifcond.22905.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.22905.left.i32.left.char.arg1 = trunc i32 %ifcond.22905.left.i32.left.char.arg1.i32 to i8
  %ifcond.22905.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.22905.left.i32.left.char.arg0, i8 %ifcond.22905.left.i32.left.char.arg1)
  %ifcond.22905.left.i32.left = zext i8 %ifcond.22905.left.i32.left.char to i32
  %ifcond.22905.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.22905.left.i32.right = zext i8 %ifcond.22905.left.i32.right.char to i32
  %ifcond.22905.left.i32.comparison = icmp eq i32 %ifcond.22905.left.i32.left, %ifcond.22905.left.i32.right
  %ifcond.22905.left.i32 = zext i1 %ifcond.22905.left.i32.comparison to i32
  %ifcond.22905.left = icmp ne i32 %ifcond.22905.left.i32, 0
  %ifcond.22905.right.i32.left = load i32, ptr %end.addr.22808
  %ifcond.22905.right.i32.right.left = load i32, ptr %start
  %ifcond.22905.right.i32.right.right = add i32 0, 1
  %ifcond.22905.right.i32.right = add i32 %ifcond.22905.right.i32.right.left, %ifcond.22905.right.i32.right.right
  %ifcond.22905.right.i32.comparison = icmp eq i32 %ifcond.22905.right.i32.left, %ifcond.22905.right.i32.right
  %ifcond.22905.right.i32 = zext i1 %ifcond.22905.right.i32.comparison to i32
  %ifcond.22905.right = icmp ne i32 %ifcond.22905.right.i32, 0
  %ifcond.22905 = and i1 %ifcond.22905.left, %ifcond.22905.right
  br i1 %ifcond.22905, label %if.then.22905, label %if.else.22905
if.then.22905:
  %valueType.addr.22925 = alloca ptr
  %valueType.init.22925.arg0 = load ptr, ptr %tokens
  %valueType.init.22925.arg1 = load i32, ptr %start
  %valueType.init.22925.arg2.arg0 = load ptr, ptr %tokens
  %valueType.init.22925.arg2.arg1 = load i32, ptr %start
  %valueType.init.22925.arg2 = call ptr @tokenTextAt(ptr %valueType.init.22925.arg2.arg0, i32 %valueType.init.22925.arg2.arg1)
  %valueType.init.22925 = call ptr @lookupVisibleValueType(ptr %valueType.init.22925.arg0, i32 %valueType.init.22925.arg1, ptr %valueType.init.22925.arg2)
  store ptr %valueType.init.22925, ptr %valueType.addr.22925
  %llvmType.addr.22944 = alloca ptr
  %llvmType.init.22944.arg0 = load ptr, ptr %valueType.addr.22925
  %llvmType.init.22944 = call ptr @irTypeName(ptr %llvmType.init.22944.arg0)
  store ptr %llvmType.init.22944, ptr %llvmType.addr.22944
  %ifcond.22954.left.left.left.left.arg0 = load ptr, ptr %llvmType.addr.22944
  %ifcond.22954.left.left.left.left.arg1 = getelementptr inbounds [4 x i8], ptr @.str.22960, i32 0, i32 0
  %ifcond.22954.left.left.left.left = call i1 @strEq(ptr %ifcond.22954.left.left.left.left.arg0, ptr %ifcond.22954.left.left.left.left.arg1)
  %ifcond.22954.left.left.left.right.arg0 = load ptr, ptr %llvmType.addr.22944
  %ifcond.22954.left.left.left.right.arg1 = getelementptr inbounds [3 x i8], ptr @.str.22967, i32 0, i32 0
  %ifcond.22954.left.left.left.right = call i1 @strEq(ptr %ifcond.22954.left.left.left.right.arg0, ptr %ifcond.22954.left.left.left.right.arg1)
  %ifcond.22954.left.left.left = or i1 %ifcond.22954.left.left.left.left, %ifcond.22954.left.left.left.right
  %ifcond.22954.left.left.right.arg0 = load ptr, ptr %llvmType.addr.22944
  %ifcond.22954.left.left.right.arg1 = getelementptr inbounds [3 x i8], ptr @.str.22974, i32 0, i32 0
  %ifcond.22954.left.left.right = call i1 @strEq(ptr %ifcond.22954.left.left.right.arg0, ptr %ifcond.22954.left.left.right.arg1)
  %ifcond.22954.left.left = or i1 %ifcond.22954.left.left.left, %ifcond.22954.left.left.right
  %ifcond.22954.left.right.arg0 = load ptr, ptr %llvmType.addr.22944
  %ifcond.22954.left.right.arg1 = getelementptr inbounds [4 x i8], ptr @.str.22981, i32 0, i32 0
  %ifcond.22954.left.right = call i1 @strEq(ptr %ifcond.22954.left.right.arg0, ptr %ifcond.22954.left.right.arg1)
  %ifcond.22954.left = or i1 %ifcond.22954.left.left, %ifcond.22954.left.right
  %ifcond.22954.right.arg0 = load ptr, ptr %llvmType.addr.22944
  %ifcond.22954.right.arg1 = getelementptr inbounds [7 x i8], ptr @.str.22988, i32 0, i32 0
  %ifcond.22954.right = call i1 @strEq(ptr %ifcond.22954.right.arg0, ptr %ifcond.22954.right.arg1)
  %ifcond.22954 = or i1 %ifcond.22954.left, %ifcond.22954.right
  br i1 %ifcond.22954, label %if.then.22954, label %if.else.22954
if.then.22954:
  %ret.22992 = load ptr, ptr %llvmType.addr.22944
  ret ptr %ret.22992
if.else.22954:
  br label %if.end.22954
if.end.22954:
  br label %if.end.22905
if.else.22905:
  br label %if.end.22905
if.end.22905:
  %op.addr.23114 = alloca i32
  %op.init.23114.arg0 = load ptr, ptr %tokens
  %op.init.23114.arg1 = load i32, ptr %start
  %op.init.23114.arg2 = load i32, ptr %end.addr.22808
  %op.init.23114 = call i32 @expressionTopLevelOperator(ptr %op.init.23114.arg0, i32 %op.init.23114.arg1, i32 %op.init.23114.arg2)
  store i32 %op.init.23114, ptr %op.addr.23114
  %ifcond.23128.left.i32.left = load i32, ptr %op.addr.23114
  %ifcond.23128.left.i32.right = load i32, ptr %start
  %ifcond.23128.left.i32.comparison = icmp sgt i32 %ifcond.23128.left.i32.left, %ifcond.23128.left.i32.right
  %ifcond.23128.left.i32 = zext i1 %ifcond.23128.left.i32.comparison to i32
  %ifcond.23128.left = icmp ne i32 %ifcond.23128.left.i32, 0
  %ifcond.23128.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.23128.right.i32.left.char.arg0 = trunc i32 %ifcond.23128.right.i32.left.char.arg0.i32 to i8
  %ifcond.23128.right.i32.left.char.arg1.i32 = load i32, ptr %op.addr.23114
  %ifcond.23128.right.i32.left.char.arg1 = trunc i32 %ifcond.23128.right.i32.left.char.arg1.i32 to i8
  %ifcond.23128.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.23128.right.i32.left.char.arg0, i8 %ifcond.23128.right.i32.left.char.arg1)
  %ifcond.23128.right.i32.left = zext i8 %ifcond.23128.right.i32.left.char to i32
  %ifcond.23128.right.i32.right.char = call i8 @kindOperator()
  %ifcond.23128.right.i32.right = zext i8 %ifcond.23128.right.i32.right.char to i32
  %ifcond.23128.right.i32.comparison = icmp eq i32 %ifcond.23128.right.i32.left, %ifcond.23128.right.i32.right
  %ifcond.23128.right.i32 = zext i1 %ifcond.23128.right.i32.comparison to i32
  %ifcond.23128.right = icmp ne i32 %ifcond.23128.right.i32, 0
  %ifcond.23128 = and i1 %ifcond.23128.left, %ifcond.23128.right
  br i1 %ifcond.23128, label %if.then.23128, label %if.else.23128
if.then.23128:
  %ifcond.23146.left.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.23146.left.arg0.arg1 = load i32, ptr %op.addr.23114
  %ifcond.23146.left.arg0 = call ptr @tokenTextAt(ptr %ifcond.23146.left.arg0.arg0, i32 %ifcond.23146.left.arg0.arg1)
  %ifcond.23146.left.arg1 = getelementptr inbounds [2 x i8], ptr @.str.23157, i32 0, i32 0
  %ifcond.23146.left = call i1 @strEq(ptr %ifcond.23146.left.arg0, ptr %ifcond.23146.left.arg1)
  %ifcond.23146.right.left.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.23146.right.left.arg0.arg1 = load i32, ptr %start
  %ifcond.23146.right.left.arg0.arg2 = load i32, ptr %op.addr.23114
  %ifcond.23146.right.left.arg0 = call ptr @llvmCallArgumentType(ptr %ifcond.23146.right.left.arg0.arg0, i32 %ifcond.23146.right.left.arg0.arg1, i32 %ifcond.23146.right.left.arg0.arg2)
  %ifcond.23146.right.left.arg1 = getelementptr inbounds [4 x i8], ptr @.str.23172, i32 0, i32 0
  %ifcond.23146.right.left = call i1 @strEq(ptr %ifcond.23146.right.left.arg0, ptr %ifcond.23146.right.left.arg1)
  %ifcond.23146.right.right.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.23146.right.right.arg0.arg1.left = load i32, ptr %op.addr.23114
  %ifcond.23146.right.right.arg0.arg1.right = add i32 0, 1
  %ifcond.23146.right.right.arg0.arg1 = add i32 %ifcond.23146.right.right.arg0.arg1.left, %ifcond.23146.right.right.arg0.arg1.right
  %ifcond.23146.right.right.arg0.arg2 = load i32, ptr %end.addr.22808
  %ifcond.23146.right.right.arg0 = call ptr @llvmCallArgumentType(ptr %ifcond.23146.right.right.arg0.arg0, i32 %ifcond.23146.right.right.arg0.arg1, i32 %ifcond.23146.right.right.arg0.arg2)
  %ifcond.23146.right.right.arg1 = getelementptr inbounds [4 x i8], ptr @.str.23188, i32 0, i32 0
  %ifcond.23146.right.right = call i1 @strEq(ptr %ifcond.23146.right.right.arg0, ptr %ifcond.23146.right.right.arg1)
  %ifcond.23146.right = or i1 %ifcond.23146.right.left, %ifcond.23146.right.right
  %ifcond.23146 = and i1 %ifcond.23146.left, %ifcond.23146.right
  br i1 %ifcond.23146, label %if.then.23146, label %if.else.23146
if.then.23146:
  %ret.23193 = getelementptr inbounds [4 x i8], ptr @.str.23194, i32 0, i32 0
  ret ptr %ret.23193
if.else.23146:
  br label %if.end.23146
if.end.23146:
  br label %if.end.23128
if.else.23128:
  br label %if.end.23128
if.end.23128:
  %ret.23198 = getelementptr inbounds [4 x i8], ptr @.str.23199, i32 0, i32 0
  ret ptr %ret.23198
}

define ptr @llvmCallArgumentValue(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName, i32 %arg.index) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %empty.return, label %integer.check
empty.return:
  ret ptr @.str.callarg.value.zero
integer.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %integer = and i1 %is.integer, %single
  br i1 %integer, label %integer.return, label %char.check
integer.return:
  %integer.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  ret ptr %integer.text
char.check:
  %char.kind = call i8 @kindChar()
  %is.char = icmp eq i8 %kind, %char.kind
  %char = and i1 %is.char, %single
  br i1 %char, label %char.return, label %bool.check
char.return:
  %char.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %char.value = call ptr @llvmCharI8Value(ptr %char.text)
  ret ptr %char.value
bool.check:
  %bool.kind = call i8 @kindBool()
  %is.bool = icmp eq i8 %kind, %bool.kind
  %bool = and i1 %is.bool, %single
  br i1 %bool, label %bool.return, label %temporary.return
bool.return:
  %bool.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  ret ptr %bool.text
temporary.return:
  %prefix = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callarg.value.arg)
  %index.i64 = sext i32 %arg.index to i64
  %index.text = call ptr @csec_to_string_i64(i64 %index.i64)
  %result = call ptr @csec_string_concat(ptr %prefix, ptr %index.text)
  ret ptr %result
}

@.str.callarg.value.zero = private unnamed_addr constant [2 x i8] c"0\00"
@.str.callarg.value.arg = private unnamed_addr constant [5 x i8] c".arg\00"
define ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %empty.return, label %string.check
empty.return:
  %e1 = call ptr @csec_string_concat(ptr @.str.callargload.prefix, ptr %arg.resultName)
  %e2 = call ptr @csec_string_concat(ptr %e1, ptr @.str.callargload.add.prefix)
  %empty.result = call ptr @csec_string_concat(ptr %e2, ptr @.str.callargload.zero)
  ret ptr %empty.result
string.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %string.kind = call i8 @kindString()
  %is.string = icmp eq i8 %kind, %string.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %string = and i1 %is.string, %single
  br i1 %string, label %string.return, label %integer.check
string.return:
  %text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %bytes = call i32 @csec_llvm_string_literal_byte_length(ptr %text)
  %bytes.i64 = sext i32 %bytes to i64
  %bytes.text = call ptr @csec_to_string_i64(i64 %bytes.i64)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %s1 = call ptr @csec_string_concat(ptr @.str.callargload.prefix, ptr %arg.resultName)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.callargload.gep.left)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %bytes.text)
  %s4 = call ptr @csec_string_concat(ptr %s3, ptr @.str.callargload.gep.middle)
  %s5 = call ptr @csec_string_concat(ptr %s4, ptr %start.text)
  %result = call ptr @csec_string_concat(ptr %s5, ptr @.str.callargload.gep.right)
  ret ptr %result
integer.check:
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %integer = and i1 %is.integer, %single
  br i1 %integer, label %integer.return, label %char.check
integer.return:
  %integer.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %i1 = call ptr @csec_string_concat(ptr @.str.callargload.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.callargload.add.prefix)
  %integer.result = call ptr @csec_string_concat(ptr %i2, ptr %integer.text)
  ret ptr %integer.result
char.check:
  %char.kind = call i8 @kindChar()
  %is.char = icmp eq i8 %kind, %char.kind
  %char = and i1 %is.char, %single
  br i1 %char, label %char.return, label %identifier.check
char.return:
  %char.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %char.value = call ptr @llvmCharI8Value(ptr %char.text)
  %c1 = call ptr @csec_string_concat(ptr @.str.callargload.prefix, ptr %arg.resultName)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.callargload.add.prefix)
  %char.result = call ptr @csec_string_concat(ptr %c2, ptr %char.value)
  ret ptr %char.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  %identifier = and i1 %is.identifier, %single
  br i1 %identifier, label %identifier.return, label %expression.type
identifier.return:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %value.type = call ptr @lookupVisibleValueType(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %load = call ptr @llvmLoadForValueType(ptr %arg.tokens, i32 %arg.start, ptr %name, ptr %value.type, ptr %arg.resultName)
  ret ptr %load
expression.type:
  %argument.type = call ptr @llvmCallArgumentType(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %pointer.argument = call i1 @strEq(ptr %argument.type, ptr @.str.callargload.ptr)
  br i1 %pointer.argument, label %pointer.expression.return, label %expression.return
pointer.expression.return:
  %pointer.expression = call ptr @generateLLVMExpressionPtr(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr %arg.resultName)
  ret ptr %pointer.expression
expression.return:
  %expression = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr %arg.resultName)
  ret ptr %expression
}

@.str.callargload.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.callargload.zero = private unnamed_addr constant [3 x i8] c"0\0A\00"
@.str.callargload.gep.left = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.callargload.gep.middle = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.callargload.gep.right = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.callargload.add.prefix = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.callargload.ptr = private unnamed_addr constant [4 x i8] c"ptr\00"
define ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %arg.argsStart, i32 %arg.argsEnd, ptr %arg.resultName) {
entry:
  %first = call i32 @skipTrivia(ptr %arg.tokens, i32 %arg.argsStart)
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %first, i32 %arg.argsEnd)
  %empty = icmp sge i32 %first, %end
  br i1 %empty, label %empty.return, label %last.find
empty.return:
  ret ptr @.str.callargloads.empty
last.find:
  %last = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %end, i32 12)
  %one = icmp slt i32 %last, %first
  br i1 %one, label %one.return, label %second.find
one.return:
  %name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg0)
  %one.result = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %first, i32 %end, ptr %name)
  ret ptr %one.result
second.find:
  %second.comma = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %last, i32 12)
  %two = icmp slt i32 %second.comma, %first
  br i1 %two, label %two.return, label %first.find
two.return:
  %name0 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg0)
  %left = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %first, i32 %last, ptr %name0)
  %second.start.raw = add i32 %last, 1
  %second.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %second.start.raw)
  %name1 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg1)
  %right = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %second.start, i32 %end, ptr %name1)
  %two.result = call ptr @csec_string_concat(ptr %left, ptr %right)
  ret ptr %two.result
first.find:
  %first.comma = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %first, i32 %second.comma, i32 12)
  %three = icmp slt i32 %first.comma, %first
  br i1 %three, label %three.return, label %four.return
three.return:
  %name0.3 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg0)
  %part0 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %first, i32 %second.comma, ptr %name0.3)
  %second.start.raw.3 = add i32 %second.comma, 1
  %second.start.3 = call i32 @skipTrivia(ptr %arg.tokens, i32 %second.start.raw.3)
  %name1.3 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg1)
  %part1 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %second.start.3, i32 %last, ptr %name1.3)
  %third.start.raw = add i32 %last, 1
  %third.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %third.start.raw)
  %name2.3 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg2)
  %part2 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %third.start, i32 %end, ptr %name2.3)
  %join0 = call ptr @csec_string_concat(ptr %part0, ptr %part1)
  %result3 = call ptr @csec_string_concat(ptr %join0, ptr %part2)
  ret ptr %result3
four.return:
  %name0.4 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg0)
  %part0.4 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %first, i32 %first.comma, ptr %name0.4)
  %second.start.raw.4 = add i32 %first.comma, 1
  %second.start.4 = call i32 @skipTrivia(ptr %arg.tokens, i32 %second.start.raw.4)
  %name1.4 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg1)
  %part1.4 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %second.start.4, i32 %second.comma, ptr %name1.4)
  %third.start.raw.4 = add i32 %second.comma, 1
  %third.start.4 = call i32 @skipTrivia(ptr %arg.tokens, i32 %third.start.raw.4)
  %name2.4 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg2)
  %part2.4 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %third.start.4, i32 %last, ptr %name2.4)
  %fourth.start.raw = add i32 %last, 1
  %fourth.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %fourth.start.raw)
  %name3.4 = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.callargloads.arg3)
  %part3.4 = call ptr @generateLLVMCallArgumentLoadI32(ptr %arg.tokens, i32 %fourth.start, i32 %end, ptr %name3.4)
  %join1 = call ptr @csec_string_concat(ptr %part0.4, ptr %part1.4)
  %join2 = call ptr @csec_string_concat(ptr %join1, ptr %part2.4)
  %result4 = call ptr @csec_string_concat(ptr %join2, ptr %part3.4)
  ret ptr %result4
}

@.str.callargloads.empty = private unnamed_addr constant [1 x i8] c"\00"
@.str.callargloads.arg0 = private unnamed_addr constant [6 x i8] c".arg0\00"
@.str.callargloads.arg1 = private unnamed_addr constant [6 x i8] c".arg1\00"
@.str.callargloads.arg2 = private unnamed_addr constant [6 x i8] c".arg2\00"
@.str.callargloads.arg3 = private unnamed_addr constant [6 x i8] c".arg3\00"
define ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %arg.argsStart, i32 %arg.argsEnd, ptr %arg.resultName) {
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
define ptr @llvmRuntimeCallName(ptr %arg.name) {
entry:
  %count = call i1 @strEq(ptr %arg.name, ptr @.str.runtime.name.count)
  br i1 %count, label %count.return, label %arg.check
count.return:
  ret ptr @.str.runtime.name.count.native
arg.check:
  %arg = call i1 @strEq(ptr %arg.name, ptr @.str.runtime.name.arg)
  br i1 %arg, label %arg.return, label %identity.return
arg.return:
  ret ptr @.str.runtime.name.arg.native
identity.return:
  ret ptr %arg.name
}

@.str.runtime.name.count = private unnamed_addr constant [20 x i8] c"commandLineArgCount\00"
@.str.runtime.name.count.native = private unnamed_addr constant [28 x i8] c"csec_command_line_arg_count\00"
@.str.runtime.name.arg = private unnamed_addr constant [15 x i8] c"commandLineArg\00"
@.str.runtime.name.arg.native = private unnamed_addr constant [22 x i8] c"csec_command_line_arg\00"
define i1 @llvmCallReturnsChar(ptr %arg.tokens, ptr %arg.name) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %name = alloca ptr
  store ptr %arg.name, ptr %name
  %ifcond.26214.left.string = load ptr, ptr %name
  %ifcond.26214.left.needle = getelementptr inbounds [5 x i8], ptr @.str.26220, i32 0, i32 0
  %ifcond.26214.left = call i32 @csec_string_starts_with(ptr %ifcond.26214.left.string, ptr %ifcond.26214.left.needle)
  %ifcond.26214 = icmp ne i32 %ifcond.26214.left, 0
  br i1 %ifcond.26214, label %if.then.26214, label %if.else.26214
if.then.26214:
  %ret.26224 = icmp eq i32 0, 0
  ret i1 %ret.26224
if.else.26214:
  br label %if.end.26214
if.end.26214:
  %ifcond.26228.arg0 = load ptr, ptr %name
  %ifcond.26228.arg1 = getelementptr inbounds [12 x i8], ptr @.str.26234, i32 0, i32 0
  %ifcond.26228 = call i1 @strEq(ptr %ifcond.26228.arg0, ptr %ifcond.26228.arg1)
  br i1 %ifcond.26228, label %if.then.26228, label %if.else.26228
if.then.26228:
  %ret.26238 = icmp eq i32 0, 0
  ret i1 %ret.26238
if.else.26228:
  br label %if.end.26228
if.end.26228:
  %ifcond.26242.arg0 = load ptr, ptr %name
  %ifcond.26242.arg1 = getelementptr inbounds [19 x i8], ptr @.str.26248, i32 0, i32 0
  %ifcond.26242 = call i1 @strEq(ptr %ifcond.26242.arg0, ptr %ifcond.26242.arg1)
  br i1 %ifcond.26242, label %if.then.26242, label %if.else.26242
if.then.26242:
  %ret.26252 = icmp eq i32 0, 0
  ret i1 %ret.26252
if.else.26242:
  br label %if.end.26242
if.end.26242:
  %ifcond.26256.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.26256.arg0.arg1 = load ptr, ptr %name
  %ifcond.26256.arg0 = call ptr @lookupFunctionReturnType(ptr %ifcond.26256.arg0.arg0, ptr %ifcond.26256.arg0.arg1)
  %ifcond.26256.arg1 = getelementptr inbounds [5 x i8], ptr @.str.26267, i32 0, i32 0
  %ifcond.26256 = call i1 @strEq(ptr %ifcond.26256.arg0, ptr %ifcond.26256.arg1)
  br i1 %ifcond.26256, label %if.then.26256, label %if.else.26256
if.then.26256:
  %ret.26271 = icmp eq i32 0, 0
  ret i1 %ret.26271
if.else.26256:
  br label %if.end.26256
if.end.26256:
  %ret.26275 = icmp eq i32 0, 1
  ret i1 %ret.26275
}

define ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %zero.return, label %integer.check
zero.return:
  %z1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %zero.result = call ptr @csec_string_concat(ptr %z1, ptr @.str.expr.i32.zero)
  ret ptr %zero.result
integer.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %integer = and i1 %is.integer, %single
  br i1 %integer, label %integer.return, label %char.check
integer.return:
  %integer.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %i1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.expr.i32.add)
  %integer.result = call ptr @csec_string_concat(ptr %i2, ptr %integer.text)
  ret ptr %integer.result
char.check:
  %char.kind = call i8 @kindChar()
  %is.char = icmp eq i8 %kind, %char.kind
  %char = and i1 %is.char, %single
  br i1 %char, label %char.return, label %identifier.check
char.return:
  %char.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %char.value = call ptr @llvmCharI8Value(ptr %char.text)
  %c1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.expr.i32.add)
  %char.result = call ptr @csec_string_concat(ptr %c2, ptr %char.value)
  ret ptr %char.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %operator.find
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.return, label %length.check
identifier.return:
  %value.type = call ptr @lookupVisibleValueType(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %is.char.type = call i1 @strEq(ptr %value.type, ptr @.str.expr.i32.char)
  br i1 %is.char.type, label %identifier.char, label %identifier.i32
identifier.char:
  %storage.char = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %h1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %h2 = call ptr @csec_string_concat(ptr %h1, ptr @.str.expr.i32.char.load)
  %h3 = call ptr @csec_string_concat(ptr %h2, ptr %storage.char)
  %h4 = call ptr @csec_string_concat(ptr %h3, ptr @.str.expr.i32.prefix)
  %h5 = call ptr @csec_string_concat(ptr %h4, ptr %arg.resultName)
  %h6 = call ptr @csec_string_concat(ptr %h5, ptr @.str.expr.i32.zext)
  %h7 = call ptr @csec_string_concat(ptr %h6, ptr %arg.resultName)
  %identifier.char.result = call ptr @csec_string_concat(ptr %h7, ptr @.str.expr.i32.char.to.i32)
  ret ptr %identifier.char.result
identifier.i32:
  %storage.i32 = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %j1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %j2 = call ptr @csec_string_concat(ptr %j1, ptr @.str.expr.i32.i32.load)
  %identifier.i32.result = call ptr @csec_string_concat(ptr %j2, ptr %storage.i32)
  ret ptr %identifier.i32.result
length.check:
  %dot.index = add i32 %arg.start, 1
  %length.index = add i32 %arg.start, 2
  %operator.kind = call i8 @kindOperator()
  %dot.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %dot.index, i8 %operator.kind, ptr @.str.expr.i32.dot)
  %dot = icmp ne i32 %dot.raw, 0
  %length.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %length.index, i8 %identifier.kind, ptr @.str.expr.i32.length)
  %length = icmp ne i32 %length.raw, 0
  %is.length = and i1 %dot, %length
  br i1 %is.length, label %length.return, label %charat.check
length.return:
  %length.storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %l1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %l2 = call ptr @csec_string_concat(ptr %l1, ptr @.str.expr.i32.object.load)
  %l3 = call ptr @csec_string_concat(ptr %l2, ptr %length.storage)
  %l4 = call ptr @csec_string_concat(ptr %l3, ptr @.str.expr.i32.prefix)
  %l5 = call ptr @csec_string_concat(ptr %l4, ptr %arg.resultName)
  %l6 = call ptr @csec_string_concat(ptr %l5, ptr @.str.expr.i32.length.call)
  %object.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i32.object)
  %l7 = call ptr @csec_string_concat(ptr %l6, ptr %object.name)
  %l8 = call ptr @csec_string_concat(ptr %l7, ptr @.str.expr.i32.prefix)
  %l9 = call ptr @csec_string_concat(ptr %l8, ptr %arg.resultName)
  %l10 = call ptr @csec_string_concat(ptr %l9, ptr @.str.expr.i32.trunc)
  %l11 = call ptr @csec_string_concat(ptr %l10, ptr %arg.resultName)
  %length.result = call ptr @csec_string_concat(ptr %l11, ptr @.str.expr.i32.i64.to.i32)
  ret ptr %length.result
charat.check:
  %open.index = add i32 %arg.start, 3
  %minimum.end = add i32 %arg.start, 4
  %has.charat = icmp slt i32 %minimum.end, %end
  br i1 %has.charat, label %charat.tokens, label %call.check
charat.tokens:
  %charat.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %length.index, i8 %identifier.kind, ptr @.str.expr.i32.charat)
  %charat = icmp ne i32 %charat.raw, 0
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.i32.open)
  %open = icmp ne i32 %open.raw, 0
  %charat.a = and i1 %dot, %charat
  %charat.b = and i1 %charat.a, %open
  br i1 %charat.b, label %charat.close, label %call.check
charat.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.expr.i32.open, ptr @.str.expr.i32.close)
  %last = sub i32 %end, 1
  %is.charat = icmp eq i32 %close, %last
  br i1 %is.charat, label %charat.return, label %call.check
charat.return:
  %charat.storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %index.start = add i32 %arg.start, 4
  %index.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i32.index)
  %index.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %index.start, i32 %last, ptr %index.name)
  %q1 = call ptr @csec_string_concat(ptr @.str.expr.i32.prefix, ptr %arg.resultName)
  %q2 = call ptr @csec_string_concat(ptr %q1, ptr @.str.expr.i32.object.load)
  %q3 = call ptr @csec_string_concat(ptr %q2, ptr %charat.storage)
  %q4 = call ptr @csec_string_concat(ptr %q3, ptr %index.code)
  %q5 = call ptr @csec_string_concat(ptr %q4, ptr @.str.expr.i32.prefix)
  %q6 = call ptr @csec_string_concat(ptr %q5, ptr %arg.resultName)
  %q7 = call ptr @csec_string_concat(ptr %q6, ptr @.str.expr.i32.charat.call)
  %object.name.charat = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i32.object)
  %q8 = call ptr @csec_string_concat(ptr %q7, ptr %object.name.charat)
  %q9 = call ptr @csec_string_concat(ptr %q8, ptr @.str.expr.i32.charat.middle)
  %q10 = call ptr @csec_string_concat(ptr %q9, ptr %index.name)
  %q11 = call ptr @csec_string_concat(ptr %q10, ptr @.str.expr.i32.close)
  %q12 = call ptr @csec_string_concat(ptr %q11, ptr @.str.expr.i32.prefix)
  %q13 = call ptr @csec_string_concat(ptr %q12, ptr %arg.resultName)
  %q14 = call ptr @csec_string_concat(ptr %q13, ptr @.str.expr.i32.zext)
  %q15 = call ptr @csec_string_concat(ptr %q14, ptr %arg.resultName)
  %charat.result = call ptr @csec_string_concat(ptr %q15, ptr @.str.expr.i32.i8.to.i32)
  ret ptr %charat.result
call.check:
  %call.open.index = add i32 %arg.start, 1
  %call.before.end = icmp slt i32 %call.open.index, %end
  br i1 %call.before.end, label %call.open.check, label %operator.find
call.open.check:
  %call.open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %call.open.index, i8 %operator.kind, ptr @.str.expr.i32.open)
  %call.open = icmp ne i32 %call.open.raw, 0
  br i1 %call.open, label %call.close.check, label %operator.find
call.close.check:
  %call.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %call.open.index, i32 %end, ptr @.str.expr.i32.open, ptr @.str.expr.i32.close)
  %call.last = sub i32 %end, 1
  %is.call = icmp eq i32 %call.close, %call.last
  br i1 %is.call, label %call.return, label %operator.find
call.return:
  %callee = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %args.start = add i32 %arg.start, 2
  %loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %arg.resultName)
  %arguments = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %arg.resultName)
  %native.callee = call ptr @llvmRuntimeCallName(ptr %callee)
  %r1 = call ptr @csec_string_concat(ptr %loads, ptr @.str.expr.i32.prefix)
  %r2 = call ptr @csec_string_concat(ptr %r1, ptr %arg.resultName)
  %r3 = call ptr @csec_string_concat(ptr %r2, ptr @.str.expr.i32.call.i32)
  %r4 = call ptr @csec_string_concat(ptr %r3, ptr %native.callee)
  %r5 = call ptr @csec_string_concat(ptr %r4, ptr @.str.expr.i32.open)
  %r6 = call ptr @csec_string_concat(ptr %r5, ptr %arguments)
  %call.result = call ptr @csec_string_concat(ptr %r6, ptr @.str.expr.i32.close.nl)
  ret ptr %call.result
operator.find:
  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %operator.return, label %zero.return
operator.return:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %mnemonic = call ptr @irOperatorName(ptr %op.text)
  %left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i32.left)
  %left = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arg.start, i32 %op, ptr %left.name)
  %right.start = add i32 %op, 1
  %right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i32.right)
  %right = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %right.start, i32 %end, ptr %right.name)
  %b1 = call ptr @csec_string_concat(ptr %left, ptr %right)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.expr.i32.prefix)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr %arg.resultName)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr @.str.expr.i32.operator.prefix)
  %b5 = call ptr @csec_string_concat(ptr %b4, ptr %mnemonic)
  %b6 = call ptr @csec_string_concat(ptr %b5, ptr @.str.expr.i32.operator.middle)
  %b7 = call ptr @csec_string_concat(ptr %b6, ptr %left.name)
  %b8 = call ptr @csec_string_concat(ptr %b7, ptr @.str.expr.i32.operator.comma)
  %binary.result = call ptr @csec_string_concat(ptr %b8, ptr %right.name)
  ret ptr %binary.result
}

@.str.expr.i32.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.expr.i32.zero = private unnamed_addr constant [17 x i8] c" = add i32 0, 0\0A\00"
@.str.expr.i32.add = private unnamed_addr constant [15 x i8] c" = add i32 0, \00"
@.str.expr.i32.char = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.expr.i32.char.load = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.expr.i32.zext = private unnamed_addr constant [12 x i8] c" = zext i8 \00"
@.str.expr.i32.char.to.i32 = private unnamed_addr constant [11 x i8] c".c to i32\0A\00"
@.str.expr.i32.i32.load = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.expr.i32.dot = private unnamed_addr constant [2 x i8] c".\00"
@.str.expr.i32.length = private unnamed_addr constant [7 x i8] c"length\00"
@.str.expr.i32.charat = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.expr.i32.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.expr.i32.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.expr.i32.close.nl = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.expr.i32.object = private unnamed_addr constant [5 x i8] c".obj\00"
@.str.expr.i32.index = private unnamed_addr constant [7 x i8] c".index\00"
@.str.expr.i32.object.load = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.expr.i32.length.call = private unnamed_addr constant [37 x i8] c" = call i64 @csec_string_length(ptr \00"
@.str.expr.i32.trunc = private unnamed_addr constant [14 x i8] c" = trunc i64 \00"
@.str.expr.i32.i64.to.i32 = private unnamed_addr constant [13 x i8] c".i64 to i32\0A\00"
@.str.expr.i32.charat.call = private unnamed_addr constant [37 x i8] c" = call i8 @csec_string_char_at(ptr \00"
@.str.expr.i32.charat.middle = private unnamed_addr constant [7 x i8] c", i32 \00"
@.str.expr.i32.i8.to.i32 = private unnamed_addr constant [12 x i8] c".i8 to i32\0A\00"
@.str.expr.i32.call.i32 = private unnamed_addr constant [14 x i8] c" = call i32 @\00"
@.str.expr.i32.left = private unnamed_addr constant [6 x i8] c".left\00"
@.str.expr.i32.right = private unnamed_addr constant [7 x i8] c".right\00"
@.str.expr.i32.operator.prefix = private unnamed_addr constant [4 x i8] c" = \00"
@.str.expr.i32.operator.middle = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.expr.i32.operator.comma = private unnamed_addr constant [3 x i8] c", \00"
define ptr @generateLLVMLocalI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.i32.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.i32.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.i32.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %with.initializer, label %without.initializer
with.initializer:
  %expr.start = add i32 %initializer, 1
  %expr = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %expr.start, i32 %arg.end, ptr %init)
  %a1 = call ptr @csec_string_concat(ptr @.str.local.i32.prefix, ptr %storage)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.local.i32.alloca)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %expr)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.local.i32.store)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %init)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.local.i32.to)
  %result = call ptr @csec_string_concat(ptr %a6, ptr %storage)
  ret ptr %result
without.initializer:
  %b1 = call ptr @csec_string_concat(ptr @.str.local.i32.prefix, ptr %storage)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.local.i32.alloca)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.local.i32.zero)
  %fallback = call ptr @csec_string_concat(ptr %b3, ptr %storage)
  ret ptr %fallback
}

@.str.local.i32.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.i32.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.i32.init = private unnamed_addr constant [7 x i8] c".init.\00"
@.str.local.i32.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.i32.alloca = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.local.i32.store = private unnamed_addr constant [13 x i8] c"  store i32 \00"
@.str.local.i32.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.i32.zero = private unnamed_addr constant [21 x i8] c"  store i32 0, ptr %\00"
define ptr @llvmI64Value(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.27286 = alloca i32
  %end.init.27286.arg0 = load ptr, ptr %tokens
  %end.init.27286.arg1 = load i32, ptr %start
  %end.init.27286.arg2 = load i32, ptr %rawEnd
  %end.init.27286 = call i32 @trimExpressionEnd(ptr %end.init.27286.arg0, i32 %end.init.27286.arg1, i32 %end.init.27286.arg2)
  store i32 %end.init.27286, ptr %end.addr.27286
  %ifcond.27300.i32.left = load i32, ptr %end.addr.27286
  %ifcond.27300.i32.right = load i32, ptr %start
  %ifcond.27300.i32.comparison = icmp sle i32 %ifcond.27300.i32.left, %ifcond.27300.i32.right
  %ifcond.27300.i32 = zext i1 %ifcond.27300.i32.comparison to i32
  %ifcond.27300 = icmp ne i32 %ifcond.27300.i32, 0
  br i1 %ifcond.27300, label %if.then.27300, label %if.else.27300
if.then.27300:
  %ret.27307 = getelementptr inbounds [2 x i8], ptr @.str.27308, i32 0, i32 0
  ret ptr %ret.27307
if.else.27300:
  br label %if.end.27300
if.end.27300:
  %ifcond.27311.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.27311.left.i32.left.char.arg0 = trunc i32 %ifcond.27311.left.i32.left.char.arg0.i32 to i8
  %ifcond.27311.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.27311.left.i32.left.char.arg1 = trunc i32 %ifcond.27311.left.i32.left.char.arg1.i32 to i8
  %ifcond.27311.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.27311.left.i32.left.char.arg0, i8 %ifcond.27311.left.i32.left.char.arg1)
  %ifcond.27311.left.i32.left = zext i8 %ifcond.27311.left.i32.left.char to i32
  %ifcond.27311.left.i32.right.char = call i8 @kindInteger()
  %ifcond.27311.left.i32.right = zext i8 %ifcond.27311.left.i32.right.char to i32
  %ifcond.27311.left.i32.comparison = icmp eq i32 %ifcond.27311.left.i32.left, %ifcond.27311.left.i32.right
  %ifcond.27311.left.i32 = zext i1 %ifcond.27311.left.i32.comparison to i32
  %ifcond.27311.left = icmp ne i32 %ifcond.27311.left.i32, 0
  %ifcond.27311.right.i32.left = load i32, ptr %end.addr.27286
  %ifcond.27311.right.i32.right.left = load i32, ptr %start
  %ifcond.27311.right.i32.right.right = add i32 0, 1
  %ifcond.27311.right.i32.right = add i32 %ifcond.27311.right.i32.right.left, %ifcond.27311.right.i32.right.right
  %ifcond.27311.right.i32.comparison = icmp eq i32 %ifcond.27311.right.i32.left, %ifcond.27311.right.i32.right
  %ifcond.27311.right.i32 = zext i1 %ifcond.27311.right.i32.comparison to i32
  %ifcond.27311.right = icmp ne i32 %ifcond.27311.right.i32, 0
  %ifcond.27311 = and i1 %ifcond.27311.left, %ifcond.27311.right
  br i1 %ifcond.27311, label %if.then.27311, label %if.else.27311
if.then.27311:
  %ret.27331.arg0 = load ptr, ptr %tokens
  %ret.27331.arg1 = load i32, ptr %start
  %ret.27331 = call ptr @tokenTextAt(ptr %ret.27331.arg0, i32 %ret.27331.arg1)
  ret ptr %ret.27331
if.else.27311:
  br label %if.end.27311
if.end.27311:
  %ifcond.27340.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.27340.left.i32.left.char.arg0 = trunc i32 %ifcond.27340.left.i32.left.char.arg0.i32 to i8
  %ifcond.27340.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.27340.left.i32.left.char.arg1 = trunc i32 %ifcond.27340.left.i32.left.char.arg1.i32 to i8
  %ifcond.27340.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.27340.left.i32.left.char.arg0, i8 %ifcond.27340.left.i32.left.char.arg1)
  %ifcond.27340.left.i32.left = zext i8 %ifcond.27340.left.i32.left.char to i32
  %ifcond.27340.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.27340.left.i32.right = zext i8 %ifcond.27340.left.i32.right.char to i32
  %ifcond.27340.left.i32.comparison = icmp eq i32 %ifcond.27340.left.i32.left, %ifcond.27340.left.i32.right
  %ifcond.27340.left.i32 = zext i1 %ifcond.27340.left.i32.comparison to i32
  %ifcond.27340.left = icmp ne i32 %ifcond.27340.left.i32, 0
  %ifcond.27340.right.i32.left = load i32, ptr %end.addr.27286
  %ifcond.27340.right.i32.right.left = load i32, ptr %start
  %ifcond.27340.right.i32.right.right = add i32 0, 1
  %ifcond.27340.right.i32.right = add i32 %ifcond.27340.right.i32.right.left, %ifcond.27340.right.i32.right.right
  %ifcond.27340.right.i32.comparison = icmp eq i32 %ifcond.27340.right.i32.left, %ifcond.27340.right.i32.right
  %ifcond.27340.right.i32 = zext i1 %ifcond.27340.right.i32.comparison to i32
  %ifcond.27340.right = icmp ne i32 %ifcond.27340.right.i32, 0
  %ifcond.27340 = and i1 %ifcond.27340.left, %ifcond.27340.right
  br i1 %ifcond.27340, label %if.then.27340, label %if.else.27340
if.then.27340:
  %ret.27360.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.27361, i32 0, i32 0
  %ret.27360.left.left.right.arg0 = load ptr, ptr %tokens
  %ret.27360.left.left.right.arg1 = load i32, ptr %start
  %ret.27360.left.left.right = call ptr @tokenTextAt(ptr %ret.27360.left.left.right.arg0, i32 %ret.27360.left.left.right.arg1)
  %ret.27360.left.left = call ptr @csec_string_concat(ptr %ret.27360.left.left.left, ptr %ret.27360.left.left.right)
  %ret.27360.left.right = getelementptr inbounds [7 x i8], ptr @.str.27370, i32 0, i32 0
  %ret.27360.left = call ptr @csec_string_concat(ptr %ret.27360.left.left, ptr %ret.27360.left.right)
  %ret.27360.right.i32 = load i32, ptr %start
  %ret.27360.right.i64 = sext i32 %ret.27360.right.i32 to i64
  %ret.27360.right = call ptr @csec_to_string_i64(i64 %ret.27360.right.i64)
  %ret.27360 = call ptr @csec_string_concat(ptr %ret.27360.left, ptr %ret.27360.right)
  ret ptr %ret.27360
if.else.27340:
  br label %if.end.27340
if.end.27340:
  %ret.27375 = getelementptr inbounds [6 x i8], ptr @.str.27376, i32 0, i32 0
  ret ptr %ret.27375
}

define ptr @generateLLVMLoadI64IfIdentifier(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.27397 = alloca i32
  %end.init.27397.arg0 = load ptr, ptr %tokens
  %end.init.27397.arg1 = load i32, ptr %start
  %end.init.27397.arg2 = load i32, ptr %rawEnd
  %end.init.27397 = call i32 @trimExpressionEnd(ptr %end.init.27397.arg0, i32 %end.init.27397.arg1, i32 %end.init.27397.arg2)
  store i32 %end.init.27397, ptr %end.addr.27397
  %ifcond.27411.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.27411.left.i32.left.char.arg0 = trunc i32 %ifcond.27411.left.i32.left.char.arg0.i32 to i8
  %ifcond.27411.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.27411.left.i32.left.char.arg1 = trunc i32 %ifcond.27411.left.i32.left.char.arg1.i32 to i8
  %ifcond.27411.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.27411.left.i32.left.char.arg0, i8 %ifcond.27411.left.i32.left.char.arg1)
  %ifcond.27411.left.i32.left = zext i8 %ifcond.27411.left.i32.left.char to i32
  %ifcond.27411.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.27411.left.i32.right = zext i8 %ifcond.27411.left.i32.right.char to i32
  %ifcond.27411.left.i32.comparison = icmp eq i32 %ifcond.27411.left.i32.left, %ifcond.27411.left.i32.right
  %ifcond.27411.left.i32 = zext i1 %ifcond.27411.left.i32.comparison to i32
  %ifcond.27411.left = icmp ne i32 %ifcond.27411.left.i32, 0
  %ifcond.27411.right.i32.left = load i32, ptr %end.addr.27397
  %ifcond.27411.right.i32.right.left = load i32, ptr %start
  %ifcond.27411.right.i32.right.right = add i32 0, 1
  %ifcond.27411.right.i32.right = add i32 %ifcond.27411.right.i32.right.left, %ifcond.27411.right.i32.right.right
  %ifcond.27411.right.i32.comparison = icmp eq i32 %ifcond.27411.right.i32.left, %ifcond.27411.right.i32.right
  %ifcond.27411.right.i32 = zext i1 %ifcond.27411.right.i32.comparison to i32
  %ifcond.27411.right = icmp ne i32 %ifcond.27411.right.i32, 0
  %ifcond.27411 = and i1 %ifcond.27411.left, %ifcond.27411.right
  br i1 %ifcond.27411, label %if.then.27411, label %if.else.27411
if.then.27411:
  %name.addr.27431 = alloca ptr
  %name.init.27431.arg0 = load ptr, ptr %tokens
  %name.init.27431.arg1 = load i32, ptr %start
  %name.init.27431 = call ptr @tokenTextAt(ptr %name.init.27431.arg0, i32 %name.init.27431.arg1)
  store ptr %name.init.27431, ptr %name.addr.27431
  %storageName.addr.27443 = alloca ptr
  %storageName.init.27443.arg0 = load ptr, ptr %tokens
  %storageName.init.27443.arg1 = load i32, ptr %start
  %storageName.init.27443.arg2 = load ptr, ptr %name.addr.27431
  %storageName.init.27443 = call ptr @lookupVisibleStorageName(ptr %storageName.init.27443.arg0, i32 %storageName.init.27443.arg1, ptr %storageName.init.27443.arg2)
  store ptr %storageName.init.27443, ptr %storageName.addr.27443
  %ret.27457.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.27458, i32 0, i32 0
  %ret.27457.left.left.left.left.left.right = load ptr, ptr %name.addr.27431
  %ret.27457.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.27457.left.left.left.left.left.left, ptr %ret.27457.left.left.left.left.left.right)
  %ret.27457.left.left.left.left.right = getelementptr inbounds [7 x i8], ptr @.str.27462, i32 0, i32 0
  %ret.27457.left.left.left.left = call ptr @csec_string_concat(ptr %ret.27457.left.left.left.left.left, ptr %ret.27457.left.left.left.left.right)
  %ret.27457.left.left.left.right.i32 = load i32, ptr %start
  %ret.27457.left.left.left.right.i64 = sext i32 %ret.27457.left.left.left.right.i32 to i64
  %ret.27457.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.27457.left.left.left.right.i64)
  %ret.27457.left.left.left = call ptr @csec_string_concat(ptr %ret.27457.left.left.left.left, ptr %ret.27457.left.left.left.right)
  %ret.27457.left.left.right = getelementptr inbounds [19 x i8], ptr @.str.27466, i32 0, i32 0
  %ret.27457.left.left = call ptr @csec_string_concat(ptr %ret.27457.left.left.left, ptr %ret.27457.left.left.right)
  %ret.27457.left.right = load ptr, ptr %storageName.addr.27443
  %ret.27457.left = call ptr @csec_string_concat(ptr %ret.27457.left.left, ptr %ret.27457.left.right)
  %ret.27457.right = getelementptr inbounds [2 x i8], ptr @.str.27470, i32 0, i32 0
  %ret.27457 = call ptr @csec_string_concat(ptr %ret.27457.left, ptr %ret.27457.right)
  ret ptr %ret.27457
if.else.27411:
  br label %if.end.27411
if.end.27411:
  %ret.27473 = getelementptr inbounds [1 x i8], ptr @.str.27474, i32 0, i32 0
  ret ptr %ret.27473
}

define ptr @generateLLVMExpressionI64(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %zero.return, label %integer.check
zero.return:
  %z1 = call ptr @csec_string_concat(ptr @.str.expr.i64.prefix, ptr %arg.resultName)
  %zero.result = call ptr @csec_string_concat(ptr %z1, ptr @.str.expr.i64.zero)
  ret ptr %zero.result
integer.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %integer = and i1 %is.integer, %single
  br i1 %integer, label %integer.return, label %identifier.check
integer.return:
  %integer.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %i1 = call ptr @csec_string_concat(ptr @.str.expr.i64.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.expr.i64.add)
  %integer.result = call ptr @csec_string_concat(ptr %i2, ptr %integer.text)
  ret ptr %integer.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %operator.find
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.return, label %call.check
identifier.return:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %n1 = call ptr @csec_string_concat(ptr @.str.expr.i64.prefix, ptr %arg.resultName)
  %n2 = call ptr @csec_string_concat(ptr %n1, ptr @.str.expr.i64.load)
  %identifier.result = call ptr @csec_string_concat(ptr %n2, ptr %storage)
  ret ptr %identifier.result
call.check:
  %open.index = add i32 %arg.start, 1
  %before.end = icmp slt i32 %open.index, %end
  br i1 %before.end, label %open.check, label %operator.find
open.check:
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.i64.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %close.check, label %operator.find
close.check:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.expr.i64.open, ptr @.str.expr.i64.close)
  %last = sub i32 %end, 1
  %is.call = icmp eq i32 %close, %last
  br i1 %is.call, label %call.return, label %operator.find
call.return:
  %args.start = add i32 %arg.start, 2
  %loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %arguments = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %callee = call ptr @llvmRuntimeCallName(ptr %name)
  %c1 = call ptr @csec_string_concat(ptr %loads, ptr @.str.expr.i64.prefix)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr %arg.resultName)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr @.str.expr.i64.call)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr %callee)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr @.str.expr.i64.open)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr %arguments)
  %call.result = call ptr @csec_string_concat(ptr %c6, ptr @.str.expr.i64.close.nl)
  ret ptr %call.result
operator.find:
  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %operator.return, label %zero.return
operator.return:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %mnemonic = call ptr @irOperatorName(ptr %op.text)
  %left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i64.left)
  %left = call ptr @generateLLVMExpressionI64(ptr %arg.tokens, i32 %arg.start, i32 %op, ptr %left.name)
  %right.start = add i32 %op, 1
  %right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i64.right)
  %right = call ptr @generateLLVMExpressionI64(ptr %arg.tokens, i32 %right.start, i32 %end, ptr %right.name)
  %b1 = call ptr @csec_string_concat(ptr %left, ptr %right)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.expr.i64.prefix)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr %arg.resultName)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr @.str.expr.i64.operator.prefix)
  %b5 = call ptr @csec_string_concat(ptr %b4, ptr %mnemonic)
  %b6 = call ptr @csec_string_concat(ptr %b5, ptr @.str.expr.i64.operator.middle)
  %b7 = call ptr @csec_string_concat(ptr %b6, ptr %left.name)
  %b8 = call ptr @csec_string_concat(ptr %b7, ptr @.str.expr.i64.operator.comma)
  %binary.result = call ptr @csec_string_concat(ptr %b8, ptr %right.name)
  ret ptr %binary.result
}

@.str.expr.i64.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.expr.i64.zero = private unnamed_addr constant [17 x i8] c" = add i64 0, 0\0A\00"
@.str.expr.i64.add = private unnamed_addr constant [15 x i8] c" = add i64 0, \00"
@.str.expr.i64.load = private unnamed_addr constant [19 x i8] c" = load i64, ptr %\00"
@.str.expr.i64.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.expr.i64.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.expr.i64.close.nl = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.expr.i64.call = private unnamed_addr constant [14 x i8] c" = call i64 @\00"
@.str.expr.i64.left = private unnamed_addr constant [6 x i8] c".left\00"
@.str.expr.i64.right = private unnamed_addr constant [7 x i8] c".right\00"
@.str.expr.i64.operator.prefix = private unnamed_addr constant [4 x i8] c" = \00"
@.str.expr.i64.operator.middle = private unnamed_addr constant [6 x i8] c" i64 \00"
@.str.expr.i64.operator.comma = private unnamed_addr constant [3 x i8] c", \00"
define ptr @generateLLVMLocalI64(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.i64.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.i64.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.i64.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %with.initializer, label %without.initializer
with.initializer:
  %expr.start = add i32 %initializer, 1
  %expr = call ptr @generateLLVMExpressionI64(ptr %arg.tokens, i32 %expr.start, i32 %arg.end, ptr %init)
  %a1 = call ptr @csec_string_concat(ptr @.str.local.i64.prefix, ptr %storage)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.local.i64.alloca)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %expr)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.local.i64.store)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %init)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.local.i64.to)
  %result = call ptr @csec_string_concat(ptr %a6, ptr %storage)
  ret ptr %result
without.initializer:
  %b1 = call ptr @csec_string_concat(ptr @.str.local.i64.prefix, ptr %storage)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.local.i64.alloca)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.local.i64.zero)
  %fallback = call ptr @csec_string_concat(ptr %b3, ptr %storage)
  ret ptr %fallback
}

@.str.local.i64.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.i64.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.i64.init = private unnamed_addr constant [8 x i8] c".linit.\00"
@.str.local.i64.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.i64.alloca = private unnamed_addr constant [15 x i8] c" = alloca i64\0A\00"
@.str.local.i64.store = private unnamed_addr constant [13 x i8] c"  store i64 \00"
@.str.local.i64.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.i64.zero = private unnamed_addr constant [21 x i8] c"  store i64 0, ptr %\00"
define ptr @llvmF64Value(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.28042 = alloca i32
  %end.init.28042.arg0 = load ptr, ptr %tokens
  %end.init.28042.arg1 = load i32, ptr %start
  %end.init.28042.arg2 = load i32, ptr %rawEnd
  %end.init.28042 = call i32 @trimExpressionEnd(ptr %end.init.28042.arg0, i32 %end.init.28042.arg1, i32 %end.init.28042.arg2)
  store i32 %end.init.28042, ptr %end.addr.28042
  %ifcond.28056.i32.left = load i32, ptr %end.addr.28042
  %ifcond.28056.i32.right = load i32, ptr %start
  %ifcond.28056.i32.comparison = icmp sle i32 %ifcond.28056.i32.left, %ifcond.28056.i32.right
  %ifcond.28056.i32 = zext i1 %ifcond.28056.i32.comparison to i32
  %ifcond.28056 = icmp ne i32 %ifcond.28056.i32, 0
  br i1 %ifcond.28056, label %if.then.28056, label %if.else.28056
if.then.28056:
  %ret.28063 = getelementptr inbounds [13 x i8], ptr @.str.28064, i32 0, i32 0
  ret ptr %ret.28063
if.else.28056:
  br label %if.end.28056
if.end.28056:
  %ifcond.28067.left.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.28067.left.left.i32.left.char.arg0 = trunc i32 %ifcond.28067.left.left.i32.left.char.arg0.i32 to i8
  %ifcond.28067.left.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.28067.left.left.i32.left.char.arg1 = trunc i32 %ifcond.28067.left.left.i32.left.char.arg1.i32 to i8
  %ifcond.28067.left.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.28067.left.left.i32.left.char.arg0, i8 %ifcond.28067.left.left.i32.left.char.arg1)
  %ifcond.28067.left.left.i32.left = zext i8 %ifcond.28067.left.left.i32.left.char to i32
  %ifcond.28067.left.left.i32.right.char = call i8 @kindFloat()
  %ifcond.28067.left.left.i32.right = zext i8 %ifcond.28067.left.left.i32.right.char to i32
  %ifcond.28067.left.left.i32.comparison = icmp eq i32 %ifcond.28067.left.left.i32.left, %ifcond.28067.left.left.i32.right
  %ifcond.28067.left.left.i32 = zext i1 %ifcond.28067.left.left.i32.comparison to i32
  %ifcond.28067.left.left = icmp ne i32 %ifcond.28067.left.left.i32, 0
  %ifcond.28067.left.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.28067.left.right.i32.left.char.arg0 = trunc i32 %ifcond.28067.left.right.i32.left.char.arg0.i32 to i8
  %ifcond.28067.left.right.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.28067.left.right.i32.left.char.arg1 = trunc i32 %ifcond.28067.left.right.i32.left.char.arg1.i32 to i8
  %ifcond.28067.left.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.28067.left.right.i32.left.char.arg0, i8 %ifcond.28067.left.right.i32.left.char.arg1)
  %ifcond.28067.left.right.i32.left = zext i8 %ifcond.28067.left.right.i32.left.char to i32
  %ifcond.28067.left.right.i32.right.char = call i8 @kindInteger()
  %ifcond.28067.left.right.i32.right = zext i8 %ifcond.28067.left.right.i32.right.char to i32
  %ifcond.28067.left.right.i32.comparison = icmp eq i32 %ifcond.28067.left.right.i32.left, %ifcond.28067.left.right.i32.right
  %ifcond.28067.left.right.i32 = zext i1 %ifcond.28067.left.right.i32.comparison to i32
  %ifcond.28067.left.right = icmp ne i32 %ifcond.28067.left.right.i32, 0
  %ifcond.28067.left = or i1 %ifcond.28067.left.left, %ifcond.28067.left.right
  %ifcond.28067.right.i32.left = load i32, ptr %end.addr.28042
  %ifcond.28067.right.i32.right.left = load i32, ptr %start
  %ifcond.28067.right.i32.right.right = add i32 0, 1
  %ifcond.28067.right.i32.right = add i32 %ifcond.28067.right.i32.right.left, %ifcond.28067.right.i32.right.right
  %ifcond.28067.right.i32.comparison = icmp eq i32 %ifcond.28067.right.i32.left, %ifcond.28067.right.i32.right
  %ifcond.28067.right.i32 = zext i1 %ifcond.28067.right.i32.comparison to i32
  %ifcond.28067.right = icmp ne i32 %ifcond.28067.right.i32, 0
  %ifcond.28067 = and i1 %ifcond.28067.left, %ifcond.28067.right
  br i1 %ifcond.28067, label %if.then.28067, label %if.else.28067
if.then.28067:
  %ret.28100.arg0 = load ptr, ptr %tokens
  %ret.28100.arg1 = load i32, ptr %start
  %ret.28100 = call ptr @tokenTextAt(ptr %ret.28100.arg0, i32 %ret.28100.arg1)
  ret ptr %ret.28100
if.else.28067:
  br label %if.end.28067
if.end.28067:
  %ifcond.28109.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.28109.left.i32.left.char.arg0 = trunc i32 %ifcond.28109.left.i32.left.char.arg0.i32 to i8
  %ifcond.28109.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.28109.left.i32.left.char.arg1 = trunc i32 %ifcond.28109.left.i32.left.char.arg1.i32 to i8
  %ifcond.28109.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.28109.left.i32.left.char.arg0, i8 %ifcond.28109.left.i32.left.char.arg1)
  %ifcond.28109.left.i32.left = zext i8 %ifcond.28109.left.i32.left.char to i32
  %ifcond.28109.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.28109.left.i32.right = zext i8 %ifcond.28109.left.i32.right.char to i32
  %ifcond.28109.left.i32.comparison = icmp eq i32 %ifcond.28109.left.i32.left, %ifcond.28109.left.i32.right
  %ifcond.28109.left.i32 = zext i1 %ifcond.28109.left.i32.comparison to i32
  %ifcond.28109.left = icmp ne i32 %ifcond.28109.left.i32, 0
  %ifcond.28109.right.i32.left = load i32, ptr %end.addr.28042
  %ifcond.28109.right.i32.right.left = load i32, ptr %start
  %ifcond.28109.right.i32.right.right = add i32 0, 1
  %ifcond.28109.right.i32.right = add i32 %ifcond.28109.right.i32.right.left, %ifcond.28109.right.i32.right.right
  %ifcond.28109.right.i32.comparison = icmp eq i32 %ifcond.28109.right.i32.left, %ifcond.28109.right.i32.right
  %ifcond.28109.right.i32 = zext i1 %ifcond.28109.right.i32.comparison to i32
  %ifcond.28109.right = icmp ne i32 %ifcond.28109.right.i32, 0
  %ifcond.28109 = and i1 %ifcond.28109.left, %ifcond.28109.right
  br i1 %ifcond.28109, label %if.then.28109, label %if.else.28109
if.then.28109:
  %ret.28129.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.28130, i32 0, i32 0
  %ret.28129.left.left.right.arg0 = load ptr, ptr %tokens
  %ret.28129.left.left.right.arg1 = load i32, ptr %start
  %ret.28129.left.left.right = call ptr @tokenTextAt(ptr %ret.28129.left.left.right.arg0, i32 %ret.28129.left.left.right.arg1)
  %ret.28129.left.left = call ptr @csec_string_concat(ptr %ret.28129.left.left.left, ptr %ret.28129.left.left.right)
  %ret.28129.left.right = getelementptr inbounds [7 x i8], ptr @.str.28139, i32 0, i32 0
  %ret.28129.left = call ptr @csec_string_concat(ptr %ret.28129.left.left, ptr %ret.28129.left.right)
  %ret.28129.right.i32 = load i32, ptr %start
  %ret.28129.right.i64 = sext i32 %ret.28129.right.i32 to i64
  %ret.28129.right = call ptr @csec_to_string_i64(i64 %ret.28129.right.i64)
  %ret.28129 = call ptr @csec_string_concat(ptr %ret.28129.left, ptr %ret.28129.right)
  ret ptr %ret.28129
if.else.28109:
  br label %if.end.28109
if.end.28109:
  %ret.28144 = getelementptr inbounds [6 x i8], ptr @.str.28145, i32 0, i32 0
  ret ptr %ret.28144
}

define ptr @generateLLVMLoadF64IfIdentifier(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.28166 = alloca i32
  %end.init.28166.arg0 = load ptr, ptr %tokens
  %end.init.28166.arg1 = load i32, ptr %start
  %end.init.28166.arg2 = load i32, ptr %rawEnd
  %end.init.28166 = call i32 @trimExpressionEnd(ptr %end.init.28166.arg0, i32 %end.init.28166.arg1, i32 %end.init.28166.arg2)
  store i32 %end.init.28166, ptr %end.addr.28166
  %ifcond.28180.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.28180.left.i32.left.char.arg0 = trunc i32 %ifcond.28180.left.i32.left.char.arg0.i32 to i8
  %ifcond.28180.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.28180.left.i32.left.char.arg1 = trunc i32 %ifcond.28180.left.i32.left.char.arg1.i32 to i8
  %ifcond.28180.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.28180.left.i32.left.char.arg0, i8 %ifcond.28180.left.i32.left.char.arg1)
  %ifcond.28180.left.i32.left = zext i8 %ifcond.28180.left.i32.left.char to i32
  %ifcond.28180.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.28180.left.i32.right = zext i8 %ifcond.28180.left.i32.right.char to i32
  %ifcond.28180.left.i32.comparison = icmp eq i32 %ifcond.28180.left.i32.left, %ifcond.28180.left.i32.right
  %ifcond.28180.left.i32 = zext i1 %ifcond.28180.left.i32.comparison to i32
  %ifcond.28180.left = icmp ne i32 %ifcond.28180.left.i32, 0
  %ifcond.28180.right.i32.left = load i32, ptr %end.addr.28166
  %ifcond.28180.right.i32.right.left = load i32, ptr %start
  %ifcond.28180.right.i32.right.right = add i32 0, 1
  %ifcond.28180.right.i32.right = add i32 %ifcond.28180.right.i32.right.left, %ifcond.28180.right.i32.right.right
  %ifcond.28180.right.i32.comparison = icmp eq i32 %ifcond.28180.right.i32.left, %ifcond.28180.right.i32.right
  %ifcond.28180.right.i32 = zext i1 %ifcond.28180.right.i32.comparison to i32
  %ifcond.28180.right = icmp ne i32 %ifcond.28180.right.i32, 0
  %ifcond.28180 = and i1 %ifcond.28180.left, %ifcond.28180.right
  br i1 %ifcond.28180, label %if.then.28180, label %if.else.28180
if.then.28180:
  %name.addr.28200 = alloca ptr
  %name.init.28200.arg0 = load ptr, ptr %tokens
  %name.init.28200.arg1 = load i32, ptr %start
  %name.init.28200 = call ptr @tokenTextAt(ptr %name.init.28200.arg0, i32 %name.init.28200.arg1)
  store ptr %name.init.28200, ptr %name.addr.28200
  %storageName.addr.28212 = alloca ptr
  %storageName.init.28212.arg0 = load ptr, ptr %tokens
  %storageName.init.28212.arg1 = load i32, ptr %start
  %storageName.init.28212.arg2 = load ptr, ptr %name.addr.28200
  %storageName.init.28212 = call ptr @lookupVisibleStorageName(ptr %storageName.init.28212.arg0, i32 %storageName.init.28212.arg1, ptr %storageName.init.28212.arg2)
  store ptr %storageName.init.28212, ptr %storageName.addr.28212
  %ret.28226.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.28227, i32 0, i32 0
  %ret.28226.left.left.left.left.left.right = load ptr, ptr %name.addr.28200
  %ret.28226.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.28226.left.left.left.left.left.left, ptr %ret.28226.left.left.left.left.left.right)
  %ret.28226.left.left.left.left.right = getelementptr inbounds [7 x i8], ptr @.str.28231, i32 0, i32 0
  %ret.28226.left.left.left.left = call ptr @csec_string_concat(ptr %ret.28226.left.left.left.left.left, ptr %ret.28226.left.left.left.left.right)
  %ret.28226.left.left.left.right.i32 = load i32, ptr %start
  %ret.28226.left.left.left.right.i64 = sext i32 %ret.28226.left.left.left.right.i32 to i64
  %ret.28226.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.28226.left.left.left.right.i64)
  %ret.28226.left.left.left = call ptr @csec_string_concat(ptr %ret.28226.left.left.left.left, ptr %ret.28226.left.left.left.right)
  %ret.28226.left.left.right = getelementptr inbounds [22 x i8], ptr @.str.28235, i32 0, i32 0
  %ret.28226.left.left = call ptr @csec_string_concat(ptr %ret.28226.left.left.left, ptr %ret.28226.left.left.right)
  %ret.28226.left.right = load ptr, ptr %storageName.addr.28212
  %ret.28226.left = call ptr @csec_string_concat(ptr %ret.28226.left.left, ptr %ret.28226.left.right)
  %ret.28226.right = getelementptr inbounds [2 x i8], ptr @.str.28239, i32 0, i32 0
  %ret.28226 = call ptr @csec_string_concat(ptr %ret.28226.left, ptr %ret.28226.right)
  ret ptr %ret.28226
if.else.28180:
  br label %if.end.28180
if.end.28180:
  %ret.28242 = getelementptr inbounds [1 x i8], ptr @.str.28243, i32 0, i32 0
  ret ptr %ret.28242
}

define ptr @generateLLVMExpressionF64(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %zero.return, label %literal.check
zero.return:
  %z1 = call ptr @csec_string_concat(ptr @.str.expr.f64.prefix, ptr %arg.resultName)
  %zero.result = call ptr @csec_string_concat(ptr %z1, ptr @.str.expr.f64.zero)
  ret ptr %zero.result
literal.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %float.kind = call i8 @kindFloat()
  %is.float = icmp eq i8 %kind, %float.kind
  %integer.kind = call i8 @kindInteger()
  %is.integer = icmp eq i8 %kind, %integer.kind
  %number = or i1 %is.float, %is.integer
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %literal = and i1 %number, %single
  br i1 %literal, label %literal.return, label %identifier.check
literal.return:
  %text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %l1 = call ptr @csec_string_concat(ptr @.str.expr.f64.prefix, ptr %arg.resultName)
  %l2 = call ptr @csec_string_concat(ptr %l1, ptr @.str.expr.f64.fadd)
  %literal.result = call ptr @csec_string_concat(ptr %l2, ptr %text)
  ret ptr %literal.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %operator.find
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.return, label %call.check
identifier.return:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %i1 = call ptr @csec_string_concat(ptr @.str.expr.f64.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.expr.f64.load)
  %identifier.result = call ptr @csec_string_concat(ptr %i2, ptr %storage)
  ret ptr %identifier.result
call.check:
  %open.index = add i32 %arg.start, 1
  %before.end = icmp slt i32 %open.index, %end
  br i1 %before.end, label %open.check, label %operator.find
open.check:
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.f64.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %close.check, label %operator.find
close.check:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.expr.f64.open, ptr @.str.expr.f64.close)
  %last = sub i32 %end, 1
  %is.call = icmp eq i32 %close, %last
  br i1 %is.call, label %call.return, label %operator.find
call.return:
  %args.start = add i32 %arg.start, 2
  %loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %arguments = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %callee = call ptr @llvmRuntimeCallName(ptr %name)
  %c1 = call ptr @csec_string_concat(ptr %loads, ptr @.str.expr.f64.prefix)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr %arg.resultName)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr @.str.expr.f64.call)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr %callee)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr @.str.expr.f64.open)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr %arguments)
  %call.result = call ptr @csec_string_concat(ptr %c6, ptr @.str.expr.f64.close.nl)
  ret ptr %call.result
operator.find:
  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %operator.check, label %zero.return
operator.check:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %add = call i1 @strEq(ptr %op.text, ptr @.str.expr.f64.add.symbol)
  br i1 %add, label %binary.add, label %sub.check
sub.check:
  %sub = call i1 @strEq(ptr %op.text, ptr @.str.expr.f64.sub.symbol)
  br i1 %sub, label %binary.sub, label %mul.check
mul.check:
  %mul = call i1 @strEq(ptr %op.text, ptr @.str.expr.f64.mul.symbol)
  br i1 %mul, label %binary.mul, label %div.check
div.check:
  %div = call i1 @strEq(ptr %op.text, ptr @.str.expr.f64.div.symbol)
  br i1 %div, label %binary.div, label %zero.return
binary.add:
  br label %binary
binary.sub:
  br label %binary
binary.mul:
  br label %binary
binary.div:
  br label %binary
binary:
  %mnemonic = phi ptr [ @.str.expr.f64.fadd.name, %binary.add ], [ @.str.expr.f64.fsub.name, %binary.sub ], [ @.str.expr.f64.fmul.name, %binary.mul ], [ @.str.expr.f64.fdiv.name, %binary.div ]
  %left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.f64.left)
  %left = call ptr @generateLLVMExpressionF64(ptr %arg.tokens, i32 %arg.start, i32 %op, ptr %left.name)
  %right.start = add i32 %op, 1
  %right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.f64.right)
  %right = call ptr @generateLLVMExpressionF64(ptr %arg.tokens, i32 %right.start, i32 %end, ptr %right.name)
  %b1 = call ptr @csec_string_concat(ptr %left, ptr %right)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.expr.f64.prefix)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr %arg.resultName)
  %b4 = call ptr @csec_string_concat(ptr %b3, ptr @.str.expr.f64.operator.prefix)
  %b5 = call ptr @csec_string_concat(ptr %b4, ptr %mnemonic)
  %b6 = call ptr @csec_string_concat(ptr %b5, ptr @.str.expr.f64.operator.middle)
  %b7 = call ptr @csec_string_concat(ptr %b6, ptr %left.name)
  %b8 = call ptr @csec_string_concat(ptr %b7, ptr @.str.expr.f64.operator.comma)
  %binary.result = call ptr @csec_string_concat(ptr %b8, ptr %right.name)
  ret ptr %binary.result
}

@.str.expr.f64.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.expr.f64.zero = private unnamed_addr constant [43 x i8] c" = fadd double 0.000000e+00, 0.000000e+00\0A\00"
@.str.expr.f64.fadd = private unnamed_addr constant [30 x i8] c" = fadd double 0.000000e+00, \00"
@.str.expr.f64.load = private unnamed_addr constant [22 x i8] c" = load double, ptr %\00"
@.str.expr.f64.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.expr.f64.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.expr.f64.close.nl = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.expr.f64.call = private unnamed_addr constant [17 x i8] c" = call double @\00"
@.str.expr.f64.add.symbol = private unnamed_addr constant [2 x i8] c"+\00"
@.str.expr.f64.sub.symbol = private unnamed_addr constant [2 x i8] c"-\00"
@.str.expr.f64.mul.symbol = private unnamed_addr constant [2 x i8] c"*\00"
@.str.expr.f64.div.symbol = private unnamed_addr constant [2 x i8] c"/\00"
@.str.expr.f64.fadd.name = private unnamed_addr constant [5 x i8] c"fadd\00"
@.str.expr.f64.fsub.name = private unnamed_addr constant [5 x i8] c"fsub\00"
@.str.expr.f64.fmul.name = private unnamed_addr constant [5 x i8] c"fmul\00"
@.str.expr.f64.fdiv.name = private unnamed_addr constant [5 x i8] c"fdiv\00"
@.str.expr.f64.left = private unnamed_addr constant [6 x i8] c".left\00"
@.str.expr.f64.right = private unnamed_addr constant [7 x i8] c".right\00"
@.str.expr.f64.operator.prefix = private unnamed_addr constant [4 x i8] c" = \00"
@.str.expr.f64.operator.middle = private unnamed_addr constant [9 x i8] c" double \00"
@.str.expr.f64.operator.comma = private unnamed_addr constant [3 x i8] c", \00"
define ptr @generateLLVMLocalF64(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.f64.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.f64.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.f64.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %with.initializer, label %without.initializer
with.initializer:
  %expr.start = add i32 %initializer, 1
  %expr = call ptr @generateLLVMExpressionF64(ptr %arg.tokens, i32 %expr.start, i32 %arg.end, ptr %init)
  %a1 = call ptr @csec_string_concat(ptr @.str.local.f64.prefix, ptr %storage)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.local.f64.alloca)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %expr)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.local.f64.store)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %init)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.local.f64.to)
  %result = call ptr @csec_string_concat(ptr %a6, ptr %storage)
  ret ptr %result
without.initializer:
  %b1 = call ptr @csec_string_concat(ptr @.str.local.f64.prefix, ptr %storage)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.local.f64.alloca)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.local.f64.zero)
  %fallback = call ptr @csec_string_concat(ptr %b3, ptr %storage)
  ret ptr %fallback
}

@.str.local.f64.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.f64.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.f64.init = private unnamed_addr constant [8 x i8] c".finit.\00"
@.str.local.f64.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.f64.alloca = private unnamed_addr constant [18 x i8] c" = alloca double\0A\00"
@.str.local.f64.store = private unnamed_addr constant [16 x i8] c"  store double \00"
@.str.local.f64.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.f64.zero = private unnamed_addr constant [35 x i8] c"  store double 0.000000e+00, ptr %\00"
define ptr @generateLLVMLocalI8(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.i8.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.i8.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.i8.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %inspect.initializer, label %fallback
inspect.initializer:
  %expr.start.raw = add i32 %initializer, 1
  %expr.start = call i32 @skipTrivia(ptr %arg.tokens, i32 %expr.start.raw)
  %expr.end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %expr.start, i32 %arg.end)
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %expr.start)
  %char.kind = call i8 @kindChar()
  %is.char = icmp eq i8 %kind, %char.kind
  %single.end = add i32 %expr.start, 1
  %is.single = icmp eq i32 %expr.end, %single.end
  %char.literal = and i1 %is.char, %is.single
  br i1 %char.literal, label %char.return, label %identifier.check
char.return:
  %char.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %expr.start)
  %char.value = call ptr @llvmCharI8Value(ptr %char.text)
  %c1 = call ptr @csec_string_concat(ptr @.str.local.i8.prefix, ptr %storage)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.local.i8.alloca)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr @.str.local.i8.store)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr %char.value)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr @.str.local.i8.to)
  %char.result = call ptr @csec_string_concat(ptr %c5, ptr %storage)
  ret ptr %char.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.single.check, label %fallback
identifier.single.check:
  br i1 %is.single, label %identifier.return, label %charat.check
identifier.return:
  %source.name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %expr.start)
  %source.storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %expr.start, ptr %source.name)
  %i1 = call ptr @csec_string_concat(ptr @.str.local.i8.prefix, ptr %storage)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.local.i8.alloca)
  %i3 = call ptr @csec_string_concat(ptr %i2, ptr @.str.local.i8.prefix)
  %i4 = call ptr @csec_string_concat(ptr %i3, ptr %init)
  %i5 = call ptr @csec_string_concat(ptr %i4, ptr @.str.local.i8.load)
  %i6 = call ptr @csec_string_concat(ptr %i5, ptr %source.storage)
  %i7 = call ptr @csec_string_concat(ptr %i6, ptr @.str.local.i8.store)
  %i8 = call ptr @csec_string_concat(ptr %i7, ptr %init)
  %i9 = call ptr @csec_string_concat(ptr %i8, ptr @.str.local.i8.to)
  %identifier.result = call ptr @csec_string_concat(ptr %i9, ptr %storage)
  ret ptr %identifier.result
charat.check:
  %dot.index = add i32 %expr.start, 1
  %method.index = add i32 %expr.start, 2
  %open.index = add i32 %expr.start, 3
  %minimum.end = add i32 %expr.start, 4
  %has.charat = icmp slt i32 %minimum.end, %expr.end
  br i1 %has.charat, label %charat.tokens, label %call.check
charat.tokens:
  %operator.kind = call i8 @kindOperator()
  %dot.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %dot.index, i8 %operator.kind, ptr @.str.local.i8.dot)
  %dot = icmp ne i32 %dot.raw, 0
  %method.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %method.index, i8 %identifier.kind, ptr @.str.local.i8.charat)
  %method = icmp ne i32 %method.raw, 0
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.local.i8.open)
  %open = icmp ne i32 %open.raw, 0
  %charat.a = and i1 %dot, %method
  %charat.b = and i1 %charat.a, %open
  br i1 %charat.b, label %charat.close, label %call.check
charat.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %expr.end, ptr @.str.local.i8.open, ptr @.str.local.i8.close)
  %last = sub i32 %expr.end, 1
  %is.charat = icmp eq i32 %close, %last
  br i1 %is.charat, label %charat.return, label %call.check
charat.return:
  %object.name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %expr.start)
  %object.storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %expr.start, ptr %object.name)
  %index.start = add i32 %expr.start, 4
  %index.a = call ptr @csec_string_concat(ptr %init, ptr @.str.local.i8.index)
  %index.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %index.start, i32 %last, ptr %index.a)
  %o1 = call ptr @csec_string_concat(ptr @.str.local.i8.prefix, ptr %storage)
  %o2 = call ptr @csec_string_concat(ptr %o1, ptr @.str.local.i8.alloca)
  %o3 = call ptr @csec_string_concat(ptr %o2, ptr @.str.local.i8.prefix)
  %o4 = call ptr @csec_string_concat(ptr %o3, ptr %init)
  %o5 = call ptr @csec_string_concat(ptr %o4, ptr @.str.local.i8.object.load)
  %o6 = call ptr @csec_string_concat(ptr %o5, ptr %object.storage)
  %o7 = call ptr @csec_string_concat(ptr %o6, ptr %index.code)
  %o8 = call ptr @csec_string_concat(ptr %o7, ptr @.str.local.i8.prefix)
  %o9 = call ptr @csec_string_concat(ptr %o8, ptr %init)
  %o10 = call ptr @csec_string_concat(ptr %o9, ptr @.str.local.i8.call.prefix)
  %object.value = call ptr @csec_string_concat(ptr %init, ptr @.str.local.i8.object)
  %o11 = call ptr @csec_string_concat(ptr %o10, ptr %object.value)
  %o12 = call ptr @csec_string_concat(ptr %o11, ptr @.str.local.i8.call.middle)
  %o13 = call ptr @csec_string_concat(ptr %o12, ptr %index.a)
  %o14 = call ptr @csec_string_concat(ptr %o13, ptr @.str.local.i8.close)
  %o15 = call ptr @csec_string_concat(ptr %o14, ptr @.str.local.i8.store)
  %o16 = call ptr @csec_string_concat(ptr %o15, ptr %init)
  %o17 = call ptr @csec_string_concat(ptr %o16, ptr @.str.local.i8.to)
  %charat.result = call ptr @csec_string_concat(ptr %o17, ptr %storage)
  ret ptr %charat.result
call.check:
  %call.open.index = add i32 %expr.start, 1
  %call.before.end = icmp slt i32 %call.open.index, %expr.end
  br i1 %call.before.end, label %call.open.check, label %fallback
call.open.check:
  %operator.kind.call = call i8 @kindOperator()
  %call.open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %call.open.index, i8 %operator.kind.call, ptr @.str.local.i8.open)
  %call.open = icmp ne i32 %call.open.raw, 0
  br i1 %call.open, label %call.close.check, label %fallback
call.close.check:
  %call.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %call.open.index, i32 %expr.end, ptr @.str.local.i8.open, ptr @.str.local.i8.close)
  %call.last = sub i32 %expr.end, 1
  %is.call = icmp eq i32 %call.close, %call.last
  br i1 %is.call, label %call.return, label %fallback
call.return:
  %callee = call ptr @tokenTextAt(ptr %arg.tokens, i32 %expr.start)
  %args.start = add i32 %expr.start, 2
  %args.loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %init)
  %args.list = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %init)
  %callee.native = call ptr @llvmRuntimeCallName(ptr %callee)
  %q1 = call ptr @csec_string_concat(ptr @.str.local.i8.prefix, ptr %storage)
  %q2 = call ptr @csec_string_concat(ptr %q1, ptr @.str.local.i8.alloca)
  %q3 = call ptr @csec_string_concat(ptr %q2, ptr %args.loads)
  %q4 = call ptr @csec_string_concat(ptr %q3, ptr @.str.local.i8.prefix)
  %q5 = call ptr @csec_string_concat(ptr %q4, ptr %init)
  %q6 = call ptr @csec_string_concat(ptr %q5, ptr @.str.local.i8.call.at)
  %q7 = call ptr @csec_string_concat(ptr %q6, ptr %callee.native)
  %q8 = call ptr @csec_string_concat(ptr %q7, ptr @.str.local.i8.open)
  %q9 = call ptr @csec_string_concat(ptr %q8, ptr %args.list)
  %q10 = call ptr @csec_string_concat(ptr %q9, ptr @.str.local.i8.close)
  %q11 = call ptr @csec_string_concat(ptr %q10, ptr @.str.local.i8.store)
  %q12 = call ptr @csec_string_concat(ptr %q11, ptr %init)
  %q13 = call ptr @csec_string_concat(ptr %q12, ptr @.str.local.i8.to)
  %call.result = call ptr @csec_string_concat(ptr %q13, ptr %storage)
  ret ptr %call.result
fallback:
  %f1 = call ptr @csec_string_concat(ptr @.str.local.i8.prefix, ptr %storage)
  %f2 = call ptr @csec_string_concat(ptr %f1, ptr @.str.local.i8.alloca)
  %f3 = call ptr @csec_string_concat(ptr %f2, ptr @.str.local.i8.zero)
  %fallback.result = call ptr @csec_string_concat(ptr %f3, ptr %storage)
  ret ptr %fallback.result
}

@.str.local.i8.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.i8.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.i8.init = private unnamed_addr constant [8 x i8] c".cinit.\00"
@.str.local.i8.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.i8.alloca = private unnamed_addr constant [14 x i8] c" = alloca i8\0A\00"
@.str.local.i8.store = private unnamed_addr constant [12 x i8] c"  store i8 \00"
@.str.local.i8.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.i8.zero = private unnamed_addr constant [20 x i8] c"  store i8 0, ptr %\00"
@.str.local.i8.load = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.local.i8.dot = private unnamed_addr constant [2 x i8] c".\00"
@.str.local.i8.charat = private unnamed_addr constant [7 x i8] c"charAt\00"
@.str.local.i8.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.local.i8.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.local.i8.index = private unnamed_addr constant [7 x i8] c".index\00"
@.str.local.i8.object = private unnamed_addr constant [5 x i8] c".obj\00"
@.str.local.i8.object.load = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.local.i8.call.prefix = private unnamed_addr constant [37 x i8] c" = call i8 @csec_string_char_at(ptr \00"
@.str.local.i8.call.middle = private unnamed_addr constant [7 x i8] c", i32 \00"
@.str.local.i8.call.at = private unnamed_addr constant [13 x i8] c" = call i8 @\00"
define ptr @generateLLVMAssignmentF64(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %end = alloca i32
  store i32 %arg.end, ptr %end
  %op.addr.29343 = alloca i32
  %op.init.29343.arg0 = load ptr, ptr %tokens
  %op.init.29343.arg1 = load i32, ptr %start
  %op.init.29343.arg2 = load i32, ptr %end
  %op.init.29343.arg3 = add i32 0, 1
  %op.init.29343 = call i32 @findTopLevelOperator(ptr %op.init.29343.arg0, i32 %op.init.29343.arg1, i32 %op.init.29343.arg2, i32 %op.init.29343.arg3)
  store i32 %op.init.29343, ptr %op.addr.29343
  %ifcond.29359.left.i32.left = load i32, ptr %op.addr.29343
  %ifcond.29359.left.i32.right = load i32, ptr %start
  %ifcond.29359.left.i32.comparison = icmp sle i32 %ifcond.29359.left.i32.left, %ifcond.29359.left.i32.right
  %ifcond.29359.left.i32 = zext i1 %ifcond.29359.left.i32.comparison to i32
  %ifcond.29359.left = icmp ne i32 %ifcond.29359.left.i32, 0
  %ifcond.29359.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29359.right.i32.left.char.arg0 = trunc i32 %ifcond.29359.right.i32.left.char.arg0.i32 to i8
  %ifcond.29359.right.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.29359.right.i32.left.char.arg1 = trunc i32 %ifcond.29359.right.i32.left.char.arg1.i32 to i8
  %ifcond.29359.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29359.right.i32.left.char.arg0, i8 %ifcond.29359.right.i32.left.char.arg1)
  %ifcond.29359.right.i32.left = zext i8 %ifcond.29359.right.i32.left.char to i32
  %ifcond.29359.right.i32.right.char = call i8 @kindIdentifier()
  %ifcond.29359.right.i32.right = zext i8 %ifcond.29359.right.i32.right.char to i32
  %ifcond.29359.right.i32.comparison = icmp ne i32 %ifcond.29359.right.i32.left, %ifcond.29359.right.i32.right
  %ifcond.29359.right.i32 = zext i1 %ifcond.29359.right.i32.comparison to i32
  %ifcond.29359.right = icmp ne i32 %ifcond.29359.right.i32, 0
  %ifcond.29359 = or i1 %ifcond.29359.left, %ifcond.29359.right
  br i1 %ifcond.29359, label %if.then.29359, label %if.else.29359
if.then.29359:
  %ret.29377 = getelementptr inbounds [1 x i8], ptr @.str.29378, i32 0, i32 0
  ret ptr %ret.29377
if.else.29359:
  br label %if.end.29359
if.end.29359:
  %name.addr.29381 = alloca ptr
  %name.init.29381.arg0 = load ptr, ptr %tokens
  %name.init.29381.arg1 = load i32, ptr %start
  %name.init.29381 = call ptr @tokenTextAt(ptr %name.init.29381.arg0, i32 %name.init.29381.arg1)
  store ptr %name.init.29381, ptr %name.addr.29381
  %storageName.addr.29393 = alloca ptr
  %storageName.init.29393.arg0 = load ptr, ptr %tokens
  %storageName.init.29393.arg1 = load i32, ptr %start
  %storageName.init.29393.arg2 = load ptr, ptr %name.addr.29381
  %storageName.init.29393 = call ptr @lookupVisibleStorageName(ptr %storageName.init.29393.arg0, i32 %storageName.init.29393.arg1, ptr %storageName.init.29393.arg2)
  store ptr %storageName.init.29393, ptr %storageName.addr.29393
  %opText.addr.29407 = alloca ptr
  %opText.init.29407.arg0 = load ptr, ptr %tokens
  %opText.init.29407.arg1 = load i32, ptr %op.addr.29343
  %opText.init.29407 = call ptr @tokenTextAt(ptr %opText.init.29407.arg0, i32 %opText.init.29407.arg1)
  store ptr %opText.init.29407, ptr %opText.addr.29407
  %ifcond.29419.left.arg0 = load ptr, ptr %opText.addr.29407
  %ifcond.29419.left.arg1 = getelementptr inbounds [2 x i8], ptr @.str.29425, i32 0, i32 0
  %ifcond.29419.left = call i1 @strEq(ptr %ifcond.29419.left.arg0, ptr %ifcond.29419.left.arg1)
  %ifcond.29419.right.arg0 = load ptr, ptr %opText.addr.29407
  %ifcond.29419.right.arg1 = getelementptr inbounds [3 x i8], ptr @.str.29432, i32 0, i32 0
  %ifcond.29419.right = call i1 @strEq(ptr %ifcond.29419.right.arg0, ptr %ifcond.29419.right.arg1)
  %ifcond.29419 = or i1 %ifcond.29419.left, %ifcond.29419.right
  br i1 %ifcond.29419, label %if.then.29419, label %if.else.29419
if.then.29419:
  %resultName.addr.29436 = alloca ptr
  %resultName.init.29436.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.29441, i32 0, i32 0
  %resultName.init.29436.left.left.right = load ptr, ptr %name.addr.29381
  %resultName.init.29436.left.left = call ptr @csec_string_concat(ptr %resultName.init.29436.left.left.left, ptr %resultName.init.29436.left.left.right)
  %resultName.init.29436.left.right = getelementptr inbounds [10 x i8], ptr @.str.29445, i32 0, i32 0
  %resultName.init.29436.left = call ptr @csec_string_concat(ptr %resultName.init.29436.left.left, ptr %resultName.init.29436.left.right)
  %resultName.init.29436.right.i32 = load i32, ptr %start
  %resultName.init.29436.right.i64 = sext i32 %resultName.init.29436.right.i32 to i64
  %resultName.init.29436.right = call ptr @csec_to_string_i64(i64 %resultName.init.29436.right.i64)
  %resultName.init.29436 = call ptr @csec_string_concat(ptr %resultName.init.29436.left, ptr %resultName.init.29436.right)
  store ptr %resultName.init.29436, ptr %resultName.addr.29436
  %ret.29449.left.left.left.left.left.arg0 = load ptr, ptr %tokens
  %ret.29449.left.left.left.left.left.arg1.left = load i32, ptr %op.addr.29343
  %ret.29449.left.left.left.left.left.arg1.right = add i32 0, 1
  %ret.29449.left.left.left.left.left.arg1 = add i32 %ret.29449.left.left.left.left.left.arg1.left, %ret.29449.left.left.left.left.left.arg1.right
  %ret.29449.left.left.left.left.left.arg2 = load i32, ptr %end
  %ret.29449.left.left.left.left.left.arg3 = load ptr, ptr %resultName.addr.29436
  %ret.29449.left.left.left.left.left = call ptr @generateLLVMExpressionF64(ptr %ret.29449.left.left.left.left.left.arg0, i32 %ret.29449.left.left.left.left.left.arg1, i32 %ret.29449.left.left.left.left.left.arg2, ptr %ret.29449.left.left.left.left.left.arg3)
  %ret.29449.left.left.left.left.right = getelementptr inbounds [16 x i8], ptr @.str.29463, i32 0, i32 0
  %ret.29449.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29449.left.left.left.left.left, ptr %ret.29449.left.left.left.left.right)
  %ret.29449.left.left.left.right = load ptr, ptr %resultName.addr.29436
  %ret.29449.left.left.left = call ptr @csec_string_concat(ptr %ret.29449.left.left.left.left, ptr %ret.29449.left.left.left.right)
  %ret.29449.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.29467, i32 0, i32 0
  %ret.29449.left.left = call ptr @csec_string_concat(ptr %ret.29449.left.left.left, ptr %ret.29449.left.left.right)
  %ret.29449.left.right = load ptr, ptr %storageName.addr.29393
  %ret.29449.left = call ptr @csec_string_concat(ptr %ret.29449.left.left, ptr %ret.29449.left.right)
  %ret.29449.right = getelementptr inbounds [2 x i8], ptr @.str.29471, i32 0, i32 0
  %ret.29449 = call ptr @csec_string_concat(ptr %ret.29449.left, ptr %ret.29449.right)
  ret ptr %ret.29449
if.else.29419:
  br label %if.end.29419
if.end.29419:
  %ret.29474 = getelementptr inbounds [1 x i8], ptr @.str.29475, i32 0, i32 0
  ret ptr %ret.29474
}

define ptr @generateLLVMAssignmentPtr(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %end = alloca i32
  store i32 %arg.end, ptr %end
  %op.addr.29496 = alloca i32
  %op.init.29496.arg0 = load ptr, ptr %tokens
  %op.init.29496.arg1 = load i32, ptr %start
  %op.init.29496.arg2 = load i32, ptr %end
  %op.init.29496.arg3 = add i32 0, 1
  %op.init.29496 = call i32 @findTopLevelOperator(ptr %op.init.29496.arg0, i32 %op.init.29496.arg1, i32 %op.init.29496.arg2, i32 %op.init.29496.arg3)
  store i32 %op.init.29496, ptr %op.addr.29496
  %ifcond.29512.left.i32.left = load i32, ptr %op.addr.29496
  %ifcond.29512.left.i32.right = load i32, ptr %start
  %ifcond.29512.left.i32.comparison = icmp sle i32 %ifcond.29512.left.i32.left, %ifcond.29512.left.i32.right
  %ifcond.29512.left.i32 = zext i1 %ifcond.29512.left.i32.comparison to i32
  %ifcond.29512.left = icmp ne i32 %ifcond.29512.left.i32, 0
  %ifcond.29512.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29512.right.i32.left.char.arg0 = trunc i32 %ifcond.29512.right.i32.left.char.arg0.i32 to i8
  %ifcond.29512.right.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.29512.right.i32.left.char.arg1 = trunc i32 %ifcond.29512.right.i32.left.char.arg1.i32 to i8
  %ifcond.29512.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29512.right.i32.left.char.arg0, i8 %ifcond.29512.right.i32.left.char.arg1)
  %ifcond.29512.right.i32.left = zext i8 %ifcond.29512.right.i32.left.char to i32
  %ifcond.29512.right.i32.right.char = call i8 @kindIdentifier()
  %ifcond.29512.right.i32.right = zext i8 %ifcond.29512.right.i32.right.char to i32
  %ifcond.29512.right.i32.comparison = icmp ne i32 %ifcond.29512.right.i32.left, %ifcond.29512.right.i32.right
  %ifcond.29512.right.i32 = zext i1 %ifcond.29512.right.i32.comparison to i32
  %ifcond.29512.right = icmp ne i32 %ifcond.29512.right.i32, 0
  %ifcond.29512 = or i1 %ifcond.29512.left, %ifcond.29512.right
  br i1 %ifcond.29512, label %if.then.29512, label %if.else.29512
if.then.29512:
  %ret.29530 = getelementptr inbounds [1 x i8], ptr @.str.29531, i32 0, i32 0
  ret ptr %ret.29530
if.else.29512:
  br label %if.end.29512
if.end.29512:
  %name.addr.29534 = alloca ptr
  %name.init.29534.arg0 = load ptr, ptr %tokens
  %name.init.29534.arg1 = load i32, ptr %start
  %name.init.29534 = call ptr @tokenTextAt(ptr %name.init.29534.arg0, i32 %name.init.29534.arg1)
  store ptr %name.init.29534, ptr %name.addr.29534
  %storageName.addr.29546 = alloca ptr
  %storageName.init.29546.arg0 = load ptr, ptr %tokens
  %storageName.init.29546.arg1 = load i32, ptr %start
  %storageName.init.29546.arg2 = load ptr, ptr %name.addr.29534
  %storageName.init.29546 = call ptr @lookupVisibleStorageName(ptr %storageName.init.29546.arg0, i32 %storageName.init.29546.arg1, ptr %storageName.init.29546.arg2)
  store ptr %storageName.init.29546, ptr %storageName.addr.29546
  %exprStart.addr.29560 = alloca i32
  %exprStart.init.29560.arg0 = load ptr, ptr %tokens
  %exprStart.init.29560.arg1.left = load i32, ptr %op.addr.29496
  %exprStart.init.29560.arg1.right = add i32 0, 1
  %exprStart.init.29560.arg1 = add i32 %exprStart.init.29560.arg1.left, %exprStart.init.29560.arg1.right
  %exprStart.init.29560 = call i32 @skipTrivia(ptr %exprStart.init.29560.arg0, i32 %exprStart.init.29560.arg1)
  store i32 %exprStart.init.29560, ptr %exprStart.addr.29560
  %exprEnd.addr.29574 = alloca i32
  %exprEnd.init.29574.arg0 = load ptr, ptr %tokens
  %exprEnd.init.29574.arg1 = load i32, ptr %exprStart.addr.29560
  %exprEnd.init.29574.arg2 = load i32, ptr %end
  %exprEnd.init.29574 = call i32 @trimExpressionEnd(ptr %exprEnd.init.29574.arg0, i32 %exprEnd.init.29574.arg1, i32 %exprEnd.init.29574.arg2)
  store i32 %exprEnd.init.29574, ptr %exprEnd.addr.29574
  %ifcond.29588.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29588.left.i32.left.char.arg0 = trunc i32 %ifcond.29588.left.i32.left.char.arg0.i32 to i8
  %ifcond.29588.left.i32.left.char.arg1.i32 = load i32, ptr %exprStart.addr.29560
  %ifcond.29588.left.i32.left.char.arg1 = trunc i32 %ifcond.29588.left.i32.left.char.arg1.i32 to i8
  %ifcond.29588.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29588.left.i32.left.char.arg0, i8 %ifcond.29588.left.i32.left.char.arg1)
  %ifcond.29588.left.i32.left = zext i8 %ifcond.29588.left.i32.left.char to i32
  %ifcond.29588.left.i32.right.char = call i8 @kindString()
  %ifcond.29588.left.i32.right = zext i8 %ifcond.29588.left.i32.right.char to i32
  %ifcond.29588.left.i32.comparison = icmp eq i32 %ifcond.29588.left.i32.left, %ifcond.29588.left.i32.right
  %ifcond.29588.left.i32 = zext i1 %ifcond.29588.left.i32.comparison to i32
  %ifcond.29588.left = icmp ne i32 %ifcond.29588.left.i32, 0
  %ifcond.29588.right.i32.left = load i32, ptr %exprEnd.addr.29574
  %ifcond.29588.right.i32.right.left = load i32, ptr %exprStart.addr.29560
  %ifcond.29588.right.i32.right.right = add i32 0, 1
  %ifcond.29588.right.i32.right = add i32 %ifcond.29588.right.i32.right.left, %ifcond.29588.right.i32.right.right
  %ifcond.29588.right.i32.comparison = icmp eq i32 %ifcond.29588.right.i32.left, %ifcond.29588.right.i32.right
  %ifcond.29588.right.i32 = zext i1 %ifcond.29588.right.i32.comparison to i32
  %ifcond.29588.right = icmp ne i32 %ifcond.29588.right.i32, 0
  %ifcond.29588 = and i1 %ifcond.29588.left, %ifcond.29588.right
  br i1 %ifcond.29588, label %if.then.29588, label %if.else.29588
if.then.29588:
  %text.addr.29608 = alloca ptr
  %text.init.29608.arg0 = load ptr, ptr %tokens
  %text.init.29608.arg1 = load i32, ptr %exprStart.addr.29560
  %text.init.29608 = call ptr @tokenTextAt(ptr %text.init.29608.arg0, i32 %text.init.29608.arg1)
  store ptr %text.init.29608, ptr %text.addr.29608
  %byteLength.addr.29620 = alloca i32
  %byteLength.init.29620.left.string = load ptr, ptr %text.addr.29608
  %byteLength.init.29620.left.length64 = call i64 @csec_string_length(ptr %byteLength.init.29620.left.string)
  %byteLength.init.29620.left = trunc i64 %byteLength.init.29620.left.length64 to i32
  %byteLength.init.29620.right = add i32 0, 1
  %byteLength.init.29620 = add i32 %byteLength.init.29620.left, %byteLength.init.29620.right
  store i32 %byteLength.init.29620, ptr %byteLength.addr.29620
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.29632, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %name.addr.29534
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.29636, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %start
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right.i64)
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [28 x i8], ptr @.str.29640, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %byteLength.addr.29620
  %ret.29631.left.left.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.29631.left.left.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.29631.left.left.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29631.left.left.left.left.left.left.left.left.left.left.right.i64)
  %ret.29631.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [19 x i8], ptr @.str.29644, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %exprStart.addr.29560
  %ret.29631.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.29631.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.29631.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29631.left.left.left.left.left.left.left.left.right.i64)
  %ret.29631.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.left.right = getelementptr inbounds [16 x i8], ptr @.str.29648, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.left.right = getelementptr inbounds [14 x i8], ptr @.str.29650, i32 0, i32 0
  %ret.29631.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.left.right = load ptr, ptr %name.addr.29534
  %ret.29631.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left.left, ptr %ret.29631.left.left.left.left.left.right)
  %ret.29631.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.29654, i32 0, i32 0
  %ret.29631.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left.left, ptr %ret.29631.left.left.left.left.right)
  %ret.29631.left.left.left.right.i32 = load i32, ptr %start
  %ret.29631.left.left.left.right.i64 = sext i32 %ret.29631.left.left.left.right.i32 to i64
  %ret.29631.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29631.left.left.left.right.i64)
  %ret.29631.left.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left.left, ptr %ret.29631.left.left.left.right)
  %ret.29631.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.29658, i32 0, i32 0
  %ret.29631.left.left = call ptr @csec_string_concat(ptr %ret.29631.left.left.left, ptr %ret.29631.left.left.right)
  %ret.29631.left.right = load ptr, ptr %storageName.addr.29546
  %ret.29631.left = call ptr @csec_string_concat(ptr %ret.29631.left.left, ptr %ret.29631.left.right)
  %ret.29631.right = getelementptr inbounds [2 x i8], ptr @.str.29662, i32 0, i32 0
  %ret.29631 = call ptr @csec_string_concat(ptr %ret.29631.left, ptr %ret.29631.right)
  ret ptr %ret.29631
if.else.29588:
  br label %if.end.29588
if.end.29588:
  %ifcond.29665.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29665.left.i32.left.char.arg0 = trunc i32 %ifcond.29665.left.i32.left.char.arg0.i32 to i8
  %ifcond.29665.left.i32.left.char.arg1.i32 = load i32, ptr %exprStart.addr.29560
  %ifcond.29665.left.i32.left.char.arg1 = trunc i32 %ifcond.29665.left.i32.left.char.arg1.i32 to i8
  %ifcond.29665.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29665.left.i32.left.char.arg0, i8 %ifcond.29665.left.i32.left.char.arg1)
  %ifcond.29665.left.i32.left = zext i8 %ifcond.29665.left.i32.left.char to i32
  %ifcond.29665.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.29665.left.i32.right = zext i8 %ifcond.29665.left.i32.right.char to i32
  %ifcond.29665.left.i32.comparison = icmp eq i32 %ifcond.29665.left.i32.left, %ifcond.29665.left.i32.right
  %ifcond.29665.left.i32 = zext i1 %ifcond.29665.left.i32.comparison to i32
  %ifcond.29665.left = icmp ne i32 %ifcond.29665.left.i32, 0
  %ifcond.29665.right.i32.left = load i32, ptr %exprEnd.addr.29574
  %ifcond.29665.right.i32.right.left = load i32, ptr %exprStart.addr.29560
  %ifcond.29665.right.i32.right.right = add i32 0, 1
  %ifcond.29665.right.i32.right = add i32 %ifcond.29665.right.i32.right.left, %ifcond.29665.right.i32.right.right
  %ifcond.29665.right.i32.comparison = icmp eq i32 %ifcond.29665.right.i32.left, %ifcond.29665.right.i32.right
  %ifcond.29665.right.i32 = zext i1 %ifcond.29665.right.i32.comparison to i32
  %ifcond.29665.right = icmp ne i32 %ifcond.29665.right.i32, 0
  %ifcond.29665 = and i1 %ifcond.29665.left, %ifcond.29665.right
  br i1 %ifcond.29665, label %if.then.29665, label %if.else.29665
if.then.29665:
  %exprStorageName.addr.29685 = alloca ptr
  %exprStorageName.init.29685.arg0 = load ptr, ptr %tokens
  %exprStorageName.init.29685.arg1 = load i32, ptr %exprStart.addr.29560
  %exprStorageName.init.29685.arg2.arg0 = load ptr, ptr %tokens
  %exprStorageName.init.29685.arg2.arg1 = load i32, ptr %exprStart.addr.29560
  %exprStorageName.init.29685.arg2 = call ptr @tokenTextAt(ptr %exprStorageName.init.29685.arg2.arg0, i32 %exprStorageName.init.29685.arg2.arg1)
  %exprStorageName.init.29685 = call ptr @lookupVisibleStorageName(ptr %exprStorageName.init.29685.arg0, i32 %exprStorageName.init.29685.arg1, ptr %exprStorageName.init.29685.arg2)
  store ptr %exprStorageName.init.29685, ptr %exprStorageName.addr.29685
  %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.29705, i32 0, i32 0
  %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %name.addr.29534
  %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.29709, i32 0, i32 0
  %ret.29704.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %start
  %ret.29704.left.left.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.29704.left.left.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.29704.left.left.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29704.left.left.left.left.left.left.left.left.left.left.right.i64)
  %ret.29704.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [19 x i8], ptr @.str.29713, i32 0, i32 0
  %ret.29704.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.left.left.right = load ptr, ptr %exprStorageName.addr.29685
  %ret.29704.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.left.right = getelementptr inbounds [2 x i8], ptr @.str.29717, i32 0, i32 0
  %ret.29704.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.left.right = getelementptr inbounds [14 x i8], ptr @.str.29719, i32 0, i32 0
  %ret.29704.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.left.right = load ptr, ptr %name.addr.29534
  %ret.29704.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left.left, ptr %ret.29704.left.left.left.left.left.right)
  %ret.29704.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.29723, i32 0, i32 0
  %ret.29704.left.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left.left, ptr %ret.29704.left.left.left.left.right)
  %ret.29704.left.left.left.right.i32 = load i32, ptr %start
  %ret.29704.left.left.left.right.i64 = sext i32 %ret.29704.left.left.left.right.i32 to i64
  %ret.29704.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.29704.left.left.left.right.i64)
  %ret.29704.left.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left.left, ptr %ret.29704.left.left.left.right)
  %ret.29704.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.29727, i32 0, i32 0
  %ret.29704.left.left = call ptr @csec_string_concat(ptr %ret.29704.left.left.left, ptr %ret.29704.left.left.right)
  %ret.29704.left.right = load ptr, ptr %storageName.addr.29546
  %ret.29704.left = call ptr @csec_string_concat(ptr %ret.29704.left.left, ptr %ret.29704.left.right)
  %ret.29704.right = getelementptr inbounds [2 x i8], ptr @.str.29731, i32 0, i32 0
  %ret.29704 = call ptr @csec_string_concat(ptr %ret.29704.left, ptr %ret.29704.right)
  ret ptr %ret.29704
if.else.29665:
  br label %if.end.29665
if.end.29665:
  %concat.addr.29863 = alloca i32
  %concat.init.29863.arg0 = load ptr, ptr %tokens
  %concat.init.29863.arg1 = load i32, ptr %exprStart.addr.29560
  %concat.init.29863.arg2 = load i32, ptr %exprEnd.addr.29574
  %concat.init.29863 = call i32 @expressionTopLevelOperator(ptr %concat.init.29863.arg0, i32 %concat.init.29863.arg1, i32 %concat.init.29863.arg2)
  store i32 %concat.init.29863, ptr %concat.addr.29863
  %ifcond.29877.left.left.i32.left = load i32, ptr %concat.addr.29863
  %ifcond.29877.left.left.i32.right = load i32, ptr %exprStart.addr.29560
  %ifcond.29877.left.left.i32.comparison = icmp sgt i32 %ifcond.29877.left.left.i32.left, %ifcond.29877.left.left.i32.right
  %ifcond.29877.left.left.i32 = zext i1 %ifcond.29877.left.left.i32.comparison to i32
  %ifcond.29877.left.left = icmp ne i32 %ifcond.29877.left.left.i32, 0
  %ifcond.29877.left.right.arg0 = load ptr, ptr %tokens
  %ifcond.29877.left.right.arg1 = load i32, ptr %concat.addr.29863
  %ifcond.29877.left.right.arg2.char = call i8 @kindOperator()
  %ifcond.29877.left.right.arg2 = zext i8 %ifcond.29877.left.right.arg2.char to i32
  %ifcond.29877.left.right.arg3 = getelementptr inbounds [2 x i8], ptr @.str.29893, i32 0, i32 0
  %ifcond.29877.left.right = call i1 @tokenIs(ptr %ifcond.29877.left.right.arg0, i32 %ifcond.29877.left.right.arg1, i32 %ifcond.29877.left.right.arg2, ptr %ifcond.29877.left.right.arg3)
  %ifcond.29877.left = and i1 %ifcond.29877.left.left, %ifcond.29877.left.right
  %ifcond.29877.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29877.right.i32.left.char.arg0 = trunc i32 %ifcond.29877.right.i32.left.char.arg0.i32 to i8
  %ifcond.29877.right.i32.left.char.arg1.i32 = load i32, ptr %exprStart.addr.29560
  %ifcond.29877.right.i32.left.char.arg1 = trunc i32 %ifcond.29877.right.i32.left.char.arg1.i32 to i8
  %ifcond.29877.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29877.right.i32.left.char.arg0, i8 %ifcond.29877.right.i32.left.char.arg1)
  %ifcond.29877.right.i32.left = zext i8 %ifcond.29877.right.i32.left.char to i32
  %ifcond.29877.right.i32.right.char = call i8 @kindIdentifier()
  %ifcond.29877.right.i32.right = zext i8 %ifcond.29877.right.i32.right.char to i32
  %ifcond.29877.right.i32.comparison = icmp eq i32 %ifcond.29877.right.i32.left, %ifcond.29877.right.i32.right
  %ifcond.29877.right.i32 = zext i1 %ifcond.29877.right.i32.comparison to i32
  %ifcond.29877.right = icmp ne i32 %ifcond.29877.right.i32, 0
  %ifcond.29877 = and i1 %ifcond.29877.left, %ifcond.29877.right
  br i1 %ifcond.29877, label %if.then.29877, label %if.else.29877
if.then.29877:
  %resultName.addr.29908 = alloca ptr
  %resultName.init.29908.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.29913, i32 0, i32 0
  %resultName.init.29908.left.left.right = load ptr, ptr %name.addr.29534
  %resultName.init.29908.left.left = call ptr @csec_string_concat(ptr %resultName.init.29908.left.left.left, ptr %resultName.init.29908.left.left.right)
  %resultName.init.29908.left.right = getelementptr inbounds [10 x i8], ptr @.str.29917, i32 0, i32 0
  %resultName.init.29908.left = call ptr @csec_string_concat(ptr %resultName.init.29908.left.left, ptr %resultName.init.29908.left.right)
  %resultName.init.29908.right.i32 = load i32, ptr %start
  %resultName.init.29908.right.i64 = sext i32 %resultName.init.29908.right.i32 to i64
  %resultName.init.29908.right = call ptr @csec_to_string_i64(i64 %resultName.init.29908.right.i64)
  %resultName.init.29908 = call ptr @csec_string_concat(ptr %resultName.init.29908.left, ptr %resultName.init.29908.right)
  store ptr %resultName.init.29908, ptr %resultName.addr.29908
  %rightStart.addr.29921 = alloca i32
  %rightStart.init.29921.arg0 = load ptr, ptr %tokens
  %rightStart.init.29921.arg1.left = load i32, ptr %concat.addr.29863
  %rightStart.init.29921.arg1.right = add i32 0, 1
  %rightStart.init.29921.arg1 = add i32 %rightStart.init.29921.arg1.left, %rightStart.init.29921.arg1.right
  %rightStart.init.29921 = call i32 @skipTrivia(ptr %rightStart.init.29921.arg0, i32 %rightStart.init.29921.arg1)
  store i32 %rightStart.init.29921, ptr %rightStart.addr.29921
  %leftStorage.addr.29935 = alloca ptr
  %leftStorage.init.29935.arg0 = load ptr, ptr %tokens
  %leftStorage.init.29935.arg1 = load i32, ptr %exprStart.addr.29560
  %leftStorage.init.29935.arg2.arg0 = load ptr, ptr %tokens
  %leftStorage.init.29935.arg2.arg1 = load i32, ptr %exprStart.addr.29560
  %leftStorage.init.29935.arg2 = call ptr @tokenTextAt(ptr %leftStorage.init.29935.arg2.arg0, i32 %leftStorage.init.29935.arg2.arg1)
  %leftStorage.init.29935 = call ptr @lookupVisibleStorageName(ptr %leftStorage.init.29935.arg0, i32 %leftStorage.init.29935.arg1, ptr %leftStorage.init.29935.arg2)
  store ptr %leftStorage.init.29935, ptr %leftStorage.addr.29935
  %rightCode.addr.29954 = alloca ptr
  %rightCode.init.29954 = getelementptr inbounds [1 x i8], ptr @.str.29959, i32 0, i32 0
  store ptr %rightCode.init.29954, ptr %rightCode.addr.29954
  %ifcond.29961.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.29961.left.i32.left.char.arg0 = trunc i32 %ifcond.29961.left.i32.left.char.arg0.i32 to i8
  %ifcond.29961.left.i32.left.char.arg1.i32 = load i32, ptr %rightStart.addr.29921
  %ifcond.29961.left.i32.left.char.arg1 = trunc i32 %ifcond.29961.left.i32.left.char.arg1.i32 to i8
  %ifcond.29961.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.29961.left.i32.left.char.arg0, i8 %ifcond.29961.left.i32.left.char.arg1)
  %ifcond.29961.left.i32.left = zext i8 %ifcond.29961.left.i32.left.char to i32
  %ifcond.29961.left.i32.right.char = call i8 @kindString()
  %ifcond.29961.left.i32.right = zext i8 %ifcond.29961.left.i32.right.char to i32
  %ifcond.29961.left.i32.comparison = icmp eq i32 %ifcond.29961.left.i32.left, %ifcond.29961.left.i32.right
  %ifcond.29961.left.i32 = zext i1 %ifcond.29961.left.i32.comparison to i32
  %ifcond.29961.left = icmp ne i32 %ifcond.29961.left.i32, 0
  %ifcond.29961.right.i32.left.left = load i32, ptr %rightStart.addr.29921
  %ifcond.29961.right.i32.left.right = add i32 0, 1
  %ifcond.29961.right.i32.left = add i32 %ifcond.29961.right.i32.left.left, %ifcond.29961.right.i32.left.right
  %ifcond.29961.right.i32.right = load i32, ptr %exprEnd.addr.29574
  %ifcond.29961.right.i32.comparison = icmp eq i32 %ifcond.29961.right.i32.left, %ifcond.29961.right.i32.right
  %ifcond.29961.right.i32 = zext i1 %ifcond.29961.right.i32.comparison to i32
  %ifcond.29961.right = icmp ne i32 %ifcond.29961.right.i32, 0
  %ifcond.29961 = and i1 %ifcond.29961.left, %ifcond.29961.right
  br i1 %ifcond.29961, label %if.then.29961, label %if.else.29961
if.then.29961:
  %rightText.addr.29981 = alloca ptr
  %rightText.init.29981.arg0 = load ptr, ptr %tokens
  %rightText.init.29981.arg1 = load i32, ptr %rightStart.addr.29921
  %rightText.init.29981 = call ptr @tokenTextAt(ptr %rightText.init.29981.arg0, i32 %rightText.init.29981.arg1)
  store ptr %rightText.init.29981, ptr %rightText.addr.29981
  %rightCode.assign.29993.left.left.left.left.left.left = getelementptr inbounds [3 x i8], ptr @.str.29995, i32 0, i32 0
  %rightCode.assign.29993.left.left.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %rightCode.assign.29993.left.left.left.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left.left.left.left.left.left, ptr %rightCode.assign.29993.left.left.left.left.left.right)
  %rightCode.assign.29993.left.left.left.left.right = getelementptr inbounds [34 x i8], ptr @.str.29999, i32 0, i32 0
  %rightCode.assign.29993.left.left.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left.left.left.left.left, ptr %rightCode.assign.29993.left.left.left.left.right)
  %rightCode.assign.29993.left.left.left.right.i32.left.string = load ptr, ptr %rightText.addr.29981
  %rightCode.assign.29993.left.left.left.right.i32.left.length64 = call i64 @csec_string_length(ptr %rightCode.assign.29993.left.left.left.right.i32.left.string)
  %rightCode.assign.29993.left.left.left.right.i32.left = trunc i64 %rightCode.assign.29993.left.left.left.right.i32.left.length64 to i32
  %rightCode.assign.29993.left.left.left.right.i32.right = add i32 0, 1
  %rightCode.assign.29993.left.left.left.right.i32 = add i32 %rightCode.assign.29993.left.left.left.right.i32.left, %rightCode.assign.29993.left.left.left.right.i32.right
  %rightCode.assign.29993.left.left.left.right.i64 = sext i32 %rightCode.assign.29993.left.left.left.right.i32 to i64
  %rightCode.assign.29993.left.left.left.right = call ptr @csec_to_string_i64(i64 %rightCode.assign.29993.left.left.left.right.i64)
  %rightCode.assign.29993.left.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left.left.left.left, ptr %rightCode.assign.29993.left.left.left.right)
  %rightCode.assign.29993.left.left.right = getelementptr inbounds [19 x i8], ptr @.str.30009, i32 0, i32 0
  %rightCode.assign.29993.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left.left.left, ptr %rightCode.assign.29993.left.left.right)
  %rightCode.assign.29993.left.right.i32 = load i32, ptr %rightStart.addr.29921
  %rightCode.assign.29993.left.right.i64 = sext i32 %rightCode.assign.29993.left.right.i32 to i64
  %rightCode.assign.29993.left.right = call ptr @csec_to_string_i64(i64 %rightCode.assign.29993.left.right.i64)
  %rightCode.assign.29993.left = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left.left, ptr %rightCode.assign.29993.left.right)
  %rightCode.assign.29993.right = getelementptr inbounds [16 x i8], ptr @.str.30013, i32 0, i32 0
  %rightCode.assign.29993 = call ptr @csec_string_concat(ptr %rightCode.assign.29993.left, ptr %rightCode.assign.29993.right)
  store ptr %rightCode.assign.29993, ptr %rightCode.addr.29954
  br label %if.end.29961
if.else.29961:
  %ifcond.30017.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.30017.left.i32.left.char.arg0 = trunc i32 %ifcond.30017.left.i32.left.char.arg0.i32 to i8
  %ifcond.30017.left.i32.left.char.arg1.i32 = load i32, ptr %rightStart.addr.29921
  %ifcond.30017.left.i32.left.char.arg1 = trunc i32 %ifcond.30017.left.i32.left.char.arg1.i32 to i8
  %ifcond.30017.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.30017.left.i32.left.char.arg0, i8 %ifcond.30017.left.i32.left.char.arg1)
  %ifcond.30017.left.i32.left = zext i8 %ifcond.30017.left.i32.left.char to i32
  %ifcond.30017.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.30017.left.i32.right = zext i8 %ifcond.30017.left.i32.right.char to i32
  %ifcond.30017.left.i32.comparison = icmp eq i32 %ifcond.30017.left.i32.left, %ifcond.30017.left.i32.right
  %ifcond.30017.left.i32 = zext i1 %ifcond.30017.left.i32.comparison to i32
  %ifcond.30017.left = icmp ne i32 %ifcond.30017.left.i32, 0
  %ifcond.30017.right.i32.left.left = load i32, ptr %rightStart.addr.29921
  %ifcond.30017.right.i32.left.right = add i32 0, 1
  %ifcond.30017.right.i32.left = add i32 %ifcond.30017.right.i32.left.left, %ifcond.30017.right.i32.left.right
  %ifcond.30017.right.i32.right = load i32, ptr %exprEnd.addr.29574
  %ifcond.30017.right.i32.comparison = icmp eq i32 %ifcond.30017.right.i32.left, %ifcond.30017.right.i32.right
  %ifcond.30017.right.i32 = zext i1 %ifcond.30017.right.i32.comparison to i32
  %ifcond.30017.right = icmp ne i32 %ifcond.30017.right.i32, 0
  %ifcond.30017 = and i1 %ifcond.30017.left, %ifcond.30017.right
  br i1 %ifcond.30017, label %if.then.30017, label %if.else.30017
if.then.30017:
  %rightCode.assign.30037.left.left.left.left = getelementptr inbounds [3 x i8], ptr @.str.30039, i32 0, i32 0
  %rightCode.assign.30037.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %rightCode.assign.30037.left.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.30037.left.left.left.left, ptr %rightCode.assign.30037.left.left.left.right)
  %rightCode.assign.30037.left.left.right = getelementptr inbounds [25 x i8], ptr @.str.30043, i32 0, i32 0
  %rightCode.assign.30037.left.left = call ptr @csec_string_concat(ptr %rightCode.assign.30037.left.left.left, ptr %rightCode.assign.30037.left.left.right)
  %rightCode.assign.30037.left.right.arg0 = load ptr, ptr %tokens
  %rightCode.assign.30037.left.right.arg1 = load i32, ptr %rightStart.addr.29921
  %rightCode.assign.30037.left.right.arg2.arg0 = load ptr, ptr %tokens
  %rightCode.assign.30037.left.right.arg2.arg1 = load i32, ptr %rightStart.addr.29921
  %rightCode.assign.30037.left.right.arg2 = call ptr @tokenTextAt(ptr %rightCode.assign.30037.left.right.arg2.arg0, i32 %rightCode.assign.30037.left.right.arg2.arg1)
  %rightCode.assign.30037.left.right = call ptr @lookupVisibleStorageName(ptr %rightCode.assign.30037.left.right.arg0, i32 %rightCode.assign.30037.left.right.arg1, ptr %rightCode.assign.30037.left.right.arg2)
  %rightCode.assign.30037.left = call ptr @csec_string_concat(ptr %rightCode.assign.30037.left.left, ptr %rightCode.assign.30037.left.right)
  %rightCode.assign.30037.right = getelementptr inbounds [2 x i8], ptr @.str.30059, i32 0, i32 0
  %rightCode.assign.30037 = call ptr @csec_string_concat(ptr %rightCode.assign.30037.left, ptr %rightCode.assign.30037.right)
  store ptr %rightCode.assign.30037, ptr %rightCode.addr.29954
  br label %if.end.30017
if.else.30017:
  br label %if.end.30017
if.end.30017:
  br label %if.end.29961
if.end.29961:
  %ifcond.30170.i32.left.string = load ptr, ptr %rightCode.addr.29954
  %ifcond.30170.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.30170.i32.left.string)
  %ifcond.30170.i32.left = trunc i64 %ifcond.30170.i32.left.length64 to i32
  %ifcond.30170.i32.right = add i32 0, 0
  %ifcond.30170.i32.comparison = icmp sgt i32 %ifcond.30170.i32.left, %ifcond.30170.i32.right
  %ifcond.30170.i32 = zext i1 %ifcond.30170.i32.comparison to i32
  %ifcond.30170 = icmp ne i32 %ifcond.30170.i32, 0
  br i1 %ifcond.30170, label %if.then.30170, label %if.else.30170
if.then.30170:
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left = getelementptr inbounds [3 x i8], ptr @.str.30180, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [24 x i8], ptr @.str.30184, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %leftStorage.addr.29935
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [2 x i8], ptr @.str.30188, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %rightCode.addr.29954
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [3 x i8], ptr @.str.30192, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %ret.30179.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [37 x i8], ptr @.str.30196, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %ret.30179.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.left.right = getelementptr inbounds [12 x i8], ptr @.str.30200, i32 0, i32 0
  %ret.30179.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %ret.30179.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.left.right = getelementptr inbounds [9 x i8], ptr @.str.30204, i32 0, i32 0
  %ret.30179.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left.left, ptr %ret.30179.left.left.left.left.left.right)
  %ret.30179.left.left.left.left.right = getelementptr inbounds [13 x i8], ptr @.str.30206, i32 0, i32 0
  %ret.30179.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left.left, ptr %ret.30179.left.left.left.left.right)
  %ret.30179.left.left.left.right = load ptr, ptr %resultName.addr.29908
  %ret.30179.left.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left.left, ptr %ret.30179.left.left.left.right)
  %ret.30179.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.30210, i32 0, i32 0
  %ret.30179.left.left = call ptr @csec_string_concat(ptr %ret.30179.left.left.left, ptr %ret.30179.left.left.right)
  %ret.30179.left.right = load ptr, ptr %storageName.addr.29546
  %ret.30179.left = call ptr @csec_string_concat(ptr %ret.30179.left.left, ptr %ret.30179.left.right)
  %ret.30179.right = getelementptr inbounds [2 x i8], ptr @.str.30214, i32 0, i32 0
  %ret.30179 = call ptr @csec_string_concat(ptr %ret.30179.left, ptr %ret.30179.right)
  ret ptr %ret.30179
if.else.30170:
  br label %if.end.30170
if.end.30170:
  br label %if.end.29877
if.else.29877:
  br label %if.end.29877
if.end.29877:
  %ret.30218 = getelementptr inbounds [1 x i8], ptr @.str.30219, i32 0, i32 0
  ret ptr %ret.30218
}

define ptr @generateLLVMAssignmentI8(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %end = alloca i32
  store i32 %arg.end, ptr %end
  %op.addr.30240 = alloca i32
  %op.init.30240.arg0 = load ptr, ptr %tokens
  %op.init.30240.arg1 = load i32, ptr %start
  %op.init.30240.arg2 = load i32, ptr %end
  %op.init.30240.arg3 = add i32 0, 1
  %op.init.30240 = call i32 @findTopLevelOperator(ptr %op.init.30240.arg0, i32 %op.init.30240.arg1, i32 %op.init.30240.arg2, i32 %op.init.30240.arg3)
  store i32 %op.init.30240, ptr %op.addr.30240
  %ifcond.30256.left.i32.left = load i32, ptr %op.addr.30240
  %ifcond.30256.left.i32.right = load i32, ptr %start
  %ifcond.30256.left.i32.comparison = icmp sle i32 %ifcond.30256.left.i32.left, %ifcond.30256.left.i32.right
  %ifcond.30256.left.i32 = zext i1 %ifcond.30256.left.i32.comparison to i32
  %ifcond.30256.left = icmp ne i32 %ifcond.30256.left.i32, 0
  %ifcond.30256.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.30256.right.i32.left.char.arg0 = trunc i32 %ifcond.30256.right.i32.left.char.arg0.i32 to i8
  %ifcond.30256.right.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.30256.right.i32.left.char.arg1 = trunc i32 %ifcond.30256.right.i32.left.char.arg1.i32 to i8
  %ifcond.30256.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.30256.right.i32.left.char.arg0, i8 %ifcond.30256.right.i32.left.char.arg1)
  %ifcond.30256.right.i32.left = zext i8 %ifcond.30256.right.i32.left.char to i32
  %ifcond.30256.right.i32.right.char = call i8 @kindIdentifier()
  %ifcond.30256.right.i32.right = zext i8 %ifcond.30256.right.i32.right.char to i32
  %ifcond.30256.right.i32.comparison = icmp ne i32 %ifcond.30256.right.i32.left, %ifcond.30256.right.i32.right
  %ifcond.30256.right.i32 = zext i1 %ifcond.30256.right.i32.comparison to i32
  %ifcond.30256.right = icmp ne i32 %ifcond.30256.right.i32, 0
  %ifcond.30256 = or i1 %ifcond.30256.left, %ifcond.30256.right
  br i1 %ifcond.30256, label %if.then.30256, label %if.else.30256
if.then.30256:
  %ret.30274 = getelementptr inbounds [1 x i8], ptr @.str.30275, i32 0, i32 0
  ret ptr %ret.30274
if.else.30256:
  br label %if.end.30256
if.end.30256:
  %name.addr.30278 = alloca ptr
  %name.init.30278.arg0 = load ptr, ptr %tokens
  %name.init.30278.arg1 = load i32, ptr %start
  %name.init.30278 = call ptr @tokenTextAt(ptr %name.init.30278.arg0, i32 %name.init.30278.arg1)
  store ptr %name.init.30278, ptr %name.addr.30278
  %storageName.addr.30290 = alloca ptr
  %storageName.init.30290.arg0 = load ptr, ptr %tokens
  %storageName.init.30290.arg1 = load i32, ptr %start
  %storageName.init.30290.arg2 = load ptr, ptr %name.addr.30278
  %storageName.init.30290 = call ptr @lookupVisibleStorageName(ptr %storageName.init.30290.arg0, i32 %storageName.init.30290.arg1, ptr %storageName.init.30290.arg2)
  store ptr %storageName.init.30290, ptr %storageName.addr.30290
  %exprStart.addr.30304 = alloca i32
  %exprStart.init.30304.arg0 = load ptr, ptr %tokens
  %exprStart.init.30304.arg1.left = load i32, ptr %op.addr.30240
  %exprStart.init.30304.arg1.right = add i32 0, 1
  %exprStart.init.30304.arg1 = add i32 %exprStart.init.30304.arg1.left, %exprStart.init.30304.arg1.right
  %exprStart.init.30304 = call i32 @skipTrivia(ptr %exprStart.init.30304.arg0, i32 %exprStart.init.30304.arg1)
  store i32 %exprStart.init.30304, ptr %exprStart.addr.30304
  %exprEnd.addr.30318 = alloca i32
  %exprEnd.init.30318.arg0 = load ptr, ptr %tokens
  %exprEnd.init.30318.arg1 = load i32, ptr %exprStart.addr.30304
  %exprEnd.init.30318.arg2 = load i32, ptr %end
  %exprEnd.init.30318 = call i32 @trimExpressionEnd(ptr %exprEnd.init.30318.arg0, i32 %exprEnd.init.30318.arg1, i32 %exprEnd.init.30318.arg2)
  store i32 %exprEnd.init.30318, ptr %exprEnd.addr.30318
  %ifcond.30332.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.30332.left.i32.left.char.arg0 = trunc i32 %ifcond.30332.left.i32.left.char.arg0.i32 to i8
  %ifcond.30332.left.i32.left.char.arg1.i32 = load i32, ptr %exprStart.addr.30304
  %ifcond.30332.left.i32.left.char.arg1 = trunc i32 %ifcond.30332.left.i32.left.char.arg1.i32 to i8
  %ifcond.30332.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.30332.left.i32.left.char.arg0, i8 %ifcond.30332.left.i32.left.char.arg1)
  %ifcond.30332.left.i32.left = zext i8 %ifcond.30332.left.i32.left.char to i32
  %ifcond.30332.left.i32.right.char = call i8 @kindChar()
  %ifcond.30332.left.i32.right = zext i8 %ifcond.30332.left.i32.right.char to i32
  %ifcond.30332.left.i32.comparison = icmp eq i32 %ifcond.30332.left.i32.left, %ifcond.30332.left.i32.right
  %ifcond.30332.left.i32 = zext i1 %ifcond.30332.left.i32.comparison to i32
  %ifcond.30332.left = icmp ne i32 %ifcond.30332.left.i32, 0
  %ifcond.30332.right.i32.left = load i32, ptr %exprEnd.addr.30318
  %ifcond.30332.right.i32.right.left = load i32, ptr %exprStart.addr.30304
  %ifcond.30332.right.i32.right.right = add i32 0, 1
  %ifcond.30332.right.i32.right = add i32 %ifcond.30332.right.i32.right.left, %ifcond.30332.right.i32.right.right
  %ifcond.30332.right.i32.comparison = icmp eq i32 %ifcond.30332.right.i32.left, %ifcond.30332.right.i32.right
  %ifcond.30332.right.i32 = zext i1 %ifcond.30332.right.i32.comparison to i32
  %ifcond.30332.right = icmp ne i32 %ifcond.30332.right.i32, 0
  %ifcond.30332 = and i1 %ifcond.30332.left, %ifcond.30332.right
  br i1 %ifcond.30332, label %if.then.30332, label %if.else.30332
if.then.30332:
  %ret.30352.left.left.left.left = getelementptr inbounds [12 x i8], ptr @.str.30353, i32 0, i32 0
  %ret.30352.left.left.left.right.arg0.arg0 = load ptr, ptr %tokens
  %ret.30352.left.left.left.right.arg0.arg1 = load i32, ptr %exprStart.addr.30304
  %ret.30352.left.left.left.right.arg0 = call ptr @tokenTextAt(ptr %ret.30352.left.left.left.right.arg0.arg0, i32 %ret.30352.left.left.left.right.arg0.arg1)
  %ret.30352.left.left.left.right = call ptr @llvmCharI8Value(ptr %ret.30352.left.left.left.right.arg0)
  %ret.30352.left.left.left = call ptr @csec_string_concat(ptr %ret.30352.left.left.left.left, ptr %ret.30352.left.left.left.right)
  %ret.30352.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.30365, i32 0, i32 0
  %ret.30352.left.left = call ptr @csec_string_concat(ptr %ret.30352.left.left.left, ptr %ret.30352.left.left.right)
  %ret.30352.left.right = load ptr, ptr %storageName.addr.30290
  %ret.30352.left = call ptr @csec_string_concat(ptr %ret.30352.left.left, ptr %ret.30352.left.right)
  %ret.30352.right = getelementptr inbounds [2 x i8], ptr @.str.30369, i32 0, i32 0
  %ret.30352 = call ptr @csec_string_concat(ptr %ret.30352.left, ptr %ret.30352.right)
  ret ptr %ret.30352
if.else.30332:
  br label %if.end.30332
if.end.30332:
  %ifcond.30372.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.30372.left.i32.left.char.arg0 = trunc i32 %ifcond.30372.left.i32.left.char.arg0.i32 to i8
  %ifcond.30372.left.i32.left.char.arg1.i32 = load i32, ptr %exprStart.addr.30304
  %ifcond.30372.left.i32.left.char.arg1 = trunc i32 %ifcond.30372.left.i32.left.char.arg1.i32 to i8
  %ifcond.30372.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.30372.left.i32.left.char.arg0, i8 %ifcond.30372.left.i32.left.char.arg1)
  %ifcond.30372.left.i32.left = zext i8 %ifcond.30372.left.i32.left.char to i32
  %ifcond.30372.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.30372.left.i32.right = zext i8 %ifcond.30372.left.i32.right.char to i32
  %ifcond.30372.left.i32.comparison = icmp eq i32 %ifcond.30372.left.i32.left, %ifcond.30372.left.i32.right
  %ifcond.30372.left.i32 = zext i1 %ifcond.30372.left.i32.comparison to i32
  %ifcond.30372.left = icmp ne i32 %ifcond.30372.left.i32, 0
  %ifcond.30372.right.i32.left = load i32, ptr %exprEnd.addr.30318
  %ifcond.30372.right.i32.right.left = load i32, ptr %exprStart.addr.30304
  %ifcond.30372.right.i32.right.right = add i32 0, 1
  %ifcond.30372.right.i32.right = add i32 %ifcond.30372.right.i32.right.left, %ifcond.30372.right.i32.right.right
  %ifcond.30372.right.i32.comparison = icmp eq i32 %ifcond.30372.right.i32.left, %ifcond.30372.right.i32.right
  %ifcond.30372.right.i32 = zext i1 %ifcond.30372.right.i32.comparison to i32
  %ifcond.30372.right = icmp ne i32 %ifcond.30372.right.i32, 0
  %ifcond.30372 = and i1 %ifcond.30372.left, %ifcond.30372.right
  br i1 %ifcond.30372, label %if.then.30372, label %if.else.30372
if.then.30372:
  %exprStorageName.addr.30392 = alloca ptr
  %exprStorageName.init.30392.arg0 = load ptr, ptr %tokens
  %exprStorageName.init.30392.arg1 = load i32, ptr %exprStart.addr.30304
  %exprStorageName.init.30392.arg2.arg0 = load ptr, ptr %tokens
  %exprStorageName.init.30392.arg2.arg1 = load i32, ptr %exprStart.addr.30304
  %exprStorageName.init.30392.arg2 = call ptr @tokenTextAt(ptr %exprStorageName.init.30392.arg2.arg0, i32 %exprStorageName.init.30392.arg2.arg1)
  %exprStorageName.init.30392 = call ptr @lookupVisibleStorageName(ptr %exprStorageName.init.30392.arg0, i32 %exprStorageName.init.30392.arg1, ptr %exprStorageName.init.30392.arg2)
  store ptr %exprStorageName.init.30392, ptr %exprStorageName.addr.30392
  %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left.left = getelementptr inbounds [4 x i8], ptr @.str.30412, i32 0, i32 0
  %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left.right = load ptr, ptr %name.addr.30278
  %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.30416, i32 0, i32 0
  %ret.30411.left.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.left.left.left.left.right.i32 = load i32, ptr %start
  %ret.30411.left.left.left.left.left.left.left.left.left.left.right.i64 = sext i32 %ret.30411.left.left.left.left.left.left.left.left.left.left.right.i32 to i64
  %ret.30411.left.left.left.left.left.left.left.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.30411.left.left.left.left.left.left.left.left.left.left.right.i64)
  %ret.30411.left.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.left.left.left.right = getelementptr inbounds [18 x i8], ptr @.str.30420, i32 0, i32 0
  %ret.30411.left.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.left.left.right = load ptr, ptr %exprStorageName.addr.30392
  %ret.30411.left.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.left.right = getelementptr inbounds [2 x i8], ptr @.str.30424, i32 0, i32 0
  %ret.30411.left.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.left.right = getelementptr inbounds [13 x i8], ptr @.str.30426, i32 0, i32 0
  %ret.30411.left.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.left.right = load ptr, ptr %name.addr.30278
  %ret.30411.left.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left.left, ptr %ret.30411.left.left.left.left.left.right)
  %ret.30411.left.left.left.left.right = getelementptr inbounds [10 x i8], ptr @.str.30430, i32 0, i32 0
  %ret.30411.left.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left.left, ptr %ret.30411.left.left.left.left.right)
  %ret.30411.left.left.left.right.i32 = load i32, ptr %start
  %ret.30411.left.left.left.right.i64 = sext i32 %ret.30411.left.left.left.right.i32 to i64
  %ret.30411.left.left.left.right = call ptr @csec_to_string_i64(i64 %ret.30411.left.left.left.right.i64)
  %ret.30411.left.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left.left, ptr %ret.30411.left.left.left.right)
  %ret.30411.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.30434, i32 0, i32 0
  %ret.30411.left.left = call ptr @csec_string_concat(ptr %ret.30411.left.left.left, ptr %ret.30411.left.left.right)
  %ret.30411.left.right = load ptr, ptr %storageName.addr.30290
  %ret.30411.left = call ptr @csec_string_concat(ptr %ret.30411.left.left, ptr %ret.30411.left.right)
  %ret.30411.right = getelementptr inbounds [2 x i8], ptr @.str.30438, i32 0, i32 0
  %ret.30411 = call ptr @csec_string_concat(ptr %ret.30411.left, ptr %ret.30411.right)
  ret ptr %ret.30411
if.else.30372:
  br label %if.end.30372
if.end.30372:
  %ret.30739 = getelementptr inbounds [1 x i8], ptr @.str.30740, i32 0, i32 0
  ret ptr %ret.30739
}

define ptr @generateLLVMAssignmentI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %kind.check, label %empty.return
kind.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %assignment.check, label %empty.return
assignment.check:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %equals = call i1 @strEq(ptr %op.text, ptr @.str.assignment.i32.equals)
  br i1 %equals, label %assignment.return, label %arrow.check
arrow.check:
  %arrow = call i1 @strEq(ptr %op.text, ptr @.str.assignment.i32.arrow)
  br i1 %arrow, label %assignment.return, label %empty.return
assignment.return:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %a1 = call ptr @csec_string_concat(ptr @.str.assignment.i32.percent, ptr %name)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.assignment.i32.result)
  %result.name = call ptr @csec_string_concat(ptr %a2, ptr %start.text)
  %rhs.start = add i32 %op, 1
  %expression = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %rhs.start, i32 %arg.end, ptr %result.name)
  %s1 = call ptr @csec_string_concat(ptr %expression, ptr @.str.assignment.i32.store)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr %result.name)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr @.str.assignment.i32.to)
  %assignment.result = call ptr @csec_string_concat(ptr %s3, ptr %storage)
  ret ptr %assignment.result
empty.return:
  ret ptr @.str.assignment.i32.empty
}

@.str.assignment.i32.equals = private unnamed_addr constant [2 x i8] c"=\00"
@.str.assignment.i32.arrow = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.assignment.i32.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.assignment.i32.result = private unnamed_addr constant [9 x i8] c".assign.\00"
@.str.assignment.i32.store = private unnamed_addr constant [13 x i8] c"  store i32 \00"
@.str.assignment.i32.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.assignment.i32.empty = private unnamed_addr constant [1 x i8] c"\00"
define ptr @generateLLVMAssignmentI64(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %kind.check, label %empty.return
kind.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %assignment.check, label %empty.return
assignment.check:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %equals = call i1 @strEq(ptr %op.text, ptr @.str.assignment.i64.equals)
  br i1 %equals, label %assignment.return, label %arrow.check
arrow.check:
  %arrow = call i1 @strEq(ptr %op.text, ptr @.str.assignment.i64.arrow)
  br i1 %arrow, label %assignment.return, label %empty.return
assignment.return:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %a1 = call ptr @csec_string_concat(ptr @.str.assignment.i64.percent, ptr %name)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.assignment.i64.result)
  %result.name = call ptr @csec_string_concat(ptr %a2, ptr %start.text)
  %rhs.start = add i32 %op, 1
  %expression = call ptr @generateLLVMExpressionI64(ptr %arg.tokens, i32 %rhs.start, i32 %arg.end, ptr %result.name)
  %s1 = call ptr @csec_string_concat(ptr %expression, ptr @.str.assignment.i64.store)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr %result.name)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr @.str.assignment.i64.to)
  %assignment.result = call ptr @csec_string_concat(ptr %s3, ptr %storage)
  ret ptr %assignment.result
empty.return:
  ret ptr @.str.assignment.i64.empty
}

@.str.assignment.i64.equals = private unnamed_addr constant [2 x i8] c"=\00"
@.str.assignment.i64.arrow = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.assignment.i64.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.assignment.i64.result = private unnamed_addr constant [10 x i8] c".lassign.\00"
@.str.assignment.i64.store = private unnamed_addr constant [13 x i8] c"  store i64 \00"
@.str.assignment.i64.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.assignment.i64.empty = private unnamed_addr constant [1 x i8] c"\00"
define ptr @generateLLVMConditionI1(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %arg.start, i8 %operator.kind, ptr @.str.condition.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %paren.close, label %not.check
paren.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr @.str.condition.open, ptr @.str.condition.close)
  %last = sub i32 %end, 1
  %wrapped = icmp eq i32 %close, %last
  br i1 %wrapped, label %paren.return, label %not.check
paren.return:
  %inner.start = add i32 %arg.start, 1
  %inner = call ptr @generateLLVMConditionI1(ptr %arg.tokens, i32 %inner.start, i32 %last, ptr %arg.resultName)
  ret ptr %inner
not.check:
  %not.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %arg.start, i8 %operator.kind, ptr @.str.condition.not)
  %not = icmp ne i32 %not.raw, 0
  br i1 %not, label %not.return, label %comparison.find
not.return:
  %not.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.condition.not.name)
  %not.start = add i32 %arg.start, 1
  %not.code = call ptr @generateLLVMConditionI1(ptr %arg.tokens, i32 %not.start, i32 %end, ptr %not.name)
  %n1 = call ptr @csec_string_concat(ptr %not.code, ptr @.str.condition.prefix)
  %n2 = call ptr @csec_string_concat(ptr %n1, ptr %arg.resultName)
  %n3 = call ptr @csec_string_concat(ptr %n2, ptr @.str.condition.xor)
  %not.result = call ptr @csec_string_concat(ptr %n3, ptr %not.name)
  ret ptr %not.result
comparison.find:
  %op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end, i32 4)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %comparison.return, label %bool.check
comparison.return:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %mnemonic = call ptr @irOperatorName(ptr %op.text)
  %mnemonic.length.i64 = call i64 @csec_string_length(ptr %mnemonic)
  %mnemonic.length = trunc i64 %mnemonic.length.i64 to i32
  %predicate.length = sub i32 %mnemonic.length, 5
  %predicate = call ptr @csec_string_substring(ptr %mnemonic, i32 5, i32 %predicate.length)
  %left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.condition.left)
  %left = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arg.start, i32 %op, ptr %left.name)
  %right.start = add i32 %op, 1
  %right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.condition.right)
  %right = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %right.start, i32 %end, ptr %right.name)
  %c1 = call ptr @csec_string_concat(ptr %left, ptr %right)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.condition.prefix)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %arg.resultName)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr @.str.condition.icmp)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr %predicate)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr @.str.condition.i32)
  %c7 = call ptr @csec_string_concat(ptr %c6, ptr %left.name)
  %c8 = call ptr @csec_string_concat(ptr %c7, ptr @.str.condition.comma)
  %comparison.result = call ptr @csec_string_concat(ptr %c8, ptr %right.name)
  ret ptr %comparison.result
bool.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %bool.kind = call i8 @kindBool()
  %is.bool = icmp eq i8 %kind, %bool.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %bool = and i1 %is.bool, %single
  br i1 %bool, label %bool.return, label %identifier.check
bool.return:
  %bool.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %is.true = call i1 @strEq(ptr %bool.text, ptr @.str.condition.true)
  br i1 %is.true, label %true.return, label %false.return
true.return:
  %t1 = call ptr @csec_string_concat(ptr @.str.condition.prefix, ptr %arg.resultName)
  %true.result = call ptr @csec_string_concat(ptr %t1, ptr @.str.condition.true.code)
  ret ptr %true.result
false.return:
  %f1 = call ptr @csec_string_concat(ptr @.str.condition.prefix, ptr %arg.resultName)
  %false.result = call ptr @csec_string_concat(ptr %f1, ptr @.str.condition.false.code)
  ret ptr %false.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %fallback
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.return, label %call.check
identifier.return:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %i1 = call ptr @csec_string_concat(ptr @.str.condition.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.condition.load)
  %identifier.result = call ptr @csec_string_concat(ptr %i2, ptr %storage)
  ret ptr %identifier.result
call.check:
  %open.index = add i32 %arg.start, 1
  %before.end = icmp slt i32 %open.index, %end
  br i1 %before.end, label %call.open.check, label %fallback
call.open.check:
  %call.open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.condition.open)
  %call.open = icmp ne i32 %call.open.raw, 0
  br i1 %call.open, label %call.close.check, label %fallback
call.close.check:
  %call.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.condition.open, ptr @.str.condition.close)
  %call.last = sub i32 %end, 1
  %is.call = icmp eq i32 %call.close, %call.last
  br i1 %is.call, label %call.return, label %fallback
call.return:
  %args.start = add i32 %arg.start, 2
  %loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %arg.resultName)
  %arguments = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %call.last, ptr %arg.resultName)
  %callee = call ptr @llvmRuntimeCallName(ptr %name)
  %r1 = call ptr @csec_string_concat(ptr %loads, ptr @.str.condition.prefix)
  %r2 = call ptr @csec_string_concat(ptr %r1, ptr %arg.resultName)
  %r3 = call ptr @csec_string_concat(ptr %r2, ptr @.str.condition.call)
  %r4 = call ptr @csec_string_concat(ptr %r3, ptr %callee)
  %r5 = call ptr @csec_string_concat(ptr %r4, ptr @.str.condition.open)
  %r6 = call ptr @csec_string_concat(ptr %r5, ptr %arguments)
  %call.result = call ptr @csec_string_concat(ptr %r6, ptr @.str.condition.close.nl)
  ret ptr %call.result
fallback:
  %x1 = call ptr @csec_string_concat(ptr @.str.condition.prefix, ptr %arg.resultName)
  %fallback.result = call ptr @csec_string_concat(ptr %x1, ptr @.str.condition.fallback)
  ret ptr %fallback.result
}

@.str.condition.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.condition.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.condition.not = private unnamed_addr constant [2 x i8] c"!\00"
@.str.condition.not.name = private unnamed_addr constant [5 x i8] c".not\00"
@.str.condition.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.condition.xor = private unnamed_addr constant [11 x i8] c" = xor i1 \00"
@.str.condition.left = private unnamed_addr constant [10 x i8] c".left.i32\00"
@.str.condition.right = private unnamed_addr constant [11 x i8] c".right.i32\00"
@.str.condition.icmp = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.condition.i32 = private unnamed_addr constant [6 x i8] c" i32 \00"
@.str.condition.comma = private unnamed_addr constant [3 x i8] c", \00"
@.str.condition.true = private unnamed_addr constant [5 x i8] c"true\00"
@.str.condition.true.code = private unnamed_addr constant [26 x i8] c" = icmp eq i1 true, true\0A\00"
@.str.condition.false.code = private unnamed_addr constant [27 x i8] c" = icmp eq i1 false, true\0A\00"
@.str.condition.load = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.condition.call = private unnamed_addr constant [13 x i8] c" = call i1 @\00"
@.str.condition.close.nl = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.condition.fallback = private unnamed_addr constant [21 x i8] c" = icmp eq i32 0, 0\0A\00"
define ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %false.return, label %paren.check
false.return:
  %f1 = call ptr @csec_string_concat(ptr @.str.expr.i1.prefix, ptr %arg.resultName)
  %false.result = call ptr @csec_string_concat(ptr %f1, ptr @.str.expr.i1.false)
  ret ptr %false.result
paren.check:
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %arg.start, i8 %operator.kind, ptr @.str.expr.i1.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %paren.close, label %not.check
paren.close:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr @.str.expr.i1.open, ptr @.str.expr.i1.close)
  %last = sub i32 %end, 1
  %wrapped = icmp eq i32 %close, %last
  br i1 %wrapped, label %paren.return, label %not.check
paren.return:
  %inner.start = add i32 %arg.start, 1
  %inner = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %inner.start, i32 %last, ptr %arg.resultName)
  ret ptr %inner
not.check:
  %not.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %arg.start, i8 %operator.kind, ptr @.str.expr.i1.not)
  %not = icmp ne i32 %not.raw, 0
  br i1 %not, label %not.return, label %bool.check
not.return:
  %not.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i1.not.name)
  %not.start = add i32 %arg.start, 1
  %not.code = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %not.start, i32 %end, ptr %not.name)
  %n1 = call ptr @csec_string_concat(ptr %not.code, ptr @.str.expr.i1.prefix)
  %n2 = call ptr @csec_string_concat(ptr %n1, ptr %arg.resultName)
  %n3 = call ptr @csec_string_concat(ptr %n2, ptr @.str.expr.i1.xor)
  %n4 = call ptr @csec_string_concat(ptr %n3, ptr %not.name)
  %not.result = call ptr @csec_string_concat(ptr %n4, ptr @.str.expr.i1.not.suffix)
  ret ptr %not.result
bool.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %bool.kind = call i8 @kindBool()
  %is.bool = icmp eq i8 %kind, %bool.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %bool = and i1 %is.bool, %single
  br i1 %bool, label %bool.return, label %identifier.check
bool.return:
  %bool.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %is.true = call i1 @strEq(ptr %bool.text, ptr @.str.expr.i1.true)
  br i1 %is.true, label %true.return, label %false.value.return
true.return:
  %t1 = call ptr @csec_string_concat(ptr @.str.expr.i1.prefix, ptr %arg.resultName)
  %true.result = call ptr @csec_string_concat(ptr %t1, ptr @.str.expr.i1.true.code)
  ret ptr %true.result
false.value.return:
  %v1 = call ptr @csec_string_concat(ptr @.str.expr.i1.prefix, ptr %arg.resultName)
  %value.false.result = call ptr @csec_string_concat(ptr %v1, ptr @.str.expr.i1.false)
  ret ptr %value.false.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  %identifier = and i1 %is.identifier, %single
  br i1 %identifier, label %identifier.return, label %or.find
identifier.return:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %i1 = call ptr @csec_string_concat(ptr @.str.expr.i1.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.expr.i1.load)
  %identifier.result = call ptr @csec_string_concat(ptr %i2, ptr %storage)
  ret ptr %identifier.result
or.find:
  %or.op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end, i32 2)
  %has.or = icmp sgt i32 %or.op, %arg.start
  br i1 %has.or, label %or.return, label %and.find
or.return:
  %or.left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i1.left)
  %or.left = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %arg.start, i32 %or.op, ptr %or.left.name)
  %or.right.start = add i32 %or.op, 1
  %or.right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i1.right)
  %or.right = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %or.right.start, i32 %end, ptr %or.right.name)
  %o1 = call ptr @csec_string_concat(ptr %or.left, ptr %or.right)
  %o2 = call ptr @csec_string_concat(ptr %o1, ptr @.str.expr.i1.prefix)
  %o3 = call ptr @csec_string_concat(ptr %o2, ptr %arg.resultName)
  %o4 = call ptr @csec_string_concat(ptr %o3, ptr @.str.expr.i1.or)
  %o5 = call ptr @csec_string_concat(ptr %o4, ptr %or.left.name)
  %o6 = call ptr @csec_string_concat(ptr %o5, ptr @.str.expr.i1.comma)
  %or.result = call ptr @csec_string_concat(ptr %o6, ptr %or.right.name)
  ret ptr %or.result
and.find:
  %and.op = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end, i32 3)
  %has.and = icmp sgt i32 %and.op, %arg.start
  br i1 %has.and, label %and.return, label %condition.return
and.return:
  %and.left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i1.left)
  %and.left = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %arg.start, i32 %and.op, ptr %and.left.name)
  %and.right.start = add i32 %and.op, 1
  %and.right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.i1.right)
  %and.right = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %and.right.start, i32 %end, ptr %and.right.name)
  %a1 = call ptr @csec_string_concat(ptr %and.left, ptr %and.right)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.expr.i1.prefix)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %arg.resultName)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.expr.i1.and)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %and.left.name)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.expr.i1.comma)
  %and.result = call ptr @csec_string_concat(ptr %a6, ptr %and.right.name)
  ret ptr %and.result
condition.return:
  %condition = call ptr @generateLLVMConditionI1(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr %arg.resultName)
  ret ptr %condition
}

@.str.expr.i1.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.expr.i1.false = private unnamed_addr constant [27 x i8] c" = icmp eq i1 false, true\0A\00"
@.str.expr.i1.true = private unnamed_addr constant [5 x i8] c"true\00"
@.str.expr.i1.true.code = private unnamed_addr constant [26 x i8] c" = icmp eq i1 true, true\0A\00"
@.str.expr.i1.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.expr.i1.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.expr.i1.not = private unnamed_addr constant [2 x i8] c"!\00"
@.str.expr.i1.not.name = private unnamed_addr constant [5 x i8] c".not\00"
@.str.expr.i1.xor = private unnamed_addr constant [11 x i8] c" = xor i1 \00"
@.str.expr.i1.not.suffix = private unnamed_addr constant [8 x i8] c", true\0A\00"
@.str.expr.i1.load = private unnamed_addr constant [18 x i8] c" = load i1, ptr %\00"
@.str.expr.i1.left = private unnamed_addr constant [6 x i8] c".left\00"
@.str.expr.i1.right = private unnamed_addr constant [7 x i8] c".right\00"
@.str.expr.i1.or = private unnamed_addr constant [10 x i8] c" = or i1 \00"
@.str.expr.i1.and = private unnamed_addr constant [11 x i8] c" = and i1 \00"
@.str.expr.i1.comma = private unnamed_addr constant [3 x i8] c", \00"
define ptr @generateLLVMLocalI1(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.i1.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.i1.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.i1.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %with.initializer, label %without.initializer
with.initializer:
  %expr.start = add i32 %initializer, 1
  %expr = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %expr.start, i32 %arg.end, ptr %init)
  %a1 = call ptr @csec_string_concat(ptr @.str.local.i1.prefix, ptr %storage)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.local.i1.alloca)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %expr)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.local.i1.store)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %init)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.local.i1.to)
  %result = call ptr @csec_string_concat(ptr %a6, ptr %storage)
  ret ptr %result
without.initializer:
  %b1 = call ptr @csec_string_concat(ptr @.str.local.i1.prefix, ptr %storage)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.local.i1.alloca)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.local.i1.false)
  %fallback = call ptr @csec_string_concat(ptr %b3, ptr %storage)
  ret ptr %fallback
}

@.str.local.i1.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.i1.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.i1.init = private unnamed_addr constant [8 x i8] c".binit.\00"
@.str.local.i1.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.i1.alloca = private unnamed_addr constant [14 x i8] c" = alloca i1\0A\00"
@.str.local.i1.store = private unnamed_addr constant [12 x i8] c"  store i1 \00"
@.str.local.i1.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.i1.false = private unnamed_addr constant [24 x i8] c"  store i1 false, ptr %\00"
define ptr @generateLLVMAssignmentI1(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %end = alloca i32
  store i32 %arg.end, ptr %end
  %op.addr.33258 = alloca i32
  %op.init.33258.arg0 = load ptr, ptr %tokens
  %op.init.33258.arg1 = load i32, ptr %start
  %op.init.33258.arg2 = load i32, ptr %end
  %op.init.33258.arg3 = add i32 0, 1
  %op.init.33258 = call i32 @findTopLevelOperator(ptr %op.init.33258.arg0, i32 %op.init.33258.arg1, i32 %op.init.33258.arg2, i32 %op.init.33258.arg3)
  store i32 %op.init.33258, ptr %op.addr.33258
  %ifcond.33274.left.i32.left = load i32, ptr %op.addr.33258
  %ifcond.33274.left.i32.right = load i32, ptr %start
  %ifcond.33274.left.i32.comparison = icmp sle i32 %ifcond.33274.left.i32.left, %ifcond.33274.left.i32.right
  %ifcond.33274.left.i32 = zext i1 %ifcond.33274.left.i32.comparison to i32
  %ifcond.33274.left = icmp ne i32 %ifcond.33274.left.i32, 0
  %ifcond.33274.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.33274.right.i32.left.char.arg0 = trunc i32 %ifcond.33274.right.i32.left.char.arg0.i32 to i8
  %ifcond.33274.right.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.33274.right.i32.left.char.arg1 = trunc i32 %ifcond.33274.right.i32.left.char.arg1.i32 to i8
  %ifcond.33274.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.33274.right.i32.left.char.arg0, i8 %ifcond.33274.right.i32.left.char.arg1)
  %ifcond.33274.right.i32.left = zext i8 %ifcond.33274.right.i32.left.char to i32
  %ifcond.33274.right.i32.right.char = call i8 @kindIdentifier()
  %ifcond.33274.right.i32.right = zext i8 %ifcond.33274.right.i32.right.char to i32
  %ifcond.33274.right.i32.comparison = icmp ne i32 %ifcond.33274.right.i32.left, %ifcond.33274.right.i32.right
  %ifcond.33274.right.i32 = zext i1 %ifcond.33274.right.i32.comparison to i32
  %ifcond.33274.right = icmp ne i32 %ifcond.33274.right.i32, 0
  %ifcond.33274 = or i1 %ifcond.33274.left, %ifcond.33274.right
  br i1 %ifcond.33274, label %if.then.33274, label %if.else.33274
if.then.33274:
  %ret.33292 = getelementptr inbounds [1 x i8], ptr @.str.33293, i32 0, i32 0
  ret ptr %ret.33292
if.else.33274:
  br label %if.end.33274
if.end.33274:
  %name.addr.33296 = alloca ptr
  %name.init.33296.arg0 = load ptr, ptr %tokens
  %name.init.33296.arg1 = load i32, ptr %start
  %name.init.33296 = call ptr @tokenTextAt(ptr %name.init.33296.arg0, i32 %name.init.33296.arg1)
  store ptr %name.init.33296, ptr %name.addr.33296
  %storageName.addr.33308 = alloca ptr
  %storageName.init.33308.arg0 = load ptr, ptr %tokens
  %storageName.init.33308.arg1 = load i32, ptr %start
  %storageName.init.33308.arg2 = load ptr, ptr %name.addr.33296
  %storageName.init.33308 = call ptr @lookupVisibleStorageName(ptr %storageName.init.33308.arg0, i32 %storageName.init.33308.arg1, ptr %storageName.init.33308.arg2)
  store ptr %storageName.init.33308, ptr %storageName.addr.33308
  %opText.addr.33322 = alloca ptr
  %opText.init.33322.arg0 = load ptr, ptr %tokens
  %opText.init.33322.arg1 = load i32, ptr %op.addr.33258
  %opText.init.33322 = call ptr @tokenTextAt(ptr %opText.init.33322.arg0, i32 %opText.init.33322.arg1)
  store ptr %opText.init.33322, ptr %opText.addr.33322
  %ifcond.33334.left.arg0 = load ptr, ptr %opText.addr.33322
  %ifcond.33334.left.arg1 = getelementptr inbounds [2 x i8], ptr @.str.33340, i32 0, i32 0
  %ifcond.33334.left = call i1 @strEq(ptr %ifcond.33334.left.arg0, ptr %ifcond.33334.left.arg1)
  %ifcond.33334.right.arg0 = load ptr, ptr %opText.addr.33322
  %ifcond.33334.right.arg1 = getelementptr inbounds [3 x i8], ptr @.str.33347, i32 0, i32 0
  %ifcond.33334.right = call i1 @strEq(ptr %ifcond.33334.right.arg0, ptr %ifcond.33334.right.arg1)
  %ifcond.33334 = or i1 %ifcond.33334.left, %ifcond.33334.right
  br i1 %ifcond.33334, label %if.then.33334, label %if.else.33334
if.then.33334:
  %resultName.addr.33351 = alloca ptr
  %resultName.init.33351.left.left.left = getelementptr inbounds [2 x i8], ptr @.str.33356, i32 0, i32 0
  %resultName.init.33351.left.left.right = load ptr, ptr %name.addr.33296
  %resultName.init.33351.left.left = call ptr @csec_string_concat(ptr %resultName.init.33351.left.left.left, ptr %resultName.init.33351.left.left.right)
  %resultName.init.33351.left.right = getelementptr inbounds [10 x i8], ptr @.str.33360, i32 0, i32 0
  %resultName.init.33351.left = call ptr @csec_string_concat(ptr %resultName.init.33351.left.left, ptr %resultName.init.33351.left.right)
  %resultName.init.33351.right.i32 = load i32, ptr %start
  %resultName.init.33351.right.i64 = sext i32 %resultName.init.33351.right.i32 to i64
  %resultName.init.33351.right = call ptr @csec_to_string_i64(i64 %resultName.init.33351.right.i64)
  %resultName.init.33351 = call ptr @csec_string_concat(ptr %resultName.init.33351.left, ptr %resultName.init.33351.right)
  store ptr %resultName.init.33351, ptr %resultName.addr.33351
  %ret.33364.left.left.left.left.left.arg0 = load ptr, ptr %tokens
  %ret.33364.left.left.left.left.left.arg1.left = load i32, ptr %op.addr.33258
  %ret.33364.left.left.left.left.left.arg1.right = add i32 0, 1
  %ret.33364.left.left.left.left.left.arg1 = add i32 %ret.33364.left.left.left.left.left.arg1.left, %ret.33364.left.left.left.left.left.arg1.right
  %ret.33364.left.left.left.left.left.arg2 = load i32, ptr %end
  %ret.33364.left.left.left.left.left.arg3 = load ptr, ptr %resultName.addr.33351
  %ret.33364.left.left.left.left.left = call ptr @generateLLVMExpressionI1(ptr %ret.33364.left.left.left.left.left.arg0, i32 %ret.33364.left.left.left.left.left.arg1, i32 %ret.33364.left.left.left.left.left.arg2, ptr %ret.33364.left.left.left.left.left.arg3)
  %ret.33364.left.left.left.left.right = getelementptr inbounds [12 x i8], ptr @.str.33378, i32 0, i32 0
  %ret.33364.left.left.left.left = call ptr @csec_string_concat(ptr %ret.33364.left.left.left.left.left, ptr %ret.33364.left.left.left.left.right)
  %ret.33364.left.left.left.right = load ptr, ptr %resultName.addr.33351
  %ret.33364.left.left.left = call ptr @csec_string_concat(ptr %ret.33364.left.left.left.left, ptr %ret.33364.left.left.left.right)
  %ret.33364.left.left.right = getelementptr inbounds [8 x i8], ptr @.str.33382, i32 0, i32 0
  %ret.33364.left.left = call ptr @csec_string_concat(ptr %ret.33364.left.left.left, ptr %ret.33364.left.left.right)
  %ret.33364.left.right = load ptr, ptr %storageName.addr.33308
  %ret.33364.left = call ptr @csec_string_concat(ptr %ret.33364.left.left, ptr %ret.33364.left.right)
  %ret.33364.right = getelementptr inbounds [2 x i8], ptr @.str.33386, i32 0, i32 0
  %ret.33364 = call ptr @csec_string_concat(ptr %ret.33364.left, ptr %ret.33364.right)
  ret ptr %ret.33364
if.else.33334:
  br label %if.end.33334
if.end.33334:
  %ret.33389 = getelementptr inbounds [1 x i8], ptr @.str.33390, i32 0, i32 0
  ret ptr %ret.33389
}

define i1 @llvmBlockEndsWithTopLevelReturn(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %first = call i32 @skipTrivia(ptr %arg.tokens, i32 %arg.bodyStart)
  br label %loop
loop:
  %cursor = phi i32 [ %first, %entry ], [ %next.cursor, %body ]
  %result = phi i1 [ false, %entry ], [ %is.return, %body ]
  %before.end = icmp slt i32 %cursor, %arg.bodyEnd
  br i1 %before.end, label %eof.check, label %done
eof.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %cursor)
  %eof.kind = call i8 @kindEof()
  %is.eof = icmp eq i8 %kind, %eof.kind
  br i1 %is.eof, label %done, label %body
body:
  %next = call i32 @advanceStatement(ptr %arg.tokens, i32 %cursor, i32 %arg.bodyEnd)
  %statement = call ptr @statementKind(ptr %arg.tokens, i32 %cursor)
  %is.return = call i1 @strEq(ptr %statement, ptr @.str.block.return)
  %next.cursor = call i32 @skipTrivia(ptr %arg.tokens, i32 %next)
  br label %loop
done:
  ret i1 %result
}

@.str.block.return = private unnamed_addr constant [7 x i8] c"return\00"
define ptr @csecGenerateLLVMStringOperand(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %string.kind = call i8 @kindString()
  %is.string = icmp eq i8 %kind, %string.kind
  %string = and i1 %is.string, %single
  br i1 %string, label %ptr.return, label %identifier.check
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %char.check
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.single, label %identifier.complex
identifier.single:
  %type = call ptr @lookupVisibleValueType(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %ir.type = call ptr @irTypeName(ptr %type)
  %ptr = call i1 @strEq(ptr %ir.type, ptr @.str.expr.ptr.ptr)
  br i1 %ptr, label %ptr.return, label %identifier.char.check
identifier.char.check:
  %character = call i1 @strEq(ptr %type, ptr @.str.expr.ptr.char)
  br i1 %character, label %char.identifier, label %i32.return
identifier.complex:
  %open.index = add i32 %arg.start, 1
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.ptr.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %call.close.check, label %substring.check
call.close.check:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.expr.ptr.open, ptr @.str.expr.ptr.close)
  %last = sub i32 %end, 1
  %is.call = icmp eq i32 %close, %last
  br i1 %is.call, label %call.type.check, label %substring.check
call.type.check:
  %return.type = call ptr @lookupFunctionReturnType(ptr %arg.tokens, ptr %name)
  %call.ir.type = call ptr @irTypeName(ptr %return.type)
  %call.ptr = call i1 @strEq(ptr %call.ir.type, ptr @.str.expr.ptr.ptr)
  br i1 %call.ptr, label %ptr.return, label %i32.return
substring.check:
  %dot.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.ptr.dot)
  %dot = icmp ne i32 %dot.raw, 0
  br i1 %dot, label %substring.name.check, label %plus.check
substring.name.check:
  %substring.index = add i32 %arg.start, 2
  %substring.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %substring.index, i8 %identifier.kind, ptr @.str.expr.ptr.substring)
  %substring = icmp ne i32 %substring.raw, 0
  br i1 %substring, label %ptr.return, label %plus.check
char.check:
  %char.kind = call i8 @kindChar()
  %is.char = icmp eq i8 %kind, %char.kind
  %char = and i1 %is.char, %single
  br i1 %char, label %char.literal, label %plus.check
char.literal:
  %text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %value = call ptr @llvmCharI8Value(ptr %text)
  %c1 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr @.str.expr.ptr.char.call)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr %text)
  %char.result = call ptr @csec_string_concat(ptr %c3, ptr @.str.expr.ptr.char.close)
  ret ptr %char.result
char.identifier:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %c4 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr @.str.expr.ptr.char.load)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr %storage)
  %c7 = call ptr @csec_string_concat(ptr %c6, ptr @.str.expr.ptr.char.nl)
  %c8 = call ptr @csec_string_concat(ptr %c7, ptr @.str.expr.ptr.prefix)
  %c9 = call ptr @csec_string_concat(ptr %c8, ptr %arg.resultName)
  %c10 = call ptr @csec_string_concat(ptr %c9, ptr @.str.expr.ptr.char.call)
  %c11 = call ptr @csec_string_concat(ptr %c10, ptr %arg.resultName)
  %char.identifier.result = call ptr @csec_string_concat(ptr %c11, ptr @.str.expr.ptr.char.value.close)
  ret ptr %char.identifier.result
plus.check:
  %plus = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %has.plus = icmp sgt i32 %plus, %arg.start
  br i1 %has.plus, label %plus.text.check, label %i32.return
plus.text.check:
  %plus.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %plus)
  %is.plus = call i1 @strEq(ptr %plus.text, ptr @.str.expr.ptr.plus)
  br i1 %is.plus, label %ptr.return, label %i32.return
ptr.return:
  %ptr.code = call ptr @generateLLVMExpressionPtr(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr %arg.resultName)
  ret ptr %ptr.code
i32.return:
  %i32.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.i32)
  %i32.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arg.start, i32 %end, ptr %i32.name)
  %i64.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.i64)
  %n1 = call ptr @csec_string_concat(ptr %i32.code, ptr @.str.expr.ptr.prefix)
  %n2 = call ptr @csec_string_concat(ptr %n1, ptr %i64.name)
  %n3 = call ptr @csec_string_concat(ptr %n2, ptr @.str.expr.ptr.sext)
  %n4 = call ptr @csec_string_concat(ptr %n3, ptr %i32.name)
  %n5 = call ptr @csec_string_concat(ptr %n4, ptr @.str.expr.ptr.to.i64)
  %n6 = call ptr @csec_string_concat(ptr %n5, ptr @.str.expr.ptr.prefix)
  %n7 = call ptr @csec_string_concat(ptr %n6, ptr %arg.resultName)
  %n8 = call ptr @csec_string_concat(ptr %n7, ptr @.str.expr.ptr.i64.call)
  %n9 = call ptr @csec_string_concat(ptr %n8, ptr %i64.name)
  %numeric.result = call ptr @csec_string_concat(ptr %n9, ptr @.str.expr.ptr.close.nl)
  ret ptr %numeric.result
}

define ptr @generateLLVMExpressionPtr(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd, ptr %arg.resultName) {
entry:
  %end = call i32 @trimExpressionEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd)
  %empty = icmp sle i32 %end, %arg.start
  br i1 %empty, label %empty.return, label %string.check
empty.return:
  %e1 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %empty.result = call ptr @csec_string_concat(ptr %e1, ptr @.str.expr.ptr.null)
  ret ptr %empty.result
string.check:
  %kind = call i8 @tokenKindAt(ptr %arg.tokens, i32 %arg.start)
  %string.kind = call i8 @kindString()
  %is.string = icmp eq i8 %kind, %string.kind
  %single.end = add i32 %arg.start, 1
  %single = icmp eq i32 %end, %single.end
  %string = and i1 %is.string, %single
  br i1 %string, label %string.return, label %identifier.check
string.return:
  %text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  %bytes = call i32 @csec_llvm_string_literal_byte_length(ptr %text)
  %bytes.i64 = sext i32 %bytes to i64
  %bytes.text = call ptr @csec_to_string_i64(i64 %bytes.i64)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %s1 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %s2 = call ptr @csec_string_concat(ptr %s1, ptr @.str.expr.ptr.gep.a)
  %s3 = call ptr @csec_string_concat(ptr %s2, ptr %bytes.text)
  %s4 = call ptr @csec_string_concat(ptr %s3, ptr @.str.expr.ptr.gep.b)
  %s5 = call ptr @csec_string_concat(ptr %s4, ptr %start.text)
  %string.result = call ptr @csec_string_concat(ptr %s5, ptr @.str.expr.ptr.gep.c)
  ret ptr %string.result
identifier.check:
  %identifier.kind = call i8 @kindIdentifier()
  %is.identifier = icmp eq i8 %kind, %identifier.kind
  br i1 %is.identifier, label %identifier.next, label %plus.find
identifier.next:
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %arg.start)
  br i1 %single, label %identifier.return, label %call.check
identifier.return:
  %storage = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %i1 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %i2 = call ptr @csec_string_concat(ptr %i1, ptr @.str.expr.ptr.load)
  %identifier.result = call ptr @csec_string_concat(ptr %i2, ptr %storage)
  ret ptr %identifier.result
call.check:
  %open.index = add i32 %arg.start, 1
  %operator.kind = call i8 @kindOperator()
  %open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.ptr.open)
  %open = icmp ne i32 %open.raw, 0
  br i1 %open, label %call.close.check, label %substring.check
call.close.check:
  %close = call i32 @findClosingToken(ptr %arg.tokens, i32 %open.index, i32 %end, ptr @.str.expr.ptr.open, ptr @.str.expr.ptr.close)
  %last = sub i32 %end, 1
  %is.call = icmp eq i32 %close, %last
  br i1 %is.call, label %call.return, label %substring.check
call.return:
  %args.start = add i32 %arg.start, 2
  %loads = call ptr @generateLLVMCallArgumentLoadsI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %arguments = call ptr @generateLLVMCallArgumentListI32(ptr %arg.tokens, i32 %args.start, i32 %last, ptr %arg.resultName)
  %callee = call ptr @llvmRuntimeCallName(ptr %name)
  %c1 = call ptr @csec_string_concat(ptr %loads, ptr @.str.expr.ptr.prefix)
  %c2 = call ptr @csec_string_concat(ptr %c1, ptr %arg.resultName)
  %c3 = call ptr @csec_string_concat(ptr %c2, ptr @.str.expr.ptr.call)
  %c4 = call ptr @csec_string_concat(ptr %c3, ptr %callee)
  %c5 = call ptr @csec_string_concat(ptr %c4, ptr @.str.expr.ptr.open)
  %c6 = call ptr @csec_string_concat(ptr %c5, ptr %arguments)
  %call.result = call ptr @csec_string_concat(ptr %c6, ptr @.str.expr.ptr.close.nl)
  ret ptr %call.result
substring.check:
  %dot.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %open.index, i8 %operator.kind, ptr @.str.expr.ptr.dot)
  %dot = icmp ne i32 %dot.raw, 0
  br i1 %dot, label %substring.name.check, label %plus.find
substring.name.check:
  %substring.index = add i32 %arg.start, 2
  %substring.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %substring.index, i8 %identifier.kind, ptr @.str.expr.ptr.substring)
  %substring = icmp ne i32 %substring.raw, 0
  br i1 %substring, label %substring.open.check, label %plus.find
substring.open.check:
  %substring.open.index = add i32 %arg.start, 3
  %substring.open.raw = call i32 @tokenIs(ptr %arg.tokens, i32 %substring.open.index, i8 %operator.kind, ptr @.str.expr.ptr.open)
  %substring.open = icmp ne i32 %substring.open.raw, 0
  br i1 %substring.open, label %substring.close.check, label %plus.find
substring.close.check:
  %substring.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %substring.open.index, i32 %end, ptr @.str.expr.ptr.open, ptr @.str.expr.ptr.close)
  %substring.last = sub i32 %end, 1
  %substring.complete = icmp eq i32 %substring.close, %substring.last
  br i1 %substring.complete, label %substring.comma.find, label %plus.find
substring.comma.find:
  %arguments.start = add i32 %arg.start, 4
  %comma = call i32 @findTokenTextInRange(ptr %arg.tokens, i32 %arguments.start, i32 %substring.last, ptr @.str.expr.ptr.comma)
  %has.comma = icmp sgt i32 %comma, %arguments.start
  br i1 %has.comma, label %substring.return, label %plus.find
substring.return:
  %storage.name = call ptr @lookupVisibleStorageName(ptr %arg.tokens, i32 %arg.start, ptr %name)
  %object.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.object)
  %start.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.start)
  %length.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.length)
  %object.prefix = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %object.name)
  %object.code = call ptr @csec_string_concat(ptr %object.prefix, ptr @.str.expr.ptr.object.load)
  %object.result = call ptr @csec_string_concat(ptr %object.code, ptr %storage.name)
  %start.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %arguments.start, i32 %comma, ptr %start.name)
  %length.start = add i32 %comma, 1
  %length.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %length.start, i32 %substring.last, ptr %length.name)
  %u1 = call ptr @csec_string_concat(ptr %object.result, ptr %start.code)
  %u2 = call ptr @csec_string_concat(ptr %u1, ptr %length.code)
  %u3 = call ptr @csec_string_concat(ptr %u2, ptr @.str.expr.ptr.prefix)
  %u4 = call ptr @csec_string_concat(ptr %u3, ptr %arg.resultName)
  %u5 = call ptr @csec_string_concat(ptr %u4, ptr @.str.expr.ptr.substring.call)
  %u6 = call ptr @csec_string_concat(ptr %u5, ptr %object.name)
  %u7 = call ptr @csec_string_concat(ptr %u6, ptr @.str.expr.ptr.substring.middle)
  %u8 = call ptr @csec_string_concat(ptr %u7, ptr %start.name)
  %u9 = call ptr @csec_string_concat(ptr %u8, ptr @.str.expr.ptr.substring.middle)
  %u10 = call ptr @csec_string_concat(ptr %u9, ptr %length.name)
  %substring.result = call ptr @csec_string_concat(ptr %u10, ptr @.str.expr.ptr.close.nl)
  ret ptr %substring.result
plus.find:
  %op = call i32 @expressionTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %end)
  %has.op = icmp sgt i32 %op, %arg.start
  br i1 %has.op, label %plus.check, label %fallback
plus.check:
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %op)
  %plus = call i1 @strEq(ptr %op.text, ptr @.str.expr.ptr.plus)
  br i1 %plus, label %plus.return, label %fallback
plus.return:
  %left.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.left)
  %left = call ptr @csecGenerateLLVMStringOperand(ptr %arg.tokens, i32 %arg.start, i32 %op, ptr %left.name)
  %right.start = add i32 %op, 1
  %right.name = call ptr @csec_string_concat(ptr %arg.resultName, ptr @.str.expr.ptr.right)
  %right = call ptr @csecGenerateLLVMStringOperand(ptr %arg.tokens, i32 %right.start, i32 %end, ptr %right.name)
  %p1 = call ptr @csec_string_concat(ptr %left, ptr %right)
  %p2 = call ptr @csec_string_concat(ptr %p1, ptr @.str.expr.ptr.prefix)
  %p3 = call ptr @csec_string_concat(ptr %p2, ptr %arg.resultName)
  %p4 = call ptr @csec_string_concat(ptr %p3, ptr @.str.expr.ptr.concat)
  %p5 = call ptr @csec_string_concat(ptr %p4, ptr %left.name)
  %p6 = call ptr @csec_string_concat(ptr %p5, ptr @.str.expr.ptr.concat.middle)
  %p7 = call ptr @csec_string_concat(ptr %p6, ptr %right.name)
  %plus.result = call ptr @csec_string_concat(ptr %p7, ptr @.str.expr.ptr.close.nl)
  ret ptr %plus.result
fallback:
  %f1 = call ptr @csec_string_concat(ptr @.str.expr.ptr.prefix, ptr %arg.resultName)
  %fallback.result = call ptr @csec_string_concat(ptr %f1, ptr @.str.expr.ptr.null)
  ret ptr %fallback.result
}

@.str.expr.ptr.prefix = private unnamed_addr constant [3 x i8] c"  \00"
@.str.expr.ptr.null = private unnamed_addr constant [26 x i8] c" = inttoptr i64 0 to ptr\0A\00"
@.str.expr.ptr.ptr = private unnamed_addr constant [4 x i8] c"ptr\00"
@.str.expr.ptr.char = private unnamed_addr constant [5 x i8] c"Char\00"
@.str.expr.ptr.open = private unnamed_addr constant [2 x i8] c"(\00"
@.str.expr.ptr.close = private unnamed_addr constant [2 x i8] c")\00"
@.str.expr.ptr.close.nl = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.expr.ptr.dot = private unnamed_addr constant [2 x i8] c".\00"
@.str.expr.ptr.substring = private unnamed_addr constant [10 x i8] c"substring\00"
@.str.expr.ptr.plus = private unnamed_addr constant [2 x i8] c"+\00"
@.str.expr.ptr.i32 = private unnamed_addr constant [5 x i8] c".i32\00"
@.str.expr.ptr.i64 = private unnamed_addr constant [5 x i8] c".i64\00"
@.str.expr.ptr.sext = private unnamed_addr constant [13 x i8] c" = sext i32 \00"
@.str.expr.ptr.to.i64 = private unnamed_addr constant [9 x i8] c" to i64\0A\00"
@.str.expr.ptr.i64.call = private unnamed_addr constant [37 x i8] c" = call ptr @csec_to_string_i64(i64 \00"
@.str.expr.ptr.gep.a = private unnamed_addr constant [28 x i8] c" = getelementptr inbounds [\00"
@.str.expr.ptr.gep.b = private unnamed_addr constant [19 x i8] c" x i8], ptr @.str.\00"
@.str.expr.ptr.gep.c = private unnamed_addr constant [16 x i8] c", i32 0, i32 0\0A\00"
@.str.expr.ptr.load = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.expr.ptr.call = private unnamed_addr constant [14 x i8] c" = call ptr @\00"
@.str.expr.ptr.comma = private unnamed_addr constant [2 x i8] c",\00"
@.str.expr.ptr.object = private unnamed_addr constant [5 x i8] c".obj\00"
@.str.expr.ptr.start = private unnamed_addr constant [7 x i8] c".start\00"
@.str.expr.ptr.length = private unnamed_addr constant [8 x i8] c".length\00"
@.str.expr.ptr.object.load = private unnamed_addr constant [19 x i8] c" = load ptr, ptr %\00"
@.str.expr.ptr.substring.call = private unnamed_addr constant [40 x i8] c" = call ptr @csec_string_substring(ptr \00"
@.str.expr.ptr.substring.middle = private unnamed_addr constant [7 x i8] c", i32 \00"
@.str.expr.ptr.left = private unnamed_addr constant [6 x i8] c".left\00"
@.str.expr.ptr.right = private unnamed_addr constant [7 x i8] c".right\00"
@.str.expr.ptr.concat = private unnamed_addr constant [37 x i8] c" = call ptr @csec_string_concat(ptr \00"
@.str.expr.ptr.concat.middle = private unnamed_addr constant [7 x i8] c", ptr \00"
@.str.expr.ptr.char.call = private unnamed_addr constant [37 x i8] c" = call ptr @csec_to_string_char(i8 \00"
@.str.expr.ptr.char.close = private unnamed_addr constant [3 x i8] c")\0A\00"
@.str.expr.ptr.char.load = private unnamed_addr constant [18 x i8] c" = load i8, ptr %\00"
@.str.expr.ptr.char.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.expr.ptr.char.value.close = private unnamed_addr constant [3 x i8] c")\0A\00"
define ptr @generateLLVMLocalPtr(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %initializer = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arg.start, i32 %arg.end, i32 1)
  %name.index = add i32 %arg.start, 1
  %name = call ptr @tokenTextAt(ptr %arg.tokens, i32 %name.index)
  %start.i64 = sext i32 %arg.start to i64
  %start.text = call ptr @csec_to_string_i64(i64 %start.i64)
  %storage.a = call ptr @csec_string_concat(ptr %name, ptr @.str.local.ptr.addr)
  %storage = call ptr @csec_string_concat(ptr %storage.a, ptr %start.text)
  %init.a = call ptr @csec_string_concat(ptr @.str.local.ptr.percent, ptr %name)
  %init.b = call ptr @csec_string_concat(ptr %init.a, ptr @.str.local.ptr.init)
  %init = call ptr @csec_string_concat(ptr %init.b, ptr %start.text)
  %has.initializer = icmp sge i32 %initializer, %arg.start
  br i1 %has.initializer, label %with.initializer, label %without.initializer
with.initializer:
  %expr.start = add i32 %initializer, 1
  %expr = call ptr @generateLLVMExpressionPtr(ptr %arg.tokens, i32 %expr.start, i32 %arg.end, ptr %init)
  %a1 = call ptr @csec_string_concat(ptr @.str.local.ptr.prefix, ptr %storage)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.local.ptr.alloca)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %expr)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.local.ptr.store)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %init)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.local.ptr.to)
  %result = call ptr @csec_string_concat(ptr %a6, ptr %storage)
  ret ptr %result
without.initializer:
  %b1 = call ptr @csec_string_concat(ptr @.str.local.ptr.prefix, ptr %storage)
  %b2 = call ptr @csec_string_concat(ptr %b1, ptr @.str.local.ptr.alloca)
  %b3 = call ptr @csec_string_concat(ptr %b2, ptr @.str.local.ptr.null)
  %fallback = call ptr @csec_string_concat(ptr %b3, ptr %storage)
  ret ptr %fallback
}

@.str.local.ptr.addr = private unnamed_addr constant [7 x i8] c".addr.\00"
@.str.local.ptr.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.local.ptr.init = private unnamed_addr constant [8 x i8] c".pinit.\00"
@.str.local.ptr.prefix = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.local.ptr.alloca = private unnamed_addr constant [15 x i8] c" = alloca ptr\0A\00"
@.str.local.ptr.store = private unnamed_addr constant [13 x i8] c"  store ptr \00"
@.str.local.ptr.to = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.local.ptr.null = private unnamed_addr constant [24 x i8] c"  store ptr null, ptr %\00"
define i1 @llvmExpressionLooksPtr(ptr %arg.tokens, i32 %arg.start, i32 %arg.rawEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %start = alloca i32
  store i32 %arg.start, ptr %start
  %rawEnd = alloca i32
  store i32 %arg.rawEnd, ptr %rawEnd
  %end.addr.35127 = alloca i32
  %end.init.35127.arg0 = load ptr, ptr %tokens
  %end.init.35127.arg1 = load i32, ptr %start
  %end.init.35127.arg2 = load i32, ptr %rawEnd
  %end.init.35127 = call i32 @trimExpressionEnd(ptr %end.init.35127.arg0, i32 %end.init.35127.arg1, i32 %end.init.35127.arg2)
  store i32 %end.init.35127, ptr %end.addr.35127
  %ifcond.35141.i32.left = load i32, ptr %end.addr.35127
  %ifcond.35141.i32.right = load i32, ptr %start
  %ifcond.35141.i32.comparison = icmp sle i32 %ifcond.35141.i32.left, %ifcond.35141.i32.right
  %ifcond.35141.i32 = zext i1 %ifcond.35141.i32.comparison to i32
  %ifcond.35141 = icmp ne i32 %ifcond.35141.i32, 0
  br i1 %ifcond.35141, label %if.then.35141, label %if.else.35141
if.then.35141:
  %ret.35148 = icmp eq i32 0, 1
  ret i1 %ret.35148
if.else.35141:
  br label %if.end.35141
if.end.35141:
  %ifcond.35152.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.35152.left.i32.left.char.arg0 = trunc i32 %ifcond.35152.left.i32.left.char.arg0.i32 to i8
  %ifcond.35152.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.35152.left.i32.left.char.arg1 = trunc i32 %ifcond.35152.left.i32.left.char.arg1.i32 to i8
  %ifcond.35152.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.35152.left.i32.left.char.arg0, i8 %ifcond.35152.left.i32.left.char.arg1)
  %ifcond.35152.left.i32.left = zext i8 %ifcond.35152.left.i32.left.char to i32
  %ifcond.35152.left.i32.right.char = call i8 @kindString()
  %ifcond.35152.left.i32.right = zext i8 %ifcond.35152.left.i32.right.char to i32
  %ifcond.35152.left.i32.comparison = icmp eq i32 %ifcond.35152.left.i32.left, %ifcond.35152.left.i32.right
  %ifcond.35152.left.i32 = zext i1 %ifcond.35152.left.i32.comparison to i32
  %ifcond.35152.left = icmp ne i32 %ifcond.35152.left.i32, 0
  %ifcond.35152.right.i32.left = load i32, ptr %end.addr.35127
  %ifcond.35152.right.i32.right.left = load i32, ptr %start
  %ifcond.35152.right.i32.right.right = add i32 0, 1
  %ifcond.35152.right.i32.right = add i32 %ifcond.35152.right.i32.right.left, %ifcond.35152.right.i32.right.right
  %ifcond.35152.right.i32.comparison = icmp eq i32 %ifcond.35152.right.i32.left, %ifcond.35152.right.i32.right
  %ifcond.35152.right.i32 = zext i1 %ifcond.35152.right.i32.comparison to i32
  %ifcond.35152.right = icmp ne i32 %ifcond.35152.right.i32, 0
  %ifcond.35152 = and i1 %ifcond.35152.left, %ifcond.35152.right
  br i1 %ifcond.35152, label %if.then.35152, label %if.else.35152
if.then.35152:
  %ret.35172 = icmp eq i32 0, 0
  ret i1 %ret.35172
if.else.35152:
  br label %if.end.35152
if.end.35152:
  %ifcond.35176.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.35176.left.i32.left.char.arg0 = trunc i32 %ifcond.35176.left.i32.left.char.arg0.i32 to i8
  %ifcond.35176.left.i32.left.char.arg1.i32 = load i32, ptr %start
  %ifcond.35176.left.i32.left.char.arg1 = trunc i32 %ifcond.35176.left.i32.left.char.arg1.i32 to i8
  %ifcond.35176.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.35176.left.i32.left.char.arg0, i8 %ifcond.35176.left.i32.left.char.arg1)
  %ifcond.35176.left.i32.left = zext i8 %ifcond.35176.left.i32.left.char to i32
  %ifcond.35176.left.i32.right.char = call i8 @kindIdentifier()
  %ifcond.35176.left.i32.right = zext i8 %ifcond.35176.left.i32.right.char to i32
  %ifcond.35176.left.i32.comparison = icmp eq i32 %ifcond.35176.left.i32.left, %ifcond.35176.left.i32.right
  %ifcond.35176.left.i32 = zext i1 %ifcond.35176.left.i32.comparison to i32
  %ifcond.35176.left = icmp ne i32 %ifcond.35176.left.i32, 0
  %ifcond.35176.right.i32.left = load i32, ptr %end.addr.35127
  %ifcond.35176.right.i32.right.left = load i32, ptr %start
  %ifcond.35176.right.i32.right.right = add i32 0, 1
  %ifcond.35176.right.i32.right = add i32 %ifcond.35176.right.i32.right.left, %ifcond.35176.right.i32.right.right
  %ifcond.35176.right.i32.comparison = icmp eq i32 %ifcond.35176.right.i32.left, %ifcond.35176.right.i32.right
  %ifcond.35176.right.i32 = zext i1 %ifcond.35176.right.i32.comparison to i32
  %ifcond.35176.right = icmp ne i32 %ifcond.35176.right.i32, 0
  %ifcond.35176 = and i1 %ifcond.35176.left, %ifcond.35176.right
  br i1 %ifcond.35176, label %if.then.35176, label %if.else.35176
if.then.35176:
  %ret.35196.arg0.arg0.arg0 = load ptr, ptr %tokens
  %ret.35196.arg0.arg0.arg1 = load i32, ptr %start
  %ret.35196.arg0.arg0.arg2.arg0 = load ptr, ptr %tokens
  %ret.35196.arg0.arg0.arg2.arg1 = load i32, ptr %start
  %ret.35196.arg0.arg0.arg2 = call ptr @tokenTextAt(ptr %ret.35196.arg0.arg0.arg2.arg0, i32 %ret.35196.arg0.arg0.arg2.arg1)
  %ret.35196.arg0.arg0 = call ptr @lookupVisibleValueType(ptr %ret.35196.arg0.arg0.arg0, i32 %ret.35196.arg0.arg0.arg1, ptr %ret.35196.arg0.arg0.arg2)
  %ret.35196.arg0 = call ptr @irTypeName(ptr %ret.35196.arg0.arg0)
  %ret.35196.arg1 = getelementptr inbounds [4 x i8], ptr @.str.35216, i32 0, i32 0
  %ret.35196 = call i1 @strEq(ptr %ret.35196.arg0, ptr %ret.35196.arg1)
  ret i1 %ret.35196
if.else.35176:
  br label %if.end.35176
if.end.35176:
  %op.addr.35384 = alloca i32
  %op.init.35384.arg0 = load ptr, ptr %tokens
  %op.init.35384.arg1 = load i32, ptr %start
  %op.init.35384.arg2 = load i32, ptr %end.addr.35127
  %op.init.35384 = call i32 @expressionTopLevelOperator(ptr %op.init.35384.arg0, i32 %op.init.35384.arg1, i32 %op.init.35384.arg2)
  store i32 %op.init.35384, ptr %op.addr.35384
  %ifcond.35398.left.left.i32.left = load i32, ptr %op.addr.35384
  %ifcond.35398.left.left.i32.right = load i32, ptr %start
  %ifcond.35398.left.left.i32.comparison = icmp sgt i32 %ifcond.35398.left.left.i32.left, %ifcond.35398.left.left.i32.right
  %ifcond.35398.left.left.i32 = zext i1 %ifcond.35398.left.left.i32.comparison to i32
  %ifcond.35398.left.left = icmp ne i32 %ifcond.35398.left.left.i32, 0
  %ifcond.35398.left.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.35398.left.right.i32.left.char.arg0 = trunc i32 %ifcond.35398.left.right.i32.left.char.arg0.i32 to i8
  %ifcond.35398.left.right.i32.left.char.arg1.i32 = load i32, ptr %op.addr.35384
  %ifcond.35398.left.right.i32.left.char.arg1 = trunc i32 %ifcond.35398.left.right.i32.left.char.arg1.i32 to i8
  %ifcond.35398.left.right.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.35398.left.right.i32.left.char.arg0, i8 %ifcond.35398.left.right.i32.left.char.arg1)
  %ifcond.35398.left.right.i32.left = zext i8 %ifcond.35398.left.right.i32.left.char to i32
  %ifcond.35398.left.right.i32.right.char = call i8 @kindOperator()
  %ifcond.35398.left.right.i32.right = zext i8 %ifcond.35398.left.right.i32.right.char to i32
  %ifcond.35398.left.right.i32.comparison = icmp eq i32 %ifcond.35398.left.right.i32.left, %ifcond.35398.left.right.i32.right
  %ifcond.35398.left.right.i32 = zext i1 %ifcond.35398.left.right.i32.comparison to i32
  %ifcond.35398.left.right = icmp ne i32 %ifcond.35398.left.right.i32, 0
  %ifcond.35398.left = and i1 %ifcond.35398.left.left, %ifcond.35398.left.right
  %ifcond.35398.right.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.35398.right.arg0.arg1 = load i32, ptr %op.addr.35384
  %ifcond.35398.right.arg0 = call ptr @tokenTextAt(ptr %ifcond.35398.right.arg0.arg0, i32 %ifcond.35398.right.arg0.arg1)
  %ifcond.35398.right.arg1 = getelementptr inbounds [2 x i8], ptr @.str.35424, i32 0, i32 0
  %ifcond.35398.right = call i1 @strEq(ptr %ifcond.35398.right.arg0, ptr %ifcond.35398.right.arg1)
  %ifcond.35398 = and i1 %ifcond.35398.left, %ifcond.35398.right
  br i1 %ifcond.35398, label %if.then.35398, label %if.else.35398
if.then.35398:
  %ret.35428.left.arg0 = load ptr, ptr %tokens
  %ret.35428.left.arg1 = load i32, ptr %start
  %ret.35428.left.arg2 = load i32, ptr %op.addr.35384
  %ret.35428.left = call i1 @llvmExpressionLooksPtr(ptr %ret.35428.left.arg0, i32 %ret.35428.left.arg1, i32 %ret.35428.left.arg2)
  %ret.35428.right.arg0 = load ptr, ptr %tokens
  %ret.35428.right.arg1.left = load i32, ptr %op.addr.35384
  %ret.35428.right.arg1.right = add i32 0, 1
  %ret.35428.right.arg1 = add i32 %ret.35428.right.arg1.left, %ret.35428.right.arg1.right
  %ret.35428.right.arg2 = load i32, ptr %end.addr.35127
  %ret.35428.right = call i1 @llvmExpressionLooksPtr(ptr %ret.35428.right.arg0, i32 %ret.35428.right.arg1, i32 %ret.35428.right.arg2)
  %ret.35428 = or i1 %ret.35428.left, %ret.35428.right
  ret i1 %ret.35428
if.else.35398:
  br label %if.end.35398
if.end.35398:
  %ret.35450 = icmp eq i32 0, 1
  ret i1 %ret.35450
}

declare ptr @csec_generate_llvm_flat_body_i32(ptr, i32, i32)

define ptr @generateLLVMFlatBodyI32(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_generate_llvm_flat_body_i32(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd)
  ret ptr %ret
}

define ptr @generateLLVMIfI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %then.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %then.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %then.open, i32 %arg.end, ptr @.str.llvm.if.brace.open, ptr @.str.llvm.if.brace.close)
  %open.ok = icmp sge i32 %open, 0
  %close.ok = icmp sgt i32 %close, %open
  %body.ok = icmp sgt i32 %then.close, %then.open
  %ok.a = and i1 %open.ok, %close.ok
  %ok = and i1 %ok.a, %body.ok
  br i1 %ok, label %build, label %bad
bad:
  ret ptr @.str.llvm.if.bad
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %condition.name.a = call ptr @csec_string_concat(ptr @.str.llvm.if.condition.name, ptr %seed)
  %expr.start = add i32 %open, 1
  %condition = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %expr.start, i32 %close, ptr %condition.name.a)
  %body.start = add i32 %then.open, 1
  %body = call ptr @generateLLVMFlatBodyI32(ptr %arg.tokens, i32 %body.start, i32 %then.close)
  %a1 = call ptr @csec_string_concat(ptr %condition, ptr @.str.llvm.if.branch)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr %seed)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr @.str.llvm.if.then.label)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr %seed)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr @.str.llvm.if.else.label)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr %seed)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr @.str.llvm.if.nl)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr @.str.llvm.if.then.name)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr %seed)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr @.str.llvm.if.colon)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr %body)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr @.str.llvm.if.end.branch)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr %seed)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr @.str.llvm.if.nl)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr @.str.llvm.if.else.name)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr %seed)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr @.str.llvm.if.colon)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr @.str.llvm.if.end.branch)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr %seed)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr @.str.llvm.if.nl)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr @.str.llvm.if.end.name)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr %seed)
  %result = call ptr @csec_string_concat(ptr %a22, ptr @.str.llvm.if.colon)
  ret ptr %result
}

@.str.llvm.if.bad = private unnamed_addr constant [18 x i8] c"  ; malformed if\0A\00"
@.str.llvm.if.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.llvm.if.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.llvm.if.condition.name = private unnamed_addr constant [7 x i8] c"%cond.\00"
@.str.llvm.if.branch = private unnamed_addr constant [15 x i8] c"  br i1 %cond.\00"
@.str.llvm.if.then.label = private unnamed_addr constant [18 x i8] c", label %if.then.\00"
@.str.llvm.if.else.label = private unnamed_addr constant [18 x i8] c", label %if.else.\00"
@.str.llvm.if.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.llvm.if.then.name = private unnamed_addr constant [9 x i8] c"if.then.\00"
@.str.llvm.if.else.name = private unnamed_addr constant [9 x i8] c"if.else.\00"
@.str.llvm.if.end.name = private unnamed_addr constant [8 x i8] c"if.end.\00"
@.str.llvm.if.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.llvm.if.end.branch = private unnamed_addr constant [20 x i8] c"  br label %if.end.\00"
define ptr @generateLLVMWhileI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %body.open, i32 %arg.end, ptr @.str.llvm.while.brace.open, ptr @.str.llvm.while.brace.close)
  %open.ok = icmp sge i32 %open, 0
  %close.ok = icmp sgt i32 %close, %open
  %body.ok = icmp sgt i32 %body.close, %body.open
  %ok.a = and i1 %open.ok, %close.ok
  %ok = and i1 %ok.a, %body.ok
  br i1 %ok, label %build, label %bad
bad:
  ret ptr @.str.llvm.while.bad
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %condition.name.a = call ptr @csec_string_concat(ptr @.str.llvm.while.condition.name, ptr %seed)
  %expr.start = add i32 %open, 1
  %condition = call ptr @generateLLVMExpressionI1(ptr %arg.tokens, i32 %expr.start, i32 %close, ptr %condition.name.a)
  %body.start = add i32 %body.open, 1
  %body = call ptr @generateLLVMFlatBodyI32(ptr %arg.tokens, i32 %body.start, i32 %body.close)
  %a1 = call ptr @csec_string_concat(ptr @.str.llvm.while.initial.branch, ptr %seed)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.llvm.while.nl)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr @.str.llvm.while.condition.label)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr %seed)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr @.str.llvm.while.colon)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr %condition)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr @.str.llvm.while.test)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr %seed)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr @.str.llvm.while.body.label)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr %seed)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr @.str.llvm.while.end.label)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr %seed)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr @.str.llvm.while.nl)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr @.str.llvm.while.body.name)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr %seed)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr @.str.llvm.while.colon)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr %body)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr @.str.llvm.while.initial.branch)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr %seed)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr @.str.llvm.while.nl)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr @.str.llvm.while.end.name)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr %seed)
  %result = call ptr @csec_string_concat(ptr %a22, ptr @.str.llvm.while.colon)
  ret ptr %result
}

@.str.llvm.while.bad = private unnamed_addr constant [21 x i8] c"  ; malformed while\0A\00"
@.str.llvm.while.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.llvm.while.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.llvm.while.condition.name = private unnamed_addr constant [12 x i8] c"%whilecond.\00"
@.str.llvm.while.initial.branch = private unnamed_addr constant [24 x i8] c"  br label %while.cond.\00"
@.str.llvm.while.condition.label = private unnamed_addr constant [12 x i8] c"while.cond.\00"
@.str.llvm.while.body.label = private unnamed_addr constant [21 x i8] c", label %while.body.\00"
@.str.llvm.while.end.label = private unnamed_addr constant [20 x i8] c", label %while.end.\00"
@.str.llvm.while.body.name = private unnamed_addr constant [12 x i8] c"while.body.\00"
@.str.llvm.while.end.name = private unnamed_addr constant [11 x i8] c"while.end.\00"
@.str.llvm.while.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.llvm.while.nl = private unnamed_addr constant [2 x i8] c"\0A\00"
@.str.llvm.while.test = private unnamed_addr constant [20 x i8] c"  br i1 %whilecond.\00"
define ptr @generateLLVMForI32(ptr %arg.tokens, i32 %arg.start, i32 %arg.end) {
entry:
  %open = call i32 @findStatementParenStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %close = call i32 @findStatementParenEnd(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.open = call i32 @findStatementBlockStart(ptr %arg.tokens, i32 %arg.start, i32 %arg.end)
  %body.close = call i32 @findClosingToken(ptr %arg.tokens, i32 %body.open, i32 %arg.end, ptr @.str.llvm.for.brace.open, ptr @.str.llvm.for.brace.close)
  %iterator.index = add i32 %open, 1
  %iterator = call ptr @tokenTextAt(ptr %arg.tokens, i32 %iterator.index)
  %arrow = call i32 @findTokenTextInRange(ptr %arg.tokens, i32 %iterator.index, i32 %close, ptr @.str.llvm.for.arrow)
  %range = call i32 @findTopLevelOperator(ptr %arg.tokens, i32 %arrow, i32 %close, i32 5)
  %valid.open = icmp sge i32 %open, 0
  %valid.arrow = icmp sgt i32 %arrow, %iterator.index
  %valid.range = icmp sgt i32 %range, %arrow
  %valid.a = and i1 %valid.open, %valid.arrow
  %valid = and i1 %valid.a, %valid.range
  br i1 %valid, label %build, label %bad
bad:
  ret ptr @.str.llvm.for.bad
build:
  %seed.i64 = sext i32 %arg.start to i64
  %seed = call ptr @csec_to_string_i64(i64 %seed.i64)
  %start.name.a = call ptr @csec_string_concat(ptr @.str.llvm.for.percent, ptr %iterator)
  %start.name.b = call ptr @csec_string_concat(ptr %start.name.a, ptr @.str.llvm.for.start)
  %end.name.a = call ptr @csec_string_concat(ptr @.str.llvm.for.percent, ptr %iterator)
  %end.name.b = call ptr @csec_string_concat(ptr %end.name.a, ptr @.str.llvm.for.end)
  %start.expr = add i32 %arrow, 1
  %start.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %start.expr, i32 %range, ptr %start.name.b)
  %end.expr = add i32 %range, 1
  %end.code = call ptr @generateLLVMExpressionI32(ptr %arg.tokens, i32 %end.expr, i32 %close, ptr %end.name.b)
  %body.start = add i32 %body.open, 1
  %body = call ptr @generateLLVMFlatBodyI32(ptr %arg.tokens, i32 %body.start, i32 %body.close)
  %op.text = call ptr @tokenTextAt(ptr %arg.tokens, i32 %range)
  %inclusive.to = call i1 @strEq(ptr %op.text, ptr @.str.llvm.for.to)
  %inclusive.dots = call i1 @strEq(ptr %op.text, ptr @.str.llvm.for.dots)
  %inclusive = or i1 %inclusive.to, %inclusive.dots
  br i1 %inclusive, label %sle, label %slt
sle:
  br label %emit
slt:
  br label %emit
emit:
  %comparison = phi ptr [ @.str.llvm.for.sle, %sle ], [ @.str.llvm.for.slt, %slt ]
  %a1 = call ptr @csec_string_concat(ptr @.str.llvm.for.alloca.a, ptr %iterator)
  %a2 = call ptr @csec_string_concat(ptr %a1, ptr @.str.llvm.for.alloca.b)
  %a3 = call ptr @csec_string_concat(ptr %a2, ptr %start.code)
  %a4 = call ptr @csec_string_concat(ptr %a3, ptr @.str.llvm.for.store.a)
  %a5 = call ptr @csec_string_concat(ptr %a4, ptr %iterator)
  %a6 = call ptr @csec_string_concat(ptr %a5, ptr @.str.llvm.for.store.b)
  %a7 = call ptr @csec_string_concat(ptr %a6, ptr %start.name.b)
  %a8 = call ptr @csec_string_concat(ptr %a7, ptr @.str.llvm.for.cond.label)
  %a9 = call ptr @csec_string_concat(ptr %a8, ptr %seed)
  %a10 = call ptr @csec_string_concat(ptr %a9, ptr @.str.llvm.for.colon)
  %a11 = call ptr @csec_string_concat(ptr %a10, ptr @.str.llvm.for.value.a)
  %a12 = call ptr @csec_string_concat(ptr %a11, ptr %iterator)
  %a13 = call ptr @csec_string_concat(ptr %a12, ptr @.str.llvm.for.value.b)
  %a14 = call ptr @csec_string_concat(ptr %a13, ptr %iterator)
  %a15 = call ptr @csec_string_concat(ptr %a14, ptr %end.code)
  %a16 = call ptr @csec_string_concat(ptr %a15, ptr @.str.llvm.for.compare.a)
  %a17 = call ptr @csec_string_concat(ptr %a16, ptr %seed)
  %a18 = call ptr @csec_string_concat(ptr %a17, ptr %comparison)
  %a19 = call ptr @csec_string_concat(ptr %a18, ptr @.str.llvm.for.compare.b)
  %a20 = call ptr @csec_string_concat(ptr %a19, ptr %iterator)
  %a21 = call ptr @csec_string_concat(ptr %a20, ptr @.str.llvm.for.compare.c)
  %a22 = call ptr @csec_string_concat(ptr %a21, ptr %iterator)
  %a23 = call ptr @csec_string_concat(ptr %a22, ptr @.str.llvm.for.branch.a)
  %a24 = call ptr @csec_string_concat(ptr %a23, ptr %seed)
  %a25 = call ptr @csec_string_concat(ptr %a24, ptr @.str.llvm.for.branch.b)
  %a26 = call ptr @csec_string_concat(ptr %a25, ptr %seed)
  %a27 = call ptr @csec_string_concat(ptr %a26, ptr @.str.llvm.for.branch.c)
  %a28 = call ptr @csec_string_concat(ptr %a27, ptr %seed)
  %a29 = call ptr @csec_string_concat(ptr %a28, ptr @.str.llvm.for.body.label)
  %a30 = call ptr @csec_string_concat(ptr %a29, ptr %seed)
  %a31 = call ptr @csec_string_concat(ptr %a30, ptr @.str.llvm.for.colon)
  %a32 = call ptr @csec_string_concat(ptr %a31, ptr %body)
  %a33 = call ptr @csec_string_concat(ptr %a32, ptr @.str.llvm.for.step.a)
  %a34 = call ptr @csec_string_concat(ptr %a33, ptr %iterator)
  %a35 = call ptr @csec_string_concat(ptr %a34, ptr @.str.llvm.for.step.b)
  %a36 = call ptr @csec_string_concat(ptr %a35, ptr %seed)
  %a37 = call ptr @csec_string_concat(ptr %a36, ptr @.str.llvm.for.step.c)
  %a38 = call ptr @csec_string_concat(ptr %a37, ptr %iterator)
  %a39 = call ptr @csec_string_concat(ptr %a38, ptr @.str.llvm.for.step.d)
  %a40 = call ptr @csec_string_concat(ptr %a39, ptr %seed)
  %a41 = call ptr @csec_string_concat(ptr %a40, ptr @.str.llvm.for.step.e)
  %a42 = call ptr @csec_string_concat(ptr %a41, ptr %iterator)
  %a43 = call ptr @csec_string_concat(ptr %a42, ptr @.str.llvm.for.step.f)
  %a44 = call ptr @csec_string_concat(ptr %a43, ptr %seed)
  %a45 = call ptr @csec_string_concat(ptr %a44, ptr @.str.llvm.for.step.g)
  %a46 = call ptr @csec_string_concat(ptr %a45, ptr %iterator)
  %a47 = call ptr @csec_string_concat(ptr %a46, ptr @.str.llvm.for.cond.branch)
  %a48 = call ptr @csec_string_concat(ptr %a47, ptr %seed)
  %a49 = call ptr @csec_string_concat(ptr %a48, ptr @.str.llvm.for.nl)
  %a50 = call ptr @csec_string_concat(ptr %a49, ptr @.str.llvm.for.end.label)
  %a51 = call ptr @csec_string_concat(ptr %a50, ptr %seed)
  %result = call ptr @csec_string_concat(ptr %a51, ptr @.str.llvm.for.colon)
  ret ptr %result
}

@.str.llvm.for.bad = private unnamed_addr constant [19 x i8] c"  ; malformed for\0A\00"
@.str.llvm.for.brace.open = private unnamed_addr constant [2 x i8] c"{\00"
@.str.llvm.for.brace.close = private unnamed_addr constant [2 x i8] c"}\00"
@.str.llvm.for.arrow = private unnamed_addr constant [3 x i8] c"<-\00"
@.str.llvm.for.to = private unnamed_addr constant [3 x i8] c"to\00"
@.str.llvm.for.dots = private unnamed_addr constant [3 x i8] c"..\00"
@.str.llvm.for.percent = private unnamed_addr constant [2 x i8] c"%\00"
@.str.llvm.for.start = private unnamed_addr constant [7 x i8] c".start\00"
@.str.llvm.for.end = private unnamed_addr constant [5 x i8] c".end\00"
@.str.llvm.for.slt = private unnamed_addr constant [4 x i8] c"slt\00"
@.str.llvm.for.sle = private unnamed_addr constant [4 x i8] c"sle\00"
@.str.llvm.for.alloca.a = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.llvm.for.alloca.b = private unnamed_addr constant [15 x i8] c" = alloca i32\0A\00"
@.str.llvm.for.store.a = private unnamed_addr constant [14 x i8] c"  store i32 %\00"
@.str.llvm.for.store.b = private unnamed_addr constant [8 x i8] c", ptr %\00"
@.str.llvm.for.cond.label = private unnamed_addr constant [22 x i8] c"  br label %for.cond.\00"
@.str.llvm.for.colon = private unnamed_addr constant [3 x i8] c":\0A\00"
@.str.llvm.for.value.a = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.llvm.for.value.b = private unnamed_addr constant [23 x i8] c".val = load i32, ptr %\00"
@.str.llvm.for.compare.a = private unnamed_addr constant [12 x i8] c"  %forcond.\00"
@.str.llvm.for.compare.b = private unnamed_addr constant [9 x i8] c" = icmp \00"
@.str.llvm.for.compare.c = private unnamed_addr constant [7 x i8] c" i32 %\00"
@.str.llvm.for.branch.a = private unnamed_addr constant [8 x i8] c".val, %\00"
@.str.llvm.for.branch.b = private unnamed_addr constant [23 x i8] c".end\0A  br i1 %forcond.\00"
@.str.llvm.for.branch.c = private unnamed_addr constant [19 x i8] c", label %for.body.\00"
@.str.llvm.for.body.label = private unnamed_addr constant [10 x i8] c"for.body.\00"
@.str.llvm.for.end.label = private unnamed_addr constant [9 x i8] c"for.end.\00"
@.str.llvm.for.step.a = private unnamed_addr constant [4 x i8] c"  %\00"
@.str.llvm.for.step.b = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.llvm.for.step.c = private unnamed_addr constant [19 x i8] c" = load i32, ptr %\00"
@.str.llvm.for.step.d = private unnamed_addr constant [5 x i8] c"\0A  %\00"
@.str.llvm.for.step.e = private unnamed_addr constant [7 x i8] c".next.\00"
@.str.llvm.for.step.f = private unnamed_addr constant [13 x i8] c" = add i32 %\00"
@.str.llvm.for.step.g = private unnamed_addr constant [7 x i8] c".step.\00"
@.str.llvm.for.cond.branch = private unnamed_addr constant [18 x i8] c", 1\0A  store i32 %\00"
@.str.llvm.for.nl = private unnamed_addr constant [7 x i8] c".next.\00"
define ptr @generateLLVMReturnI32(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %bodyStart = alloca i32
  store i32 %arg.bodyStart, ptr %bodyStart
  %bodyEnd = alloca i32
  store i32 %arg.bodyEnd, ptr %bodyEnd
  %cursor.addr.38983 = alloca i32
  %cursor.init.38983 = load i32, ptr %bodyStart
  store i32 %cursor.init.38983, ptr %cursor.addr.38983
  br label %while.cond.38990
while.cond.38990:
  %whilecond.38990.left.i32.left = load i32, ptr %cursor.addr.38983
  %whilecond.38990.left.i32.right = load i32, ptr %bodyEnd
  %whilecond.38990.left.i32.comparison = icmp slt i32 %whilecond.38990.left.i32.left, %whilecond.38990.left.i32.right
  %whilecond.38990.left.i32 = zext i1 %whilecond.38990.left.i32.comparison to i32
  %whilecond.38990.left = icmp ne i32 %whilecond.38990.left.i32, 0
  %whilecond.38990.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %whilecond.38990.right.i32.left.char.arg0 = trunc i32 %whilecond.38990.right.i32.left.char.arg0.i32 to i8
  %whilecond.38990.right.i32.left.char.arg1.i32 = load i32, ptr %cursor.addr.38983
  %whilecond.38990.right.i32.left.char.arg1 = trunc i32 %whilecond.38990.right.i32.left.char.arg1.i32 to i8
  %whilecond.38990.right.i32.left.char = call i8 @tokenKindAt(i8 %whilecond.38990.right.i32.left.char.arg0, i8 %whilecond.38990.right.i32.left.char.arg1)
  %whilecond.38990.right.i32.left = zext i8 %whilecond.38990.right.i32.left.char to i32
  %whilecond.38990.right.i32.right.char = call i8 @kindEof()
  %whilecond.38990.right.i32.right = zext i8 %whilecond.38990.right.i32.right.char to i32
  %whilecond.38990.right.i32.comparison = icmp ne i32 %whilecond.38990.right.i32.left, %whilecond.38990.right.i32.right
  %whilecond.38990.right.i32 = zext i1 %whilecond.38990.right.i32.comparison to i32
  %whilecond.38990.right = icmp ne i32 %whilecond.38990.right.i32, 0
  %whilecond.38990 = and i1 %whilecond.38990.left, %whilecond.38990.right
  br i1 %whilecond.38990, label %while.body.38990, label %while.end.38990
while.body.38990:
  %ifcond.39008.arg0 = load ptr, ptr %tokens
  %ifcond.39008.arg1 = load i32, ptr %cursor.addr.38983
  %ifcond.39008.arg2.char = call i8 @kindKeyword()
  %ifcond.39008.arg2 = zext i8 %ifcond.39008.arg2.char to i32
  %ifcond.39008.arg3 = getelementptr inbounds [7 x i8], ptr @.str.39020, i32 0, i32 0
  %ifcond.39008 = call i1 @tokenIs(ptr %ifcond.39008.arg0, i32 %ifcond.39008.arg1, i32 %ifcond.39008.arg2, ptr %ifcond.39008.arg3)
  br i1 %ifcond.39008, label %if.then.39008, label %if.else.39008
if.then.39008:
  %ifcond.39024.left.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %ifcond.39024.left.i32.left.char.arg0 = trunc i32 %ifcond.39024.left.i32.left.char.arg0.i32 to i8
  %ifcond.39024.left.i32.left.char.arg1.i32.left = load i32, ptr %cursor.addr.38983
  %ifcond.39024.left.i32.left.char.arg1.i32.right = add i32 0, 1
  %ifcond.39024.left.i32.left.char.arg1.i32 = add i32 %ifcond.39024.left.i32.left.char.arg1.i32.left, %ifcond.39024.left.i32.left.char.arg1.i32.right
  %ifcond.39024.left.i32.left.char.arg1 = trunc i32 %ifcond.39024.left.i32.left.char.arg1.i32 to i8
  %ifcond.39024.left.i32.left.char = call i8 @tokenKindAt(i8 %ifcond.39024.left.i32.left.char.arg0, i8 %ifcond.39024.left.i32.left.char.arg1)
  %ifcond.39024.left.i32.left = zext i8 %ifcond.39024.left.i32.left.char to i32
  %ifcond.39024.left.i32.right.char = call i8 @kindInteger()
  %ifcond.39024.left.i32.right = zext i8 %ifcond.39024.left.i32.right.char to i32
  %ifcond.39024.left.i32.comparison = icmp eq i32 %ifcond.39024.left.i32.left, %ifcond.39024.left.i32.right
  %ifcond.39024.left.i32 = zext i1 %ifcond.39024.left.i32.comparison to i32
  %ifcond.39024.left = icmp ne i32 %ifcond.39024.left.i32, 0
  %ifcond.39024.right.arg0 = load ptr, ptr %tokens
  %ifcond.39024.right.arg1.left = load i32, ptr %cursor.addr.38983
  %ifcond.39024.right.arg1.right = add i32 0, 2
  %ifcond.39024.right.arg1 = add i32 %ifcond.39024.right.arg1.left, %ifcond.39024.right.arg1.right
  %ifcond.39024.right.arg2.char = call i8 @kindOperator()
  %ifcond.39024.right.arg2 = zext i8 %ifcond.39024.right.arg2.char to i32
  %ifcond.39024.right.arg3 = getelementptr inbounds [2 x i8], ptr @.str.39051, i32 0, i32 0
  %ifcond.39024.right = call i1 @tokenIs(ptr %ifcond.39024.right.arg0, i32 %ifcond.39024.right.arg1, i32 %ifcond.39024.right.arg2, ptr %ifcond.39024.right.arg3)
  %ifcond.39024 = and i1 %ifcond.39024.left, %ifcond.39024.right
  br i1 %ifcond.39024, label %if.then.39024, label %if.else.39024
if.then.39024:
  %ret.39055.left.left = getelementptr inbounds [11 x i8], ptr @.str.39056, i32 0, i32 0
  %ret.39055.left.right.arg0 = load ptr, ptr %tokens
  %ret.39055.left.right.arg1.left = load i32, ptr %cursor.addr.38983
  %ret.39055.left.right.arg1.right = add i32 0, 1
  %ret.39055.left.right.arg1 = add i32 %ret.39055.left.right.arg1.left, %ret.39055.left.right.arg1.right
  %ret.39055.left.right = call ptr @tokenTextAt(ptr %ret.39055.left.right.arg0, i32 %ret.39055.left.right.arg1)
  %ret.39055.left = call ptr @csec_string_concat(ptr %ret.39055.left.left, ptr %ret.39055.left.right)
  %ret.39055.right = getelementptr inbounds [2 x i8], ptr @.str.39067, i32 0, i32 0
  %ret.39055 = call ptr @csec_string_concat(ptr %ret.39055.left, ptr %ret.39055.right)
  ret ptr %ret.39055
if.else.39024:
  br label %if.end.39024
if.end.39024:
  %ret.39070.left.arg0 = load ptr, ptr %tokens
  %ret.39070.left.arg1.left = load i32, ptr %cursor.addr.38983
  %ret.39070.left.arg1.right = add i32 0, 1
  %ret.39070.left.arg1 = add i32 %ret.39070.left.arg1.left, %ret.39070.left.arg1.right
  %ret.39070.left.arg2.arg0 = load ptr, ptr %tokens
  %ret.39070.left.arg2.arg1 = load i32, ptr %cursor.addr.38983
  %ret.39070.left.arg2.arg2 = load i32, ptr %bodyEnd
  %ret.39070.left.arg2 = call i32 @advanceStatement(ptr %ret.39070.left.arg2.arg0, i32 %ret.39070.left.arg2.arg1, i32 %ret.39070.left.arg2.arg2)
  %ret.39070.left.arg3 = getelementptr inbounds [5 x i8], ptr @.str.39088, i32 0, i32 0
  %ret.39070.left = call ptr @generateLLVMExpressionI32(ptr %ret.39070.left.arg0, i32 %ret.39070.left.arg1, i32 %ret.39070.left.arg2, ptr %ret.39070.left.arg3)
  %ret.39070.right = getelementptr inbounds [16 x i8], ptr @.str.39091, i32 0, i32 0
  %ret.39070 = call ptr @csec_string_concat(ptr %ret.39070.left, ptr %ret.39070.right)
  ret ptr %ret.39070
if.else.39008:
  br label %if.end.39008
if.end.39008:
  %cursor.assign.39094.left = load i32, ptr %cursor.addr.38983
  %cursor.assign.39094.right = add i32 0, 1
  %cursor.assign.39094 = add i32 %cursor.assign.39094.left, %cursor.assign.39094.right
  store i32 %cursor.assign.39094, ptr %cursor.addr.38983
  br label %while.cond.38990
while.end.38990:
  %ret.39101 = getelementptr inbounds [13 x i8], ptr @.str.39102, i32 0, i32 0
  ret ptr %ret.39101
}

define ptr @generateLLVMDottedStatementCall(ptr %arg.tokens, i32 %arg.cursor, i32 %arg.next) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %cursor = alloca i32
  store i32 %arg.cursor, ptr %cursor
  %next = alloca i32
  store i32 %arg.next, ptr %next
  %ret.39483 = getelementptr inbounds [1 x i8], ptr @.str.39484, i32 0, i32 0
  ret ptr %ret.39483
}

declare ptr @csec_llvm_main_body_fallback()

define ptr @generateLLVMMainBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_main_body_fallback()
  ret ptr %ret
}

define ptr @generateLLVMParamList(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_llvm_param_list(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

define ptr @generateLLVMParamListSlow(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_llvm_param_list(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

define ptr @generateLLVMParamAllocas(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_llvm_param_allocas(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

define ptr @generateLLVMParamAllocasSlow(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_function_llvm_param_allocas(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

declare ptr @csec_llvm_body_fallback(i32)

define ptr @generateLLVMBooleanBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_body_fallback(i32 1)
  ret ptr %ret
}

define ptr @generateLLVMCharBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_body_fallback(i32 2)
  ret ptr %ret
}

define ptr @generateLLVMDoubleBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_body_fallback(i32 3)
  ret ptr %ret
}

define ptr @generateLLVMLongBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_body_fallback(i32 4)
  ret ptr %ret
}

define i32 @llvmStringLiteralByteLength(ptr %arg.text) {
entry:
  %ret = call i32 @csec_llvm_string_literal_byte_length(ptr %arg.text)
  ret i32 %ret
}

define ptr @generateLLVMPointerBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %ret = call ptr @csec_llvm_body_fallback(i32 5)
  ret ptr %ret
}

define ptr @generateLLVMVoidBodyFromRange(ptr %arg.tokens, i32 %arg.bodyStart, i32 %arg.bodyEnd) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %bodyStart = alloca i32
  store i32 %arg.bodyStart, ptr %bodyStart
  %bodyEnd = alloca i32
  store i32 %arg.bodyEnd, ptr %bodyEnd
  %cursor.addr.43878 = alloca i32
  %cursor.init.43878.arg0 = load ptr, ptr %tokens
  %cursor.init.43878.arg1 = load i32, ptr %bodyStart
  %cursor.init.43878 = call i32 @skipTrivia(ptr %cursor.init.43878.arg0, i32 %cursor.init.43878.arg1)
  store i32 %cursor.init.43878, ptr %cursor.addr.43878
  %builder.addr.43890 = alloca i64
  %builder.init.43890 = call i64 @csec_string_builder_new()
  store i64 %builder.init.43890, ptr %builder.addr.43890
  br label %while.cond.43899
while.cond.43899:
  %whilecond.43899.left.i32.left = load i32, ptr %cursor.addr.43878
  %whilecond.43899.left.i32.right = load i32, ptr %bodyEnd
  %whilecond.43899.left.i32.comparison = icmp slt i32 %whilecond.43899.left.i32.left, %whilecond.43899.left.i32.right
  %whilecond.43899.left.i32 = zext i1 %whilecond.43899.left.i32.comparison to i32
  %whilecond.43899.left = icmp ne i32 %whilecond.43899.left.i32, 0
  %whilecond.43899.right.i32.left.char.arg0.i32 = load i32, ptr %tokens
  %whilecond.43899.right.i32.left.char.arg0 = trunc i32 %whilecond.43899.right.i32.left.char.arg0.i32 to i8
  %whilecond.43899.right.i32.left.char.arg1.i32 = load i32, ptr %cursor.addr.43878
  %whilecond.43899.right.i32.left.char.arg1 = trunc i32 %whilecond.43899.right.i32.left.char.arg1.i32 to i8
  %whilecond.43899.right.i32.left.char = call i8 @tokenKindAt(i8 %whilecond.43899.right.i32.left.char.arg0, i8 %whilecond.43899.right.i32.left.char.arg1)
  %whilecond.43899.right.i32.left = zext i8 %whilecond.43899.right.i32.left.char to i32
  %whilecond.43899.right.i32.right.char = call i8 @kindEof()
  %whilecond.43899.right.i32.right = zext i8 %whilecond.43899.right.i32.right.char to i32
  %whilecond.43899.right.i32.comparison = icmp ne i32 %whilecond.43899.right.i32.left, %whilecond.43899.right.i32.right
  %whilecond.43899.right.i32 = zext i1 %whilecond.43899.right.i32.comparison to i32
  %whilecond.43899.right = icmp ne i32 %whilecond.43899.right.i32, 0
  %whilecond.43899 = and i1 %whilecond.43899.left, %whilecond.43899.right
  br i1 %whilecond.43899, label %while.body.43899, label %while.end.43899
while.body.43899:
  %next.addr.43917 = alloca i32
  %next.init.43917.arg0 = load ptr, ptr %tokens
  %next.init.43917.arg1 = load i32, ptr %cursor.addr.43878
  %next.init.43917.arg2 = load i32, ptr %bodyEnd
  %next.init.43917 = call i32 @advanceStatement(ptr %next.init.43917.arg0, i32 %next.init.43917.arg1, i32 %next.init.43917.arg2)
  store i32 %next.init.43917, ptr %next.addr.43917
  %ifcond.43931.arg0.arg0 = load ptr, ptr %tokens
  %ifcond.43931.arg0.arg1 = load i32, ptr %cursor.addr.43878
  %ifcond.43931.arg0 = call ptr @statementKind(ptr %ifcond.43931.arg0.arg0, i32 %ifcond.43931.arg0.arg1)
  %ifcond.43931.arg1 = getelementptr inbounds [7 x i8], ptr @.str.43942, i32 0, i32 0
  %ifcond.43931 = call i1 @strEq(ptr %ifcond.43931.arg0, ptr %ifcond.43931.arg1)
  br i1 %ifcond.43931, label %if.then.43931, label %if.else.43931
if.then.43931:
  %discard.43946.arg0 = load i32, ptr %builder.addr.43890
  %discard.43946.arg1 = getelementptr inbounds [12 x i8], ptr @.str.43950, i32 0, i32 0
  %discard.43946 = call i32 @csec_string_builder_append(i32 %discard.43946.arg0, ptr %discard.43946.arg1)
  %ret.43953.arg0 = load i32, ptr %builder.addr.43890
  %ret.43953 = call ptr @csec_string_builder_finish(i32 %ret.43953.arg0)
  ret ptr %ret.43953
if.else.43931:
  br label %if.end.43931
if.end.43931:
  %discard.43960.arg0 = load i32, ptr %builder.addr.43890
  %discard.43960.arg1.arg0 = load ptr, ptr %tokens
  %discard.43960.arg1.arg1 = load i32, ptr %cursor.addr.43878
  %discard.43960.arg1.arg2 = load i32, ptr %next.addr.43917
  %discard.43960.arg1 = call i32 @generateLLVMFlatBodyI32(ptr %discard.43960.arg1.arg0, i32 %discard.43960.arg1.arg1, i32 %discard.43960.arg1.arg2)
  %discard.43960 = call i32 @csec_string_builder_append(i32 %discard.43960.arg0, i32 %discard.43960.arg1)
  %cursor.assign.43974.arg0 = load ptr, ptr %tokens
  %cursor.assign.43974.arg1 = load i32, ptr %next.addr.43917
  %cursor.assign.43974 = call i32 @skipTrivia(ptr %cursor.assign.43974.arg0, i32 %cursor.assign.43974.arg1)
  store i32 %cursor.assign.43974, ptr %cursor.addr.43878
  br label %while.cond.43899
while.end.43899:
  %discard.43984.arg0 = load i32, ptr %builder.addr.43890
  %discard.43984.arg1 = getelementptr inbounds [12 x i8], ptr @.str.43988, i32 0, i32 0
  %discard.43984 = call i32 @csec_string_builder_append(i32 %discard.43984.arg0, ptr %discard.43984.arg1)
  %ret.43991.arg0 = load i32, ptr %builder.addr.43890
  %ret.43991 = call ptr @csec_string_builder_finish(i32 %ret.43991.arg0)
  ret ptr %ret.43991
}

define ptr @llvmStringLiteralBytes(ptr %arg.text) {
entry:
  %text = alloca ptr
  store ptr %arg.text, ptr %text
  %ret.44008.arg0 = load ptr, ptr %text
  %ret.44008 = call ptr @csec_llvm_string_literal_bytes(ptr %ret.44008.arg0)
  ret ptr %ret.44008
}

declare ptr @csec_generate_llvm_string_literal_globals(ptr)

define ptr @generateLLVMStringLiteralGlobals(ptr %arg.tokens) {
entry:
  %ret = call ptr @csec_generate_llvm_string_literal_globals(ptr %arg.tokens)
  ret ptr %ret
}

declare ptr @csec_generate_llvm_function_definition(ptr, i32)

define ptr @generateLLVMFunctionDefinition(ptr %arg.tokens, i32 %arg.declStart) {
entry:
  %ret = call ptr @csec_generate_llvm_function_definition(ptr %arg.tokens, i32 %arg.declStart)
  ret ptr %ret
}

declare i32 @csec_generate_llvm_module_into(ptr, i64)

define i32 @generateLLVMModuleInto(ptr %arg.tokens, i64 %arg.builder) {
entry:
  %ret = call i32 @csec_generate_llvm_module_into(ptr %arg.tokens, i64 %arg.builder)
  ret i32 %ret
}

define ptr @generateLLVMModule(ptr %arg.tokens) {
entry:
  %valid = call i1 @parseProgram(ptr %arg.tokens)
  br i1 %valid, label %build, label %empty
build:
  %builder = call i64 @csec_string_builder_new()
  %written = call i32 @generateLLVMModuleInto(ptr %arg.tokens, i64 %builder)
  %result = call ptr @csec_string_builder_finish(i64 %builder)
  ret ptr %result
empty:
  ret ptr null
}

define ptr @generateExecutionC(ptr %arg.tokens) {
entry:
  %tokens = alloca ptr
  store ptr %arg.tokens, ptr %tokens
  %ifcond.47339.value.arg0 = load ptr, ptr %tokens
  %ifcond.47339.value = call i1 @parseProgram(ptr %ifcond.47339.value.arg0)
  %ifcond.47339 = xor i1 %ifcond.47339.value, true
  br i1 %ifcond.47339, label %if.then.47339, label %if.else.47339
if.then.47339:
  %ret.47348 = getelementptr inbounds [1 x i8], ptr @.str.47349, i32 0, i32 0
  ret ptr %ret.47348
if.else.47339:
  br label %if.end.47339
if.end.47339:
  %ret.47352.arg0 = load ptr, ptr %tokens
  %ret.47352 = call ptr @generateMainExecutionC(ptr %ret.47352.arg0)
  ret ptr %ret.47352
}

define ptr @trimAscii(ptr %arg.text) {
entry:
  %text = alloca ptr
  store ptr %arg.text, ptr %text
  %start.addr.47369 = alloca i32
  %start.init.47369 = add i32 0, 0
  store i32 %start.init.47369, ptr %start.addr.47369
  %end.addr.47376 = alloca i32
  %end.init.47376.string = load ptr, ptr %text
  %end.init.47376.length64 = call i64 @csec_string_length(ptr %end.init.47376.string)
  %end.init.47376 = trunc i64 %end.init.47376.length64 to i32
  store i32 %end.init.47376, ptr %end.addr.47376
  br label %while.cond.47385
while.cond.47385:
  %whilecond.47385.left.i32.left = load i32, ptr %start.addr.47369
  %whilecond.47385.left.i32.right = load i32, ptr %end.addr.47376
  %whilecond.47385.left.i32.comparison = icmp slt i32 %whilecond.47385.left.i32.left, %whilecond.47385.left.i32.right
  %whilecond.47385.left.i32 = zext i1 %whilecond.47385.left.i32.comparison to i32
  %whilecond.47385.left = icmp ne i32 %whilecond.47385.left.i32, 0
  %whilecond.47385.right.arg0.string = load ptr, ptr %text
  %whilecond.47385.right.arg0.index = load i32, ptr %start.addr.47369
  %whilecond.47385.right.arg0.char = call i8 @csec_string_char_at(ptr %whilecond.47385.right.arg0.string, i32 %whilecond.47385.right.arg0.index)
  %whilecond.47385.right.arg0 = zext i8 %whilecond.47385.right.arg0.char to i32
  %whilecond.47385.right = call i1 @isWhitespace(i32 %whilecond.47385.right.arg0)
  %whilecond.47385 = and i1 %whilecond.47385.left, %whilecond.47385.right
  br i1 %whilecond.47385, label %while.body.47385, label %while.end.47385
while.body.47385:
  %start.assign.47402.left = load i32, ptr %start.addr.47369
  %start.assign.47402.right = add i32 0, 1
  %start.assign.47402 = add i32 %start.assign.47402.left, %start.assign.47402.right
  store i32 %start.assign.47402, ptr %start.addr.47369
  br label %while.cond.47385
while.end.47385:
  br label %while.cond.47409
while.cond.47409:
  %whilecond.47409.left.i32.left = load i32, ptr %end.addr.47376
  %whilecond.47409.left.i32.right = load i32, ptr %start.addr.47369
  %whilecond.47409.left.i32.comparison = icmp sgt i32 %whilecond.47409.left.i32.left, %whilecond.47409.left.i32.right
  %whilecond.47409.left.i32 = zext i1 %whilecond.47409.left.i32.comparison to i32
  %whilecond.47409.left = icmp ne i32 %whilecond.47409.left.i32, 0
  %whilecond.47409.right.arg0.string = load ptr, ptr %text
  %whilecond.47409.right.arg0.index.left = load i32, ptr %end.addr.47376
  %whilecond.47409.right.arg0.index.right = add i32 0, 1
  %whilecond.47409.right.arg0.index = sub i32 %whilecond.47409.right.arg0.index.left, %whilecond.47409.right.arg0.index.right
  %whilecond.47409.right.arg0.char = call i8 @csec_string_char_at(ptr %whilecond.47409.right.arg0.string, i32 %whilecond.47409.right.arg0.index)
  %whilecond.47409.right.arg0 = zext i8 %whilecond.47409.right.arg0.char to i32
  %whilecond.47409.right = call i1 @isWhitespace(i32 %whilecond.47409.right.arg0)
  %whilecond.47409 = and i1 %whilecond.47409.left, %whilecond.47409.right
  br i1 %whilecond.47409, label %while.body.47409, label %while.end.47409
while.body.47409:
  %end.assign.47428.left = load i32, ptr %end.addr.47376
  %end.assign.47428.right = add i32 0, 1
  %end.assign.47428 = sub i32 %end.assign.47428.left, %end.assign.47428.right
  store i32 %end.assign.47428, ptr %end.addr.47376
  br label %while.cond.47409
while.end.47409:
  %ret.47435.string = load ptr, ptr %text
  %ret.47435.offset = load i32, ptr %start.addr.47369
  %ret.47435.length.left = load i32, ptr %end.addr.47376
  %ret.47435.length.right = load i32, ptr %start.addr.47369
  %ret.47435.length = sub i32 %ret.47435.length.left, %ret.47435.length.right
  %ret.47435 = call ptr @csec_string_substring(ptr %ret.47435.string, i32 %ret.47435.offset, i32 %ret.47435.length)
  ret ptr %ret.47435
}

define i1 @stringEndsWith(ptr %arg.text, ptr %arg.suffix) {
entry:
  %text = alloca ptr
  store ptr %arg.text, ptr %text
  %suffix = alloca ptr
  store ptr %arg.suffix, ptr %suffix
  %ifcond.47462.i32.left.string = load ptr, ptr %suffix
  %ifcond.47462.i32.left.length64 = call i64 @csec_string_length(ptr %ifcond.47462.i32.left.string)
  %ifcond.47462.i32.left = trunc i64 %ifcond.47462.i32.left.length64 to i32
  %ifcond.47462.i32.right.string = load ptr, ptr %text
  %ifcond.47462.i32.right.length64 = call i64 @csec_string_length(ptr %ifcond.47462.i32.right.string)
  %ifcond.47462.i32.right = trunc i64 %ifcond.47462.i32.right.length64 to i32
  %ifcond.47462.i32.comparison = icmp sgt i32 %ifcond.47462.i32.left, %ifcond.47462.i32.right
  %ifcond.47462.i32 = zext i1 %ifcond.47462.i32.comparison to i32
  %ifcond.47462 = icmp ne i32 %ifcond.47462.i32, 0
  br i1 %ifcond.47462, label %if.then.47462, label %if.else.47462
if.then.47462:
  %ret.47473 = icmp eq i32 0, 1
  ret i1 %ret.47473
if.else.47462:
  br label %if.end.47462
if.end.47462:
  %start.addr.47477 = alloca i32
  %start.init.47477.left.string = load ptr, ptr %text
  %start.init.47477.left.length64 = call i64 @csec_string_length(ptr %start.init.47477.left.string)
  %start.init.47477.left = trunc i64 %start.init.47477.left.length64 to i32
  %start.init.47477.right.string = load ptr, ptr %suffix
  %start.init.47477.right.length64 = call i64 @csec_string_length(ptr %start.init.47477.right.string)
  %start.init.47477.right = trunc i64 %start.init.47477.right.length64 to i32
  %start.init.47477 = sub i32 %start.init.47477.left, %start.init.47477.right
  store i32 %start.init.47477, ptr %start.addr.47477
  %tail.addr.47490 = alloca ptr
  %tail.init.47490.string = load ptr, ptr %text
  %tail.init.47490.offset = load i32, ptr %start.addr.47477
  %tail.init.47490.length.string = load ptr, ptr %suffix
  %tail.init.47490.length.length64 = call i64 @csec_string_length(ptr %tail.init.47490.length.string)
  %tail.init.47490.length = trunc i64 %tail.init.47490.length.length64 to i32
  %tail.init.47490 = call ptr @csec_string_substring(ptr %tail.init.47490.string, i32 %tail.init.47490.offset, i32 %tail.init.47490.length)
  store ptr %tail.init.47490, ptr %tail.addr.47490
  %ret.47506.i32.string = load ptr, ptr %tail.addr.47490
  %ret.47506.i32.needle = load ptr, ptr %suffix
  %ret.47506.i32 = call i32 @csec_string_starts_with(ptr %ret.47506.i32.string, ptr %ret.47506.i32.needle)
  %ret.47506 = icmp ne i32 %ret.47506.i32, 0
  ret i1 %ret.47506
}

define ptr @parentDirectory(ptr %arg.path) {
entry:
  %path = alloca ptr
  store ptr %arg.path, ptr %path
  %cursor.addr.47525 = alloca i32
  %cursor.init.47525.left.string = load ptr, ptr %path
  %cursor.init.47525.left.length64 = call i64 @csec_string_length(ptr %cursor.init.47525.left.string)
  %cursor.init.47525.left = trunc i64 %cursor.init.47525.left.length64 to i32
  %cursor.init.47525.right = add i32 0, 1
  %cursor.init.47525 = sub i32 %cursor.init.47525.left, %cursor.init.47525.right
  store i32 %cursor.init.47525, ptr %cursor.addr.47525
  br label %while.cond.47536
while.cond.47536:
  %whilecond.47536.i32.left = load i32, ptr %cursor.addr.47525
  %whilecond.47536.i32.right = add i32 0, 0
  %whilecond.47536.i32.comparison = icmp sge i32 %whilecond.47536.i32.left, %whilecond.47536.i32.right
  %whilecond.47536.i32 = zext i1 %whilecond.47536.i32.comparison to i32
  %whilecond.47536 = icmp ne i32 %whilecond.47536.i32, 0
  br i1 %whilecond.47536, label %while.body.47536, label %while.end.47536
while.body.47536:
  %ch.addr.47543 = alloca i8
  %ch.init.47543.i32.string = load ptr, ptr %path
  %ch.init.47543.i32.index = load i32, ptr %cursor.addr.47525
  %ch.init.47543.i32.char = call i8 @csec_string_char_at(ptr %ch.init.47543.i32.string, i32 %ch.init.47543.i32.index)
  %ch.init.47543.i32 = zext i8 %ch.init.47543.i32.char to i32
  %ch.init.47543 = trunc i32 %ch.init.47543.i32 to i8
  store i8 %ch.init.47543, ptr %ch.addr.47543
  %ifcond.47555.left.left = load i8, ptr %ch.addr.47543
  %ifcond.47555.left.right = add i8 0, 47
  %ifcond.47555.left = icmp eq i8 %ifcond.47555.left.left, %ifcond.47555.left.right
  %ifcond.47555.right.left = load i8, ptr %ch.addr.47543
  %ifcond.47555.right.right = add i8 0, 0
  %ifcond.47555.right = icmp eq i8 %ifcond.47555.right.left, %ifcond.47555.right.right
  %ifcond.47555 = or i1 %ifcond.47555.left, %ifcond.47555.right
  br i1 %ifcond.47555, label %if.then.47555, label %if.else.47555
if.then.47555:
  %ret.47566.string = load ptr, ptr %path
  %ret.47566.offset = add i32 0, 0
  %ret.47566.length = load i32, ptr %cursor.addr.47525
  %ret.47566 = call ptr @csec_string_substring(ptr %ret.47566.string, i32 %ret.47566.offset, i32 %ret.47566.length)
  ret ptr %ret.47566
if.else.47555:
  br label %if.end.47555
if.end.47555:
  %cursor.assign.47577.left = load i32, ptr %cursor.addr.47525
  %cursor.assign.47577.right = add i32 0, 1
  %cursor.assign.47577 = sub i32 %cursor.assign.47577.left, %cursor.assign.47577.right
  store i32 %cursor.assign.47577, ptr %cursor.addr.47525
  br label %while.cond.47536
while.end.47536:
  %ret.47584 = getelementptr inbounds [2 x i8], ptr @.str.47585, i32 0, i32 0
  ret ptr %ret.47584
}

declare ptr @csec_replace_dots_with_slash(ptr)

define ptr @replaceDotsWithSlash(ptr %arg.text) {
entry:
  %ret = call ptr @csec_replace_dots_with_slash(ptr %arg.text)
  ret ptr %ret
}

declare ptr @csec_import_target_from_line(ptr)

define ptr @importTargetFromLine(ptr %arg.line) {
entry:
  %ret = call ptr @csec_import_target_from_line(ptr %arg.line)
  ret ptr %ret
}

declare ptr @csec_import_candidate(ptr)

define ptr @importCandidate(ptr %arg.target) {
entry:
  %ret = call ptr @csec_import_candidate(ptr %arg.target)
  ret ptr %ret
}

declare ptr @csec_resolve_import_path(ptr, ptr)

define ptr @resolveImportPath(ptr %arg.currentPath, ptr %arg.target) {
entry:
  %ret = call ptr @csec_resolve_import_path(ptr %arg.currentPath, ptr %arg.target)
  ret ptr %ret
}

define ptr @expandImportsFromFile(ptr %arg.path, ptr %arg.seen) {
entry:
  %ret = call ptr @csec_expand_imports(ptr %arg.path)
  ret ptr %ret
}

define ptr @compileSource(ptr %arg.source, ptr %arg.mode) {
entry:
  %source = alloca ptr
  store ptr %arg.source, ptr %source
  %mode = alloca ptr
  store ptr %arg.mode, ptr %mode
  %tokens.addr.48354 = alloca ptr
  %tokens.init.48354.arg0 = load ptr, ptr %source
  %tokens.init.48354 = call ptr @tokenize(ptr %tokens.init.48354.arg0)
  store ptr %tokens.init.48354, ptr %tokens.addr.48354
  %ifcond.48364.left.string = load ptr, ptr %mode
  %ifcond.48364.left.needle = getelementptr inbounds [9 x i8], ptr @.str.48370, i32 0, i32 0
  %ifcond.48364.left = call i32 @csec_string_starts_with(ptr %ifcond.48364.left.string, ptr %ifcond.48364.left.needle)
  %ifcond.48364 = icmp ne i32 %ifcond.48364.left, 0
  br i1 %ifcond.48364, label %if.then.48364, label %if.else.48364
if.then.48364:
  %ret.48374.left.left = getelementptr inbounds [8 x i8], ptr @.str.48375, i32 0, i32 0
  %ret.48374.left.right.i32.string = load ptr, ptr %tokens.addr.48354
  %ret.48374.left.right.i32.length64 = call i64 @csec_string_length(ptr %ret.48374.left.right.i32.string)
  %ret.48374.left.right.i32 = trunc i64 %ret.48374.left.right.i32.length64 to i32
  %ret.48374.left.right.i64 = sext i32 %ret.48374.left.right.i32 to i64
  %ret.48374.left.right = call ptr @csec_to_string_i64(i64 %ret.48374.left.right.i64)
  %ret.48374.left = call ptr @csec_string_concat(ptr %ret.48374.left.left, ptr %ret.48374.left.right)
  %ret.48374.right = getelementptr inbounds [2 x i8], ptr @.str.48381, i32 0, i32 0
  %ret.48374 = call ptr @csec_string_concat(ptr %ret.48374.left, ptr %ret.48374.right)
  ret ptr %ret.48374
if.else.48364:
  br label %if.end.48364
if.end.48364:
  %ifcond.48384.left.string = load ptr, ptr %mode
  %ifcond.48384.left.needle = getelementptr inbounds [7 x i8], ptr @.str.48390, i32 0, i32 0
  %ifcond.48384.left = call i32 @csec_string_starts_with(ptr %ifcond.48384.left.string, ptr %ifcond.48384.left.needle)
  %ifcond.48384 = icmp ne i32 %ifcond.48384.left, 0
  br i1 %ifcond.48384, label %if.then.48384, label %if.else.48384
if.then.48384:
  %ret.48394 = load ptr, ptr %tokens.addr.48354
  ret ptr %ret.48394
if.else.48384:
  br label %if.end.48384
if.end.48384:
  %ifcond.48398.left.string = load ptr, ptr %mode
  %ifcond.48398.left.needle = getelementptr inbounds [4 x i8], ptr @.str.48404, i32 0, i32 0
  %ifcond.48398.left = call i32 @csec_string_starts_with(ptr %ifcond.48398.left.string, ptr %ifcond.48398.left.needle)
  %ifcond.48398 = icmp ne i32 %ifcond.48398.left, 0
  br i1 %ifcond.48398, label %if.then.48398, label %if.else.48398
if.then.48398:
  %ret.48408.arg0 = load ptr, ptr %tokens.addr.48354
  %ret.48408 = call ptr @generateAST(ptr %ret.48408.arg0)
  ret ptr %ret.48408
if.else.48398:
  br label %if.end.48398
if.end.48398:
  %ifcond.48415.left.string = load ptr, ptr %mode
  %ifcond.48415.left.needle = getelementptr inbounds [8 x i8], ptr @.str.48421, i32 0, i32 0
  %ifcond.48415.left = call i32 @csec_string_starts_with(ptr %ifcond.48415.left.string, ptr %ifcond.48415.left.needle)
  %ifcond.48415 = icmp ne i32 %ifcond.48415.left, 0
  br i1 %ifcond.48415, label %if.then.48415, label %if.else.48415
if.then.48415:
  %ret.48425.arg0 = load ptr, ptr %tokens.addr.48354
  %ret.48425 = call ptr @generateSymbolTable(ptr %ret.48425.arg0)
  ret ptr %ret.48425
if.else.48415:
  br label %if.end.48415
if.end.48415:
  %ifcond.48432.left.string = load ptr, ptr %mode
  %ifcond.48432.left.needle = getelementptr inbounds [3 x i8], ptr @.str.48438, i32 0, i32 0
  %ifcond.48432.left = call i32 @csec_string_starts_with(ptr %ifcond.48432.left.string, ptr %ifcond.48432.left.needle)
  %ifcond.48432 = icmp ne i32 %ifcond.48432.left, 0
  br i1 %ifcond.48432, label %if.then.48432, label %if.else.48432
if.then.48432:
  %ret.48442.arg0 = load ptr, ptr %tokens.addr.48354
  %ret.48442 = call ptr @generateIR(ptr %ret.48442.arg0)
  ret ptr %ret.48442
if.else.48432:
  br label %if.end.48432
if.end.48432:
  %ifcond.48449.left.string = load ptr, ptr %mode
  %ifcond.48449.left.needle = getelementptr inbounds [5 x i8], ptr @.str.48455, i32 0, i32 0
  %ifcond.48449.left = call i32 @csec_string_starts_with(ptr %ifcond.48449.left.string, ptr %ifcond.48449.left.needle)
  %ifcond.48449 = icmp ne i32 %ifcond.48449.left, 0
  br i1 %ifcond.48449, label %if.then.48449, label %if.else.48449
if.then.48449:
  %ret.48459.arg0 = load ptr, ptr %tokens.addr.48354
  %ret.48459 = call ptr @generateLLVMModule(ptr %ret.48459.arg0)
  ret ptr %ret.48459
if.else.48449:
  br label %if.end.48449
if.end.48449:
  %ret.48466.arg0 = load ptr, ptr %tokens.addr.48354
  %ret.48466 = call ptr @generateExecutionC(ptr %ret.48466.arg0)
  ret ptr %ret.48466
}

define i32 @compileFile(ptr %arg.inputPath, ptr %arg.outputPath, ptr %arg.mode) {
entry:
  %llvm.raw = call i32 @csec_string_starts_with(ptr %arg.mode, ptr @.str.driver.llvm)
  %llvm = icmp ne i32 %llvm.raw, 0
  br i1 %llvm, label %build, label %unsupported
build:
  %source = call ptr @csec_expand_imports(ptr %arg.inputPath)
  %builder = call i64 @csec_string_builder_new_file(ptr %arg.outputPath)
  %ready = icmp ne i64 %builder, 0
  br i1 %ready, label %generate, label %failed
generate:
  %tokens = call ptr @tokenize(ptr %source)
  %generated = call i32 @generateLLVMModuleInto(ptr %tokens, i64 %builder)
  call void @csec_token_buffer_release(ptr %tokens)
  %success = icmp eq i32 %generated, 0
  br i1 %success, label %write, label %return.generated
write:
  %written = call i32 @csec_string_builder_write_to_file(i64 %builder, ptr %arg.outputPath)
  ret i32 %written
return.generated:
  ret i32 %generated
failed:
  ret i32 1
unsupported:
  ret i32 1
}

@.str.driver.llvm = private unnamed_addr constant [5 x i8] c"llvm\00"

define i32 @main() {
entry:
  %argc = call i32 @csec_command_line_arg_count()
  %provided = icmp sge i32 %argc, 4
  br i1 %provided, label %compile, label %missing
compile:
  %input = call ptr @csec_command_line_arg(i32 1)
  %output = call ptr @csec_command_line_arg(i32 2)
  %mode = call ptr @csec_command_line_arg(i32 3)
  %status = call i32 @compileFile(ptr %input, ptr %output, ptr %mode)
  ret i32 %status
missing:
  ret i32 1
}

