#include "codegen.h"

#include "dict_support.h"
#include "ArrayLiteralNode.h"
#include "ArrayAccessNode.h"
#include "BinaryExpressionNode.h"
#include "DictLiteralNode.h"
#include "ASTVisitor.h"
#include "type_utils.h"

#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>

namespace {

// Kinds shared with the runtime (src/native_runtime/dict_runtime.inc).
enum DictKind : int {
    KindInt = 0,
    KindDouble = 1,
    KindBool = 2,
    KindString = 3,
    KindChar = 4,
    KindDict = 5,
    KindArray = 6,
    KindOpaque = 7,
};

const Type* unborrow(const Type* type) {
    while (type && (type->getKind() == Type::Kind::BORROW || type->getKind() == Type::Kind::MUTABLE_BORROW)) {
        auto* borrow = dynamic_cast<const BorrowType*>(type);
        if (!borrow || !borrow->baseType) break;
        type = borrow->baseType.get();
    }
    return type;
}

bool isBoolType(const Type* type) {
    return type && type->isBasicType() && (type->getName() == "Bool" || type->getName() == "Boolean");
}

bool isFloatingType(const Type* type) {
    return type && (type->isFloatTy() || type->isDoubleTy());
}

// Array (ArrayType, Array[T] or Vector[T]) -> its element type, else nullptr.
const Type* arrayElementType(const Type* type) {
    type = unborrow(type);
    if (!type) return nullptr;
    if (auto* array = dynamic_cast<const ArrayType*>(type)) return array->elementType.get();
    if (type->getKind() == Type::Kind::GENERIC && (type->getName() == "Array" || type->getName() == "Vector")) {
        auto* generic = dynamic_cast<const GenericType*>(type);
        if (generic && generic->typeArguments.size() == 1) return generic->typeArguments[0].get();
    }
    return nullptr;
}

int integerRank(const std::string& name) {
    if (name == "Byte") return 1;
    if (name == "Short") return 2;
    if (name == "Int") return 3;
    return 4;  // Long, Natural, Integer
}

int kindOf(const Type* type) {
    type = unborrow(type);
    if (!type) return KindOpaque;
    if (type->isIntegerTy()) return KindInt;
    if (isFloatingType(type)) return KindDouble;
    if (isBoolType(type)) return KindBool;
    if (type->isStringTy()) return KindString;
    if (type->isCharTy()) return KindChar;
    if (isDictType(type) || isSetType(type)) return KindDict;  // printed through the runtime object
    if (arrayElementType(type)) return KindArray;
    return KindOpaque;
}

llvm::Function* runtimeFunction(const std::string& name, llvm::Type* returnType, const std::vector<llvm::Type*>& params) {
    auto& cg = CodeGenerator::getInstance();
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* functionType = llvm::FunctionType::get(returnType, params, false);
    return llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, cg.module.get());
}

llvm::Type* i64Type() { return llvm::Type::getInt64Ty(CodeGenerator::getInstance().context); }
llvm::Type* i32Type() { return llvm::Type::getInt32Ty(CodeGenerator::getInstance().context); }
llvm::Type* ptrType() { return llvm::PointerType::getUnqual(CodeGenerator::getInstance().context); }

// The LLVM type a dictionary key/value of `type` has as an SSA value. Arrays are their buffer pointer.
llvm::Type* slotType(const Type* type) {
    type = unborrow(type);
    if (!type) return nullptr;
    if (arrayElementType(type) || isDictType(type) || isSetType(type)) return ptrType();
    // Reference classes are pointers (getLLVMType gives the class's struct type).
    return getABIStorageType(type);
}

// Load a String / Dict handle when an expression produced the address of the slot holding it.
llvm::Value* loadHandleSlot(llvm::Value* value, const Type* type) {
    auto& cg = CodeGenerator::getInstance();
    type = unborrow(type);
    if (!value || !type || !(type->isStringTy() || isDictType(type))) return value;
    if (auto* slot = llvm::dyn_cast<llvm::AllocaInst>(value)) {
        if (slot->getAllocatedType()->isPointerTy()) {
            return cg.builder.CreateLoad(ptrType(), slot, "dict.handle.load");
        }
    }
    return value;
}

// Convert a scalar SSA value to `target` (integer widening/narrowing, int <-> float).
llvm::Value* convertScalar(llvm::Value* value, llvm::Type* target, const Type* sourceType) {
    auto& cg = CodeGenerator::getInstance();
    if (!value || !target || value->getType() == target) return value;
    llvm::Type* source = value->getType();
    if (source->isIntegerTy() && target->isIntegerTy()) {
        unsigned from = source->getIntegerBitWidth();
        unsigned to = target->getIntegerBitWidth();
        if (from < to) {
            const bool unsignedSource = from == 1 || isBoolType(sourceType) || (sourceType && sourceType->isCharTy());
            return unsignedSource ? cg.builder.CreateZExt(value, target, "dict.zext")
                                  : cg.builder.CreateSExt(value, target, "dict.sext");
        }
        return cg.builder.CreateTrunc(value, target, "dict.trunc");
    }
    return coerceValueToLLVMType(value, target);
}

std::unique_ptr<Type> vectorOf(const Type* element) {
    std::vector<std::unique_ptr<Type>> arguments;
    arguments.push_back(cloneType(element));
    std::unique_ptr<Type> base = std::make_unique<ClassType>("Vector");
    return std::make_unique<GenericType>(base, arguments);
}

