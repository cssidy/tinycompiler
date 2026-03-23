#pragma once
#include "AST.h"
#include "Lexer.h"

namespace tinyjs {

    class Scope; // defined in Sema.cpp

    class Sema {
        DiagnosticsEngine &Diags;
        Scope *CurrentScope;
        Decl *CurrentDecl;
    public:
        Sema(DiagnosticsEngine &Diags);

        void enterScope(Decl *D);
        void leaveScope();

        FunctionDecl *actOnFunctionDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnParamDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnVarDecl(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnVarRef(llvm::SMLoc Loc, llvm::StringRef Name);
        Decl *actOnFunctionRef(llvm::SMLoc Loc, llvm::StringRef Name);
    };

} // namespace tinyjs
