// main.cpp
#include "source_map.h"
#include "verifier.h"
#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "mangling.h"
#include "type_checker.h"
#include "utils.h"
#include "NativeRuntime.h"
#include "import_alias.h"

#include <iostream>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include "llvm_compat.h"
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "ProgramNode.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

// Declared in source_map.h (outside the anonymous namespace: the verifier and codegen use them).
std::vector<SourceLocation>& expandedLineMap() {
    static std::vector<SourceLocation> map;
    return map;
}

SourceLocation sourceLocationOf(int expandedLine) {
    const auto& map = expandedLineMap();
    if (expandedLine < 1 || static_cast<size_t>(expandedLine) > map.size()) return {};
    return map[static_cast<size_t>(expandedLine - 1)];
}

namespace {
enum class OutputMode {
    Run,
    RunIR,
    SyntaxOnly,
    Verify,
    EmitIR,
    EmitObject,
    EmitExecutable
};

void printUsage(const char* programName) {
    std::cerr
        << "Usage: " << programName << " [options] <input.csec>\n"
        << "Options:\n"
        << "  --verify            Prove requires/ensures/assert/invariant with Z3 (no code is generated)\n"
        << "  --verify-wrap       --verify with wrap-around (bit-vector) integers; overflow is not an error\n"
        << "  --verify-overflow   with --verify-wrap, still report possible integer overflow\n"
        << "  --verify-timeout <ms>  --verify with a per-check solver time limit (default 10000)\n"
        << "  --syntax-only, --parse-only  Parse only, no code generation\n"
        << "  --run                        Generate and run with LLVM interpreter\n"
        << "  --run-ir <file.ll>            Run an existing LLVM IR module with the interpreter\n"
        << "  --emit-ir, -S                Write LLVM IR (.ll)\n"
        << "  --emit-obj, -c               Write native object code (.obj/.o)\n"
        << "  --emit-exe                   Write native executable (.exe) (default)\n"
        << "  --static                     Link the runtime into the .exe (no System.Native.dll needed)\n"
        << "  -o <path>                    Output path for --emit-ir/--emit-obj/--emit-exe\n"
        << "  -O0 / -O1 / -O2 / -O3 / -O   Optimization level (-O = -O2; default: mem2reg only)\n"
        << "  -march=native                Generate code for this machine's CPU (e.g. AVX2/AVX-512)\n"
        << "  --link-lib <name-or-path>    Link an additional native library/import library\n"
        << "  --lib-path <dir>             Add a standard-library root searched by import\n"
        << "  --link-path <path>           Add a native library search path\n"
        << "  --strict-ownership           Force M2 strict ownership on (default; move-check all non-copy types)\n"
        << "  --no-strict-ownership        Opt out of default strict ownership (box-only move-checking)\n"
        << "  -- <args...>                 Pass arguments to a program run with --run\n"
        << "  --mangle <itanium|msvc> <signature>\n"
        << "                               Print a C++ ABI mangled name and exit\n";
}

std::string trimInputPath(std::string path) {
    const auto first = path.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = path.find_last_not_of(" \t\r\n");
    path = path.substr(first, last - first + 1);

    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }

    return path;
}

std::string promptForInputFile(const char* programName) {
    std::cout << "Input file path: ";

    std::string inputFile;
    if (!std::getline(std::cin, inputFile)) {
        std::cerr << "No input file provided." << std::endl;
        printUsage(programName);
        return "";
    }

    inputFile = trimInputPath(inputFile);
    if (inputFile.empty()) {
        std::cerr << "No input file provided." << std::endl;
        printUsage(programName);
    }

    return inputFile;
}

int printMangledName(const char* programName, const std::string& style, const std::string& signature) {
    try {
        if (style == "itanium") {
            std::cout << mangleItaniumSignature(signature) << std::endl;
            return 0;
        }
        if (style == "msvc") {
            std::cout << mangleMSVCSignature(signature) << std::endl;
            return 0;
        }
        std::cerr << "Unknown mangling style: " << style << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Mangling failed: " << ex.what() << std::endl;
    }
    std::cerr << "Usage: " << programName << " --mangle <itanium|msvc> <signature>" << std::endl;
    return 1;
}

std::string defaultOutputPath(const std::string& inputFile, OutputMode mode) {
    std::filesystem::path path(inputFile);
    if (path.has_parent_path()) {
        path = path.parent_path() / path.stem();
    }
    else {
        path = path.stem();
    }

    if (mode == OutputMode::EmitIR) {
        path.replace_extension(".ll");
    }
    else if (mode == OutputMode::EmitObject) {
#ifdef _WIN32
        path.replace_extension(".obj");
#else
        path.replace_extension(".o");
#endif
    }
    else if (mode == OutputMode::EmitExecutable) {
#ifdef _WIN32
        path.replace_extension(".exe");
#else
        path.replace_extension("");
#endif
    }

    return path.string();
}

llvm::Value* coerceReturnToInt32(llvm::IRBuilder<>& builder, llvm::Value* value) {
    if (!value) {
        return builder.getInt32(0);
    }

    llvm::Type* sourceType = value->getType();
    llvm::Type* int32Type = builder.getInt32Ty();
    if (sourceType == int32Type) {
        return value;
    }
    if (sourceType->isIntegerTy()) {
        unsigned sourceBits = sourceType->getIntegerBitWidth();
        if (sourceBits < 32) {
            return builder.CreateSExt(value, int32Type, "main.ret.sext");
        }
        if (sourceBits > 32) {
            return builder.CreateTrunc(value, int32Type, "main.ret.trunc");
        }
    }
    if (sourceType->isFloatingPointTy()) {
        return builder.CreateFPToSI(value, int32Type, "main.ret.fptosi");
    }
    return builder.getInt32(0);
}

llvm::Function* rebuildRuntimeMain(CodeGenerator& codeGen) {
    llvm::Function* userMain = codeGen.module->getFunction("_main");
    if (!userMain || userMain->arg_size() != 0) {
        return codeGen.mainFunction;
    }

    codeGen.mainFunction->deleteBody();
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(codeGen.context, "entry", codeGen.mainFunction);
    llvm::IRBuilder<> builder(entry);

    auto* i8PtrTy = llvm::PointerType::getUnqual(codeGen.context);
    auto* argvTy = llvm::PointerType::getUnqual(i8PtrTy->getContext());
    auto* setArgs = codeGen.module->getFunction("csec_set_command_line_args");
    if (!setArgs) {
        setArgs = llvm::Function::Create(
            llvm::FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), argvTy}, false),
            llvm::Function::ExternalLinkage,
            "csec_set_command_line_args",
            codeGen.module.get());
    }
    codeGen.requireSystemNative();
    builder.CreateCall(setArgs, {codeGen.mainFunction->getArg(0), codeGen.mainFunction->getArg(1)});

    llvm::Value* result = nullptr;
    if (userMain->getReturnType()->isVoidTy()) {
        builder.CreateCall(userMain);
    }
    else {
        result = builder.CreateCall(userMain, {}, "user.main");
    }

    builder.CreateRet(coerceReturnToInt32(builder, result));

    auto* jitFunction = llvm::Function::Create(
        llvm::FunctionType::get(builder.getInt32Ty(), false),
        llvm::Function::InternalLinkage,
        "__csec_jit_entry",
        codeGen.module.get());
    llvm::BasicBlock* jitEntry = llvm::BasicBlock::Create(codeGen.context, "entry", jitFunction);
    llvm::IRBuilder<> jitBuilder(jitEntry);
    llvm::Value* jitResult = nullptr;
    if (userMain->getReturnType()->isVoidTy()) {
        jitBuilder.CreateCall(userMain);
    }
    else {
        jitResult = jitBuilder.CreateCall(userMain, {}, "user.main");
    }
    jitBuilder.CreateRet(coerceReturnToInt32(jitBuilder, jitResult));
    return jitFunction;
}