llvm::Value* heapArrayLiteral(ArrayLiteralNode* literal, const Type* elementType) {
    auto& cg = CodeGenerator::getInstance();
    llvm::Type* elementLLVM = slotType(elementType);
    if (!elementLLVM) return nullptr;
    const auto& layout = cg.module->getDataLayout();
    const uint64_t elementSize = layout.getTypeAllocSize(elementLLVM);
    const uint64_t count = literal->elements.size();
    llvm::Value* bytes = llvm::ConstantInt::get(i64Type(), elementSize * (count > 0 ? count : 1));
    llvm::Value* buffer = cg.builder.CreateCall(cg.mallocFunction, bytes, "dict.list");
    for (uint64_t i = 0; i < count; ++i) {
        ASTNode* element = literal->elements[i].get();
        llvm::Value* value = nullptr;
        if (auto* nested = dynamic_cast<ArrayLiteralNode*>(element)) {
            const Type* nestedElement = arrayElementType(elementType);
            if (!nestedElement) {
                std::cerr << "Type error: nested list in a dictionary value needs a list element type" << std::endl;
                return nullptr;
            }
            value = heapArrayLiteral(nested, nestedElement);
        }
        else if (auto* dict = dynamic_cast<DictLiteralNode*>(element)) {
            if (isDictType(elementType) || isSetType(elementType)) dict->type = cloneType(elementType);
            value = dict->codegen();
        }
        else {
            value = element ? element->codegen() : nullptr;
            auto sourceType = element ? element->getType() : nullptr;
            value = loadHandleSlot(value, sourceType.get());
            value = convertScalar(value, elementLLVM, sourceType.get());
        }
        if (!value) return nullptr;
        if (value->getType() != elementLLVM) {
            std::cerr << "Type error: dictionary list element does not match its element type" << std::endl;
            return nullptr;
        }
        llvm::Value* slot = cg.builder.CreateInBoundsGEP(
            elementLLVM, buffer, llvm::ConstantInt::get(i64Type(), i), "dict.list.slot");
        if (auto elemType = element ? element->getType() : nullptr; elemType && elemType->getName() == "String") value = cg.takeOwnedString(value);
        cg.builder.CreateStore(value, slot);
    }
    return buffer;
}

llvm::Value* codegenKeyPayload(ASTNode* keyExpr, const Type* keyType) {
    long long ignored = -1;
    return codegenDictPayload(keyExpr, keyType, &ignored);
}

} // namespace

bool isDictType(const Type* type) {
    type = unborrow(type);
    if (!type || type->getKind() != Type::Kind::GENERIC || type->getName() != "Dict") return false;
    auto* generic = dynamic_cast<const GenericType*>(type);
    return generic && generic->typeArguments.size() == 2 && generic->typeArguments[0] && generic->typeArguments[1];
}

std::unique_ptr<Type> makeDictType(const Type* keyType, const Type* valueType) {
    std::vector<std::unique_ptr<Type>> arguments;
    arguments.push_back(cloneType(keyType));
    arguments.push_back(cloneType(valueType));
    std::unique_ptr<Type> base = std::make_unique<ClassType>("Dict");
    return std::make_unique<GenericType>(base, arguments);
}

const Type* dictKeyType(const Type* type) {
    if (!isDictType(type)) return nullptr;
    return static_cast<const GenericType*>(unborrow(type))->typeArguments[0].get();
}

const Type* dictValueType(const Type* type) {
    if (!isDictType(type)) return nullptr;
    return static_cast<const GenericType*>(unborrow(type))->typeArguments[1].get();
}

bool isValidDictKeyType(const Type* type) {
    type = unborrow(type);
    return type && (type->isIntegerTy() || type->isStringTy() || type->isCharTy() || isBoolType(type));
}

bool isValidDictValueType(const Type* type) {
    type = unborrow(type);
    if (!type || type->getKind() == Type::Kind::UNKNOWN || type->isVoidTy()) return false;
    if (type->getKind() == Type::Kind::TUPLE || type->getKind() == Type::Kind::BOX) return false;
    if (isDictType(type)) return isValidDictKeyType(dictKeyType(type)) && isValidDictValueType(dictValueType(type));
    if (isSetType(type)) return isValidDictKeyType(setElementType(type));
    if (const Type* element = arrayElementType(type)) return isValidDictValueType(element);
    if (isStructClassType(type)) return false;
    // Reference classes (also Nat, Tensor) are pointers. Checked by kind: during type checking the
    // classes have no LLVM types yet.
    if (type->getKind() == Type::Kind::CLASS) return true;
    llvm::Type* llvmType = getABIStorageType(type);
    return llvmType && (llvmType->isIntegerTy() || llvmType->isFloatingPointTy() || llvmType->isPointerTy());
}

