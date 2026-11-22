// module_loader.cpp

#include "module_loader.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "utils.h"
#include "verifier.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

ModuleLoader::ModuleLoader() {
    // NOTE: Deferred until std module file exists to avoid silent failure.
}

SymbolTable* ModuleLoader::loadModule(const std::vector<std::string>& path) {
    std::ostringstream oss;
    for (size_t i = 0; i < path.size(); ++i) {
        oss << path[i];
        if (i != path.size() - 1) {
            oss << ".";
        }
    }
    std::string moduleName = oss.str();

    auto it = moduleCache.find(moduleName);
    if (it != moduleCache.end()) {
        return it->second.get();
    }

    std::string filepath;
    for (size_t i = 0; i < path.size(); ++i) {
        filepath += path[i];
        if (i != path.size() - 1) {
            filepath += "/";
        }
    }
    std::ifstream probe(filepath + ".t", std::ios::binary);
    filepath += probe.is_open() ? ".t" : ".csec";

    auto moduleSymbols = std::make_unique<SymbolTable>();
    if (!parseModuleFile(filepath, *moduleSymbols)) {
        std::cerr << "Error: Failed to parse module file: " << filepath << std::endl;
        return nullptr;
    }

    auto* moduleSymbolsRaw = moduleSymbols.get();
    moduleCache[moduleName] = std::move(moduleSymbols);

    return moduleSymbolsRaw;
}

llvm::Function* ModuleLoader::loadFunction(const std::string& name, llvm::Module* module) {
    if (module->getFunction(name)) {
        return module->getFunction(name);
    }

    return nullptr;
}

bool ModuleLoader::parseModuleFile(const std::string& filepath, SymbolTable& moduleSymbols) {
    std::string code = read_utf8_file(filepath);
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open module file: " << filepath << std::endl;
        return false;
    }

    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();

    std::unique_ptr<ASTNode> ast;
    try {
        Parser parser(tokens);
        ast = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse module file '" << filepath << "': " << e.what() << std::endl;
        return false;
    }

    auto& codegenSymbolTable = CodeGenerator::getInstance().symbolTable;
    auto savedSymbolTable = std::move(codegenSymbolTable);
    try {
        codegenSymbolTable = std::move(moduleSymbols);
        stripSpecifications(ast.get());
        ast->codegen();
        moduleSymbols = std::move(codegenSymbolTable);
        codegenSymbolTable = std::move(savedSymbolTable);
    } catch (const std::exception& e) {
        codegenSymbolTable = std::move(savedSymbolTable);
        std::cerr << "Error: Failed to codegen module file '" << filepath << "': " << e.what() << std::endl;
        return false;
    } catch (...) {
        codegenSymbolTable = std::move(savedSymbolTable);
        std::cerr << "Error: Failed to codegen module file '" << filepath << "'" << std::endl;
        return false;
    }

    return true;
}