int runIRFile(const std::string& inputFile, const std::vector<std::string>& runtimeArgs, const char* programName) {
    LLVMLinkInMCJIT();
    LLVMLinkInInterpreter();
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // The runtime sits next to csec++ (System.Native.dll, libSystem.Native.so, libSystem.Native.dylib).
    // Locate csec++ itself rather than trusting argv[0], which has no directory when run from PATH.
    static int mainExecutableAnchor = 0;
    std::filesystem::path executable = llvm::sys::fs::getMainExecutable(programName, &mainExecutableAnchor);
    if (executable.empty()) executable = std::filesystem::path(programName);
#if defined(_WIN32)
    const char* runtimeFileName = "System.Native.dll";
#elif defined(__APPLE__)
    const char* runtimeFileName = "libSystem.Native.dylib";
#else
    const char* runtimeFileName = "libSystem.Native.so";
#endif
    std::filesystem::path runtimePath = executable.parent_path() / runtimeFileName;
    std::string runtimeLoadError;
    llvm::sys::DynamicLibrary runtimeLibrary = llvm::sys::DynamicLibrary::getPermanentLibrary(
        runtimePath.string().c_str(), &runtimeLoadError);
    if (!runtimeLibrary.isValid()) {
        std::cerr << "Failed to load native runtime '" << runtimePath.string()
                  << "': " << runtimeLoadError << std::endl;
        return 1;
    }

    auto context = std::make_unique<llvm::LLVMContext>();
    llvm::SMDiagnostic diagnostic;
    auto module = llvm::parseIRFile(inputFile, diagnostic, *context);
    if (!module) {
        diagnostic.print(programName, llvm::errs());
        return 1;
    }
    if (llvm::verifyModule(*module, &llvm::errs())) {
        std::cerr << "Input LLVM IR is invalid." << std::endl;
        return 1;
    }

    // The host compiler emits numeric string-global identifiers (for example
    // @0). LLVM preserves those as unnamed GlobalValues, which the legacy
    // Interpreter backend rejects during module setup. Give them stable names
    // before handing the module to the execution engine.
    unsigned anonymousGlobalIndex = 0;
    for (llvm::GlobalVariable& global : module->globals()) {
        if (!global.hasName()) {
            global.setName("__csec_runir_global_" + std::to_string(anonymousGlobalIndex++));
        }
    }

    // The Interpreter does not reliably discover DLL exports through the
    // platform loader. Bind every external declaration that System.Native
    // exports before creating the execution engine.
    std::vector<std::pair<std::string, void*>> runtimeSymbols;
#ifdef _WIN32
    HMODULE runtimeModule = ::LoadLibraryW(runtimePath.c_str());
    if (!runtimeModule) {
        std::cerr << "Failed to get a Windows module handle for native runtime '"
                  << runtimePath.string() << "'." << std::endl;
        return 1;
    }
#endif
    for (llvm::Function& function : *module) {
        if (!function.isDeclaration() || function.isIntrinsic()) {
            continue;
        }
        const std::string name = function.getName().str();
#ifdef _WIN32
        void* address = reinterpret_cast<void*>(::GetProcAddress(runtimeModule, name.c_str()));
#else
        void* address = runtimeLibrary.getAddressOfSymbol(name.c_str());
#endif
        if (address) {
            llvm::sys::DynamicLibrary::AddSymbol(function.getName(), address);
            runtimeSymbols.emplace_back(name, address);
        }
    }

    std::string engineError;
    auto engine = llvm::EngineBuilder(std::move(module))
        .setErrorStr(&engineError)
        .setEngineKind(llvm::EngineKind::JIT)
        .setOptLevel(llvm::CodeGenOpt::getLevel(0).value())
        .create();
    if (!engine) {
        std::cerr << "Failed to create ExecutionEngine: " << engineError << std::endl;
        return 1;
    }

    for (const auto& [name, address] : runtimeSymbols) {
        if (llvm::GlobalValue* symbol = engine->FindFunctionNamed(name)) {
            engine->addGlobalMapping(symbol, address);
        }
    }

    llvm::Function* entryFunction = engine->FindFunctionNamed("main");
    if (!entryFunction) {
        // Host-emitted IR uses this zero-argument entrypoint instead of a C
        // ABI main function. It is still a valid executable CSEC program.
        entryFunction = engine->FindFunctionNamed("__csec_jit_entry");
    }
    if (!entryFunction) {
        std::cerr << "Input LLVM IR has no executable entry function." << std::endl;
        return 1;
    }

    std::vector<std::string> arguments;
    arguments.reserve(runtimeArgs.size() + 1);
    arguments.push_back(inputFile);
    arguments.insert(arguments.end(), runtimeArgs.begin(), runtimeArgs.end());

    // The program reads its arguments through the runtime (commandLineArg / csec_command_line_arg),
    // e.g. the self-hosted compiler's input and output paths: hand them to the loaded runtime.
    using SetCommandLineArgs = void (*)(int, char**);
#ifdef _WIN32
    auto setCommandLineArgs = reinterpret_cast<SetCommandLineArgs>(
        ::GetProcAddress(runtimeModule, "csec_set_command_line_args"));
#else
    auto setCommandLineArgs = reinterpret_cast<SetCommandLineArgs>(
        runtimeLibrary.getAddressOfSymbol("csec_set_command_line_args"));
#endif
    if (!setCommandLineArgs) {
        std::cerr << "Native runtime does not export csec_set_command_line_args." << std::endl;
        return 1;
    }
    std::vector<char*> runtimeArgv;
    runtimeArgv.reserve(arguments.size());
    for (std::string& argument : arguments) {
        runtimeArgv.push_back(argument.data());
    }
    setCommandLineArgs(static_cast<int>(runtimeArgv.size()), runtimeArgv.data());

    return engine->runFunctionAsMain(entryFunction, arguments, {});
}

std::string trimAscii(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool startsWithImportKeyword(const std::string& text) {
    if (text.rfind("import", 0) != 0) {
        return false;
    }
    if (text.size() == 6) {
        return true;
    }
    char next = text[6];
    return next == ' ' || next == '\t' || next == '"' || next == '\'';
}

std::string parseImportTarget(const std::string& trimmedLine) {
    if (!startsWithImportKeyword(trimmedLine)) {
        return "";
    }

    // `import a.b.c`, `import "path.csec"`, optionally with an alias: `import a.b.c = name;`
    return csec_imports::splitImport(trimmedLine.substr(6)).target;
}

// Errors found while expanding imports (bad aliases); compilation stops after expansion if any.
std::vector<std::string>& importErrors() {
    static std::vector<std::string> errors;
    return errors;
}

// Build a path from a UTF-8 string. Constructing std::filesystem::path directly from a narrow
// std::string decodes it with the active code page on Windows, which hangs in the MSVC STL for
// some multi-byte sequences (e.g. Korean identifiers in an import path); u8path decodes UTF-8.
std::filesystem::path utf8Path(const std::string& utf8) {
    return std::filesystem::u8path(utf8);
}

std::filesystem::path resolveImportPath(
    const std::filesystem::path& includingFile,
    const std::string& target) {
    std::filesystem::path baseDir = includingFile.parent_path();
    // Interpret the import target as UTF-8. Constructing a path directly from a narrow std::string
    // routes through the active code page on Windows, which hangs in the MSVC STL for some
    // multi-byte (e.g. Korean) sequences; u8path decodes the bytes as UTF-8 instead.
    std::filesystem::path requested = utf8Path(target);
    if (!requested.has_extension()) {
        requested.replace_extension(".csec");
    }
    if (requested.is_absolute()) {
        return requested;
    }

    std::vector<std::filesystem::path> candidates = {
        baseDir / requested,
        std::filesystem::current_path() / requested
    };

    if (target.find('.') != std::string::npos && target.find('/') == std::string::npos &&
        target.find('\\') == std::string::npos) {
        std::string dotted = target;
        for (char& ch : dotted) {
            if (ch == '.') {
                ch = static_cast<char>(std::filesystem::path::preferred_separator);
            }
        }
        std::filesystem::path dottedPath = utf8Path(dotted);
        if (!dottedPath.has_extension()) {
            dottedPath.replace_extension(".csec");
        }
        candidates.push_back(baseDir / dottedPath);
        candidates.push_back(std::filesystem::current_path() / dottedPath);
    }

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_regular_file(candidate, ec)) {
            return candidate;
        }
    }
    // Fall back to the standard library shipped with the compiler (shared with the self-host path).
    if (char* library = csec_find_library_module(target.c_str())) {
        std::string found = library;
        std::free(library);
        if (!found.empty()) return utf8Path(found);
    }
    return candidates.front();
}

