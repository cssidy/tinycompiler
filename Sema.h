#pragma once
#include "AST.h"
#include "Lexer.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"

namespace tinyjs {

    class Scope; // defined in Sema.cpp

    class Sema {
        DiagnosticsEngine &Diags;
        Scope *CurrentScope;
        Decl *CurrentDecl;
        llvm::StringMap<ClassDecl *> ClassTable;
        llvm::DenseMap<Decl *, ClassDecl *> VarTypes;
    public:
        Sema(DiagnosticsEngine &Diags);

        void enterScope(Decl *D);
        void leaveScope();

        FunctionDecl *actOnFunctionDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnParamDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnVarDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnVarRef(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnFunctionRef(llvm::SMLoc Loc, llvm::StringRef Name);

        // class support
        ClassDecl *actOnClassDecl(llvm::SMLoc Loc, llvm::StringRef Name, llvm::StringRef BaseName);
        FieldDecl *actOnFieldDecl(llvm::SMLoc Loc, llvm::StringRef Name, long Default);
        MethodDecl *actOnMethodDecl(llvm::SMLoc Loc, llvm::StringRef Name, bool IsVirtual);
        ClassDecl *resolveClass(llvm::StringRef Name);
        void recordVarClass(Decl *D, ClassDecl *C);
        ClassDecl *getVarClass(Decl *D);
    };

} // namespace tinyjs
