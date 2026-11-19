#include "codegen.h"
#include "AssignmentExpressionNode.h"
#include "ASTVisitor.h"
#include "IdentifierNode.h"
#include "ArrayAccessNode.h"
#include "AccessFieldNode.h"
#include "type_utils.h"
#include "dict_support.h"

#include <iostream>
#include <llvm/IR/DataLayout.h>

void AssignmentExpressionNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

llvm::Value* AssignmentExpressionNode::codegen() {
    if (isNamedArgument) {
        // Every named argument is rewritten into a positional one before code generation; one left
        // here belongs to a call whose parameters were never matched.
        CodeGenerator::getInstance().error("named argument '",
            dynamic_cast<IdentifierNode*>(left.get()) ? static_cast<IdentifierNode*>(left.get())->value : std::string("?"),
            "' could not be matched to a parameter",
            line > 0 ? " (line " + std::to_string(line) + ")" : std::string());
        return nullptr;
    }
    // `d[key] = value` (and `d[key] op= value`) stores into a dictionary; the left side is not an
    // address, so it must not be evaluated as a read (which would raise KeyError for a new key).
    if (auto* access = dynamic_cast<ArrayAccessNode*>(left.get())) {
        auto containerType = access->array ? access->array->getType() : nullptr;
        if (isDictType(containerType.get())) {
            ASTNode* key = access->index ? access->index.get()
                : (!access->indices.empty() && access->indices[0] ? access->indices[0]->index.get() : nullptr);
            if (!key) return nullptr;
            return codegenDictSet(access->array.get(), key, right.get(), op);
        }
        if (access->hasSlice()) {
            llvm::Value* rightValue = right->codegen();
            if (!rightValue) {
                std::cerr << "Error: Assignment failed" << std::endl;
                return nullptr;
            }
            auto rightType = right->getType();
            return access->codegenSliceAssign(rightValue, rightType.get());
        }
    }
    llvm::Value* leftValue = left->codegen();
    llvm::Value* rightValue = right->codegen();
    if (!leftValue || !rightValue) {
        std::cerr << "Error: Assignment failed" << std::endl;
        return nullptr;
    }

    llvm::Value* targetPtr = leftValue;
    if (auto* id = dynamic_cast<IdentifierNode*>(left.get())) {
        auto symbolOpt = CodeGenerator::getInstance().symbolTable.lookup(id->value);
        if (!symbolOpt || !symbolOpt->value) {
            std::cerr << "Error: Invalid assignment target" << std::endl;
            return nullptr;
        }
        targetPtr = symbolOpt->value;
        // A Nat variable holds its handle in a slot: promote an integer RHS to a handle and store it.
        if (symbolOpt->type && symbolOpt->type->getName() == "Nat" &&
            symbolOpt->value && llvm::isa<llvm::AllocaInst>(symbolOpt->value)) {
            rightValue = coerceToNat(rightValue, symbolOpt->type.get(), right.get());
            if (rightValue && rightValue->getType()->isPointerTy()) {
                CodeGenerator::getInstance().builder.CreateStore(rightValue, symbolOpt->value);
                return rightValue;
            }
            std::cerr << "Type error: Assignment value for Nat variable '" << id->value << "' is not a Nat" << std::endl;
            return nullptr;
        }
        if (isStructClassType(symbolOpt->type.get())) {
            llvm::Type* targetType = CodeGenerator::getInstance().getLLVMType(symbolOpt->type.get());
            rightValue = coerceValueToLLVMType(rightValue, targetType);
            if (!rightValue || rightValue->getType() != targetType) {
                std::cerr << "Type error: Assignment value does not match struct variable '" << id->value << "'" << std::endl;
                return nullptr;
            }
        }
    }
    else if (auto* access = dynamic_cast<ArrayAccessNode*>(left.get())) {
        // A colon section on the left (`t[:] = v`, `t[i:j] = v`) assigns the whole section -- a scalar
        // broadcast to every element, or an element-wise copy from another tensor (Fortran `:`).
        if (access->hasSlice()) {
            auto rightType = right ? right->getType() : nullptr;
            llvm::Value* assigned = access->codegenSliceAssign(rightValue, rightType.get());
            if (!assigned) {
                return nullptr;
            }
            return assigned;
        }
        targetPtr = access->codegenElementPointer();
        if (!targetPtr) {
            std::cerr << "Error: Invalid array assignment target" << std::endl;
            return nullptr;
        }
    }
    else if (auto* field = dynamic_cast<AccessFieldNode*>(left.get())) {
        targetPtr = field->codegenFieldPointer();
        if (!targetPtr) {
            std::cerr << "Error: Invalid field assignment target" << std::endl;
            return nullptr;
        }
    }

    if (!targetPtr->getType()->isPointerTy()) {
        std::cerr << "Error: Left-hand side of assignment is not assignable" << std::endl;
        return nullptr;
    }

    // Move-assignment `target <- source` between owned box variables copies the source box's payload
    // into the target box. Both allocations stay valid and are each freed once at scope exit, and the
    // type checker has already marked `source` moved. The generic store below would instead write the
    // source *pointer* (pointer-sized) into the target box's payload-sized allocation — an overflow
    // that corrupts the heap and hangs the allocator on the next free (e.g. `box Int`/empty classes).
    if (op == "<-") {
        auto boxNodeType = left->getType();
        if (boxNodeType && boxNodeType->getKind() == Type::Kind::BOX &&
            leftValue->getType()->isPointerTy() && rightValue->getType()->isPointerTy()) {
            auto& cg = CodeGenerator::getInstance();
            auto* boxType = dynamic_cast<BoxType*>(boxNodeType.get());
            llvm::Type* payloadType =
                boxType && boxType->baseType ? cg.getLLVMType(boxType->baseType.get()) : nullptr;
            if (payloadType) {
                uint64_t size = cg.module->getDataLayout().getTypeAllocSize(payloadType);
                if (size > 0) {
                    cg.builder.CreateMemCpy(leftValue, llvm::MaybeAlign(), rightValue,
                                            llvm::MaybeAlign(), size);
                }
                return leftValue;
            }
        }
    }

    llvm::Type* targetType = CodeGenerator::getInstance().getLLVMType(left->getType().get());
    // Tensor elements are stored as doubles whatever the declared element type.
    if (auto* access = dynamic_cast<ArrayAccessNode*>(left.get())) {
        auto container = access->array ? access->array->getType() : nullptr;
        if (container && container->getName() == "Tensor" && targetType && !targetType->isDoubleTy() &&
            (targetType->isIntegerTy() || targetType->isFloatTy())) {
            targetType = llvm::Type::getDoubleTy(CodeGenerator::getInstance().context);
        }
    }
    rightValue = coerceValueToLLVMType(rightValue, targetType);
    if (!rightValue || !targetType || rightValue->getType() != targetType) {
        std::cerr << "Type error: Assignment value does not match its target type" << std::endl;
        return nullptr;
    }

    if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=") {
        llvm::Value* oldValue = CodeGenerator::getInstance().builder.CreateLoad(targetType, targetPtr, "assign.old");
        if (targetType->isFloatingPointTy()) {
            if (op == "+=") rightValue = CodeGenerator::getInstance().builder.CreateFAdd(oldValue, rightValue, "assign.next");
            else if (op == "-=") rightValue = CodeGenerator::getInstance().builder.CreateFSub(oldValue, rightValue, "assign.next");
            else if (op == "*=") rightValue = CodeGenerator::getInstance().builder.CreateFMul(oldValue, rightValue, "assign.next");
            else if (op == "/=") rightValue = CodeGenerator::getInstance().builder.CreateFDiv(oldValue, rightValue, "assign.next");
            else {
                std::cerr << "Type error: Remainder assignment is not supported for floating-point values" << std::endl;
                return nullptr;
            }
        } else if (op == "+=") rightValue = CodeGenerator::getInstance().builder.CreateAdd(oldValue, rightValue, "assign.next");
        else if (op == "-=") rightValue = CodeGenerator::getInstance().builder.CreateSub(oldValue, rightValue, "assign.next");
        else if (op == "*=") rightValue = CodeGenerator::getInstance().builder.CreateMul(oldValue, rightValue, "assign.next");
        else if (op == "/=") rightValue = CodeGenerator::getInstance().builder.CreateSDiv(oldValue, rightValue, "assign.next");
        else rightValue = CodeGenerator::getInstance().builder.CreateSRem(oldValue, rightValue, "assign.next");
    }
    CodeGenerator::getInstance().builder.CreateStore(rightValue, targetPtr);
    CodeGenerator::clearFreshTensor(rightValue);
    return rightValue;
}

std::unique_ptr<Type> AssignmentExpressionNode::getType() {
    return left->getType();
}