std::unique_ptr<Type> unifyDictElementTypes(const Type* a, const Type* b) {
    a = unborrow(a);
    b = unborrow(b);
    if (!a || !b || a->getKind() == Type::Kind::UNKNOWN || b->getKind() == Type::Kind::UNKNOWN) return nullptr;
    if (isBoolType(a) && isBoolType(b)) return cloneType(a);
    if (a->isIntegerTy() && b->isIntegerTy()) {
        return integerRank(a->getName()) >= integerRank(b->getName()) ? cloneType(a) : cloneType(b);
    }
    const bool aNumeric = a->isIntegerTy() || isFloatingType(a);
    const bool bNumeric = b->isIntegerTy() || isFloatingType(b);
    if (aNumeric && bNumeric) {
        if (a->isFloatTy() && (b->isFloatTy() || b->isIntegerTy())) return cloneType(a);
        if (b->isFloatTy() && a->isIntegerTy()) return cloneType(b);
        return std::make_unique<BasicType>("Double");
    }
    if (isSetType(a) && isSetType(b)) {
        auto element = unifyDictElementTypes(setElementType(a), setElementType(b));
        return element ? makeSetType(element.get()) : nullptr;
    }
    if (isDictType(a) && isDictType(b)) {
        auto key = unifyDictElementTypes(dictKeyType(a), dictKeyType(b));
        auto value = unifyDictElementTypes(dictValueType(a), dictValueType(b));
        if (!key || !value) return nullptr;
        return makeDictType(key.get(), value.get());
    }
    const Type* elementA = arrayElementType(a);
    const Type* elementB = arrayElementType(b);
    if (elementA && elementB) {
        auto element = unifyDictElementTypes(elementA, elementB);
        if (!element) return nullptr;
        return vectorOf(element.get());
    }
    if (a->equals(*b)) return cloneType(a);
    // Other types (classes included) must match exactly: the language has no implicit upcast.
    return nullptr;
}

llvm::Value* dictPayloadFromValue(llvm::Value* value, const Type* type) {
    auto& cg = CodeGenerator::getInstance();
    if (!value) return nullptr;
    llvm::Type* source = value->getType();
    if (source->isIntegerTy()) {
        return convertScalar(value, i64Type(), type);
    }
    if (source->isFloatingPointTy()) {
        llvm::Value* asDouble = source->isDoubleTy()
            ? value
            : cg.builder.CreateFPExt(value, llvm::Type::getDoubleTy(cg.context), "dict.fpext");
        return cg.builder.CreateBitCast(asDouble, i64Type(), "dict.bits");
    }
    if (source->isPointerTy()) {
        return cg.builder.CreatePtrToInt(value, i64Type(), "dict.ptrbits");
    }
    std::cerr << "Type error: value of type '" << (type ? type->getName() : "?")
              << "' cannot be stored in a dictionary" << std::endl;
    return nullptr;
}

llvm::Value* dictValueFromPayload(llvm::Value* payload, const Type* type) {
    auto& cg = CodeGenerator::getInstance();
    llvm::Type* target = slotType(type);
    if (!payload || !target) return nullptr;
    if (target->isIntegerTy()) {
        if (target->getIntegerBitWidth() == 64) return payload;
        return cg.builder.CreateTrunc(payload, target, "dict.value");
    }
    if (target->isFloatingPointTy()) {
        llvm::Value* asDouble = cg.builder.CreateBitCast(payload, llvm::Type::getDoubleTy(cg.context), "dict.double");
        if (target->isDoubleTy()) return asDouble;
        return cg.builder.CreateFPTrunc(asDouble, target, "dict.float");
    }
    if (target->isPointerTy()) {
        return cg.builder.CreateIntToPtr(payload, target, "dict.ptr");
    }
    std::cerr << "Type error: dictionary value type '" << (type ? type->getName() : "?") << "' is not supported" << std::endl;
    return nullptr;
}

llvm::Value* codegenDictPayload(ASTNode* expr, const Type* targetType, long long* lengthOut) {
    if (lengthOut) *lengthOut = -1;
    if (!expr || !targetType) return nullptr;
    targetType = unborrow(targetType);

    if (auto* literal = dynamic_cast<ArrayLiteralNode*>(expr)) {
        if (const Type* element = arrayElementType(targetType)) {
            llvm::Value* buffer = heapArrayLiteral(literal, element);
            if (lengthOut) *lengthOut = static_cast<long long>(literal->elements.size());
            return buffer ? dictPayloadFromValue(buffer, targetType) : nullptr;
        }
    }
    if (auto* dict = dynamic_cast<DictLiteralNode*>(expr)) {
        if (isDictType(targetType) || isSetType(targetType)) dict->type = cloneType(targetType);
    }

    llvm::Value* value = expr->codegen();
    auto sourceType = expr->getType();
    value = loadHandleSlot(value, sourceType.get());
    if (!value) return nullptr;
    if (targetType->getName() == "String" && value->getType()->isPointerTy()) {
        value = CodeGenerator::getInstance().takeOwnedString(value);
    }
    llvm::Type* target = slotType(targetType);
    if (target && (target->isIntegerTy() || target->isFloatingPointTy())) {
        value = convertScalar(value, target, sourceType.get());
    }
    return dictPayloadFromValue(value, targetType);
}

llvm::Value* codegenDictHandle(ASTNode* dictExpr) {
    if (!dictExpr) return nullptr;
    llvm::Value* value = dictExpr->codegen();
    auto type = dictExpr->getType();
    value = loadHandleSlot(value, type.get());
    if (value && !value->getType()->isPointerTy()) {
        std::cerr << "Type error: dictionary expression did not produce a dictionary handle" << std::endl;
        return nullptr;
    }
    return value;
}

