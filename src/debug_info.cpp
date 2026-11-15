// debug_info.cpp -- source-level debug information for `csec++ -g` (see debug_info.h).
#include "codegen.h"

#include "debug_info.h"
#include "source_map.h"

#include <filesystem>
#include <map>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>

namespace {
struct DebugState {
    std::unique_ptr<llvm::DIBuilder> builder;
    llvm::DICompileUnit* unit = nullptr;
    std::string mainFile;
    bool codeView = false;
    std::map<std::string, llvm::DIFile*> files;
};

DebugState& state() {
    static DebugState s;
    return s;
}

llvm::DIFile* fileFor(const std::string& path) {
    auto& s = state();
    const std::string key = path.empty() ? s.mainFile : path;
    auto it = s.files.find(key);
    if (it != s.files.end()) return it->second;
    std::filesystem::path p = std::filesystem::u8path(key);
    std::error_code ec;
    auto absolute = std::filesystem::absolute(p, ec);
    if (!ec) p = absolute;
    llvm::DIFile* file = s.builder->createFile(p.filename().u8string(), p.parent_path().u8string());
    s.files[key] = file;
    return file;
}
}  // namespace

namespace debuginfo {

void enable(const std::string& mainFile, bool codeView) {
    auto& cg = CodeGenerator::getInstance();
    auto& s = state();
    s.mainFile = mainFile;
    s.codeView = codeView;
    s.builder = std::make_unique<llvm::DIBuilder>(*cg.module);
    s.unit = s.builder->createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, fileFor(mainFile), "csec++ (Tessera)",
                                          false, "", 0);
}

bool enabled() { return state().builder != nullptr; }

void beginFunction(llvm::Function* function, const std::string& name, int expandedLine) {
    auto& s = state();
    if (!s.builder || !function || function->getSubprogram()) return;
    const SourceLocation location = sourceLocationOf(expandedLine);
    llvm::DIFile* file = fileFor(location.file);
    const unsigned line = static_cast<unsigned>(location.line > 0 ? location.line : 0);
    auto* type = s.builder->createSubroutineType(s.builder->getOrCreateTypeArray({}));
    llvm::DISubprogram* sp = s.builder->createFunction(
        file, name, function->getName(), file, line, type, line, llvm::DINode::FlagPrototyped,
        llvm::DISubprogram::SPFlagDefinition);
    function->setSubprogram(sp);
    setLine(expandedLine);
}

void setLine(int expandedLine) {
    auto& cg = CodeGenerator::getInstance();
    if (!state().builder) return;
    llvm::BasicBlock* block = cg.builder.GetInsertBlock();
    llvm::DISubprogram* sp = block ? block->getParent()->getSubprogram() : nullptr;
    if (!sp) {
        cg.builder.SetCurrentDebugLocation(llvm::DebugLoc());
        return;
    }
    const SourceLocation location = sourceLocationOf(expandedLine);
    // A line of another file (an expanded import inside this function cannot happen; keep the
    // function's own line rather than point into the wrong file).
    unsigned line = expandedLine > 0 && location.line > 0 ? static_cast<unsigned>(location.line) : sp->getLine();
    if (!location.file.empty() && fileFor(location.file) != sp->getFile()) line = sp->getLine();
    cg.builder.SetCurrentDebugLocation(llvm::DILocation::get(cg.context, line, 0, sp));
}

void finalize() {
    auto& cg = CodeGenerator::getInstance();
    auto& s = state();
    if (!s.builder) return;
    // Code generated while the builder pointed into another function keeps that function's
    // location; give it its own function's line. Calls in a function with debug information need a
    // location; functions without a subprogram (the entry wrapper, thunks) carry none.
    for (llvm::Function& function : *cg.module) {
        llvm::DISubprogram* sp = function.getSubprogram();
        for (llvm::Instruction& instruction : llvm::instructions(function)) {
            const llvm::DebugLoc& loc = instruction.getDebugLoc();
            if (!sp) {
                if (loc) instruction.setDebugLoc(llvm::DebugLoc());
                continue;
            }
            if (loc) {
                auto* scope = loc->getScope() ? loc->getScope()->getSubprogram() : nullptr;
                if (scope == sp && !loc->getInlinedAt()) continue;
            }
            instruction.setDebugLoc(llvm::DILocation::get(cg.context, sp->getLine(), 0, sp));
        }
    }
    s.builder->finalize();
    cg.module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
    if (s.codeView) cg.module->addModuleFlag(llvm::Module::Warning, "CodeView", 1);
    else cg.module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 4);
}

}  // namespace debuginfo
