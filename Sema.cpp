#include "Sema.h"
#include "llvm/ADT/StringMap.h"

namespace tinyjs {

class Scope {
    Scope *Parent;
    llvm::StringMap<Decl *> Symbols;
public:
    Scope(Scope *Parent = nullptr) : Parent(Parent) {}

    bool insert(Decl *D) {
        return Symbols.insert({D->getName(), D}).second;
    }

    Decl *lookup(llvm::StringRef Name) {
        Scope *S = this;
        while (S) {
            auto It = S->Symbols.find(Name);
            if (It != S->Symbols.end())
                return It->second;
            S = S->Parent;
        }
        return nullptr;
    }

    Scope *getParent() { return Parent; }
};

} // namespace tinyjs

using namespace tinyjs;

Sema::Sema(DiagnosticsEngine &Diags)
    : Diags(Diags), CurrentScope(new Scope()), CurrentDecl(nullptr) {}

void Sema::enterScope(Decl *D) {
    CurrentScope = new Scope(CurrentScope);
    CurrentDecl = D;
}

void Sema::leaveScope() {
    Scope *Parent = CurrentScope->getParent();
    delete CurrentScope;
    CurrentScope = Parent;
    if (CurrentDecl)
        CurrentDecl = CurrentDecl->getEnclosingDecl();
}

FunctionDecl *Sema::actOnFunctionDecl(llvm::SMLoc Loc, llvm::StringRef Name) {
    auto *FD = new FunctionDecl(CurrentDecl, Loc, Name);
    if (!CurrentScope->insert(FD))
        Diags.report(Loc, diag::err_sym_declared, Name);
    return FD;
}

Decl *Sema::actOnParamDecl(llvm::SMLoc Loc, llvm::StringRef Name) {
    auto *PD = new ParamDecl(CurrentDecl, Loc, Name);
    if (!CurrentScope->insert(PD))
        Diags.report(Loc, diag::err_sym_declared, Name);
    return PD;
}

Decl *Sema::actOnVarDecl(llvm::SMLoc Loc, llvm::StringRef Name) {
    auto *VD = new VarDecl(CurrentDecl, Loc, Name);
    if (!CurrentScope->insert(VD))
        Diags.report(Loc, diag::err_sym_declared, Name);
    return VD;
}

Decl *Sema::actOnVarRef(llvm::SMLoc Loc, llvm::StringRef Name) {
    Decl *D = CurrentScope->lookup(Name);
    if (!D)
        Diags.report(Loc, diag::err_sym_undeclared, Name);
    return D;
}

Decl *Sema::actOnFunctionRef(llvm::SMLoc Loc, llvm::StringRef Name) {
    Decl *D = CurrentScope->lookup(Name);
    if (!D)
        Diags.report(Loc, diag::err_sym_undeclared, Name);
    else if (!llvm::isa<FunctionDecl>(D))
        Diags.report(Loc, diag::err_sym_undeclared, Name);
    return D;
}

ClassDecl *Sema::actOnClassDecl(llvm::SMLoc Loc, llvm::StringRef Name, llvm::StringRef BaseName) {
    auto *CD = new ClassDecl(CurrentDecl, Loc, Name, BaseName);
    if (!CurrentScope->insert(CD))
        Diags.report(Loc, diag::err_sym_declared, Name);
    ClassTable[Name] = CD;
    return CD;
}

FieldDecl *Sema::actOnFieldDecl(llvm::SMLoc Loc, llvm::StringRef Name, long Default) {
    return new FieldDecl(CurrentDecl, Loc, Name, Default);
}

MethodDecl *Sema::actOnMethodDecl(llvm::SMLoc Loc, llvm::StringRef Name, bool IsVirtual) {
    return new MethodDecl(CurrentDecl, Loc, Name, IsVirtual);
}

ClassDecl *Sema::resolveClass(llvm::StringRef Name) {
    auto It = ClassTable.find(Name);
    return It != ClassTable.end() ? It->second : nullptr;
}

void Sema::recordVarClass(Decl *D, ClassDecl *C) { VarTypes[D] = C; }

ClassDecl *Sema::getVarClass(Decl *D) {
    auto It = VarTypes.find(D);
    return It != VarTypes.end() ? It->second : nullptr;
}