llvm::Value* codegenDictNew(const Type* dictType) {
    auto& cg = CodeGenerator::getInstance();
    const Type* keyType = dictKeyType(dictType);
    const Type* valueType = dictValueType(dictType);
    if (!keyType || !valueType) return nullptr;
    int elementKind = KindInt;
    int elementBytes = 8;
    if (const Type* element = arrayElementType(valueType)) {
        elementKind = kindOf(element);
        if (llvm::Type* elementLLVM = slotType(element)) {
            elementBytes = static_cast<int>(cg.module->getDataLayout().getTypeAllocSize(elementLLVM));
        }
    }
    auto* fn = runtimeFunction("csec_dict_new", ptrType(), {i32Type(), i32Type(), i32Type(), i32Type()});
    return cg.builder.CreateCall(fn, {
        llvm::ConstantInt::get(i32Type(), kindOf(keyType)),
        llvm::ConstantInt::get(i32Type(), kindOf(valueType)),
        llvm::ConstantInt::get(i32Type(), elementKind),
        llvm::ConstantInt::get(i32Type(), elementBytes),
    }, "dict.new");
}

llvm::Value* codegenDictToString(llvm::Value* handle) {
    auto& cg = CodeGenerator::getInstance();
    if (!handle) return nullptr;
    return cg.builder.CreateCall(runtimeFunction("csec_dict_to_string", ptrType(), {ptrType()}), {handle}, "dict.str");
}

llvm::Value* codegenDictGet(ASTNode* dictExpr, ASTNode* keyExpr) {
    auto& cg = CodeGenerator::getInstance();
    auto dictType = dictExpr->getType();
    const Type* keyType = dictKeyType(dictType.get());
    const Type* valueType = dictValueType(dictType.get());
    if (!keyType || !valueType) return nullptr;
    llvm::Value* handle = codegenDictHandle(dictExpr);
    llvm::Value* key = codegenKeyPayload(keyExpr, keyType);
    if (!handle || !key) return nullptr;
    llvm::Value* payload = cg.builder.CreateCall(
        runtimeFunction("csec_dict_get", i64Type(), {ptrType(), i64Type()}), {handle, key}, "dict.get");
    return dictValueFromPayload(payload, valueType);
}

llvm::Value* codegenDictSet(ASTNode* dictExpr, ASTNode* keyExpr, ASTNode* valueExpr, const std::string& op) {
    auto& cg = CodeGenerator::getInstance();
    auto dictType = dictExpr->getType();
    const Type* keyType = dictKeyType(dictType.get());
    const Type* valueType = dictValueType(dictType.get());
    if (!keyType || !valueType || !valueExpr) return nullptr;

    // `d[k] op= v` is `d[k] = d[k] op v`.
    std::unique_ptr<ASTNode> combined;
    if (op != "=" && op != "<-" && op.size() == 2 && op[1] == '=') {
        auto current = std::make_unique<ArrayAccessNode>();
        current->array = dictExpr->clone();
        current->index = keyExpr->clone();
        auto spec = std::make_unique<ArrayIndexSpec>();
        spec->index = keyExpr->clone();
        current->indices.push_back(std::move(spec));
        combined = std::make_unique<BinaryExpressionNode>(std::move(current), op.substr(0, 1), valueExpr->clone());
        valueExpr = combined.get();
    }

    llvm::Value* handle = codegenDictHandle(dictExpr);
    llvm::Value* key = codegenKeyPayload(keyExpr, keyType);
    long long length = -1;
    llvm::Value* payload = codegenDictPayload(valueExpr, valueType, &length);
    if (!handle || !key || !payload) return nullptr;
    if (length >= 0) {
        cg.builder.CreateCall(
            runtimeFunction("csec_dict_set_array", llvm::Type::getVoidTy(cg.context), {ptrType(), i64Type(), i64Type(), i64Type()}),
            {handle, key, payload, llvm::ConstantInt::get(i64Type(), length)});
    }
    else {
        cg.builder.CreateCall(
            runtimeFunction("csec_dict_set", llvm::Type::getVoidTy(cg.context), {ptrType(), i64Type(), i64Type()}),
            {handle, key, payload});
    }
    return dictValueFromPayload(payload, valueType);
}

llvm::Value* codegenDictContains(ASTNode* dictExpr, ASTNode* keyExpr) {
    auto& cg = CodeGenerator::getInstance();
    auto dictType = dictExpr->getType();
    const Type* keyType = dictKeyType(dictType.get());
    if (!keyType) return nullptr;
    llvm::Value* handle = codegenDictHandle(dictExpr);
    llvm::Value* key = codegenKeyPayload(keyExpr, keyType);
    if (!handle || !key) return nullptr;
    llvm::Value* found = cg.builder.CreateCall(
        runtimeFunction("csec_dict_contains", i32Type(), {ptrType(), i64Type()}), {handle, key}, "dict.has");
    return cg.builder.CreateICmpNE(found, llvm::ConstantInt::get(i32Type(), 0), "dict.has.bool");
}

std::unique_ptr<Type> dictMethodType(const Type* dictType, const std::string& method, size_t argumentCount) {
    const Type* keyType = dictKeyType(dictType);
    const Type* valueType = dictValueType(dictType);
    if (!keyType || !valueType) return nullptr;
    if ((method == "size" || method == "length" || method == "count" || method == "len") && argumentCount == 0) {
        return std::make_unique<BasicType>("Long");
    }
    if (method == "isEmpty" && argumentCount == 0) return std::make_unique<BasicType>("Boolean");
    if ((method == "contains" || method == "has" || method == "containsKey") && argumentCount == 1) {
        return std::make_unique<BasicType>("Boolean");
    }
    if (method == "remove" && argumentCount == 1) return std::make_unique<BasicType>("Boolean");
    if (method == "get" && (argumentCount == 1 || argumentCount == 2)) return cloneType(valueType);
    if (method == "keyAt" && argumentCount == 1) return cloneType(keyType);
    if (method == "valueAt" && argumentCount == 1) return cloneType(valueType);
    if (method == "clear" && argumentCount == 0) return std::make_unique<BasicType>("Unit");
    if (method == "copy" && argumentCount == 0) return makeDictType(keyType, valueType);
    if (method == "toString" && argumentCount == 0) return std::make_unique<BasicType>("String");
    if (method == "keys" && argumentCount == 0) return makeSetType(keyType);
    return nullptr;
}