std::string expandImports(
    const std::filesystem::path& inputPath,
    std::vector<std::filesystem::path>& includeStack,
    std::vector<std::filesystem::path>& includedFiles) {
    std::error_code ec;
    auto canonicalPath = std::filesystem::weakly_canonical(inputPath, ec);
    if (ec) {
        canonicalPath = std::filesystem::absolute(inputPath, ec);
    }
    if (ec) {
        canonicalPath = inputPath;
    }

    for (const auto& active : includeStack) {
        if (active == canonicalPath) {
            std::cerr << "Import cycle detected at " << canonicalPath.string() << std::endl;
            return "";
        }
    }
    for (const auto& included : includedFiles) {
        if (included == canonicalPath) {
            return "";
        }
    }

    std::string source = read_utf8_file(canonicalPath.string());
    if (source.empty() && (!std::filesystem::exists(canonicalPath, ec) ||
                           !std::filesystem::is_regular_file(canonicalPath, ec))) {
        std::cerr << "Unable to read import: " << canonicalPath.string() << std::endl;
        return "";
    }

    // `import a.b = name;` makes `name.f` refer to the module's top-level function f in this file.
    {
        std::vector<std::string> errors;
        source = csec_imports::applyImportAliases(
            source,
            [&](const std::string& target) {
                const auto path = resolveImportPath(canonicalPath, target);
                std::error_code existsEc;
                return std::filesystem::is_regular_file(path, existsEc) ? path.string() : std::string();
            },
            [](const std::string& path) { return read_utf8_file(path); },
            errors);
        for (const auto& error : errors) {
            importErrors().push_back(canonicalPath.string() + ": " + error);
        }
    }

    includeStack.push_back(canonicalPath);
    includedFiles.push_back(canonicalPath);

    std::string output;
    size_t lineStart = 0;
    int lineNumber = 0;
    auto& lineMap = expandedLineMap();
    while (lineStart <= source.size()) {
        ++lineNumber;
        size_t lineEnd = source.find('\n', lineStart);
        bool hasNewline = lineEnd != std::string::npos;
        if (!hasNewline) {
            lineEnd = source.size();
        }

        std::string line = source.substr(lineStart, lineEnd - lineStart);
        std::string target = parseImportTarget(trimAscii(line));
        if (!target.empty()) {
            auto importPath = resolveImportPath(canonicalPath, target);
            std::error_code importEc;
            if (std::filesystem::exists(importPath, importEc) && std::filesystem::is_regular_file(importPath, importEc)) {
                const std::string imported = expandImports(importPath, includeStack, includedFiles);
                output += imported;
                // The "\n" closes the module's last line when it had no newline; otherwise it is an
                // empty line standing for this import line.
                if (imported.empty() || imported.back() == '\n') {
                    lineMap.push_back({canonicalPath.string(), lineNumber});
                }
                output += "\n";
            }
            else {
                // Unresolved imports stay in place (and are ignored); drop an alias the parser does not need.
                output += csec_imports::splitImport(trimAscii(line).substr(6)).alias.empty() ? line : "import " + target;
                lineMap.push_back({canonicalPath.string(), lineNumber});
                if (hasNewline) {
                    output += "\n";
                }
            }
        }
        else {
            output += line;
            lineMap.push_back({canonicalPath.string(), lineNumber});
            if (hasNewline) {
                output += "\n";
            }
        }

        if (!hasNewline) {
            break;
        }
        lineStart = lineEnd + 1;
    }

    includeStack.pop_back();
    return output;
}

std::string expandImports(const std::string& inputFile) {
    std::vector<std::filesystem::path> includeStack;
    std::vector<std::filesystem::path> includedFiles;
    return expandImports(std::filesystem::path(inputFile), includeStack, includedFiles);
}

bool writeIRToFile(llvm::Module& module, const std::string& outputPath, bool announce = true) {
    std::error_code ec;
    llvm::raw_fd_ostream output(outputPath, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        std::cerr << "Failed to open IR output file '" << outputPath << "': " << ec.message() << std::endl;
        return false;
    }

    module.print(output, nullptr);
    if (announce) {
        std::cout << "Wrote LLVM IR: " << outputPath << std::endl;
    }
    return true;
}

std::string quoteCommandArg(const std::string& value) {
    std::string quoted = "\"";
    for (char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        }
        else {
            quoted += ch;
        }
    }
    quoted += "\"";
    return quoted;
}

std::string getEnvironmentVariable(const std::string& name) {
#ifdef _WIN32
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, name.c_str()) != 0 || !value) {
        return "";
    }
    std::string result(value);
    free(value);
    return result;
#else
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : "";
#endif
}

std::filesystem::path findToolPath(const std::string& envVar, const std::string& fallbackName) {
    std::string envValue = getEnvironmentVariable(envVar);
    if (!envValue.empty()) {
        std::filesystem::path path(envValue);
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec)) {
            path /= fallbackName;
        }
        if (std::filesystem::exists(path, ec)) {
            auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
            return ec ? path : canonicalPath;
        }
        return path;
    }
    return fallbackName;
}

std::filesystem::path findLlvmTool(const std::string& toolName) {
    std::string llvmBin = getEnvironmentVariable("LLVM_BIN");
    if (!llvmBin.empty()) {
        std::filesystem::path candidate = std::filesystem::path(llvmBin) / toolName;
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            auto canonicalPath = std::filesystem::weakly_canonical(candidate, ec);
            return ec ? candidate : canonicalPath;
        }
    }

    std::filesystem::path candidate = std::filesystem::absolute(
        std::filesystem::path("..") / "llvm-project" / "build" / "bin" / toolName);
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
        auto canonicalPath = std::filesystem::weakly_canonical(candidate, ec);
        return ec ? candidate : canonicalPath;
    }
    return toolName;
}

