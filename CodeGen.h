#pragma once
#include "AST.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/DenseMap.h"

namespace tinyjs {

// per-block SSA state for the Braun et al. algorithm
struct BasicBlockDef {
    llvm::DenseMap<Decl *, llvm::TrackingVH<llvm::Value>> Defs;
    llvm::DenseMap<Decl *, llvm::PHINode *> IncompletePhis;
    bool Sealed = false;
};

class CGProcedure {
    llvm::Function *Fn;
    llvm::IRBuilder<> Builder;
    llvm::DenseMap<llvm::BasicBlock *, BasicBlockDef> BBDefs;

    void writeLocalVar(llvm::BasicBlock *BB, Decl *D, llvm::Value *Val);
    llvm::Value *readLocalVar(llvm::BasicBlock *BB, Decl *D);
    llvm::Value *readLocalVarRecursive(llvm::BasicBlock *BB, Decl *D);
    llvm::PHINode *addEmptyPhi(llvm::BasicBlock *BB);
    llvm::Value *addPhiOperands(llvm::BasicBlock *BB, Decl *D, llvm::PHINode *Phi);
    llvm::Value *optimizePhi(llvm::PHINode *Phi);
    void sealBlock(llvm::BasicBlock *BB);

    llvm::Value *emitExpr(Expr *E);
    llvm::Value *toI64(llvm::Value *V);
    void emitStmt(Stmt *S);
    void emitBlock(const StmtList &Stmts);

public:
    CGProcedure(llvm::Function *Fn) : Fn(Fn), Builder(Fn->getContext()) {}
    void run(FunctionDecl *FD);
};

class CGModule {
    llvm::Module *M;
public:
    CGModule(llvm::Module *M) : M(M) {}
    void run(DeclList &Decls);
};

class CodeGenerator {
    llvm::LLVMContext Ctx;
    llvm::Module *M;
public:
    CodeGenerator(llvm::StringRef ModuleName);
    ~CodeGenerator() { delete M; }
    void run(DeclList &Decls);
    llvm::Module *getModule() { return M; }
};

} // namespace tinyjs
