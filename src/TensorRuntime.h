#pragma once

#include "codegen.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

namespace TensorRuntime {

inline bool isTensorTypeName(const std::string& name) {
    return name == "Tensor" || name.rfind("Tensor$", 0) == 0;
}

inline bool isIntegerLiteralName(const std::string& name) {
    if (name.empty()) return false;
    size_t start = name[0] == '-' ? 1 : 0;
    if (start == name.size()) return false;
    for (size_t i = start; i < name.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return false;
    }
    return true;
}

inline llvm::StructType* getTensorStructType(CodeGenerator& cg) {
    if (auto* existing = llvm::StructType::getTypeByName(cg.context, "csec.tensor")) {
        return existing;
    }

    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* tensorTy = llvm::StructType::create(cg.context, "csec.tensor");
    tensorTy->setBody({
        i64Ty,
        llvm::PointerType::getUnqual(i64Ty->getContext()),
        i64Ty,
        llvm::PointerType::getUnqual(f64Ty->getContext())
    });
    return tensorTy;
}

inline llvm::PointerType* getTensorPointerType(CodeGenerator& cg) {
    return llvm::PointerType::getUnqual((getTensorStructType(cg))->getContext());
}

inline llvm::ConstantInt* i64(CodeGenerator& cg, int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(cg.context), value, true);
}

inline llvm::Value* mallocBytes(CodeGenerator& cg, llvm::Value* byteCount, llvm::Type* resultType, const char* name) {
    llvm::Value* raw = cg.builder.CreateCall(cg.mallocFunction, byteCount, "tensor.malloc");
    return cg.builder.CreateBitCast(raw, resultType, name);
}

inline llvm::Value* tensorField(CodeGenerator& cg, llvm::Value* tensor, unsigned index, const char* name) {
    return cg.builder.CreateStructGEP(getTensorStructType(cg), tensor, index, name);
}

inline llvm::Value* loadRank(CodeGenerator& cg, llvm::Value* tensor) {
    return cg.builder.CreateLoad(llvm::Type::getInt64Ty(cg.context), tensorField(cg, tensor, 0, "tensor.rank.ptr"), "tensor.rank");
}

inline llvm::Value* loadDims(CodeGenerator& cg, llvm::Value* tensor) {
    auto* i64PtrTy = llvm::PointerType::getUnqual(cg.context);
    return cg.builder.CreateLoad(i64PtrTy, tensorField(cg, tensor, 1, "tensor.dims.ptr"), "tensor.dims");
}

inline llvm::Value* loadCount(CodeGenerator& cg, llvm::Value* tensor) {
    return cg.builder.CreateLoad(llvm::Type::getInt64Ty(cg.context), tensorField(cg, tensor, 2, "tensor.count.ptr"), "tensor.count");
}

inline llvm::Value* loadData(CodeGenerator& cg, llvm::Value* tensor) {
    auto* f64PtrTy = llvm::PointerType::getUnqual(cg.context);
    return cg.builder.CreateLoad(f64PtrTy, tensorField(cg, tensor, 3, "tensor.data.ptr"), "tensor.data");
}

inline llvm::Value* toDouble(CodeGenerator& cg, llvm::Value* value) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    if (!value) return llvm::ConstantFP::get(f64Ty, 0.0);
    if (value->getType() == f64Ty) return value;
    if (value->getType()->isFloatTy()) return cg.builder.CreateFPExt(value, f64Ty, "tensor.fpext");
    if (value->getType()->isIntegerTy()) return cg.builder.CreateSIToFP(value, f64Ty, "tensor.sitofp");
    return llvm::ConstantFP::get(f64Ty, 0.0);
}

template <typename Body>
inline void emitCountedLoop(CodeGenerator& cg, llvm::Value* count, const std::string& name, Body body) {
    auto* function = cg.builder.GetInsertBlock()->getParent();
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* indexPtr = cg.builder.CreateAlloca(i64Ty, nullptr, name + ".i");
    cg.builder.CreateStore(i64(cg, 0), indexPtr);

    auto* condBB = llvm::BasicBlock::Create(cg.context, name + ".cond", function);
    auto* bodyBB = llvm::BasicBlock::Create(cg.context, name + ".body", function);
    auto* endBB = llvm::BasicBlock::Create(cg.context, name + ".end", function);

    cg.builder.CreateBr(condBB);
    cg.builder.SetInsertPoint(condBB);
    llvm::Value* index = cg.builder.CreateLoad(i64Ty, indexPtr, name + ".idx");
    llvm::Value* keepGoing = cg.builder.CreateICmpULT(index, count, name + ".keep");
    cg.builder.CreateCondBr(keepGoing, bodyBB, endBB);

    cg.builder.SetInsertPoint(bodyBB);
    body(index);
    if (!csec_llvm::terminatorOrNull(cg.builder.GetInsertBlock())) {
        llvm::Value* current = cg.builder.CreateLoad(i64Ty, indexPtr, name + ".current");
        llvm::Value* next = cg.builder.CreateAdd(current, i64(cg, 1), name + ".next");
        cg.builder.CreateStore(next, indexPtr);
        cg.builder.CreateBr(condBB);
    }

    cg.builder.SetInsertPoint(endBB);
}

