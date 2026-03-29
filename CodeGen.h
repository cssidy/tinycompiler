#pragma once
#include "AST.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/DenseMap.h"

namespace tinyjs {

class CGModule; // forward declaration

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
    CGModule &CGM;

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
    CGProcedure(llvm::Function *Fn, CGModule &CGM)
        : Fn(Fn), Builder(Fn->getContext()), CGM(CGM) {}
    void run(FunctionDecl *FD);
};

class CGModule {
    llvm::Module *M;
    llvm::DIBuilder DBuilder;
    llvm::DICompileUnit *CU;
    llvm::DIFile *DbgFile;
    llvm::DIBasicType *DbgI64Ty;
public:
    CGModule(llvm::Module *M, llvm::StringRef Filename);
    void run(DeclList &Decls);
    // accessors used by CGProcedure to attach debug info to functions
    llvm::DIBuilder &getDIBuilder() { return DBuilder; }
    llvm::DICompileUnit *getCU() { return CU; }
    llvm::DIFile *getDbgFile() { return DbgFile; }
    llvm::DIBasicType *getDbgI64Ty() { return DbgI64Ty; }
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
