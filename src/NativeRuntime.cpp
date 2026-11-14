#include "NativeRuntime.h"

#include <cmath>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

#include "import_alias.h"

// Implementation remains one translation unit so internal helpers retain private linkage.
#include "native_runtime/runtime_support.inc"
#include "native_runtime/llvm_helper_calls.inc"
#include "native_runtime/llvm_helper_control.inc"
#include "native_runtime/lexer_runtime.inc"
#include "native_runtime/syntax_analysis.inc"
#include "native_runtime/llvm_classes.inc"
#include "native_runtime/llvm_arrays.inc"
#include "native_runtime/llvm_dicts.inc"
#include "native_runtime/llvm_expressions.inc"
#include "native_runtime/llvm_module.inc"
#include "native_runtime/runtime_services.inc"
#include "native_runtime/bigint_runtime.inc"
#include "native_runtime/tensor_runtime.inc"
#include "native_runtime/process_runtime.inc"
#include "native_runtime/thread_runtime.inc"
#include "native_runtime/ipc_runtime.inc"
#include "native_runtime/io_runtime.inc"
#include "native_runtime/parquet_codec.inc"
#include "native_runtime/dataframe_runtime.inc"
#include "native_runtime/dataframe_expr.inc"
#include "native_runtime/symbolic_runtime.inc"
#include "native_runtime/json_runtime.inc"
#include "native_runtime/http_runtime.inc"
#include "native_runtime/dict_runtime.inc"
#include "native_runtime/algebra_runtime.inc"