inline llvm::Value* allocateTensor(CodeGenerator& cg, llvm::Value* rank, llvm::Value* count) {
    auto* tensorPtrTy = getTensorPointerType(cg);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);

    llvm::Value* tensor = mallocBytes(cg, i64(cg, 32), tensorPtrTy, "tensor");
    llvm::Value* dims = mallocBytes(
        cg,
        cg.builder.CreateMul(rank, i64(cg, 8), "tensor.dims.bytes"),
        llvm::PointerType::getUnqual(i64Ty->getContext()),
        "tensor.dims.alloc");
    llvm::Value* data = mallocBytes(
        cg,
        cg.builder.CreateMul(count, i64(cg, 8), "tensor.data.bytes"),
        llvm::PointerType::getUnqual(f64Ty->getContext()),
        "tensor.data.alloc");

    cg.builder.CreateStore(rank, tensorField(cg, tensor, 0, "tensor.rank.slot"));
    cg.builder.CreateStore(dims, tensorField(cg, tensor, 1, "tensor.dims.slot"));
    cg.builder.CreateStore(count, tensorField(cg, tensor, 2, "tensor.count.slot"));
    cg.builder.CreateStore(data, tensorField(cg, tensor, 3, "tensor.data.slot"));
    cg.markFreshTensor(tensor);
    return tensor;
}

inline std::vector<int64_t> dimensionsFromTemplateArgs(const std::vector<std::unique_ptr<Type>>& templateArgs) {
    std::vector<int64_t> dims;
    for (size_t i = 1; i < templateArgs.size(); ++i) {
        if (!templateArgs[i] || !isIntegerLiteralName(templateArgs[i]->getName())) continue;
        int64_t dim = std::stoll(templateArgs[i]->getName());
        if (dim > 0) dims.push_back(dim);
    }
    return dims;
}

inline llvm::Value* createTensor(CodeGenerator& cg, const std::vector<int64_t>& dims, llvm::Value* fillValue) {
    int64_t count = 1;
    for (int64_t dim : dims) count *= dim;

    llvm::Value* tensor = allocateTensor(cg, i64(cg, static_cast<int64_t>(dims.size())), i64(cg, count));
    llvm::Value* dimsPtr = loadDims(cg, tensor);
    llvm::Value* dataPtr = loadData(cg, tensor);

    for (size_t i = 0; i < dims.size(); ++i) {
        llvm::Value* slot = cg.builder.CreateGEP(llvm::Type::getInt64Ty(cg.context), dimsPtr, i64(cg, static_cast<int64_t>(i)), "tensor.dim.slot");
        cg.builder.CreateStore(i64(cg, dims[i]), slot);
    }

    llvm::Value* fill = toDouble(cg, fillValue);
    emitCountedLoop(cg, i64(cg, count), "tensor.fill", [&](llvm::Value* index) {
        llvm::Value* slot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), dataPtr, index, "tensor.data.slot");
        cg.builder.CreateStore(fill, slot);
    });
    return tensor;
}

inline llvm::Value* cloneTensor(CodeGenerator& cg, llvm::Value* source) {
    llvm::Value* rank = loadRank(cg, source);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* srcDims = loadDims(cg, source);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* srcData = loadData(cg, source);
    llvm::Value* dstData = loadData(cg, result);

    emitCountedLoop(cg, rank, "tensor.clone.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(llvm::Type::getInt64Ty(cg.context), srcDims, index, "tensor.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(llvm::Type::getInt64Ty(cg.context), dstDims, index, "tensor.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(llvm::Type::getInt64Ty(cg.context), srcSlot, "tensor.dim"), dstSlot);
    });

    emitCountedLoop(cg, count, "tensor.clone.data", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), srcData, index, "tensor.src.val");
        llvm::Value* dstSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), dstData, index, "tensor.dst.val");
        cg.builder.CreateStore(cg.builder.CreateLoad(llvm::Type::getDoubleTy(cg.context), srcSlot, "tensor.val"), dstSlot);
    });
    return result;
}

inline llvm::Value* applyElementwiseOp(CodeGenerator& cg, const std::string& op, llvm::Value* left, llvm::Value* right) {
    if (op == "+") return cg.builder.CreateFAdd(left, right, "tensor.add");
    if (op == "-") return cg.builder.CreateFSub(left, right, "tensor.sub");
    if (op == "*") return cg.builder.CreateFMul(left, right, "tensor.mul");
    if (op == "/") return cg.builder.CreateFDiv(left, right, "tensor.div");
    return left;
}