std::filesystem::path findClangTool() {
#ifdef _WIN32
    std::filesystem::path fromLlvmBin = findLlvmTool("clang.exe");
#else
    std::filesystem::path fromLlvmBin = findLlvmTool("clang");
#endif
    std::error_code ec;
    if (std::filesystem::exists(fromLlvmBin, ec)) {
        auto canonicalPath = std::filesystem::weakly_canonical(fromLlvmBin, ec);
        return ec ? fromLlvmBin : canonicalPath;
    }

#ifdef _WIN32
    std::filesystem::path programFilesClang = R"(C:\Program Files\LLVM\bin\clang.exe)";
    if (std::filesystem::exists(programFilesClang, ec)) {
        auto canonicalPath = std::filesystem::weakly_canonical(programFilesClang, ec);
        return ec ? programFilesClang : canonicalPath;
    }
    return findToolPath("CLANG", "clang.exe");
#else
    return findToolPath("CLANG", "clang");
#endif
}

std::filesystem::path findNativeRuntimeSource() {
    std::vector<std::filesystem::path> candidates = {
        std::filesystem::path("src") / "NativeRuntime.cpp",
        std::filesystem::path("..") / "src" / "NativeRuntime.cpp",
        std::filesystem::path("..") / ".." / "src" / "NativeRuntime.cpp",
        std::filesystem::path("NativeRuntime.cpp")
    };
    std::error_code ec;
    for (const auto& candidate : candidates) {
        auto absolute = std::filesystem::absolute(candidate);
        if (std::filesystem::exists(absolute, ec)) {
            auto canonicalPath = std::filesystem::weakly_canonical(absolute, ec);
            return ec ? absolute : canonicalPath;
        }
    }
    return std::filesystem::absolute(std::filesystem::path("src") / "NativeRuntime.cpp");
}

bool writeObjectToFile(llvm::Module& module, const std::string& outputPath, bool announce = true) {
#ifdef _WIN32
    std::filesystem::path llcPath = findLlvmTool("llc.exe");
#else
    std::filesystem::path llcPath = findLlvmTool("llc");
#endif

    std::filesystem::path tempIR = std::filesystem::path(outputPath).replace_extension(".tmp.ll");
    if (!writeIRToFile(module, tempIR.string(), false)) {
        return false;
    }

    std::error_code ec;
    std::string command;
    if (std::filesystem::exists(llcPath, ec)) {
        command = quoteCommandArg(llcPath.string()) +
            " -filetype=obj -o " + quoteCommandArg(outputPath) + " " + quoteCommandArg(tempIR.string());
    }
    else {
        std::filesystem::path clangPath = findClangTool();
        command = quoteCommandArg(clangPath.string()) +
            " -Wno-override-module -c " + quoteCommandArg(tempIR.string()) +
            " -o " + quoteCommandArg(outputPath);
    }
#ifdef _WIN32
    command = "\"" + command + "\"";
#endif
    int result = std::system(command.c_str());
    std::error_code removeError;
    std::filesystem::remove(tempIR, removeError);
    if (result != 0) {
        std::cerr << "Failed to emit object file via LLVM toolchain. Command exited with code " << result << std::endl;
        return false;
    }

    if (announce) {
        std::cout << "Wrote object file: " << outputPath << std::endl;
    }
    return true;
}

bool startsWithSystemLibraryPrefix(const std::string& library) {
    return library.rfind("System.", 0) == 0;
}

bool hasLibraryFileExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    return extension == ".lib" || extension == ".dll" || extension == ".so" ||
           extension == ".a" || extension == ".dylib" || extension == ".o" ||
           extension == ".obj";
}

std::string systemSharedLibraryFileName(const std::string& library) {
    std::filesystem::path path(library);
    if (hasLibraryFileExtension(path)) {
        return path.filename().string();
    }
#ifdef _WIN32
    return library + ".dll";
#elif defined(__APPLE__)
    return "lib" + library + ".dylib";
#else
    return library + ".so";
#endif
}

std::string systemLinkLibraryFileName(const std::string& library) {
    std::filesystem::path path(library);
    if (hasLibraryFileExtension(path)) {
        return path.filename().string();
    }
#ifdef _WIN32
    return library + ".lib";
#elif defined(__APPLE__)
    return "lib" + library + ".dylib";
#else
    return library + ".so";
#endif
}

std::vector<std::filesystem::path> systemLibrarySearchDirectories(
    const std::vector<std::string>& linkPaths,
    const std::string& outputPath) {
    std::vector<std::filesystem::path> directories;
    auto addDirectory = [&](std::filesystem::path path) {
        if (path.empty()) return;
        std::error_code ec;
        path = std::filesystem::absolute(path, ec);
        if (ec) return;
        for (const auto& existing : directories) {
            if (existing == path) return;
        }
        directories.push_back(path);
    };

    for (const auto& linkPath : linkPaths) {
        addDirectory(linkPath);
    }
    addDirectory(std::filesystem::path(outputPath).parent_path());
    addDirectory(std::filesystem::current_path());
    addDirectory(std::filesystem::current_path() / "Debug");
    addDirectory(std::filesystem::current_path() / "Release");
    addDirectory(std::filesystem::current_path() / "x64" / "Debug");
    addDirectory(std::filesystem::current_path() / "x64" / "Release");
    addDirectory(std::filesystem::current_path() / "build");
    addDirectory(std::filesystem::current_path() / "build-cmake-check");

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path(), ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec)) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (name.rfind("build", 0) == 0) {
            addDirectory(entry.path() / "Debug");
            addDirectory(entry.path() / "Release");
        }
    }

    return directories;
}

std::filesystem::path findLibraryFile(
    const std::string& library,
    const std::string& fileName,
    const std::vector<std::filesystem::path>& directories) {
    std::filesystem::path libraryPath(library);
    std::error_code ec;
    if ((libraryPath.has_parent_path() || hasLibraryFileExtension(libraryPath)) &&
        std::filesystem::exists(libraryPath, ec)) {
        auto canonicalPath = std::filesystem::weakly_canonical(libraryPath, ec);
        return ec ? libraryPath : canonicalPath;
    }

    for (const auto& directory : directories) {
        std::filesystem::path candidate = directory / fileName;
        if (std::filesystem::exists(candidate, ec)) {
            auto canonicalPath = std::filesystem::weakly_canonical(candidate, ec);
            return ec ? candidate : canonicalPath;
        }
    }

    return {};
}

void copyRuntimeLibraryNextToOutput(
    const std::filesystem::path& runtimeLibrary,
    const std::string& outputPath) {
    if (runtimeLibrary.empty()) {
        return;
    }

    std::filesystem::path outputDirectory = std::filesystem::path(outputPath).parent_path();
    if (outputDirectory.empty()) {
        outputDirectory = std::filesystem::current_path();
    }
    std::error_code ec;
    std::filesystem::create_directories(outputDirectory, ec);
    std::filesystem::copy_file(
        runtimeLibrary,
        outputDirectory / runtimeLibrary.filename(),
        std::filesystem::copy_options::overwrite_existing,
        ec);
}

void copyRuntimeLibrariesNextToOutput(
    const std::vector<std::filesystem::path>& runtimeLibraries,
    const std::string& outputPath) {
    for (const auto& library : runtimeLibraries) {
        copyRuntimeLibraryNextToOutput(library, outputPath);
    }
}

