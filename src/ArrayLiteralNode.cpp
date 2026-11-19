#include "codegen.h"
#include "ArrayLiteralNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Constants.h>

void ArrayLiteralNode::accept(ASTVisitor& visitor) {
	visitor.visit(*this);
}

llvm::Value* ArrayLiteralNode::codegen() {
	if (elements.empty()) return nullptr;

	auto& cg = CodeGenerator::getInstance();

	// Get element type from first element
	llvm::Value* firstElem = elements[0]->codegen();
	if (!firstElem) return nullptr;

	llvm::Type* elementType = firstElem->getType();
	// Store each element in its type's storage form: a struct value (an enum, a `struct`) is copied into
	// the array, not referenced through the temporary that built it.
	if (auto declared = getType()) {
		if (auto* arrayType = dynamic_cast<ArrayType*>(declared.get())) {
			if (llvm::Type* storage = arrayType->elementType ? getABIStorageType(arrayType->elementType.get()) : nullptr) {
				if (storage->isStructTy()) elementType = storage;
			}
		}
	}
	firstElem = coerceValueToLLVMType(firstElem, elementType);

	// Allocate array on stack
	llvm::Value* arraySize = cg.builder.getInt32(elements.size());
	llvm::Value* arrayPtr = cg.builder.CreateAlloca(elementType, arraySize, "arrlit");

	// Store first element
	llvm::Value* idx0 = llvm::ConstantInt::get(cg.builder.getInt32Ty(), 0);
	llvm::Value* elemPtr0 = cg.builder.CreateGEP(elementType, arrayPtr, idx0);
	if (auto firstType = elements[0]->getType(); firstType && firstType->getName() == "String") firstElem = cg.takeOwnedString(firstElem);
	cg.builder.CreateStore(firstElem, elemPtr0);

	// Store remaining elements
	for (size_t i = 1; i < elements.size(); ++i) {
		llvm::Value* elemValue = elements[i]->codegen();
		if (!elemValue) return nullptr;
		if (elementType->isStructTy()) elemValue = coerceValueToLLVMType(elemValue, elementType);
		if (elemValue->getType() != elementType) {
			std::cerr << "Type error: Array literal elements must have the same type" << std::endl;
			return nullptr;
		}
		llvm::Value* index = llvm::ConstantInt::get(cg.builder.getInt32Ty(), i);
		llvm::Value* elemPtr = cg.builder.CreateGEP(elementType, arrayPtr, index);
		if (auto elemType = elements[i]->getType(); elemType && elemType->getName() == "String") elemValue = cg.takeOwnedString(elemValue);
		cg.builder.CreateStore(elemValue, elemPtr);
	}

	// The element count, for loops over the literal once it is bound to a name.
	cg.arrayRuntimeLength[arrayPtr] = cg.builder.getInt32(static_cast<uint32_t>(elements.size()));
	return arrayPtr;
}

std::unique_ptr<Type> ArrayLiteralNode::getType() {
	if (type) return type->clone();
	if (elements.empty()) return std::make_unique<UnknownType>();

	auto firstType = elements[0]->getType();
	if (!firstType) return std::make_unique<UnknownType>();
	for (size_t i = 1; i < elements.size(); ++i) {
		auto elemType = elements[i]->getType();
		if (!elemType || !firstType->equals(elemType)) {
			return std::make_unique<UnknownType>();
		}
	}
	return std::make_unique<ArrayType>(firstType, static_cast<int>(elements.size()));
}
