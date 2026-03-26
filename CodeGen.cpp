#include "CodeGen.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"

using namespace tinyjs;

// mangle: _t<len><name>  e.g. gcd -> _t3gcd
static std::string mangle(llvm::StringRef Name) {
    return "_t" + std::to_string(Name.size()) + Name.str();
}

void CGProcedure::writeLocalVar(llvm::BasicBlock *BB, Decl *D, llvm::Value *Val) {
    BBDefs[BB].Defs[D] = Val;
}

llvm::Value *CGProcedure::readLocalVar(llvm::BasicBlock *BB, Decl *D) {
    auto &Def = BBDefs[BB];
    auto It = Def.Defs.find(D);
    if (It != Def.Defs.end())
        return It->second;
    return readLocalVarRecursive(BB, D);
}

llvm::Value *CGProcedure::readLocalVarRecursive(llvm::BasicBlock *BB, Decl *D) {
    llvm::Value *Val;
    auto &Def = BBDefs[BB];
    if (!Def.Sealed) {
        // block not yet sealed, add an incomplete phi
        llvm::PHINode *Phi = addEmptyPhi(BB);
        Def.IncompletePhis[D] = Phi;
        Val = Phi;
    } else {
        auto Preds = llvm::predecessors(BB);
        auto PredIt = Preds.begin();
        if (PredIt == Preds.end()) {
            // no predecessors, undefined (shouldn't happen in valid programs)
            Val = llvm::UndefValue::get(llvm::Type::getInt64Ty(BB->getContext()));
        } else {
            auto Next = PredIt;
            ++Next;
            if (Next == Preds.end()) {
                // single predecessor, just recurse
                Val = readLocalVar(*PredIt, D);
            } else {
                // multiple predecessors, need a phi
                llvm::PHINode *Phi = addEmptyPhi(BB);
                writeLocalVar(BB, D, Phi);
                Val = addPhiOperands(BB, D, Phi);
            }
        }
    }
    writeLocalVar(BB, D, Val);
    return Val;
}

llvm::PHINode *CGProcedure::addEmptyPhi(llvm::BasicBlock *BB) {
    // insert phi at the top of the block
    return llvm::PHINode::Create(
        llvm::Type::getInt64Ty(BB->getContext()), 0, "", BB->begin());
}

llvm::Value *CGProcedure::addPhiOperands(llvm::BasicBlock *BB, Decl *D, llvm::PHINode *Phi) {
    for (auto *Pred : llvm::predecessors(BB))
        Phi->addIncoming(readLocalVar(Pred, D), Pred);
    return optimizePhi(Phi);
}

llvm::Value *CGProcedure::optimizePhi(llvm::PHINode *Phi) {
    llvm::Value *Same = nullptr;
    for (llvm::Value *Op : Phi->incoming_values()) {
        if (Op == Same || Op == Phi)
            continue;
        if (Same != nullptr)
            return Phi; // different values, keep the phi
        Same = Op;
    }
    if (!Same)
        Same = llvm::UndefValue::get(Phi->getType());
    Phi->replaceAllUsesWith(Same);
    Phi->eraseFromParent();
    return Same;
}

void CGProcedure::sealBlock(llvm::BasicBlock *BB) {
    auto &Def = BBDefs[BB];
    for (auto &[D, Phi] : Def.IncompletePhis)
        addPhiOperands(BB, D, Phi);
    Def.IncompletePhis.clear();
    Def.Sealed = true;
}

// zero-extend i1 to i64 when needed in arithmetic context
llvm::Value *CGProcedure::toI64(llvm::Value *V) {
    if (V->getType()->isIntegerTy(1))
        return Builder.CreateZExt(V, llvm::Type::getInt64Ty(Builder.getContext()));
    return V;
}