std::string nativeLinkLibraryArg(const std::string& library) {
    if (library.empty()) {
        return "";
    }
    if (library.rfind("-l", 0) == 0 || library.rfind("-Wl,", 0) == 0) {
        return library;
    }

    std::filesystem::path path(library);
    const bool hasDirectory = path.has_parent_path();
    const std::string extension = path.extension().string();
    if (hasDirectory || extension == ".o") {
        return quoteCommandArg(library);
    }
    if (extension == ".so") {
        return "-l:" + path.filename().string();
    }
    if (extension == ".a") {
#ifdef __APPLE__
        std::string stemName = path.stem().string();
        if (stemName.rfind("lib", 0) == 0 && stemName.size() > 3) {
            return "-l" + stemName.substr(3);
        }
        return quoteCommandArg(library);
#else
        return "-l:" + path.filename().string();
#endif
    }
    if (extension == ".dylib") {
        std::string stem = path.stem().string();
        if (stem.rfind("lib", 0) == 0 && stem.size() > 3) {
            return "-l" + stem.substr(3);
        }
        return quoteCommandArg(library);
    }
    std::string stem = path.stem().string();
    if (path.filename().string().rfind("lib", 0) == 0 && stem.size() > 3) {
        return "-l" + stem.substr(3);
    }
    return "-l" + library;
}

std::string resolvedNativeLinkLibraryArg(
    const std::string& library,
    const std::vector<std::string>& linkPaths,
    const std::string& outputPath,
    std::vector<std::filesystem::path>& runtimeLibraries) {
    if (!startsWithSystemLibraryPrefix(library)) {
        return nativeLinkLibraryArg(library);
    }

    auto directories = systemLibrarySearchDirectories(linkPaths, outputPath);
    std::filesystem::path linkLibrary = findLibraryFile(library, systemLinkLibraryFileName(library), directories);
    std::filesystem::path sharedLibrary = findLibraryFile(library, systemSharedLibraryFileName(library), directories);
    if (!sharedLibrary.empty()) {
        runtimeLibraries.push_back(sharedLibrary);
    }
    if (!linkLibrary.empty()) {
        return quoteCommandArg(linkLibrary.string());
    }

#ifdef __APPLE__
    return "-l:" + systemLinkLibraryFileName(library);
#else
    return "-l:" + systemLinkLibraryFileName(library);
#endif
}

std::string resolvedWindowsLinkLibraryArg(
    const std::string& library,
    const std::vector<std::string>& linkPaths,
    const std::string& outputPath,
    std::vector<std::filesystem::path>& runtimeLibraries) {
    if (!startsWithSystemLibraryPrefix(library)) {
        return quoteCommandArg(library);
    }

    auto directories = systemLibrarySearchDirectories(linkPaths, outputPath);
    std::filesystem::path importLibrary = findLibraryFile(library, systemLinkLibraryFileName(library), directories);
    std::filesystem::path sharedLibrary = findLibraryFile(library, systemSharedLibraryFileName(library), directories);
    if (!sharedLibrary.empty()) {
        runtimeLibraries.push_back(sharedLibrary);
    }
    if (!importLibrary.empty()) {
        return quoteCommandArg(importLibrary.string());
    }
    return quoteCommandArg(systemLinkLibraryFileName(library));
}

std::filesystem::path findWindowsLinker() {
    std::filesystem::path linkerPath = "link.exe";
    if (std::filesystem::exists(linkerPath)) {
        return linkerPath;
    }

#ifdef _WIN32
    std::vector<std::filesystem::path> msvcRoots = {
        R"(C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC)",
        R"(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC)"
    };
    std::error_code ec;
    for (const auto& msvcRoot : msvcRoots) {
        if (std::filesystem::exists(msvcRoot, ec)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                     msvcRoot, std::filesystem::directory_options::skip_permission_denied, ec)) {
                if (ec) {
                    break;
                }

                auto path = entry.path();
                std::string pathText = path.string();
                if (entry.is_regular_file(ec) && path.filename() == "link.exe" &&
                    pathText.find("\\bin\\Hostx64\\x64\\") != std::string::npos) {
                    auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
                    return ec ? path : canonicalPath;
                }
            }
        }
    }
#endif

    return linkerPath;
}

#ifdef _WIN32
std::filesystem::path findMsvcX64LibPath(const std::filesystem::path& linkPath) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(linkPath, ec);
    const auto path = ec ? linkPath : canonical;
    const auto versionDir = path.parent_path().parent_path().parent_path().parent_path();
    auto libPath = versionDir / "lib" / "x64";
    if (std::filesystem::exists(libPath / "libcmt.lib", ec)) {
        return libPath;
    }
    return {};
}

std::filesystem::path findWindowsSdkLibRoot() {
    std::filesystem::path sdkLibRoot = R"(C:\Program Files (x86)\Windows Kits\10\Lib)";
    std::error_code ec;
    if (!std::filesystem::exists(sdkLibRoot, ec)) {
        return {};
    }

    std::filesystem::path best;
    for (const auto& entry : std::filesystem::directory_iterator(sdkLibRoot, ec)) {
        if (ec || !entry.is_directory(ec)) {
            continue;
        }
        auto candidate = entry.path();
        if (std::filesystem::exists(candidate / "ucrt" / "x64" / "ucrt.lib", ec) &&
            std::filesystem::exists(candidate / "um" / "x64" / "kernel32.lib", ec) &&
            (best.empty() || candidate.filename().string() > best.filename().string())) {
            best = candidate;
        }
    }
    return best;
}

std::filesystem::path findNativeRuntimeObject() {
    std::vector<std::filesystem::path> candidates = {
        std::filesystem::absolute(std::filesystem::path("csec++") / "x64" / "Debug" / "NativeRuntime.obj"),
        std::filesystem::absolute(std::filesystem::path("csec++") / "x64" / "Release" / "NativeRuntime.obj"),
        std::filesystem::absolute(std::filesystem::path("x64") / "Debug" / "NativeRuntime.obj"),
        std::filesystem::absolute(std::filesystem::path("x64") / "Release" / "NativeRuntime.obj"),
        std::filesystem::absolute(std::filesystem::path("Debug") / "NativeRuntime.obj"),
        std::filesystem::absolute(std::filesystem::path("Release") / "NativeRuntime.obj")
    };
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
    return {};
}

bool compileNativeRuntimeObject(const std::filesystem::path& linkPath, const std::filesystem::path& outputObject) {
    std::filesystem::path clPath = linkPath.parent_path() / "cl.exe";
    std::error_code ec;
    if (!std::filesystem::exists(clPath, ec)) {
        std::cerr << "MSVC compiler not found next to linker: " << clPath << std::endl;
        return false;
    }

    std::filesystem::path sourcePath = findNativeRuntimeSource();
    const auto versionDir = linkPath.parent_path().parent_path().parent_path().parent_path();
    const auto vcInclude = versionDir / "include";
    const auto sdkRoot = findWindowsSdkLibRoot();
    std::string command =
        quoteCommandArg(clPath.string()) +
        " /nologo /c /EHsc /std:c++17 /MT /O2 /DCSEC_NATIVE_STATIC /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS " +
        "/I" + quoteCommandArg(vcInclude.string()) + " ";
    if (!sdkRoot.empty()) {
        auto sdkBase = sdkRoot.parent_path().parent_path();
        command +=
            "/I" + quoteCommandArg((sdkBase / "Include" / sdkRoot.filename() / "ucrt").string()) + " " +
            "/I" + quoteCommandArg((sdkBase / "Include" / sdkRoot.filename() / "shared").string()) + " " +
            "/I" + quoteCommandArg((sdkBase / "Include" / sdkRoot.filename() / "um").string()) + " ";
    }
    command +=
        quoteCommandArg(sourcePath.string()) +
        " /Fo:" + quoteCommandArg(outputObject.string());
    command = "\"" + command + "\"";
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to compile NativeRuntime.cpp. Command exited with code " << result << std::endl;
        return false;
    }
    return true;
}
#endif