inline llvm::Function* getOrCreateUnaryDoubleRuntime(CodeGenerator& cg, const std::string& name) {
    if (auto* function = cg.module->getFunction(name)) return function;
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    auto* functionTy = llvm::FunctionType::get(f64Ty, {f64Ty}, false);
    return llvm::Function::Create(functionTy, llvm::Function::ExternalLinkage, name, cg.module.get());
}

inline llvm::Value* mapTensor(CodeGenerator& cg, llvm::Value* source, const std::string& op) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* rank = loadRank(cg, source);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* srcDims = loadDims(cg, source);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* srcData = loadData(cg, source);
    llvm::Value* dstData = loadData(cg, result);

    emitCountedLoop(cg, rank, "tensor.map.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, srcDims, index, "tensor.map.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.map.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.map.dim"), dstSlot);
    });

    emitCountedLoop(cg, count, "tensor.map.data", [&](llvm::Value* index) {
        llvm::Value* value = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, srcData, index, "tensor.map.src"), "tensor.map.value");
        llvm::Value* mapped = value;
        if (op == "relu") {
            mapped = cg.builder.CreateSelect(
                cg.builder.CreateFCmpOGT(value, llvm::ConstantFP::get(f64Ty, 0.0), "tensor.relu.gt0"),
                value,
                llvm::ConstantFP::get(f64Ty, 0.0),
                "tensor.relu");
        }
        cg.builder.CreateStore(mapped, cg.builder.CreateGEP(f64Ty, dstData, index, "tensor.map.dst"));
    });
    return result;
}

inline llvm::Value* sumTensor(CodeGenerator& cg, llvm::Value* source) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* data = loadData(cg, source);
    llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.sum.ptr");
    cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);
    emitCountedLoop(cg, count, "tensor.sum", [&](llvm::Value* index) {
        llvm::Value* value = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, data, index, "tensor.sum.slot"), "tensor.sum.value");
        llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.sum.current");
        cg.builder.CreateStore(cg.builder.CreateFAdd(current, value, "tensor.sum.next"), sumPtr);
    });
    return cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.sum.result");
}

inline llvm::Value* meanTensor(CodeGenerator& cg, llvm::Value* source) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* countDouble = cg.builder.CreateUIToFP(count, f64Ty, "tensor.mean.count");
    return cg.builder.CreateFDiv(sumTensor(cg, source), countDouble, "tensor.mean");
}

inline llvm::Value* normTensor(CodeGenerator& cg, llvm::Value* source) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* data = loadData(cg, source);
    llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.norm.sum.ptr");
    cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);
    emitCountedLoop(cg, count, "tensor.norm", [&](llvm::Value* index) {
        llvm::Value* value = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, data, index, "tensor.norm.slot"), "tensor.norm.value");
        llvm::Value* square = cg.builder.CreateFMul(value, value, "tensor.norm.square");
        llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.norm.current");
        cg.builder.CreateStore(cg.builder.CreateFAdd(current, square, "tensor.norm.next"), sumPtr);
    });
    return cg.builder.CreateCall(
        getOrCreateUnaryDoubleRuntime(cg, "csec_math_sqrt"),
        {cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.norm.sum")},
        "tensor.norm.result");
}

inline llvm::Value* softmaxTensor(CodeGenerator& cg, llvm::Value* source) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* rank = loadRank(cg, source);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* srcDims = loadDims(cg, source);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* srcData = loadData(cg, source);
    llvm::Value* dstData = loadData(cg, result);

    emitCountedLoop(cg, rank, "tensor.softmax.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, srcDims, index, "tensor.softmax.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.softmax.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.softmax.dim"), dstSlot);
    });

    llvm::Function* expFn = getOrCreateUnaryDoubleRuntime(cg, "csec_math_exp");
    llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.softmax.sum.ptr");
    cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);

    emitCountedLoop(cg, count, "tensor.softmax.exp", [&](llvm::Value* index) {
        llvm::Value* value = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, srcData, index, "tensor.softmax.src"), "tensor.softmax.value");
        llvm::Value* expValue = cg.builder.CreateCall(expFn, {value}, "tensor.softmax.exp.value");
        cg.builder.CreateStore(expValue, cg.builder.CreateGEP(f64Ty, dstData, index, "tensor.softmax.exp.dst"));
        llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.softmax.sum.current");
        cg.builder.CreateStore(cg.builder.CreateFAdd(current, expValue, "tensor.softmax.sum.next"), sumPtr);
    });

    emitCountedLoop(cg, count, "tensor.softmax.norm", [&](llvm::Value* index) {
        llvm::Value* slot = cg.builder.CreateGEP(f64Ty, dstData, index, "tensor.softmax.norm.slot");
        llvm::Value* expValue = cg.builder.CreateLoad(f64Ty, slot, "tensor.softmax.exp.load");
        llvm::Value* sum = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.softmax.sum");
        cg.builder.CreateStore(cg.builder.CreateFDiv(expValue, sum, "tensor.softmax.prob"), slot);
    });

    return result;
}