llvm::Value *CGProcedure::emitExpr(Expr *E) {
    switch (E->getKind()) {
    case Expr::EK_IntLiteral:
        return llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(Builder.getContext()),
            llvm::cast<IntLiteralExpr>(E)->getValue());

    case Expr::EK_BoolLiteral:
        return llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(Builder.getContext()),
            llvm::cast<BoolLiteralExpr>(E)->getValue() ? 1 : 0);

    case Expr::EK_Var: {
        Decl *D = llvm::cast<VarExpr>(E)->getDecl();
        return toI64(readLocalVar(Builder.GetInsertBlock(), D));
    }

    case Expr::EK_BinOp: {
        auto *B = llvm::cast<BinOpExpr>(E);
        llvm::Value *L = toI64(emitExpr(B->getLeft()));
        llvm::Value *R = toI64(emitExpr(B->getRight()));
        switch (B->getOp()) {
        case BinOpExpr::Add: return Builder.CreateAdd(L, R);
        case BinOpExpr::Sub: return Builder.CreateSub(L, R);
        case BinOpExpr::Mul: return Builder.CreateMul(L, R);
        case BinOpExpr::Div: return Builder.CreateSDiv(L, R);
        case BinOpExpr::Mod: return Builder.CreateSRem(L, R);
        case BinOpExpr::Eq:  return Builder.CreateICmpEQ(L, R);
        case BinOpExpr::Neq: return Builder.CreateICmpNE(L, R);
        case BinOpExpr::Lt:  return Builder.CreateICmpSLT(L, R);
        case BinOpExpr::Gt:  return Builder.CreateICmpSGT(L, R);
        }
        break;
    }

    case Expr::EK_Call: {
        auto *CE = llvm::cast<CallExpr>(E);
        std::string MangledName = mangle(CE->getDecl()->getName());
        llvm::Function *Callee = Fn->getParent()->getFunction(MangledName);
        std::vector<llvm::Value *> Args;
        for (Expr *Arg : CE->getArgs())
            Args.push_back(toI64(emitExpr(Arg)));
        return Builder.CreateCall(Callee, Args);
    }
    }
    return nullptr;
}

void CGProcedure::emitStmt(Stmt *S) {
    switch (S->getKind()) {
    case Stmt::SK_VarDecl: {
        auto *VD = llvm::cast<VarDeclStmt>(S);
        llvm::Value *Val = nullptr;
        if (VD->getInit())
            Val = toI64(emitExpr(VD->getInit()));
        else
            Val = llvm::ConstantInt::get(
                llvm::Type::getInt64Ty(Builder.getContext()), 0);
        writeLocalVar(Builder.GetInsertBlock(), VD->getDecl(), Val);
        break;
    }
    case Stmt::SK_Assign: {
        auto *AS = llvm::cast<AssignStmt>(S);
        llvm::Value *Val = toI64(emitExpr(AS->getValue()));
        writeLocalVar(Builder.GetInsertBlock(), AS->getDecl(), Val);
        break;
    }
    case Stmt::SK_If: {
        auto *IS = llvm::cast<IfStmt>(S);
        llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(Builder.getContext(), "if.then", Fn);
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(Builder.getContext(), "if.merge", Fn);
        llvm::BasicBlock *ElseBB = IS->getElse().empty() ? MergeBB
            : llvm::BasicBlock::Create(Builder.getContext(), "if.else", Fn);

        llvm::Value *Cond = emitExpr(IS->getCond());
        // ensure i1 for branch condition
        if (!Cond->getType()->isIntegerTy(1))
            Cond = Builder.CreateICmpNE(Cond,
                llvm::ConstantInt::get(Cond->getType(), 0));
        Builder.CreateCondBr(Cond, ThenBB, ElseBB);

        // then block
        Builder.SetInsertPoint(ThenBB);
        sealBlock(ThenBB);
        emitBlock(IS->getThen());
        if (!Builder.GetInsertBlock()->getTerminator())
            Builder.CreateBr(MergeBB);

        // else block
        if (ElseBB != MergeBB) {
            Builder.SetInsertPoint(ElseBB);
            sealBlock(ElseBB);
            emitBlock(IS->getElse());
            if (!Builder.GetInsertBlock()->getTerminator())
                Builder.CreateBr(MergeBB);
        }

        // Only use MergeBB if at least one branch jumps to it otherwise it is dead code.
        if (MergeBB->hasNPredecessorsOrMore(1)) {
            Builder.SetInsertPoint(MergeBB);
            sealBlock(MergeBB);
        } else {
            MergeBB->eraseFromParent();
        }
        break;
    }
    case Stmt::SK_While: {
        auto *WS = llvm::cast<WhileStmt>(S);
        llvm::BasicBlock *CondBB  = llvm::BasicBlock::Create(Builder.getContext(), "while.cond", Fn);
        llvm::BasicBlock *BodyBB  = llvm::BasicBlock::Create(Builder.getContext(), "while.body", Fn);
        llvm::BasicBlock *AfterBB = llvm::BasicBlock::Create(Builder.getContext(), "while.after", Fn);

        // jump into the condition block; CondBB is left unsealed (back-edge not yet known)
        Builder.CreateBr(CondBB);

        // emit condition
        Builder.SetInsertPoint(CondBB);
        llvm::Value *Cond = emitExpr(WS->getCond());
        if (!Cond->getType()->isIntegerTy(1))
            Cond = Builder.CreateICmpNE(Cond,
                llvm::ConstantInt::get(Cond->getType(), 0));
        Builder.CreateCondBr(Cond, BodyBB, AfterBB);

        // emit body
        Builder.SetInsertPoint(BodyBB);
        sealBlock(BodyBB);
        emitBlock(WS->getBody());
        if (!Builder.GetInsertBlock()->getTerminator())
            Builder.CreateBr(CondBB);

        // now the back-edge is known, seal CondBB
        sealBlock(CondBB);

        Builder.SetInsertPoint(AfterBB);
        sealBlock(AfterBB);
        break;
    }
    case Stmt::SK_Return: {
        auto *RS = llvm::cast<ReturnStmt>(S);
        if (RS->getValue())
            Builder.CreateRet(toI64(emitExpr(RS->getValue())));
        else
            Builder.CreateRet(
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(Fn->getContext()), 0));
        break;
    }
    case Stmt::SK_Call: {
        auto *CS = llvm::cast<CallStmt>(S);
        emitExpr(CS->getCall());
        break;
    }
    }
}