llvm::Value* codegenDictMethod(ASTNode* dictExpr, const std::string& method,
                               std::vector<std::unique_ptr<ASTNode>>& arguments, bool& handled) {
    auto& cg = CodeGenerator::getInstance();
    auto dictType = dictExpr->getType();
    handled = dictMethodType(dictType.get(), method, arguments.size()) != nullptr;
    if (!handled) {
        std::cerr << "Error: Dict has no method '" << method << "' taking " << arguments.size()
                  << " argument(s)" << std::endl;
        handled = true;
        return nullptr;
    }
    const Type* keyType = dictKeyType(dictType.get());
    const Type* valueType = dictValueType(dictType.get());
    auto* voidTy = llvm::Type::getVoidTy(cg.context);

    if (method == "contains" || method == "has" || method == "containsKey") {
        return codegenDictContains(dictExpr, arguments[0].get());
    }
    if (method == "get" && arguments.size() == 1) {
        return codegenDictGet(dictExpr, arguments[0].get());
    }

    llvm::Value* handle = codegenDictHandle(dictExpr);
    if (!handle) return nullptr;

    if (method == "size" || method == "length" || method == "count" || method == "len") {
        return cg.builder.CreateCall(runtimeFunction("csec_dict_size", i64Type(), {ptrType()}), {handle}, "dict.size");
    }
    if (method == "isEmpty") {
        llvm::Value* size = cg.builder.CreateCall(runtimeFunction("csec_dict_size", i64Type(), {ptrType()}), {handle}, "dict.size");
        return cg.builder.CreateICmpEQ(size, llvm::ConstantInt::get(i64Type(), 0), "dict.empty");
    }
    if (method == "get") {
        llvm::Value* key = codegenKeyPayload(arguments[0].get(), keyType);
        long long ignored = -1;
        llvm::Value* fallback = codegenDictPayload(arguments[1].get(), valueType, &ignored);
        if (!key || !fallback) return nullptr;
        llvm::Value* payload = cg.builder.CreateCall(
            runtimeFunction("csec_dict_get_or", i64Type(), {ptrType(), i64Type(), i64Type()}),
            {handle, key, fallback}, "dict.get.or");
        return dictValueFromPayload(payload, valueType);
    }
    if (method == "remove") {
        llvm::Value* key = codegenKeyPayload(arguments[0].get(), keyType);
        if (!key) return nullptr;
        llvm::Value* removed = cg.builder.CreateCall(
            runtimeFunction("csec_dict_remove", i32Type(), {ptrType(), i64Type()}), {handle, key}, "dict.remove");
        return cg.builder.CreateICmpNE(removed, llvm::ConstantInt::get(i32Type(), 0), "dict.removed");
    }
    if (method == "keyAt" || method == "valueAt") {
        long long ignored = -1;
        BasicType longType("Long");
        llvm::Value* index = codegenDictPayload(arguments[0].get(), &longType, &ignored);
        if (!index) return nullptr;
        const bool keys = method == "keyAt";
        llvm::Value* payload = cg.builder.CreateCall(
            runtimeFunction(keys ? "csec_dict_key_at" : "csec_dict_value_at", i64Type(), {ptrType(), i64Type()}),
            {handle, index}, keys ? "dict.key" : "dict.value");
        return dictValueFromPayload(payload, keys ? keyType : valueType);
    }
    if (method == "clear") {
        cg.builder.CreateCall(runtimeFunction("csec_dict_clear", voidTy, {ptrType()}), {handle});
        return llvm::ConstantInt::get(i32Type(), 0);
    }
    if (method == "copy") {
        return cg.builder.CreateCall(runtimeFunction("csec_dict_copy", ptrType(), {ptrType()}), {handle}, "dict.copy");
    }
    if (method == "toString") {
        return codegenDictToString(handle);
    }
    if (method == "keys") {
        return cg.builder.CreateCall(runtimeFunction("csec_dict_keys", ptrType(), {ptrType()}), {handle}, "dict.keys");
    }
    return nullptr;
}

// ---- DictLiteralNode ----

void DictLiteralNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string DictLiteralNode::inferenceError() {
    if (isSet) {
        std::unique_ptr<Type> elementType;
        for (size_t i = 0; i < keys.size(); ++i) {
            auto e = keys[i] ? keys[i]->getType() : nullptr;
            if (!e || e->getKind() == Type::Kind::UNKNOWN) return "cannot infer the type of set element #" + std::to_string(i + 1);
            if (!isValidDictKeyType(e.get())) {
                return "set elements must be Int, Long, Char, Bool or String, not '" + e->getName() + "'";
            }
            auto previous = elementType ? elementType->clone() : nullptr;
            elementType = elementType ? unifyDictElementTypes(elementType.get(), e.get()) : e->clone();
            if (!elementType) {
                return "set elements have different types ('" + previous->getName() + "' and '" + e->getName() +
                       "'); a Set holds one element type";
            }
        }
        return "";
    }
    if (keys.empty()) {
        return "cannot infer the type of an empty dictionary literal '{}'; declare it with a type, "
               "e.g. 'val d: Dict<String, Int> = {}'";
    }
    std::unique_ptr<Type> keyType;
    std::unique_ptr<Type> valueType;
    for (size_t i = 0; i < keys.size(); ++i) {
        auto k = keys[i] ? keys[i]->getType() : nullptr;
        auto v = values[i] ? values[i]->getType() : nullptr;
        if (!k || k->getKind() == Type::Kind::UNKNOWN) return "cannot infer the type of dictionary key #" + std::to_string(i + 1);
        if (!v || v->getKind() == Type::Kind::UNKNOWN) return "cannot infer the type of dictionary value #" + std::to_string(i + 1);
        if (!isValidDictKeyType(k.get())) {
            return "dictionary keys must be Int, Long, Char, Bool or String, not '" + k->getName() + "'";
        }
        keyType = keyType ? unifyDictElementTypes(keyType.get(), k.get()) : k->clone();
        if (!keyType) return "dictionary keys have different types (key #" + std::to_string(i + 1) + " is '" + k->getName() + "')";
        auto previous = valueType ? valueType->clone() : nullptr;
        valueType = valueType ? unifyDictElementTypes(valueType.get(), v.get()) : v->clone();
        if (!valueType) {
            return "dictionary values have different types ('" + previous->getName() + "' and '" + v->getName() +
                   "'); a Dict holds one value type";
        }
        if (!isValidDictValueType(valueType.get())) {
            return "values of type '" + valueType->getName() + "' cannot be stored in a dictionary";
        }
    }
    return "";
}

std::unique_ptr<Type> DictLiteralNode::getType() {
    if (type) return type->clone();
    if (keys.empty()) return std::make_unique<UnknownType>();
    if (isSet) {
        std::unique_ptr<Type> elementType;
        for (auto& key : keys) {
            auto e = key ? key->getType() : nullptr;
            if (!e) return std::make_unique<UnknownType>();
            elementType = elementType ? unifyDictElementTypes(elementType.get(), e.get()) : e->clone();
            if (!elementType) return std::make_unique<UnknownType>();
        }
        if (!isValidDictKeyType(elementType.get())) return std::make_unique<UnknownType>();
        return makeSetType(elementType.get());
    }
    std::unique_ptr<Type> keyType;
    std::unique_ptr<Type> valueType;
    for (size_t i = 0; i < keys.size(); ++i) {
        auto k = keys[i] ? keys[i]->getType() : nullptr;
        auto v = values[i] ? values[i]->getType() : nullptr;
        if (!k || !v) return std::make_unique<UnknownType>();
        keyType = keyType ? unifyDictElementTypes(keyType.get(), k.get()) : k->clone();
        valueType = valueType ? unifyDictElementTypes(valueType.get(), v.get()) : v->clone();
        if (!keyType || !valueType) return std::make_unique<UnknownType>();
    }
    if (!isValidDictKeyType(keyType.get()) || !isValidDictValueType(valueType.get())) {
        return std::make_unique<UnknownType>();
    }
    // A list literal value is stored as a heap list: its static type is Vector[T] whatever its length.
    if (const Type* element = arrayElementType(valueType.get())) {
        valueType = vectorOf(element);
    }
    return makeDictType(keyType.get(), valueType.get());
}

llvm::Value* DictLiteralNode::codegen() {
    auto dictType = getType();
    if (isSetType(dictType.get())) {
        return codegenSetLiteral(dictType.get(), keys);
    }
    if (!isDictType(dictType.get())) {
        std::cerr << "Type error: " << inferenceError() << std::endl;
        return nullptr;
    }
    llvm::Value* handle = codegenDictNew(dictType.get());
    if (!handle) return nullptr;
    auto& cg = CodeGenerator::getInstance();
    const Type* keyType = dictKeyType(dictType.get());
    const Type* valueType = dictValueType(dictType.get());
    for (size_t i = 0; i < keys.size(); ++i) {
        llvm::Value* key = codegenKeyPayload(keys[i].get(), keyType);
        long long length = -1;
        llvm::Value* value = codegenDictPayload(values[i].get(), valueType, &length);
        if (!key || !value) return nullptr;
        if (length >= 0) {
            cg.builder.CreateCall(
                runtimeFunction("csec_dict_set_array", llvm::Type::getVoidTy(cg.context), {ptrType(), i64Type(), i64Type(), i64Type()}),
                {handle, key, value, llvm::ConstantInt::get(i64Type(), length)});
        }
        else {
            cg.builder.CreateCall(
                runtimeFunction("csec_dict_set", llvm::Type::getVoidTy(cg.context), {ptrType(), i64Type(), i64Type()}),
                {handle, key, value});
        }
    }
    return handle;
}

// ---- Sets ----

bool isSetType(const Type* type) {
    type = unborrow(type);
    if (!type || type->getKind() != Type::Kind::GENERIC || type->getName() != "Set") return false;
    auto* generic = dynamic_cast<const GenericType*>(type);
    return generic && generic->typeArguments.size() == 1 && generic->typeArguments[0];
}