inline llvm::Value* mseTensor(CodeGenerator& cg, llvm::Value* prediction, llvm::Value* target) {
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* predCount = loadCount(cg, prediction);
    llvm::Value* targetCount = loadCount(cg, target);
    llvm::Value* count = cg.builder.CreateSelect(cg.builder.CreateICmpULT(predCount, targetCount), predCount, targetCount, "tensor.mse.count");
    llvm::Value* predData = loadData(cg, prediction);
    llvm::Value* targetData = loadData(cg, target);
    llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.mse.sum.ptr");
    cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);

    emitCountedLoop(cg, count, "tensor.mse", [&](llvm::Value* index) {
        llvm::Value* predValue = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, predData, index, "tensor.mse.pred"), "tensor.mse.pred.value");
        llvm::Value* targetValue = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, targetData, index, "tensor.mse.target"), "tensor.mse.target.value");
        llvm::Value* diff = cg.builder.CreateFSub(predValue, targetValue, "tensor.mse.diff");
        llvm::Value* square = cg.builder.CreateFMul(diff, diff, "tensor.mse.square");
        llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.mse.current");
        cg.builder.CreateStore(cg.builder.CreateFAdd(current, square, "tensor.mse.next"), sumPtr);
    });

    llvm::Value* countDouble = cg.builder.CreateUIToFP(count, f64Ty, "tensor.mse.count.fp");
    return cg.builder.CreateFDiv(cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.mse.sum"), countDouble, "tensor.mse.result");
}

inline llvm::Value* elementwiseTensorTensor(CodeGenerator& cg, llvm::Value* left, llvm::Value* right, const std::string& op) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* rank = loadRank(cg, left);
    llvm::Value* count = loadCount(cg, left);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* leftDims = loadDims(cg, left);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* leftData = loadData(cg, left);
    llvm::Value* rightData = loadData(cg, right);
    llvm::Value* dstData = loadData(cg, result);

    emitCountedLoop(cg, rank, "tensor.elem.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, leftDims, index, "tensor.elem.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.elem.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.elem.dim"), dstSlot);
    });

    emitCountedLoop(cg, count, "tensor.elem.data", [&](llvm::Value* index) {
        llvm::Value* leftValue = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, leftData, index, "tensor.elem.left"), "tensor.elem.l");
        llvm::Value* rightValue = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, rightData, index, "tensor.elem.right"), "tensor.elem.r");
        cg.builder.CreateStore(
            applyElementwiseOp(cg, op, leftValue, rightValue),
            cg.builder.CreateGEP(f64Ty, dstData, index, "tensor.elem.dst"));
    });
    return result;
}

inline llvm::Value* elementwiseTensorScalar(CodeGenerator& cg, llvm::Value* tensor, llvm::Value* scalar, const std::string& op, bool tensorOnLeft) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* rank = loadRank(cg, tensor);
    llvm::Value* count = loadCount(cg, tensor);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* srcDims = loadDims(cg, tensor);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* srcData = loadData(cg, tensor);
    llvm::Value* dstData = loadData(cg, result);
    llvm::Value* scalarValue = toDouble(cg, scalar);

    emitCountedLoop(cg, rank, "tensor.scalar.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, srcDims, index, "tensor.scalar.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.scalar.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.scalar.dim"), dstSlot);
    });

    emitCountedLoop(cg, count, "tensor.scalar.data", [&](llvm::Value* index) {
        llvm::Value* tensorValue = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, srcData, index, "tensor.scalar.src"), "tensor.scalar.tensor");
        llvm::Value* leftValue = tensorOnLeft ? tensorValue : scalarValue;
        llvm::Value* rightValue = tensorOnLeft ? scalarValue : tensorValue;
        cg.builder.CreateStore(
            applyElementwiseOp(cg, op, leftValue, rightValue),
            cg.builder.CreateGEP(f64Ty, dstData, index, "tensor.scalar.dst"));
    });
    return result;
}

inline llvm::Value* tensorLinearIndex(CodeGenerator& cg, llvm::Value* coords, llvm::Value* dims, llvm::Value* rank) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* resultPtr = cg.builder.CreateAlloca(i64Ty, nullptr, "tensor.linear.ptr");
    cg.builder.CreateStore(i64(cg, 0), resultPtr);
    emitCountedLoop(cg, rank, "tensor.linear", [&](llvm::Value* axis) {
        llvm::Value* dim = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, dims, axis, "tensor.linear.dim.ptr"), "tensor.linear.dim");
        llvm::Value* coord = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, coords, axis, "tensor.linear.coord.ptr"), "tensor.linear.coord");
        llvm::Value* current = cg.builder.CreateLoad(i64Ty, resultPtr, "tensor.linear.current");
        llvm::Value* next = cg.builder.CreateAdd(cg.builder.CreateMul(current, dim, "tensor.linear.mul"), coord, "tensor.linear.next");
        cg.builder.CreateStore(next, resultPtr);
    });
    return cg.builder.CreateLoad(i64Ty, resultPtr, "tensor.linear.result");
}

