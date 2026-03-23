#pragma once
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"
#include <vector>

namespace tinyjs {

// forward declarations
class Decl;
class Stmt;
class Expr;
using DeclList = std::vector<Decl *>;
using StmtList = std::vector<Stmt *>;
using ExprList = std::vector<Expr *>;

// declaration nodes

class Decl {
public:
    enum DeclKind { DK_Var, DK_Param, DK_Function };
private:
    const DeclKind Kind;
protected:
    Decl *EnclosingDecl;
    llvm::SMLoc Loc;
    llvm::StringRef Name;
public:
    Decl(DeclKind Kind, Decl *EnclosingDecl, llvm::SMLoc Loc, llvm::StringRef Name)
        : Kind(Kind), EnclosingDecl(EnclosingDecl), Loc(Loc), Name(Name) {}
    DeclKind getKind() const { return Kind; }
    llvm::SMLoc getLocation() { return Loc; }
    llvm::StringRef getName() { return Name; }
    Decl *getEnclosingDecl() { return EnclosingDecl; }
};

class VarDecl : public Decl {
public:
    VarDecl(Decl *EnclosingDecl, llvm::SMLoc Loc, llvm::StringRef Name)
        : Decl(DK_Var, EnclosingDecl, Loc, Name) {}
    static bool classof(const Decl *D) { return D->getKind() == DK_Var; }
};

class ParamDecl : public Decl {
public:
    ParamDecl(Decl *EnclosingDecl, llvm::SMLoc Loc, llvm::StringRef Name)
        : Decl(DK_Param, EnclosingDecl, Loc, Name) {}
    static bool classof(const Decl *D) { return D->getKind() == DK_Param; }
};

class FunctionDecl : public Decl {
    DeclList Params;
    StmtList Body;
public:
    FunctionDecl(Decl *EnclosingDecl, llvm::SMLoc Loc, llvm::StringRef Name)
        : Decl(DK_Function, EnclosingDecl, Loc, Name) {}
    void setParams(DeclList &P) { Params = P; }
    void setBody(StmtList &B) { Body = B; }
    const DeclList &getParams() const { return Params; }
    const StmtList &getBody() const { return Body; }
    static bool classof(const Decl *D) { return D->getKind() == DK_Function; }
};

// expression nodes

class Expr {
public:
    enum ExprKind { EK_IntLiteral, EK_BoolLiteral, EK_Var, EK_BinOp, EK_Call };
private:
    const ExprKind Kind;
public:
    Expr(ExprKind Kind) : Kind(Kind) {}
    ExprKind getKind() const { return Kind; }
};

class IntLiteralExpr : public Expr {
    long Value;
public:
    IntLiteralExpr(long Value) : Expr(EK_IntLiteral), Value(Value) {}
    long getValue() const { return Value; }
    static bool classof(const Expr *E) { return E->getKind() == EK_IntLiteral; }
};

class BoolLiteralExpr : public Expr {
    bool Value;
public:
    BoolLiteralExpr(bool Value) : Expr(EK_BoolLiteral), Value(Value) {}
    bool getValue() const { return Value; }
    static bool classof(const Expr *E) { return E->getKind() == EK_BoolLiteral; }
};

class VarExpr : public Expr {
    Decl *D;
public:
    VarExpr(Decl *D) : Expr(EK_Var), D(D) {}
    Decl *getDecl() { return D; }
    static bool classof(const Expr *E) { return E->getKind() == EK_Var; }
};

class BinOpExpr : public Expr {
public:
    enum Op { Add, Sub, Mul, Div, Mod, Eq, Neq, Lt, Gt };
private:
    Op OpKind;
    Expr *Left, *Right;
public:
    BinOpExpr(Op OpKind, Expr *Left, Expr *Right)
        : Expr(EK_BinOp), OpKind(OpKind), Left(Left), Right(Right) {}
    Op getOp() const { return OpKind; }
    Expr *getLeft() { return Left; }
    Expr *getRight() { return Right; }
    static bool classof(const Expr *E) { return E->getKind() == EK_BinOp; }
};

class CallExpr : public Expr {
    Decl *D;
    ExprList Args;
public:
    CallExpr(Decl *D, ExprList &Args) : Expr(EK_Call), D(D), Args(Args) {}
    Decl *getDecl() { return D; }
    const ExprList &getArgs() const { return Args; }
    static bool classof(const Expr *E) { return E->getKind() == EK_Call; }
};

// statement nodes

class Stmt {
public:
    enum StmtKind { SK_VarDecl, SK_Assign, SK_If, SK_While, SK_Return, SK_Call };
private:
    const StmtKind Kind;
public:
    Stmt(StmtKind Kind) : Kind(Kind) {}
    StmtKind getKind() const { return Kind; }
};

class VarDeclStmt : public Stmt {
    Decl *D;
    Expr *Init;
public:
    VarDeclStmt(Decl *D, Expr *Init) : Stmt(SK_VarDecl), D(D), Init(Init) {}
    Decl *getDecl() { return D; }
    Expr *getInit() { return Init; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_VarDecl; }
};

class AssignStmt : public Stmt {
    Decl *D;
    Expr *Value;
public:
    AssignStmt(Decl *D, Expr *Value) : Stmt(SK_Assign), D(D), Value(Value) {}
    Decl *getDecl() { return D; }
    Expr *getValue() { return Value; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_Assign; }
};

class IfStmt : public Stmt {
    Expr *Cond;
    StmtList Then, Else;
public:
    IfStmt(Expr *Cond, StmtList &Then, StmtList &Else)
        : Stmt(SK_If), Cond(Cond), Then(Then), Else(Else) {}
    Expr *getCond() { return Cond; }
    const StmtList &getThen() const { return Then; }
    const StmtList &getElse() const { return Else; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_If; }
};

class WhileStmt : public Stmt {
    Expr *Cond;
    StmtList Body;
public:
    WhileStmt(Expr *Cond, StmtList &Body)
        : Stmt(SK_While), Cond(Cond), Body(Body) {}
    Expr *getCond() { return Cond; }
    const StmtList &getBody() const { return Body; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_While; }
};

class ReturnStmt : public Stmt {
    Expr *Value;
public:
    ReturnStmt(Expr *Value) : Stmt(SK_Return), Value(Value) {}
    Expr *getValue() { return Value; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_Return; }
};

class CallStmt : public Stmt {
    CallExpr *Call;
public:
    CallStmt(CallExpr *Call) : Stmt(SK_Call), Call(Call) {}
    CallExpr *getCall() { return Call; }
    static bool classof(const Stmt *S) { return S->getKind() == SK_Call; }
};

} // namespace tinyjs