bool writeExecutableToFile(
    llvm::Module& module,
    const std::string& outputPath,
    const std::vector<std::string>& linkLibraries,
    const std::vector<std::string>& linkPaths,
    bool staticRuntime) {
#ifndef _WIN32
    std::filesystem::path tempObject = std::filesystem::path(outputPath).replace_extension(".tmp.o");
    if (!writeObjectToFile(module, tempObject.string(), false)) {
        return false;
    }

    std::filesystem::path cxxPath = findToolPath("CXX", "c++");
    std::string cxx = cxxPath.string();
    std::vector<std::filesystem::path> runtimeLibrariesToCopy;

    // --static: compile the native runtime (NativeRuntime.cpp + its .inc units) straight into a temp
    // object and link it in, so the executable carries every csec_* symbol itself and does not depend
    // on a System.Native shared object at run time.
    std::filesystem::path staticRuntimeObject;
    if (staticRuntime) {
        staticRuntimeObject = std::filesystem::path(outputPath).replace_extension(".runtime.o");
        std::filesystem::path runtimeSource = findNativeRuntimeSource();
        std::string compileCommand =
            quoteCommandArg(cxx) + " -std=c++17 -O2 -DCSEC_NATIVE_STATIC -c " +
            quoteCommandArg(runtimeSource.string()) + " -o " + quoteCommandArg(staticRuntimeObject.string());
        if (std::system(compileCommand.c_str()) != 0) {
            std::cerr << "Failed to compile native runtime for --static from " << runtimeSource << std::endl;
            std::filesystem::remove(tempObject);
            return false;
        }
    }

    std::string linkCommand =
        quoteCommandArg(cxx) +
        " -o " + quoteCommandArg(outputPath) + " " +
        quoteCommandArg(tempObject.string());
    if (staticRuntime) {
        linkCommand += " " + quoteCommandArg(staticRuntimeObject.string());
    }
    for (const auto& linkPathArg : linkPaths) {
        if (!linkPathArg.empty()) {
            linkCommand += " -L" + quoteCommandArg(linkPathArg);
        }
    }
    for (const auto& library : linkLibraries) {
        // The runtime object already satisfies the System.* symbols under --static; skip its shared lib.
        if (staticRuntime && startsWithSystemLibraryPrefix(library)) {
            continue;
        }
        std::string linkArg = resolvedNativeLinkLibraryArg(library, linkPaths, outputPath, runtimeLibrariesToCopy);
        if (!linkArg.empty()) {
            linkCommand += " " + linkArg;
        }
    }
    linkCommand += " -pthread";
#ifndef __APPLE__
    if (!runtimeLibrariesToCopy.empty()) {
        linkCommand += " -Wl,-rpath,\\$ORIGIN";
    }
#endif
#ifndef __APPLE__
    linkCommand += " -ldl";
#endif

    int linkResult = std::system(linkCommand.c_str());
    std::error_code removeError;
    std::filesystem::remove(tempObject, removeError);
    if (!staticRuntimeObject.empty()) {
        std::filesystem::remove(staticRuntimeObject, removeError);
    }
    if (linkResult != 0) {
        std::cerr << "Failed to emit executable via native linker. Command exited with code " << linkResult << std::endl;
        return false;
    }

    copyRuntimeLibrariesNextToOutput(runtimeLibrariesToCopy, outputPath);
    std::cout << "Wrote executable: " << outputPath << (staticRuntime ? " (static runtime)" : "") << std::endl;
    return true;
#else
    std::filesystem::path tempObject = std::filesystem::path(outputPath).replace_extension(".tmp.obj");
    if (!writeObjectToFile(module, tempObject.string(), false)) {
        return false;
    }

    std::filesystem::path linkPath = findWindowsLinker();
    std::vector<std::filesystem::path> runtimeLibrariesToCopy;
    std::vector<std::string> linkArgs = {
        quoteCommandArg(linkPath.string()),
        "/NOLOGO",
        "/SUBSYSTEM:CONSOLE",
        "/STACK:16777216",
        "/OUT:" + quoteCommandArg(outputPath),
        quoteCommandArg(tempObject.string())
    };

    // --static: compile the native runtime (NativeRuntime.cpp + its .inc units) into a temp object with
    // CSEC_NATIVE_STATIC (plain, undecorated symbols) and link it straight in, so the executable carries
    // every csec_* symbol itself and needs no System.Native.dll beside it at run time.
    std::filesystem::path staticRuntimeObject;
    if (staticRuntime) {
        staticRuntimeObject = std::filesystem::path(outputPath).replace_extension(".runtime.obj");
        if (!compileNativeRuntimeObject(linkPath, staticRuntimeObject)) {
            std::cerr << "Failed to build native runtime object for --static." << std::endl;
            std::filesystem::remove(tempObject);
            return false;
        }
        linkArgs.push_back(quoteCommandArg(staticRuntimeObject.string()));
    }

    auto msvcLibPath = findMsvcX64LibPath(linkPath);
    if (!msvcLibPath.empty()) {
        linkArgs.push_back("/LIBPATH:" + quoteCommandArg(msvcLibPath.string()));
    }
    auto sdkLibRoot = findWindowsSdkLibRoot();
    if (!sdkLibRoot.empty()) {
        linkArgs.push_back("/LIBPATH:" + quoteCommandArg((sdkLibRoot / "ucrt" / "x64").string()));
        linkArgs.push_back("/LIBPATH:" + quoteCommandArg((sdkLibRoot / "um" / "x64").string()));
    }
    for (const auto& linkPathArg : linkPaths) {
        if (!linkPathArg.empty()) {
            linkArgs.push_back("/LIBPATH:" + quoteCommandArg(linkPathArg));
        }
    }
    linkArgs.push_back("libcmt.lib");
    linkArgs.push_back("libvcruntime.lib");
    linkArgs.push_back("libucrt.lib");
    linkArgs.push_back("kernel32.lib");
    linkArgs.push_back("ws2_32.lib");
    linkArgs.push_back("legacy_stdio_definitions.lib");
    for (const auto& library : linkLibraries) {
        if (!library.empty()) {
            // The statically linked runtime object already satisfies the System.* symbols; skip its
            // import lib so no System.Native.dll dependency (or DLL copy) is introduced.
            if (staticRuntime && startsWithSystemLibraryPrefix(library)) {
                continue;
            }
            linkArgs.push_back(resolvedWindowsLinkLibraryArg(library, linkPaths, outputPath, runtimeLibrariesToCopy));
        }
    }

    std::string command;
    for (size_t i = 0; i < linkArgs.size(); ++i) {
        if (i > 0) {
            command += " ";
        }
        command += linkArgs[i];
    }
    command = "\"" + command + "\"";
    int result = std::system(command.c_str());
    std::error_code removeError;
    std::filesystem::remove(tempObject, removeError);
    if (!staticRuntimeObject.empty()) {
        std::filesystem::remove(staticRuntimeObject, removeError);
    }
    if (result != 0) {
        std::cerr << "Failed to emit executable via link.exe. Command exited with code " << result << std::endl;
        return false;
    }

    copyRuntimeLibrariesNextToOutput(runtimeLibrariesToCopy, outputPath);
    std::cout << "Wrote executable: " << outputPath << (staticRuntime ? " (static runtime)" : "") << std::endl;
    return true;
#endif
}
}

// The host target machine. The module gets its triple and data layout before code generation, so every
// size computed while generating code (allocation sizes, environment copies) matches the layout the
// object code uses, and the optimiser gets the target's cost model (needed, e.g., to vectorise loops).
// The CPU is the portable baseline ("generic": SSE2 on x86-64) unless -march=native asks for the host
// CPU and all of its features.
struct HostTarget {
    std::unique_ptr<llvm::TargetMachine> machine;
    std::string cpu;
    std::string features;
};

