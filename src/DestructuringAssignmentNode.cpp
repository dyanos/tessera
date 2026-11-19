#include "codegen.h"
#include "DestructuringAssignmentNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/DerivedTypes.h>

void DestructuringAssignmentNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::unique_ptr<Type> DestructuringAssignmentNode::getType() {
    return std::make_unique<BasicType>("Unit");
}

llvm::Value* DestructuringAssignmentNode::codegen() {
    auto& cg = CodeGenerator::getInstance();
    if (!value) {
        return nullptr;
    }
    // Step 1-2 of the seven-step protocol: evaluate the RHS once and materialize the tuple.
    llvm::Value* tupleValue = value->codegen();
    if (!tupleValue) {
        return nullptr;
    }
    auto* structType = llvm::dyn_cast<llvm::StructType>(tupleValue->getType());
    if (!structType) {
        std::cerr << "Error: right-hand side of a destructuring assignment is not a tuple" << std::endl;
        return nullptr;
    }

    auto rhsType = value->getType();
    auto* tupleType = dynamic_cast<TupleType*>(rhsType.get());

    for (size_t i = 0; i < targets.size() && i < structType->getNumElements(); ++i) {
        // Step 5/6: extract each slot; a "_" target is materialized then discarded.
        llvm::Value* element = cg.builder.CreateExtractValue(tupleValue, { static_cast<unsigned>(i) }, "destr.elt");
        if (targets[i].name == "_") {
            continue;
        }
        std::unique_ptr<Type> elementType =
            (tupleType && i < tupleType->elementTypes.size() && tupleType->elementTypes[i])
                ? tupleType->elementTypes[i]->clone()
                : std::make_unique<UnknownType>();
        llvm::Type* slotType = structType->getElementType(static_cast<unsigned>(i));
        // Bind the target as a fresh local: alloca of the slot type, store the moved element, and
        // register the binding so later uses load it. Ownership of owned (pointer-backed) elements
        // transfers to the new binding.
        llvm::AllocaInst* slot = cg.createEntryAlloca(slotType, targets[i].name.c_str());
        cg.builder.CreateStore(element, slot);
        cg.symbolTable.addSymbol(
            targets[i].name,
            std::make_unique<Symbol>(targets[i].name, std::move(elementType), slot,
                                     targets[i].isMutable, SymbolType::VARIABLE));
    }
    return nullptr;
}