void CGProcedure::emitBlock(const StmtList &Stmts) {
    for (Stmt *S : Stmts) {
        emitStmt(S);
        // stop emitting after a terminator (e.g. return)
        if (Builder.GetInsertBlock()->getTerminator())
            break;
    }
}

void CGProcedure::run(FunctionDecl *FD) {
    llvm::BasicBlock *EntryBB = llvm::BasicBlock::Create(
        Fn->getContext(), "entry", Fn);
    Builder.SetInsertPoint(EntryBB);

    // write each formal parameter into the entry block
    unsigned Idx = 0;
    for (Decl *P : FD->getParams()) {
        llvm::Argument *Arg = Fn->getArg(Idx++);
        Arg->setName(P->getName());
        writeLocalVar(EntryBB, P, Arg);
    }

    emitBlock(FD->getBody());

    // seal entry block last, after all statements are emitted
    sealBlock(EntryBB);

    // add a default return if the function fell off the end
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateRet(
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(Fn->getContext()), 0));
}

void CGModule::run(DeclList &Decls) {
    llvm::LLVMContext &Ctx = M->getContext();
    llvm::Type *I64 = llvm::Type::getInt64Ty(Ctx);

    // first pass: declare all functions so calls can find them
    for (Decl *D : Decls) {
        auto *FD = llvm::dyn_cast<FunctionDecl>(D);
        if (!FD) continue;
        std::vector<llvm::Type *> ParamTypes(FD->getParams().size(), I64);
        llvm::FunctionType *FT = llvm::FunctionType::get(I64, ParamTypes, false);
        llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                               mangle(FD->getName()), M);
    }

    // second pass: emit bodies
    for (Decl *D : Decls) {
        auto *FD = llvm::dyn_cast<FunctionDecl>(D);
        if (!FD) continue;
        llvm::Function *Fn = M->getFunction(mangle(FD->getName()));
        CGProcedure CGP(Fn);
        CGP.run(FD);
    }
}

CodeGenerator::CodeGenerator(llvm::StringRef ModuleName) {
    M = new llvm::Module(ModuleName, Ctx);
}

void CodeGenerator::run(DeclList &Decls) {
    CGModule CGM(M);
    CGM.run(Decls);
}