// Address of a single tensor element t[i0, i1, ...]. Mirrors the full-scalar-index read in
// sliceTensor (same row-major linear index into the data buffer) but returns the element pointer,
// so `t[i, j] = v` can store through it. `indices` must give one scalar per axis (no slices).
inline llvm::Value* elementPointer(CodeGenerator& cg, llvm::Value* tensor,
                                   const std::vector<llvm::Value*>& indices) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* rank = loadRank(cg, tensor);
    llvm::Value* dims = loadDims(cg, tensor);
    llvm::Value* data = loadData(cg, tensor);
    llvm::Value* coords = cg.builder.CreateAlloca(i64Ty, rank, "tensor.set.coords");
    for (size_t axis = 0; axis < indices.size(); ++axis) {
        llvm::Value* coord = indices[axis] ? indices[axis] : i64(cg, 0);
        cg.builder.CreateStore(coord,
            cg.builder.CreateGEP(i64Ty, coords, i64(cg, static_cast<int64_t>(axis)), "tensor.set.coord.slot"));
    }
    llvm::Value* index = tensorLinearIndex(cg, coords, dims, rank);
    return cg.builder.CreateGEP(f64Ty, data, index, "tensor.set.addr");
}

inline llvm::Value* sliceTensor(
    CodeGenerator& cg,
    llvm::Value* source,
    const std::vector<bool>& isSlice,
    const std::vector<llvm::Value*>& starts,
    const std::vector<llvm::Value*>& ends,
    const std::vector<llvm::Value*>& steps) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* srcRank = loadRank(cg, source);
    llvm::Value* srcDims = loadDims(cg, source);
    llvm::Value* srcData = loadData(cg, source);
    size_t axisCount = isSlice.size();
    int64_t sliceRankConst = 0;
    for (bool axisIsSlice : isSlice) {
        if (axisIsSlice) ++sliceRankConst;
    }

    if (sliceRankConst == 0) {
        llvm::Value* coords = cg.builder.CreateAlloca(i64Ty, srcRank, "tensor.index.coords");
        for (size_t axis = 0; axis < axisCount; ++axis) {
            llvm::Value* coord = starts[axis] ? starts[axis] : i64(cg, 0);
            cg.builder.CreateStore(coord, cg.builder.CreateGEP(i64Ty, coords, i64(cg, static_cast<int64_t>(axis)), "tensor.index.coord.slot"));
        }
        llvm::Value* srcIndex = tensorLinearIndex(cg, coords, srcDims, srcRank);
        return cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, srcData, srcIndex, "tensor.index.data"), "tensor.index.value");
    }

    llvm::Value* result = allocateTensor(cg, i64(cg, sliceRankConst), i64(cg, 1));
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* countPtr = cg.builder.CreateAlloca(i64Ty, nullptr, "tensor.slice.count.ptr");
    cg.builder.CreateStore(i64(cg, 1), countPtr);
    int64_t dstAxis = 0;
    for (size_t axis = 0; axis < axisCount; ++axis) {
        if (!isSlice[axis]) continue;
        llvm::Value* dim = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, srcDims, i64(cg, static_cast<int64_t>(axis)), "tensor.slice.dim.ptr"), "tensor.slice.dim");
        llvm::Value* start = starts[axis] ? starts[axis] : i64(cg, 0);
        llvm::Value* end = ends[axis] ? ends[axis] : dim;
        llvm::Value* step = steps[axis] ? steps[axis] : i64(cg, 1);
        llvm::Value* span = cg.builder.CreateSub(end, start, "tensor.slice.span");
        llvm::Value* length = cg.builder.CreateUDiv(cg.builder.CreateAdd(span, cg.builder.CreateSub(step, i64(cg, 1))), step, "tensor.slice.len");
        cg.builder.CreateStore(length, cg.builder.CreateGEP(i64Ty, dstDims, i64(cg, dstAxis), "tensor.slice.dst.dim"));
        llvm::Value* currentCount = cg.builder.CreateLoad(i64Ty, countPtr, "tensor.slice.count");
        cg.builder.CreateStore(cg.builder.CreateMul(currentCount, length, "tensor.slice.next.count"), countPtr);
        ++dstAxis;
    }

    llvm::Value* count = cg.builder.CreateLoad(i64Ty, countPtr, "tensor.slice.total");
    result = allocateTensor(cg, i64(cg, sliceRankConst), count);
    dstDims = loadDims(cg, result);
    llvm::Value* dstData = loadData(cg, result);
    dstAxis = 0;
    for (size_t axis = 0; axis < axisCount; ++axis) {
        if (!isSlice[axis]) continue;
        llvm::Value* dim = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, srcDims, i64(cg, static_cast<int64_t>(axis))), "tensor.slice.dim");
        llvm::Value* start = starts[axis] ? starts[axis] : i64(cg, 0);
        llvm::Value* end = ends[axis] ? ends[axis] : dim;
        llvm::Value* step = steps[axis] ? steps[axis] : i64(cg, 1);
        llvm::Value* span = cg.builder.CreateSub(end, start, "tensor.slice.span");
        llvm::Value* length = cg.builder.CreateUDiv(cg.builder.CreateAdd(span, cg.builder.CreateSub(step, i64(cg, 1))), step, "tensor.slice.len");
        cg.builder.CreateStore(length, cg.builder.CreateGEP(i64Ty, dstDims, i64(cg, dstAxis), "tensor.slice.dst.dim"));
        ++dstAxis;
    }

    llvm::Value* srcCoords = cg.builder.CreateAlloca(i64Ty, srcRank, "tensor.slice.src.coords");
    emitCountedLoop(cg, count, "tensor.slice.copy", [&](llvm::Value* dstIndex) {
        llvm::Value* remainingPtr = cg.builder.CreateAlloca(i64Ty, nullptr, "tensor.slice.remaining.ptr");
        cg.builder.CreateStore(dstIndex, remainingPtr);
        int64_t outAxis = sliceRankConst - 1;
        for (int64_t axis = static_cast<int64_t>(axisCount) - 1; axis >= 0; --axis) {
            llvm::Value* coord = starts[axis] ? starts[axis] : i64(cg, 0);
            if (isSlice[axis]) {
                llvm::Value* len = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, dstDims, i64(cg, outAxis), "tensor.slice.len.ptr"), "tensor.slice.len");
                llvm::Value* remaining = cg.builder.CreateLoad(i64Ty, remainingPtr, "tensor.slice.remaining");
                llvm::Value* local = cg.builder.CreateURem(remaining, len, "tensor.slice.local");
                cg.builder.CreateStore(cg.builder.CreateUDiv(remaining, len, "tensor.slice.next.remaining"), remainingPtr);
                llvm::Value* step = steps[axis] ? steps[axis] : i64(cg, 1);
                coord = cg.builder.CreateAdd(coord, cg.builder.CreateMul(local, step, "tensor.slice.offset"), "tensor.slice.src.coord");
                --outAxis;
            }
            cg.builder.CreateStore(coord, cg.builder.CreateGEP(i64Ty, srcCoords, i64(cg, axis), "tensor.slice.src.coord.slot"));
        }
        llvm::Value* srcIndex = tensorLinearIndex(cg, srcCoords, srcDims, srcRank);
        llvm::Value* value = cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, srcData, srcIndex, "tensor.slice.src"), "tensor.slice.value");
        cg.builder.CreateStore(value, cg.builder.CreateGEP(f64Ty, dstData, dstIndex, "tensor.slice.dst"));
    });
    return result;
}

