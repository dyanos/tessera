cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
endif()

# C is enabled because LLVM's exported CMake package (e.g. Homebrew/conda LLVM on macOS) runs
# check_c_source_compiles() in FindFFI/FindTerminfo/zlib/zstd; with CXX only, try_compile fails
# with 'Unknown extension ".c"'.
project(csec++ C CXX)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(MSVC AND NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
if(CMAKE_VS_PLATFORM_NAME)
    message("${CMAKE_VS_PLATFORM_NAME} architecture in use")
endif()

if(MSVC AND NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_RELEASE "")
endif()

# Clang compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -fansi-escape-codes")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################

























set(PROJECT_NAME csec++)

################################################################################
# Source groups
################################################################################
set(samples
    "sample1.csec"
    "sample2.csec"
)
source_group("samples" FILES ${samples})

set(_
    "README.md"
)
source_group("由ъ냼???뚯씪" FILES ${_})

set(_
    "ast.cpp"
    "ASTPrinter.cpp"
    "codegen.cpp"
    "LatexMathEqParser.cpp"
    "lexer.cpp"
    "main.cpp"
    "mangling.cpp"
    "module_loader.cpp"
    "symbol_table.cpp"
    "type.cpp"
    "type_checker.cpp"
    "type_utils.cpp"
    "utils.cpp"
    "verifier.cpp"
)
source_group("?뚯뒪 ?뚯씪" FILES ${_})

set(___AST
    "AccessFieldNode.cpp"
    "ArrayAccessNode.cpp"
    "ArrayCreationExpressionNode.cpp"
    "ArrayCreationNode.cpp"
    "ArrayLiteralNode.cpp"
    "dict_support.cpp"
    "AssignmentExpressionNode.cpp"
    "AssignmentNode.cpp"
    "AttributeNode.cpp"
    "BinaryExpressionNode.cpp"
    "BlockNode.cpp"
    "CallExpressionNode.cpp"
    "CastingExpressionNode.cpp"
    "ClassBodyNode.cpp"
    "ClassDeclarationNode.cpp"
    "ClassInstanceCreationNode.cpp"
    "ExpressionNode.cpp"
    "FilterStatementNode.cpp"
    "ForStatementNode.cpp"
    "FunctionCallNode.cpp"
    "FunctionDeclarationNode.cpp"
    "IdentifierNode.cpp"
    "IfStatementNode.cpp"
    "ImportNode.cpp"
    "LambdaExpressionNode.cpp"
    "MapStatementNode.cpp"
    "MatchExpressionNode.cpp"
    "MethodCallNode.cpp"
    "ObjectDeclarationNode.cpp"
    "ParameterNode.cpp"
    "PMapStatementNode.cpp"
    "PostfixExpressionNode.cpp"
    "PrefixExpressionNode.cpp"
    "ProgramNode.cpp"
    "RangeExpressionNode.cpp"
    "ReduceStatementNode.cpp"
    "ReturnStatementNode.cpp"
    "UnaryExpressionNode.cpp"
    "UnitNode.cpp"
    "ValueNode.cpp"
    "VariableDeclarationNode.cpp"
    "WhileStatementNode.cpp"
)
source_group("?뚯뒪 ?뚯씪\\AST" FILES ${___AST})

set(___Parser
    "Declaration.cpp"
    "Expression.cpp"
    "parser.cpp"
    "Statement.cpp"
    "Token.cpp"
)
source_group("?뚯뒪 ?뚯씪\\Parser" FILES ${___Parser})

set(_
    "all_ast.h"
    "ast.h"
    "codegen.h"
    "context.h"
    "LatexMathEqParser.h"
    "lexer.h"
    "mangling.h"
    "module_loader.h"
    "parser.h"
    "symbol.h"
    "symbol_table.h"
    "token.h"
    "type.h"
    "type_checker.h"
    "type_utils.h"
    "utils.h"
    "intrinsics.h"
    "builtin_structs.h"
    "llvm_compat.h"
)
source_group("?ㅻ뜑 ?뚯씪" FILES ${_})

set(___AST
    "AccessFieldNode.h"
    "ArrayAccessNode.h"
    "ArrayCreationExpressionNode.h"
    "ArrayCreationNode.h"
    "ArrayLiteralNode.h"
    "DictLiteralNode.h"
    "dict_support.h"
    "AssignmentExpressionNode.h"
    "AssignmentNode.h"
    "ASTVisitor.h"
    "AttributeNode.h"
    "BinaryExpressionNode.h"
    "BlockNode.h"
    "CallExpressionNode.h"
    "CastingExpressionNode.h"
    "ClassBodyNode.h"
    "ClassDeclarationNode.h"
    "ClassInstanceCreationNode.h"
    "ExpressionNode.h"
    "FilterStatementNode.h"
    "ForStatementNode.h"
    "FunctionCallNode.h"
    "FunctionDeclarationNode.h"
    "IdentifierNode.h"
    "IfStatementNode.h"
    "ImportNode.h"
    "LambdaExpressionNode.h"
    "MapStatementNode.h"
    "MatchExpressionNode.h"
    "MethodCallNode.h"
    "ObjectDeclarationNode.h"
    "ParameterNode.h"
    "PMapStatementNode.h"
    "PostfixExpressionNode.h"
    "PrefixExpressionNode.h"
    "ProgramNode.h"
    "RangeExpressionNode.h"
    "ReduceStatementNode.h"
    "ReturnStatementNode.h"
    "UnaryExpressionNode.h"
    "UnitNode.h"
    "ValueNode.h"
    "VariableDeclarationNode.h"
    "WhileStatementNode.h"
)
source_group("?ㅻ뜑 ?뚯씪\\AST" FILES ${___AST})

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/NativeRuntime.cpp")
file(GLOB PROJECT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)
file(GLOB NATIVE_RUNTIME_MODULES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/native_runtime/*.inc"
)

set(ALL_FILES
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${samples}
    "README.md"
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

add_library(csec_native_runtime SHARED
    "src/NativeRuntime.cpp"
    "src/NativeRuntime.h"
    ${NATIVE_RUNTIME_MODULES}
)
source_group("Native Runtime" FILES ${NATIVE_RUNTIME_MODULES})
target_compile_features(csec_native_runtime PRIVATE cxx_std_17)
target_compile_definitions(csec_native_runtime PRIVATE CSEC_NATIVE_RUNTIME_BUILD)
# std.dataframe reads ZSTD-compressed Parquet pages only when libzstd is available (the other codecs
# are built in). AUTO uses it when found; point CMAKE_PREFIX_PATH at its install if it is elsewhere.
set(CSEC_WITH_ZSTD AUTO CACHE STRING "Link libzstd for ZSTD Parquet pages (AUTO, ON, OFF)")
set_property(CACHE CSEC_WITH_ZSTD PROPERTY STRINGS AUTO ON OFF)
if(NOT CSEC_WITH_ZSTD STREQUAL "OFF")
    find_path(CSEC_ZSTD_INCLUDE_DIR zstd.h)
    find_library(CSEC_ZSTD_LIBRARY NAMES zstd libzstd zstd_static)
    if(CSEC_ZSTD_INCLUDE_DIR AND CSEC_ZSTD_LIBRARY)
        message(STATUS "std.dataframe: ZSTD Parquet pages via ${CSEC_ZSTD_LIBRARY}")
        target_include_directories(csec_native_runtime PRIVATE "${CSEC_ZSTD_INCLUDE_DIR}")
        target_compile_definitions(csec_native_runtime PRIVATE CSEC_HAVE_ZSTD)
        target_link_libraries(csec_native_runtime PRIVATE "${CSEC_ZSTD_LIBRARY}")
    elseif(CSEC_WITH_ZSTD STREQUAL "ON")
        message(FATAL_ERROR "CSEC_WITH_ZSTD=ON but zstd.h / libzstd were not found")
    else()
        message(STATUS "std.dataframe: libzstd not found; ZSTD Parquet pages are not supported")
    endif()
endif()
find_package(Threads REQUIRED)
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    target_link_libraries(csec_native_runtime PRIVATE OpenMP::OpenMP_CXX)
endif()
if(WIN32)
    target_compile_definitions(csec_native_runtime PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_definitions(csec_native_runtime PRIVATE _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)
    endif()
endif()
set_target_properties(csec_native_runtime PROPERTIES
    OUTPUT_NAME "System.Native"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
if(WIN32)
    target_link_libraries(csec_native_runtime PRIVATE ws2_32 winhttp)
else()
    target_link_libraries(csec_native_runtime PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()
# ELF shared libraries resolve even their own exported functions through the global symbol table,
# and `csec++ --run-ir` loads a second copy of the runtime (the one beside csec++) next to the one
# csec++ links. Without -Bsymbolic a call inside one copy can land in the other, whose state (handle
# tables, threads, channels) is different. Bind the runtime's references to its own definitions.
# (macOS two-level namespaces already do this; Windows DLLs never interpose.)
if(UNIX AND NOT APPLE)
    target_link_options(csec_native_runtime PRIVATE "LINKER:-Bsymbolic")
endif()
add_dependencies(${PROJECT_NAME} csec_native_runtime)
target_link_libraries(${PROJECT_NAME} PRIVATE csec_native_runtime Threads::Threads)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:csec_native_runtime>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:csec_native_runtime>
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/csec_native_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}"
)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_LINKER_FILE:csec_native_runtime>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(${PROJECT_NAME} PRIVATE _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)
endif()

# `csec++ --verify` proves contracts with the Z3 SMT solver (an external library). AUTO uses it when
# found: deps/z3 (scripts/fetch_z3.ps1 / scripts/fetch_z3.sh), Z3_ROOT, or the system paths.
#
# CSEC_Z3_LINK=SHARED (default) links the shared library (libz3.dll / libz3.so / libz3.dylib): csec++
# needs it at run time, and std.logic loads it on demand. CSEC_Z3_LINK=STATIC links a static libz3
# into both csec++ and the System.Native runtime, so neither needs the shared library; build it with
# scripts/build_z3_static.ps1 / scripts/build_z3_static.sh (deps/z3-static) or point Z3_ROOT at one.
set(CSEC_WITH_Z3 AUTO CACHE STRING "Link Z3 for --verify (AUTO, ON, OFF)")
set_property(CACHE CSEC_WITH_Z3 PROPERTY STRINGS AUTO ON OFF)
set(CSEC_Z3_LINK SHARED CACHE STRING "How to link Z3: SHARED (the libz3 shared library) or STATIC")
set_property(CACHE CSEC_Z3_LINK PROPERTY STRINGS SHARED STATIC)
if(NOT CSEC_WITH_Z3 STREQUAL "OFF" AND CSEC_Z3_LINK STREQUAL "STATIC")
    set(_csec_z3_hints "${CMAKE_CURRENT_SOURCE_DIR}/deps/z3-static" "$ENV{Z3_ROOT}" "${Z3_ROOT}")
    find_path(CSEC_Z3_STATIC_INCLUDE_DIR z3++.h HINTS ${_csec_z3_hints} PATH_SUFFIXES include NO_DEFAULT_PATH)
    if(WIN32)
        set(_csec_z3_static_names libz3-static.lib libz3.lib z3.lib)
    else()
        set(_csec_z3_static_names libz3.a libz3-static.a)
    endif()
    find_library(CSEC_Z3_STATIC_LIBRARY NAMES ${_csec_z3_static_names} HINTS ${_csec_z3_hints} PATH_SUFFIXES lib lib64 NO_DEFAULT_PATH)
    if(CSEC_Z3_STATIC_INCLUDE_DIR AND CSEC_Z3_STATIC_LIBRARY)
        message(STATUS "--verify and std.logic: static Z3 from ${CSEC_Z3_STATIC_LIBRARY}")
        foreach(_csec_target ${PROJECT_NAME} csec_native_runtime)
            target_include_directories(${_csec_target} PRIVATE "${CSEC_Z3_STATIC_INCLUDE_DIR}")
            target_compile_definitions(${_csec_target} PRIVATE CSEC_HAVE_Z3 CSEC_Z3_STATIC)
            target_link_libraries(${_csec_target} PRIVATE "${CSEC_Z3_STATIC_LIBRARY}")
            if(NOT WIN32)
                target_link_libraries(${_csec_target} PRIVATE Threads::Threads)
            endif()
        endforeach()
    elseif(CSEC_WITH_Z3 STREQUAL "ON")
        message(FATAL_ERROR "CSEC_Z3_LINK=STATIC but no static libz3 was found in deps/z3-static or Z3_ROOT "
                            "(run scripts/build_z3_static.ps1 or scripts/build_z3_static.sh)")
    else()
        message(STATUS "--verify: no static Z3 found (CSEC_Z3_LINK=STATIC); csec++ is built without the verifier")
    endif()
elseif(NOT CSEC_WITH_Z3 STREQUAL "OFF")
    set(_csec_z3_hints "${CMAKE_CURRENT_SOURCE_DIR}/deps/z3" "$ENV{Z3_ROOT}" "${Z3_ROOT}")
    find_path(CSEC_Z3_INCLUDE_DIR z3++.h HINTS ${_csec_z3_hints} PATH_SUFFIXES include include/z3)
    find_library(CSEC_Z3_LIBRARY NAMES libz3 z3 HINTS ${_csec_z3_hints} PATH_SUFFIXES bin lib)
    if(CSEC_Z3_INCLUDE_DIR AND CSEC_Z3_LIBRARY)
        message(STATUS "--verify: Z3 from ${CSEC_Z3_LIBRARY}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${CSEC_Z3_INCLUDE_DIR}")
        target_compile_definitions(${PROJECT_NAME} PRIVATE CSEC_HAVE_Z3)
        target_link_libraries(${PROJECT_NAME} PRIVATE "${CSEC_Z3_LIBRARY}")
        if(WIN32)
            # The import library sits next to libz3.dll; copy the DLL beside csec++.exe.
            get_filename_component(_csec_z3_dir "${CSEC_Z3_LIBRARY}" DIRECTORY)
            if(EXISTS "${_csec_z3_dir}/libz3.dll")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_csec_z3_dir}/libz3.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
            endif()
        endif()
    elseif(CSEC_WITH_Z3 STREQUAL "ON")
        message(FATAL_ERROR "CSEC_WITH_Z3=ON but z3++.h / libz3 were not found (run scripts/fetch_z3.ps1)")
    else()
        message(STATUS "--verify: Z3 not found; csec++ is built without the verifier")
    endif()
endif()

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE csec)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
################################################################################
# MSVC runtime library
################################################################################
# Release links the static CRT by default. Prebuilt LLVM packages built against the DLL CRT (/MD:
# e.g. conda-forge llvmdev) need -DCSEC_RELEASE_DLL_CRT=ON so the CRTs match.
option(CSEC_RELEASE_DLL_CRT "Use the DLL CRT (/MD) for Release builds of csec++" OFF)
if(CSEC_RELEASE_DLL_CRT)
    set(CSEC_RELEASE_CRT MultiThreadedDLL)
else()
    set(CSEC_RELEASE_CRT MultiThreaded)
endif()
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        ${CSEC_RELEASE_CRT}
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
find_package(LLVM CONFIG QUIET
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../llvm-project/build/lib/cmake/llvm"
    NO_DEFAULT_PATH
)
if(NOT LLVM_FOUND)
    find_package(LLVM CONFIG REQUIRED)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${LLVM_INCLUDE_DIRS}
)
# Some LLVM classes change layout with NDEBUG, so code using the LLVM headers must agree with the
# LLVM build. Prebuilt packages (Homebrew, conda-forge, apt) are built without assertions: compile
# csec++ with NDEBUG for them even in a Debug build, or it crashes inside LLVM. (MSVC builds use a
# matching vcpkg LLVM per configuration.)
if(NOT MSVC AND NOT LLVM_ENABLE_ASSERTIONS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
endif()

################################################################################
# Compile definitions
################################################################################
if(MSVC AND "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
elseif(MSVC AND "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /TP
        >
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
        /permissive-;
        /std:c++17;
        /utf-8;
        /sdl-;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
   
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Dependencies
################################################################################
# csec++ targets the host (InitializeNativeTarget/AsmPrinter/AsmParser), so link the host target's
# libraries: X86 on x86-64, AArch64 on Apple Silicon and arm64 Linux. A target component expands to
# its CodeGen, AsmParser, Desc, Info, ... libraries.
if(NOT LLVM_NATIVE_ARCH)
    message(FATAL_ERROR "The LLVM package does not report LLVM_NATIVE_ARCH; cannot pick the host target.")
endif()
if(NOT LLVM_NATIVE_ARCH IN_LIST LLVM_TARGETS_TO_BUILD)
    message(FATAL_ERROR "LLVM was built without the host target ${LLVM_NATIVE_ARCH} (has: ${LLVM_TARGETS_TO_BUILD}).")
endif()
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION}, host target ${LLVM_NATIVE_ARCH}")
llvm_map_components_to_libnames(LLVM_COMPONENT_LIBS
    AggressiveInstCombine
    Analysis
    AsmParser
    AsmPrinter
    BinaryFormat
    BitReader
    BitWriter
    CodeGen
    Core
    ExecutionEngine
    InstCombine
    Interpreter
    IPO
    IRPrinter
    IRReader
    JITLink
    Linker
    MC
    MCJIT
    MCParser
    Object
    OrcJIT
    Passes
    RuntimeDyld
    ScalarOpts
    Support
    Target
    TargetParser
    TransformUtils
    Vectorize
    ${LLVM_NATIVE_ARCH}
)

# Link the LLVM component libraries only. The C entry points csec++ uses (LLVMLinkInMCJIT,
# LLVMLinkInInterpreter) live in those static libraries; also linking the LLVM-C shared library
# (shipped e.g. by conda-forge) would give the process two copies of LLVM, so the JIT registered in
# the DLL's copy would be invisible to the static ExecutionEngine ("JIT has not been linked in").
#
# Packages built with LLVM_LINK_LLVM_DYLIB (conda-forge, Homebrew, apt's llvm-dev) are meant to be
# used through their single shared libLLVM; linking their static component archives instead
# crashes inside the X86 backend on Linux. Use libLLVM there (it also holds the C entry points),
# and keep the static components for vcpkg / source builds and on Windows.
if(NOT DEFINED CSEC_LLVM_SHARED)
    if(NOT WIN32 AND LLVM_LINK_LLVM_DYLIB AND TARGET LLVM)
        set(_csec_llvm_shared_default ON)
    else()
        set(_csec_llvm_shared_default OFF)
    endif()
    set(CSEC_LLVM_SHARED ${_csec_llvm_shared_default} CACHE BOOL "Link the shared libLLVM instead of the static LLVM components")
endif()
if(CSEC_LLVM_SHARED)
    message(STATUS "Linking the shared libLLVM")
    target_link_libraries(${PROJECT_NAME} PRIVATE LLVM)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LLVM_COMPONENT_LIBS}
    )
endif()
if(WIN32)
    find_library(CSEC_ZLIB_DEBUG_LIBRARY
        NAMES zlibd z
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/debug/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/vcpkg/blds/zlib/x64-windows-dbg"
        NO_DEFAULT_PATH
    )
    find_library(CSEC_ZLIB_RELEASE_LIBRARY
        NAMES zlib z
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/vcpkg/blds/zlib/x64-windows-rel"
        NO_DEFAULT_PATH
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:${CSEC_ZLIB_DEBUG_LIBRARY}>"
        "$<$<CONFIG:Release>:${CSEC_ZLIB_RELEASE_LIBRARY}>"
        ntdll
    )
endif()

if(WIN32 AND LLVM_LIBRARY_DIRS)
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${LLVM_LIBRARY_DIRS}
    )
endif()
