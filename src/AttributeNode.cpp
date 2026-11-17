#include "AttributeNode.h"
#include "ASTVisitor.h"

#include <iostream>

#include "IdentifierNode.h"
#include "FunctionCallNode.h"
#include "FunctionDeclarationNode.h"
#include "ValueNode.h"
#include "codegen.h"
#include "mangling.h"

namespace {
std::string stringArgument(const FunctionCallNode* call, size_t index) {
    if (!call || index >= call->arguments.size()) {
        return "";
    }
    auto* valueNode = dynamic_cast<ValueNode*>(call->arguments[index].get());
    if (!valueNode || valueNode->valueType != TokenType::STRING_LITERAL) {
        return "";
    }
    return valueNode->value;
}

CppMangleStyle platformDefaultMangleStyle() {
#ifdef _WIN32
    return CppMangleStyle::MSVC;
#else
    return CppMangleStyle::Itanium;
#endif
}

CppMangleStyle mangleStyleArgument(const FunctionCallNode* call, size_t index) {
    std::string value = stringArgument(call, index);
    if (value == "itanium" || value == "Itanium" || value == "gnu" || value == "GCC" || value == "clang") {
        return CppMangleStyle::Itanium;
    }
    if (value == "msvc" || value == "MSVC" || value == "windows") {
        return CppMangleStyle::MSVC;
    }
    return platformDefaultMangleStyle();
}
}

void AttributeNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* AttributeNode::codegen() {
    auto applyToTarget = [&]() {
        return target ? target->codegen() : nullptr;
    };
    auto& codeGen = CodeGenerator::getInstance();

    if (auto* function = dynamic_cast<FunctionDeclarationNode*>(this->target.get())) {
        if (function->isIntrinsic) {
            return nullptr;
        }
    }

    if (auto* callNode = dynamic_cast<FunctionCallNode*>(this->expr.get())) {
        if (callNode->functionName == "LinkLibrary" || callNode->functionName == "NativeLibrary") {
            codeGen.addExternalLinkLibrary(stringArgument(callNode, 0));
            return applyToTarget();
        }
        if (callNode->functionName == "DllImport" || callNode->functionName == "StaticLibraryImport") {
            std::string library = stringArgument(callNode, 0);
            std::string symbol = stringArgument(callNode, 1);
            if (auto* function = dynamic_cast<FunctionDeclarationNode*>(this->target.get())) {
                function->dllImportLibrary = library;
                function->externalSymbolName = symbol;
                function->isExternal = true;
            }
            codeGen.addExternalLinkLibrary(library);
            return applyToTarget();
        }
        if (callNode->functionName == "CppImport" || callNode->functionName == "CxxImport") {
            std::string library = stringArgument(callNode, 0);
            std::string signature = stringArgument(callNode, 1);
            std::string symbol = mangleCppSignature(signature, mangleStyleArgument(callNode, 2));
            if (auto* function = dynamic_cast<FunctionDeclarationNode*>(this->target.get())) {
                function->dllImportLibrary = library;
                function->externalSymbolName = symbol;
                function->isExternal = true;
            }
            codeGen.addExternalLinkLibrary(library);
            return applyToTarget();
        }
        if (callNode->functionName == "LinkPath") {
            codeGen.addExternalLinkPath(stringArgument(callNode, 0));
            return applyToTarget();
        }
    }

    // Node가 IdentifierNode이라고 하더라도 FunctionCall로 취급
    switch (this->expr->nodeType) {
    case ASTNodeType::FUNCTION_CALL:
        // An attribute with arguments the compiler does not know (`[@native("fast")]`) has no effect;
        // the declaration it annotates is still compiled (it used to be dropped).
        break;

    case ASTNodeType::IDENTIFIER:
    {
        auto identifierNode = dynamic_cast<IdentifierNode*>(this->expr.get());
        if (!identifierNode) {
            std::cerr << "Error: Invalid identifier node" << std::endl;
            return nullptr;
        }

        // Attribute 종류에 따른 처리
        if (identifierNode->value == "DllImport") {
            // 이거 다음의 함수는 외부 DLL에서 가져오는 함수로 처리
            if (auto* function = dynamic_cast<FunctionDeclarationNode*>(this->target.get())) {
                function->isExternal = true;
            }
        }
        else if (identifierNode->value == "StaticLibraryImport") {
            // 이거 다음의 함수는 외부 Static Library에서 가져오는 함수로 처리
        }
        else if (identifierNode->value == "Native") {
            // 이거 다음의 함수는 네이티브 함수로 처리 = Compiler에서 지원하는 함수
        }
        else {
            // 나머지는 사용자 지정으로 할 예정
        }
    }
    break;

    default:
        std::cerr << "Error: Unsupported expression type in AttributeNode" << std::endl;
        break;
    }

    return applyToTarget();
}