inline llvm::Value* transposeTensor(CodeGenerator& cg, llvm::Value* source) {
    llvm::Value* rank = loadRank(cg, source);
    llvm::Value* count = loadCount(cg, source);
    llvm::Value* result = allocateTensor(cg, rank, count);
    llvm::Value* srcDims = loadDims(cg, source);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* srcData = loadData(cg, source);
    llvm::Value* dstData = loadData(cg, result);

    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    llvm::Value* one = i64(cg, 1);
    llvm::Value* two = i64(cg, 2);
    llvm::Value* last = cg.builder.CreateSub(rank, one, "tensor.last.dim");
    llvm::Value* prev = cg.builder.CreateSub(rank, two, "tensor.prev.dim");

    emitCountedLoop(cg, rank, "tensor.transpose.dims", [&](llvm::Value* index) {
        llvm::Value* useLast = cg.builder.CreateICmpEQ(index, prev, "tensor.use.last");
        llvm::Value* usePrev = cg.builder.CreateICmpEQ(index, last, "tensor.use.prev");
        llvm::Value* srcIndex = cg.builder.CreateSelect(useLast, last, cg.builder.CreateSelect(usePrev, prev, index), "tensor.transpose.dim.index");
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, srcDims, srcIndex, "tensor.src.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.dim"), dstSlot);
    });

    llvm::Value* rowsSlot = cg.builder.CreateGEP(i64Ty, srcDims, prev, "tensor.rows.slot");
    llvm::Value* colsSlot = cg.builder.CreateGEP(i64Ty, srcDims, last, "tensor.cols.slot");
    llvm::Value* rows = cg.builder.CreateLoad(i64Ty, rowsSlot, "tensor.rows");
    llvm::Value* cols = cg.builder.CreateLoad(i64Ty, colsSlot, "tensor.cols");
    llvm::Value* rankIsAtLeastTwo = cg.builder.CreateICmpUGE(rank, two, "tensor.rank.ge2");

    auto* function = cg.builder.GetInsertBlock()->getParent();
    auto* transposeBB = llvm::BasicBlock::Create(cg.context, "tensor.transpose.data", function);
    auto* copyBB = llvm::BasicBlock::Create(cg.context, "tensor.transpose.copy", function);
    auto* endBB = llvm::BasicBlock::Create(cg.context, "tensor.transpose.end", function);
    cg.builder.CreateCondBr(rankIsAtLeastTwo, transposeBB, copyBB);

    cg.builder.SetInsertPoint(transposeBB);
    emitCountedLoop(cg, rows, "tensor.transpose.rows", [&](llvm::Value* row) {
        emitCountedLoop(cg, cols, "tensor.transpose.cols", [&](llvm::Value* col) {
            llvm::Value* srcIndex = cg.builder.CreateAdd(cg.builder.CreateMul(row, cols), col, "tensor.transpose.src.index");
            llvm::Value* dstIndex = cg.builder.CreateAdd(cg.builder.CreateMul(col, rows), row, "tensor.transpose.dst.index");
            llvm::Value* srcSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), srcData, srcIndex, "tensor.transpose.src");
            llvm::Value* dstSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), dstData, dstIndex, "tensor.transpose.dst");
            cg.builder.CreateStore(cg.builder.CreateLoad(llvm::Type::getDoubleTy(cg.context), srcSlot, "tensor.transpose.val"), dstSlot);
        });
    });
    cg.builder.CreateBr(endBB);

    cg.builder.SetInsertPoint(copyBB);
    emitCountedLoop(cg, count, "tensor.transpose.copy.data", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), srcData, index, "tensor.src.val");
        llvm::Value* dstSlot = cg.builder.CreateGEP(llvm::Type::getDoubleTy(cg.context), dstData, index, "tensor.dst.val");
        cg.builder.CreateStore(cg.builder.CreateLoad(llvm::Type::getDoubleTy(cg.context), srcSlot, "tensor.val"), dstSlot);
    });
    cg.builder.CreateBr(endBB);

    cg.builder.SetInsertPoint(endBB);
    return result;
}