std::unique_ptr<Type> makeSetType(const Type* elementType) {
    std::vector<std::unique_ptr<Type>> arguments;
    arguments.push_back(cloneType(elementType));
    std::unique_ptr<Type> base = std::make_unique<ClassType>("Set");
    return std::make_unique<GenericType>(base, arguments);
}

const Type* setElementType(const Type* type) {
    if (!isSetType(type)) return nullptr;
    return static_cast<const GenericType*>(unborrow(type))->typeArguments[0].get();
}

llvm::Value* codegenSetNew(const Type* setType) {
    auto& cg = CodeGenerator::getInstance();
    const Type* element = setElementType(setType);
    if (!element) return nullptr;
    return cg.builder.CreateCall(runtimeFunction("csec_set_new", ptrType(), {i32Type()}),
                                 {llvm::ConstantInt::get(i32Type(), kindOf(element))}, "set.new");
}

llvm::Value* codegenSetLiteral(const Type* setType, std::vector<std::unique_ptr<ASTNode>>& elements) {
    auto& cg = CodeGenerator::getInstance();
    const Type* elementType = setElementType(setType);
    llvm::Value* handle = codegenSetNew(setType);
    if (!handle || !elementType) return nullptr;
    auto* add = runtimeFunction("csec_set_add", i32Type(), {ptrType(), i64Type()});
    for (auto& element : elements) {
        llvm::Value* payload = codegenKeyPayload(element.get(), elementType);
        if (!payload) return nullptr;
        cg.builder.CreateCall(add, {handle, payload});
    }
    return handle;
}

llvm::Value* codegenSetContains(ASTNode* setExpr, ASTNode* elementExpr) {
    auto& cg = CodeGenerator::getInstance();
    auto setType = setExpr->getType();
    const Type* elementType = setElementType(setType.get());
    if (!elementType) return nullptr;
    llvm::Value* handle = codegenDictHandle(setExpr);
    llvm::Value* element = codegenKeyPayload(elementExpr, elementType);
    if (!handle || !element) return nullptr;
    llvm::Value* found = cg.builder.CreateCall(
        runtimeFunction("csec_dict_contains", i32Type(), {ptrType(), i64Type()}), {handle, element}, "set.has");
    return cg.builder.CreateICmpNE(found, llvm::ConstantInt::get(i32Type(), 0), "set.has.bool");
}

namespace {
// The runtime function and result shape of a set operator.
struct SetOperation {
    const char* runtime = nullptr;  // csec_set_*
    bool producesSet = false;
    bool swap = false;    // evaluate runtime(right, left)
    bool negate = false;  // !runtime(...)
    bool proper = false;  // also require the sets to differ
};

bool setOperation(const std::string& op, SetOperation& out) {
    out = SetOperation();
    if (op == "|" || op == "union") { out.runtime = "csec_set_union"; out.producesSet = true; return true; }
    if (op == "&" || op == "intersect") { out.runtime = "csec_set_intersection"; out.producesSet = true; return true; }
    if (op == "-" || op == "diff" || op == "without") { out.runtime = "csec_set_difference"; out.producesSet = true; return true; }
    if (op == "^" || op == "xor" || op == "symdiff") { out.runtime = "csec_set_symmetric_difference"; out.producesSet = true; return true; }
    if (op == "==") { out.runtime = "csec_set_equals"; return true; }
    if (op == "!=") { out.runtime = "csec_set_equals"; out.negate = true; return true; }
    if (op == "<=" || op == "subset") { out.runtime = "csec_set_is_subset"; return true; }
    if (op == ">=" || op == "superset") { out.runtime = "csec_set_is_subset"; out.swap = true; return true; }
    if (op == "<" || op == "properSubset") { out.runtime = "csec_set_is_subset"; out.proper = true; return true; }
    if (op == ">" || op == "properSuperset") { out.runtime = "csec_set_is_subset"; out.swap = true; out.proper = true; return true; }
    return false;
}
} // namespace

bool isSetBinaryOperator(const std::string& op) {
    SetOperation ignored;
    return setOperation(op, ignored);
}

std::unique_ptr<Type> setBinaryOperatorType(const std::string& op, const Type* leftType, const Type* rightType) {
    SetOperation operation;
    if (!isSetType(leftType) || !isSetType(rightType) || !setOperation(op, operation)) return nullptr;
    if (!operation.producesSet) return std::make_unique<BasicType>("Boolean");
    return unifyDictElementTypes(leftType, rightType);
}

llvm::Value* codegenSetBinary(const std::string& op, ASTNode* left, ASTNode* right) {
    auto& cg = CodeGenerator::getInstance();
    SetOperation operation;
    if (!setOperation(op, operation)) return nullptr;
    llvm::Value* a = codegenDictHandle(left);
    llvm::Value* b = codegenDictHandle(right);
    if (!a || !b) return nullptr;
    if (operation.swap) std::swap(a, b);
    if (operation.producesSet) {
        return cg.builder.CreateCall(runtimeFunction(operation.runtime, ptrType(), {ptrType(), ptrType()}), {a, b}, "set.op");
    }
    llvm::Value* raw = cg.builder.CreateCall(runtimeFunction(operation.runtime, i32Type(), {ptrType(), ptrType()}), {a, b}, "set.test");
    llvm::Value* result = cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Type(), 0), "set.test.bool");
    if (operation.negate) result = cg.builder.CreateNot(result, "set.test.not");
    if (operation.proper) {
        llvm::Value* same = cg.builder.CreateCall(runtimeFunction("csec_set_equals", i32Type(), {ptrType(), ptrType()}), {a, b}, "set.same");
        llvm::Value* differs = cg.builder.CreateICmpEQ(same, llvm::ConstantInt::get(i32Type(), 0), "set.differs");
        result = cg.builder.CreateAnd(result, differs, "set.proper");
    }
    return result;
}

