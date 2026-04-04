#pragma once
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

namespace tinyjs {

// max trip count eligible for unrolling
static constexpr unsigned UnrollThreshold = 8;

class MyOptimizerPass : public llvm::PassInfoMixin<MyOptimizerPass> {
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

void registerOptimizer(llvm::PassBuilder &PB);

} // namespace tinyjs