inline llvm::Value* innerProduct(CodeGenerator& cg, llvm::Value* left, llvm::Value* right) {
    llvm::Value* leftCount = loadCount(cg, left);
    llvm::Value* rightCount = loadCount(cg, right);
    llvm::Value* count = cg.builder.CreateSelect(cg.builder.CreateICmpULT(leftCount, rightCount), leftCount, rightCount, "tensor.inner.count");
    llvm::Value* leftData = loadData(cg, left);
    llvm::Value* rightData = loadData(cg, right);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.inner.sum.ptr");
    cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);

    emitCountedLoop(cg, count, "tensor.inner", [&](llvm::Value* index) {
        llvm::Value* leftSlot = cg.builder.CreateGEP(f64Ty, leftData, index, "tensor.inner.left");
        llvm::Value* rightSlot = cg.builder.CreateGEP(f64Ty, rightData, index, "tensor.inner.right");
        llvm::Value* product = cg.builder.CreateFMul(
            cg.builder.CreateLoad(f64Ty, leftSlot, "tensor.inner.l"),
            cg.builder.CreateLoad(f64Ty, rightSlot, "tensor.inner.r"),
            "tensor.inner.product");
        llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.inner.current");
        cg.builder.CreateStore(cg.builder.CreateFAdd(current, product, "tensor.inner.next"), sumPtr);
    });
    return cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.inner.result");
}

inline llvm::Value* outerProduct(CodeGenerator& cg, llvm::Value* left, llvm::Value* right) {
    llvm::Value* leftRank = loadRank(cg, left);
    llvm::Value* rightRank = loadRank(cg, right);
    llvm::Value* leftCount = loadCount(cg, left);
    llvm::Value* rightCount = loadCount(cg, right);
    llvm::Value* rank = cg.builder.CreateAdd(leftRank, rightRank, "tensor.outer.rank");
    llvm::Value* count = cg.builder.CreateMul(leftCount, rightCount, "tensor.outer.count");
    llvm::Value* result = allocateTensor(cg, rank, count);

    llvm::Value* leftDims = loadDims(cg, left);
    llvm::Value* rightDims = loadDims(cg, right);
    llvm::Value* dstDims = loadDims(cg, result);
    llvm::Value* leftData = loadData(cg, left);
    llvm::Value* rightData = loadData(cg, right);
    llvm::Value* dstData = loadData(cg, result);
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);

    emitCountedLoop(cg, leftRank, "tensor.outer.left.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, leftDims, index, "tensor.outer.left.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, index, "tensor.outer.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.outer.dim"), dstSlot);
    });
    emitCountedLoop(cg, rightRank, "tensor.outer.right.dims", [&](llvm::Value* index) {
        llvm::Value* srcSlot = cg.builder.CreateGEP(i64Ty, rightDims, index, "tensor.outer.right.dim");
        llvm::Value* dstSlot = cg.builder.CreateGEP(i64Ty, dstDims, cg.builder.CreateAdd(leftRank, index), "tensor.outer.dst.dim");
        cg.builder.CreateStore(cg.builder.CreateLoad(i64Ty, srcSlot, "tensor.outer.dim"), dstSlot);
    });

    emitCountedLoop(cg, leftCount, "tensor.outer.left", [&](llvm::Value* leftIndex) {
        llvm::Value* leftSlot = cg.builder.CreateGEP(f64Ty, leftData, leftIndex, "tensor.outer.left.val");
        llvm::Value* leftValue = cg.builder.CreateLoad(f64Ty, leftSlot, "tensor.outer.l");
        emitCountedLoop(cg, rightCount, "tensor.outer.right", [&](llvm::Value* rightIndex) {
            llvm::Value* rightSlot = cg.builder.CreateGEP(f64Ty, rightData, rightIndex, "tensor.outer.right.val");
            llvm::Value* dstIndex = cg.builder.CreateAdd(cg.builder.CreateMul(leftIndex, rightCount), rightIndex, "tensor.outer.dst.index");
            llvm::Value* dstSlot = cg.builder.CreateGEP(f64Ty, dstData, dstIndex, "tensor.outer.dst.val");
            llvm::Value* product = cg.builder.CreateFMul(leftValue, cg.builder.CreateLoad(f64Ty, rightSlot, "tensor.outer.r"), "tensor.outer.product");
            cg.builder.CreateStore(product, dstSlot);
        });
    });
    return result;
}