HostTarget createHostTarget(bool nativeCpu) {
    HostTarget host;
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    const llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    std::string error;
    const llvm::Target* target = csec_llvm::lookupTarget(triple, error);
    if (!target) return host;
    host.cpu = "generic";
    if (nativeCpu) {
        host.cpu = llvm::sys::getHostCPUName().str();
        for (const auto& feature : csec_llvm::hostCPUFeatures()) {
            if (!host.features.empty()) host.features += ",";
            host.features += (feature.getValue() ? "+" : "-") + feature.getKey().str();
        }
    }
    llvm::TargetOptions options;
    host.machine.reset(csec_llvm::createTargetMachine(*target, triple, host.cpu, host.features, options, llvm::Reloc::PIC_));
    return host;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // Report failed assertions and abort() on stderr instead of a modal dialog, which would block
    // test runs and scripts until someone clicks it.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    // CSEC_STACK_TRACE=1: print a symbolized stack trace on a crash or failed LLVM assertion (opt-in:
    // the handler makes a normal Debug-CRT exit report abort).
    if (const char* trace = std::getenv("CSEC_STACK_TRACE"); trace && *trace && *trace != '0') {
        llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    }
    //_CrtSetBreakAlloc(3404);

    if (argc > 1 && std::string(argv[1]) == "--mangle") {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0] << " --mangle <itanium|msvc> <signature>" << std::endl;
            return 1;
        }
        return printMangledName(argv[0], argv[2], argv[3]);
    }

    /*if (argc < 2) {
        std::cout << "No input file provided." << std::endl;
        return -1;
    }

    // ?뚯씪??議댁옱?섎뒗吏 泥댄겕
    if (FILE* file = fopen(argv[1], "r")) {
        // ?꾨Т寃껊룄 ?섏? ?딅뒗??
    }
    else {
        std::cout << "File not found: " << argv[1] << std::endl;
        return -1;
    }*/

    char* cwd =
#ifdef _WIN32
        _getcwd(NULL, 0);
#else
        getcwd(NULL, 0);