std::unique_ptr<Type> setMethodType(const Type* setType, const std::string& method, size_t argumentCount) {
    const Type* elementType = setElementType(setType);
    if (!elementType) return nullptr;
    if ((method == "size" || method == "length" || method == "count" || method == "len") && argumentCount == 0) {
        return std::make_unique<BasicType>("Long");
    }
    if (method == "isEmpty" && argumentCount == 0) return std::make_unique<BasicType>("Boolean");
    if ((method == "add" || method == "remove" || method == "discard" || method == "contains" || method == "has") &&
        argumentCount == 1) {
        return std::make_unique<BasicType>("Boolean");
    }
    if (method == "elementAt" && argumentCount == 1) return cloneType(elementType);
    if (method == "clear" && argumentCount == 0) return std::make_unique<BasicType>("Unit");
    if (method == "copy" && argumentCount == 0) return makeSetType(elementType);
    if (method == "toString" && argumentCount == 0) return std::make_unique<BasicType>("String");
    if ((method == "union" || method == "intersection" || method == "difference" || method == "symmetricDifference") &&
        argumentCount == 1) {
        return makeSetType(elementType);
    }
    if ((method == "isSubset" || method == "isSuperset" || method == "isDisjoint") && argumentCount == 1) {
        return std::make_unique<BasicType>("Boolean");
    }
    return nullptr;
}

llvm::Value* codegenSetMethod(ASTNode* setExpr, const std::string& method,
                              std::vector<std::unique_ptr<ASTNode>>& arguments, bool& handled) {
    auto& cg = CodeGenerator::getInstance();
    auto setType = setExpr->getType();
    handled = true;
    if (!setMethodType(setType.get(), method, arguments.size())) {
        std::cerr << "Error: Set has no method '" << method << "' taking " << arguments.size() << " argument(s)" << std::endl;
        return nullptr;
    }
    const Type* elementType = setElementType(setType.get());
    if (method == "contains" || method == "has") return codegenSetContains(setExpr, arguments[0].get());
    if (method == "union") return codegenSetBinary("|", setExpr, arguments[0].get());
    if (method == "intersection") return codegenSetBinary("&", setExpr, arguments[0].get());
    if (method == "difference") return codegenSetBinary("-", setExpr, arguments[0].get());
    if (method == "symmetricDifference") return codegenSetBinary("^", setExpr, arguments[0].get());
    if (method == "isSubset") return codegenSetBinary("<=", setExpr, arguments[0].get());
    if (method == "isSuperset") return codegenSetBinary(">=", setExpr, arguments[0].get());

    llvm::Value* handle = codegenDictHandle(setExpr);
    if (!handle) return nullptr;
    if (method == "isDisjoint") {
        llvm::Value* other = codegenDictHandle(arguments[0].get());
        if (!other) return nullptr;
        llvm::Value* raw = cg.builder.CreateCall(
            runtimeFunction("csec_set_is_disjoint", i32Type(), {ptrType(), ptrType()}), {handle, other}, "set.disjoint");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Type(), 0), "set.disjoint.bool");
    }
    if (method == "size" || method == "length" || method == "count" || method == "len" || method == "isEmpty") {
        llvm::Value* size = cg.builder.CreateCall(runtimeFunction("csec_dict_size", i64Type(), {ptrType()}), {handle}, "set.size");
        if (method != "isEmpty") return size;
        return cg.builder.CreateICmpEQ(size, llvm::ConstantInt::get(i64Type(), 0), "set.empty");
    }
    if (method == "add" || method == "remove" || method == "discard") {
        llvm::Value* element = codegenKeyPayload(arguments[0].get(), elementType);
        if (!element) return nullptr;
        const bool adding = method == "add";
        llvm::Value* raw = cg.builder.CreateCall(
            runtimeFunction(adding ? "csec_set_add" : "csec_dict_remove", i32Type(), {ptrType(), i64Type()}),
            {handle, element}, adding ? "set.added" : "set.removed");
        return cg.builder.CreateICmpNE(raw, llvm::ConstantInt::get(i32Type(), 0), "set.changed");
    }
    if (method == "elementAt") {
        long long ignored = -1;
        BasicType longType("Long");
        llvm::Value* index = codegenDictPayload(arguments[0].get(), &longType, &ignored);
        if (!index) return nullptr;
        llvm::Value* payload = cg.builder.CreateCall(
            runtimeFunction("csec_dict_key_at", i64Type(), {ptrType(), i64Type()}), {handle, index}, "set.element");
        return dictValueFromPayload(payload, elementType);
    }
    if (method == "clear") {
        cg.builder.CreateCall(runtimeFunction("csec_dict_clear", llvm::Type::getVoidTy(cg.context), {ptrType()}), {handle});
        return llvm::ConstantInt::get(i32Type(), 0);
    }
    if (method == "copy") {
        return cg.builder.CreateCall(runtimeFunction("csec_dict_copy", ptrType(), {ptrType()}), {handle}, "set.copy");
    }
    if (method == "toString") return codegenDictToString(handle);
    return nullptr;
}
