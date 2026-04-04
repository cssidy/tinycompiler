#include "Optimizer.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"

using namespace llvm;
using namespace tinyjs;

// collect all innermost loops in preorder
static void collectInnermostLoops(Loop *L, SmallVectorImpl<Loop *> &Out) {
    if (L->isInnermost()) {
        Out.push_back(L);
        return;
    }
    for (Loop *Sub : *L)
        collectInnermostLoops(Sub, Out);
}

PreservedAnalyses MyOptimizerPass::run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI  = AM.getResult<LoopAnalysis>(F);
    auto &SE  = AM.getResult<ScalarEvolutionAnalysis>(F);
    auto &DT  = AM.getResult<DominatorTreeAnalysis>(F);
    auto &AC  = AM.getResult<AssumptionAnalysis>(F);
    auto &ORE = AM.getResult<OptimizationRemarkEmitterAnalysis>(F);

    SmallVector<Loop *, 8> Loops;
    for (Loop *TopL : LI)
        collectInnermostLoops(TopL, Loops);

    for (Loop *L : Loops) {
        unsigned TripCount = SE.getSmallConstantTripCount(L);
        if (TripCount == 0 || TripCount > UnrollThreshold)
            continue;

        // LCSSA form is required by UnrollLoop
        formLCSSARecursively(*L, DT, &LI, &SE);

        UnrollLoopOptions ULO;
        ULO.Count                    = TripCount;
        ULO.Force                    = true;
        ULO.Runtime                  = false;
        ULO.AllowExpensiveTripCount  = false;
        ULO.UnrollRemainder          = false;
        ULO.ForgetAllSCEV            = false;
        ULO.SCEVExpansionBudget      = 0;

        auto Result = UnrollLoop(L, ULO, &LI, &SE, &DT, &AC,
                                 nullptr, &ORE, /*PreserveLCSSA=*/true);

        if (Result != LoopUnrollResult::Unmodified)
            return PreservedAnalyses::none();
    }

    return PreservedAnalyses::all();
}

void tinyjs::registerOptimizer(PassBuilder &PB) {
    // allow --passes="optimizer" via opt-style CLI
    PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
            if (Name == "optimizer") {
                FPM.addPass(MyOptimizerPass());
                return true;
            }
            return false;
        });

    // inject at the start of every default pipeline
    PB.registerPipelineStartEPCallback(
        [](ModulePassManager &MPM, OptimizationLevel) {
            MPM.addPass(createModuleToFunctionPassAdaptor(MyOptimizerPass()));
        });
}