#endif
    if (cwd) {
        std::cout << "get current directory: " << cwd << std::endl;
        free(cwd);
    }
    else {
        std::cout << "get current directory: <unavailable>" << std::endl;
    }
    OutputMode outputMode = OutputMode::EmitExecutable;
    std::string inputFile;
    bool inputFileSet = false;
    std::string outputFile;
    std::vector<std::string> linkLibraries;
    std::vector<std::string> linkPaths;
    std::vector<std::string> runtimeArgs;
    bool parsingRuntimeArgs = false;
    // Optimization level: -1 = unspecified (legacy default: mem2reg only), 0 = -O0 (no passes),
    // 1/2/3 = -O1/-O2/-O3 (LLVM per-module default pipeline).
    int optLevel = -1;
    bool marchNative = false;
    // --static: link the native runtime directly into the emitted executable (no System.Native.dll
    // dependency at run time). Only affects --emit-exe.
    bool staticRuntime = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (parsingRuntimeArgs) {
            runtimeArgs.push_back(std::move(arg));
            continue;
        }
        if (arg == "--") {
            parsingRuntimeArgs = true;
            continue;
        }
        if (arg == "--mangle") {
            if (i + 2 >= argc) {
                std::cerr << "Usage: " << argv[0] << " --mangle <itanium|msvc> <signature>" << std::endl;
                return 1;
            }
            std::string style = argv[++i];
            std::string signature = argv[++i];
            return printMangledName(argv[0], style, signature);
        }
        if (arg == "--syntax-only" || arg == "--parse-only") {
            outputMode = OutputMode::SyntaxOnly;
            continue;
        }
        if (arg == "--verify") {
            // Type-check, then prove the contracts and assertions with Z3; no code is generated.
            outputMode = OutputMode::Verify;
            continue;
        }
        if (arg == "--verify-timeout") {
            // Per-check solver time limit in milliseconds (default 10000).
            if (i + 1 >= argc) {
                std::cerr << "Missing milliseconds after --verify-timeout" << std::endl;
                return 1;
            }
            outputMode = OutputMode::Verify;
            verifierOptions().timeoutMilliseconds = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 10));
            continue;
        }
        if (arg == "--verify-wrap") {
            // Integers wrap around as in the compiled program (bit-vectors) instead of overflow being an error.
            outputMode = OutputMode::Verify;
            verifierOptions().wrapAround = true;
            continue;
        }
        if (arg == "--verify-overflow") {
            // With --verify-wrap: still report where arithmetic may overflow.
            outputMode = OutputMode::Verify;
            verifierOptions().reportOverflow = true;
            continue;
        }
        if (arg == "--strict-ownership") {
            // M2 strict ownership is on by default; kept for explicitness / forcing it back on.
            setStrictOwnership(true);
            continue;
        }
        if (arg == "--no-strict-ownership") {
            // Opt out of default strict ownership for legacy/unmigrated code (e.g. the self-host
            // source): move-checking falls back to box-only.
            setStrictOwnership(false);
            continue;
        }
        if (arg == "--run") {
            outputMode = OutputMode::Run;
            continue;
        }
        if (arg == "--run-ir") {
            outputMode = OutputMode::RunIR;
            continue;
        }
        if (arg == "--emit-ir" || arg == "-S") {
            outputMode = OutputMode::EmitIR;
            continue;
        }
        if (arg == "--emit-obj" || arg == "-c") {
            outputMode = OutputMode::EmitObject;
            continue;
        }
        if (arg == "--emit-exe") {
            outputMode = OutputMode::EmitExecutable;
            continue;
        }
        if (arg == "--static") {
            staticRuntime = true;
            continue;
        }
        if (arg == "-O0") { optLevel = 0; continue; }
        if (arg == "-O1") { optLevel = 1; continue; }
        if (arg == "-O2" || arg == "-O") { optLevel = 2; continue; }
        if (arg == "-O3") { optLevel = 3; continue; }
        if (arg == "-march=native") { marchNative = true; continue; }
        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Missing output path after -o" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
            outputFile = argv[++i];
            continue;
        }
        if (arg == "--lib-path") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --lib-path requires a directory" << std::endl;
                return 1;
            }
            csec_add_library_root(argv[++i]);
            continue;
        }
        if (arg == "--link-lib") {
            if (i + 1 >= argc) {
                std::cerr << "Missing library after --link-lib" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
            linkLibraries.push_back(argv[++i]);
            continue;
        }
        if (arg == "--link-path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing path after --link-path" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
            linkPaths.push_back(argv[++i]);
            continue;
        }
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (!inputFileSet) {
            inputFile = arg;
            inputFileSet = true;
            continue;
        }
        std::cerr << "Unexpected argument: " << arg << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    if (!inputFileSet) {
        inputFile = promptForInputFile(argv[0]);
        if (inputFile.empty()) {
            return 1;
        }
        inputFileSet = true;
    }

    if (outputMode == OutputMode::RunIR) {
        return runIRFile(inputFile, runtimeArgs, argv[0]);
    }

    std::string code = expandImports(inputFile);
    if (!importErrors().empty()) {
        for (const auto& error : importErrors()) {
            std::cerr << "Error: " << error << std::endl;
        }
        return 1;
    }
    if (code.empty()) {
        std::error_code ec;
        if (!std::filesystem::exists(inputFile, ec) || !std::filesystem::is_regular_file(inputFile, ec)) {
            std::cerr << "Unable to read input file: " << inputFile << std::endl;
            return 1;
        }
    }

    // 肄붾뱶 ?앹꽦
    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();
    for (const auto& token : tokens) {
        if (token.type == TokenType::UNKNOWN) {
            std::cerr << "Lexing failed: unknown token '" << token.value
                      << "' at line " << token.line << ", column " << token.column << std::endl;
            return 1;
        }
    }

    std::unique_ptr<ASTNode> ast;
    try {
        Parser parser(tokens);
        ast = parser.parse();
    }
    catch (const std::exception& e) {
        std::cerr << "Parsing failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Parsing completed successfully." << std::endl;
    if (outputMode == OutputMode::SyntaxOnly) {
        ast.reset();
        return 0;
    }

    //ASTPrinter printer;
    //ast->accept(printer);

    HostTarget hostTarget = createHostTarget(marchNative);
    if (hostTarget.machine) {
        auto& targetModule = *CodeGenerator::getInstance().module;
        csec_llvm::setTargetTriple(targetModule, hostTarget.machine->getTargetTriple());
        targetModule.setDataLayout(hostTarget.machine->createDataLayout());
    }

    TypeChecker typeChecker;
    ast->accept(typeChecker);
    if (typeChecker.hasErrors()) {
        std::cerr << "Type checking found " << typeChecker.getErrorCount()
                  << " error(s). Aborting code generation." << std::endl;
        return 1;
    }
    if (outputMode == OutputMode::Verify) {
        std::error_code canonicalError;
        auto mainFile = std::filesystem::weakly_canonical(std::filesystem::path(inputFile), canonicalError);
        return verifyProgram(ast.get(), canonicalError ? inputFile : mainFile.string());
    }
    CodeGenerator::getInstance().generatingCode = true;
    ast->codegen();

    // Keep IR dumping opt-in; unconditional dumps make large template suites unreasonably slow.
    // CodeGenerator::getInstance().dumpIR();

    LLVMLinkInMCJIT();
    LLVMLinkInInterpreter();
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto& codeGen = CodeGenerator::getInstance();
    llvm::Function* runtimeEntry = rebuildRuntimeMain(codeGen);
    if (codeGen.mainFunction && !codeGen.mainFunction->empty()) {
        llvm::BasicBlock& rootEntry = codeGen.mainFunction->getEntryBlock();
        if (!csec_llvm::terminatorOrNull(&rootEntry)) {
            llvm::IRBuilder<> rootBuilder(&rootEntry);
            rootBuilder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(codeGen.context), 0));
        }
    }

    if (llvm::verifyModule(*codeGen.module, &llvm::errs())) {
        std::cerr << "Generated LLVM IR is invalid. Aborting execution." << std::endl;
        // Debug aid: CSEC_DUMP_INVALID_IR=<path> writes the rejected module for inspection.
        const std::string dumpPath = getEnvironmentVariable("CSEC_DUMP_INVALID_IR");
        if (!dumpPath.empty()) {
            std::error_code dumpError;
            llvm::raw_fd_ostream dump(dumpPath, dumpError, llvm::sys::fs::OF_Text);
            if (!dumpError) codeGen.module->print(dump, nullptr);
        }
        return 1;
    }

    // With -march=native every function carries the host CPU so llc/clang generate code for it.
    if (marchNative && hostTarget.machine) {
        for (auto& function : *codeGen.module) {
            if (function.isDeclaration()) continue;
            function.addFnAttr("target-cpu", hostTarget.cpu);
            if (!hostTarget.features.empty()) function.addFnAttr("target-features", hostTarget.features);
        }
    }

    llvm::PassBuilder PB(hostTarget.machine.get());
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    PB.registerModuleAnalyses(MAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Choose the optimization pipeline by -O level. -O1/-O2/-O3 run LLVM's full per-module pipeline
    // (which already includes mem2reg, inlining, GVN, DCE, loop opts, ...). -O0 runs nothing. The
    // legacy default (no -O flag) promotes allocas to registers only.
    //
    // The `--run` path executes with the LLVM *interpreter*, which cannot run fully optimized IR
    // (vector ops, intrinsics), so it always uses the interpreter-safe mem2reg pipeline; the -O level
    // takes effect for emitted output (--emit-ir/--emit-obj/--emit-exe), and a program run from that
    // output (or via --run-ir) sees the optimization.
    const bool interpreterRun = (outputMode == OutputMode::Run);
    if (interpreterRun && optLevel >= 1) {
        std::cerr << "Note: -O optimization applies to emitted output, not the --run interpreter; "
                     "use --emit-ir/--emit-exe (or --run-ir) to run optimized code." << std::endl;
    }

    llvm::ModulePassManager MPM;
    if (!interpreterRun && optLevel >= 1) {
        llvm::OptimizationLevel level = llvm::OptimizationLevel::O2;
        if (optLevel == 1) level = llvm::OptimizationLevel::O1;
        else if (optLevel == 3) level = llvm::OptimizationLevel::O3;
        MPM = PB.buildPerModuleDefaultPipeline(level);
    }
    else if (interpreterRun || optLevel < 0) {
        llvm::FunctionPassManager FPM;
        FPM.addPass(llvm::PromotePass());
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
    }
    // optLevel == 0 with an emit mode: leave the module unoptimized.
    MPM.run(*codeGen.module, MAM);

    if (outputMode == OutputMode::EmitIR || outputMode == OutputMode::EmitObject ||
        outputMode == OutputMode::EmitExecutable) {
        if (outputFile.empty()) {
            outputFile = defaultOutputPath(inputFile, outputMode);
        }

        if (outputMode == OutputMode::EmitIR) {
            return writeIRToFile(*codeGen.module, outputFile) ? 0 : 1;
        }

        if (outputMode == OutputMode::EmitObject) {
            return writeObjectToFile(*codeGen.module, outputFile) ? 0 : 1;
        }

        std::vector<std::string> allLinkLibraries = linkLibraries;
        for (const auto& library : codeGen.externalLinkLibraries) {
            bool exists = false;
            for (const auto& current : allLinkLibraries) {
                if (current == library) {
                    exists = true;
                    break;
                }
            }
            if (!exists) allLinkLibraries.push_back(library);
        }
        std::vector<std::string> allLinkPaths = linkPaths;
        for (const auto& path : codeGen.externalLinkPaths) {
            bool exists = false;
            for (const auto& current : allLinkPaths) {
                if (current == path) {
                    exists = true;
                    break;
                }
            }
            if (!exists) allLinkPaths.push_back(path);
        }
        return writeExecutableToFile(*codeGen.module, outputFile, allLinkLibraries, allLinkPaths, staticRuntime) ? 0 : 1;
    }

    std::string errStr;
    auto engine = llvm::EngineBuilder(std::move(codeGen.module))
        .setErrorStr(&errStr)
        .setEngineKind(llvm::EngineKind::Interpreter)
        .setOptLevel(llvm::CodeGenOpt::getLevel(0).value())
        .create();

    if (!engine) {
        std::cerr << "Failed to create ExecutionEngine: " << errStr << std::endl;
        std::cerr << "Execution backend is unavailable. Skipping run phase." << std::endl;
        return 0;
    }

    // main ?⑥닔 ?ㅽ뻾
    std::vector<char*> runtimeArgv;
    runtimeArgv.reserve(runtimeArgs.size() + 1);
    runtimeArgv.push_back(argv[0]);
    for (auto& runtimeArg : runtimeArgs) {
        runtimeArgv.push_back(runtimeArg.data());
    }
    csec_set_command_line_args(static_cast<int>(runtimeArgv.size()), runtimeArgv.data());

    auto result = engine->runFunction(runtimeEntry, {});

    return 0;
}



