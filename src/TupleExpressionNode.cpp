#include "codegen.h"
#include "TupleExpressionNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

void TupleExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::unique_ptr<Type> TupleExpressionNode::getType() {
    std::vector<std::unique_ptr<Type>> elems;
    elems.reserve(elements.size());
    for (auto& e : elements) {
        elems.push_back(e ? e->getType() : std::make_unique<UnknownType>());
    }
    return std::make_unique<TupleType>(std::move(elems));
}

llvm::Value* TupleExpressionNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    auto tupleType = getType();
    llvm::Type* llvmTupleType = cg.getLLVMType(tupleType.get());
    auto* structType = llvm::dyn_cast_or_null<llvm::StructType>(llvmTupleType);
    if (!structType || structType->getNumElements() != elements.size()) {
        std::cerr << "Error: could not lower tuple expression" << std::endl;
        return nullptr;
    }
    llvm::Value* aggregate = llvm::UndefValue::get(structType);
    for (size_t i = 0; i < elements.size(); ++i) {
        llvm::Value* value = elements[i] ? elements[i]->codegen() : nullptr;
        if (!value) {
            return nullptr;
        }
        value = coerceValueToLLVMType(value, structType->getElementType(static_cast<unsigned>(i)));
        { auto elemType = elements[i]->getType(); value = cg.takeOwned(value, elemType.get()); }
        aggregate = cg.builder.CreateInsertValue(aggregate, value, { static_cast<unsigned>(i) }, "tuple.elt");
    }
    return aggregate;
}