inline llvm::Value* matrixProduct(CodeGenerator& cg, llvm::Value* left, llvm::Value* right) {
    auto* i64Ty = llvm::Type::getInt64Ty(cg.context);
    auto* f64Ty = llvm::Type::getDoubleTy(cg.context);
    llvm::Value* leftDims = loadDims(cg, left);
    llvm::Value* rightDims = loadDims(cg, right);
    llvm::Value* rows = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, leftDims, i64(cg, 0), "tensor.matmul.rows.ptr"), "tensor.matmul.rows");
    llvm::Value* shared = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, leftDims, i64(cg, 1), "tensor.matmul.shared.ptr"), "tensor.matmul.shared");
    llvm::Value* cols = cg.builder.CreateLoad(i64Ty, cg.builder.CreateGEP(i64Ty, rightDims, i64(cg, 1), "tensor.matmul.cols.ptr"), "tensor.matmul.cols");
    llvm::Value* count = cg.builder.CreateMul(rows, cols, "tensor.matmul.count");
    llvm::Value* result = allocateTensor(cg, i64(cg, 2), count);
    llvm::Value* dstDims = loadDims(cg, result);
    cg.builder.CreateStore(rows, cg.builder.CreateGEP(i64Ty, dstDims, i64(cg, 0), "tensor.matmul.dst.rows"));
    cg.builder.CreateStore(cols, cg.builder.CreateGEP(i64Ty, dstDims, i64(cg, 1), "tensor.matmul.dst.cols"));

    llvm::Value* leftData = loadData(cg, left);
    llvm::Value* rightData = loadData(cg, right);
    llvm::Value* dstData = loadData(cg, result);
    emitCountedLoop(cg, rows, "tensor.matmul.i", [&](llvm::Value* row) {
        emitCountedLoop(cg, cols, "tensor.matmul.j", [&](llvm::Value* col) {
            llvm::Value* sumPtr = cg.builder.CreateAlloca(f64Ty, nullptr, "tensor.matmul.sum.ptr");
            cg.builder.CreateStore(llvm::ConstantFP::get(f64Ty, 0.0), sumPtr);
            emitCountedLoop(cg, shared, "tensor.matmul.k", [&](llvm::Value* k) {
                llvm::Value* leftIndex = cg.builder.CreateAdd(cg.builder.CreateMul(row, shared), k, "tensor.matmul.left.index");
                llvm::Value* rightIndex = cg.builder.CreateAdd(cg.builder.CreateMul(k, cols), col, "tensor.matmul.right.index");
                llvm::Value* product = cg.builder.CreateFMul(
                    cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, leftData, leftIndex), "tensor.matmul.left"),
                    cg.builder.CreateLoad(f64Ty, cg.builder.CreateGEP(f64Ty, rightData, rightIndex), "tensor.matmul.right"),
                    "tensor.matmul.product");
                llvm::Value* current = cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.matmul.current");
                cg.builder.CreateStore(cg.builder.CreateFAdd(current, product, "tensor.matmul.next"), sumPtr);
            });
            llvm::Value* dstIndex = cg.builder.CreateAdd(cg.builder.CreateMul(row, cols), col, "tensor.matmul.dst.index");
            cg.builder.CreateStore(
                cg.builder.CreateLoad(f64Ty, sumPtr, "tensor.matmul.sum"),
                cg.builder.CreateGEP(f64Ty, dstData, dstIndex, "tensor.matmul.dst"));
        });
    });
    return result;
}

} // namespace TensorRuntime
