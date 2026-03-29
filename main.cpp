#include "ClassGen.h"
#include "CodeGen.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace tinyjs;

static void ind(llvm::raw_ostream &OS, int depth) {
    for (int i = 0; i < depth; ++i)
        OS << "  ";
}

static llvm::StringRef opStr(BinOpExpr::Op op) {
    switch (op) {
    case BinOpExpr::Add: return "+";
    case BinOpExpr::Sub: return "-";
    case BinOpExpr::Mul: return "*";
    case BinOpExpr::Div: return "/";
    case BinOpExpr::Mod: return "%";
    case BinOpExpr::Eq:  return "==";
    case BinOpExpr::Neq: return "!=";
    case BinOpExpr::Lt:  return "<";
    case BinOpExpr::Gt:  return ">";
    }
    return "?";
}

static void dumpExpr(Expr *E, llvm::raw_ostream &OS, int depth = 0) {
    if (!E) { ind(OS, depth); OS << "<null>\n"; return; }
    switch (E->getKind()) {
    case Expr::EK_IntLiteral:
        ind(OS, depth);
        OS << "IntLiteral " << llvm::cast<IntLiteralExpr>(E)->getValue() << "\n";
        break;
    case Expr::EK_BoolLiteral:
        ind(OS, depth);
        OS << "BoolLiteral " << (llvm::cast<BoolLiteralExpr>(E)->getValue() ? "true" : "false") << "\n";
        break;
    case Expr::EK_Var:
        ind(OS, depth);
        OS << "Var " << llvm::cast<VarExpr>(E)->getDecl()->getName() << "\n";
        break;
    case Expr::EK_BinOp: {
        auto *B = llvm::cast<BinOpExpr>(E);
        ind(OS, depth);
        OS << "BinOp " << opStr(B->getOp()) << "\n";
        dumpExpr(B->getLeft(),  OS, depth + 1);
        dumpExpr(B->getRight(), OS, depth + 1);
        break;
    }
    case Expr::EK_Call: {
        auto *C = llvm::cast<CallExpr>(E);
        ind(OS, depth);
        OS << "Call " << C->getDecl()->getName() << "\n";
        for (Expr *Arg : C->getArgs())
            dumpExpr(Arg, OS, depth + 1);
        break;
    }
    case Expr::EK_StringLiteral:
        ind(OS, depth);
        OS << "StringLiteral " << llvm::cast<StringLiteralExpr>(E)->getValue() << "\n";
        break;
    case Expr::EK_New: {
        auto *N = llvm::cast<NewExpr>(E);
        ind(OS, depth);
        OS << "New " << N->getClass()->getName() << "\n";
        for (Expr *Arg : N->getArgs())
            dumpExpr(Arg, OS, depth + 1);
        break;
    }
    }
}

static void dumpStmt(Stmt *S, llvm::raw_ostream &OS, int depth = 0) {
    if (!S) { ind(OS, depth); OS << "<null>\n"; return; }
    switch (S->getKind()) {
    case Stmt::SK_VarDecl: {
        auto *V = llvm::cast<VarDeclStmt>(S);
        ind(OS, depth);
        OS << "VarDecl " << V->getDecl()->getName() << "\n";
        if (V->getInit())
            dumpExpr(V->getInit(), OS, depth + 1);
        break;
    }
    case Stmt::SK_Assign: {
        auto *A = llvm::cast<AssignStmt>(S);
        ind(OS, depth);
        OS << "Assign " << A->getDecl()->getName() << "\n";
        dumpExpr(A->getValue(), OS, depth + 1);
        break;
    }
    case Stmt::SK_If: {
        auto *I = llvm::cast<IfStmt>(S);
        ind(OS, depth); OS << "IfStmt\n";
        ind(OS, depth + 1); OS << "Cond\n";
        dumpExpr(I->getCond(), OS, depth + 2);
        ind(OS, depth + 1); OS << "Then\n";
        for (Stmt *St : I->getThen()) dumpStmt(St, OS, depth + 2);
        if (!I->getElse().empty()) {
            ind(OS, depth + 1); OS << "Else\n";
            for (Stmt *St : I->getElse()) dumpStmt(St, OS, depth + 2);
        }
        break;
    }
    case Stmt::SK_While: {
        auto *W = llvm::cast<WhileStmt>(S);
        ind(OS, depth); OS << "WhileStmt\n";
        ind(OS, depth + 1); OS << "Cond\n";
        dumpExpr(W->getCond(), OS, depth + 2);
        ind(OS, depth + 1); OS << "Body\n";
        for (Stmt *St : W->getBody()) dumpStmt(St, OS, depth + 2);
        break;
    }
    case Stmt::SK_Return: {
        auto *R = llvm::cast<ReturnStmt>(S);
        ind(OS, depth); OS << "ReturnStmt\n";
        if (R->getValue())
            dumpExpr(R->getValue(), OS, depth + 1);
        break;
    }
    case Stmt::SK_Call: {
        auto *C = llvm::cast<CallStmt>(S);
        dumpExpr(C->getCall(), OS, depth);
        break;
    }
    case Stmt::SK_MethodCall: {
        auto *MC = llvm::cast<MethodCallStmt>(S);
        ind(OS, depth);
        OS << "MethodCall " << MC->getObjDecl()->getName() << "."
           << MC->getMethod()->getName() << "\n";
        for (Expr *Arg : MC->getArgs())
            dumpExpr(Arg, OS, depth + 1);
        break;
    }
    }
}

static void dumpDecl(Decl *D, llvm::raw_ostream &OS) {
    if (auto *FD = llvm::dyn_cast<FunctionDecl>(D)) {
        OS << "FunctionDecl " << FD->getName() << "(";
        bool First = true;
        for (Decl *P : FD->getParams()) {
            if (!First) OS << ", ";
            OS << P->getName();
            First = false;
        }
        OS << ")\n";
        for (Stmt *S : FD->getBody())
            dumpStmt(S, OS, 1);
    } else if (auto *CD = llvm::dyn_cast<ClassDecl>(D)) {
        OS << "ClassDecl " << CD->getName();
        if (CD->getBase()) OS << " extends " << CD->getBase()->getName();
        OS << "\n";
        for (auto *F : CD->getFields())
            OS << "  Field " << F->getName() << " = " << F->getDefaultValue() << "\n";
        for (auto *M : CD->getMethods()) {
            OS << "  " << (M->isVirtual() ? "virtual " : "") << "Method " << M->getName() << "\n";
            for (Stmt *S : M->getBody())
                dumpStmt(S, OS, 2);
        }
    }
}

int main(int argc, const char **argv) {
    llvm::InitLLVM X(argc, argv);

    if (argc < 2) {
        llvm::errs() << "Usage: tinycompiler <file>\n";
        return 1;
    }

    llvm::SourceMgr SrcMgr;
    DiagnosticsEngine Diags(SrcMgr);

    auto FileOrErr = llvm::MemoryBuffer::getFile(argv[1]);
    if (!FileOrErr) {
        llvm::errs() << "Error reading file: " << argv[1] << "\n";
        return 1;
    }

    SrcMgr.AddNewSourceBuffer(std::move(*FileOrErr), llvm::SMLoc());

    Lexer Lex(SrcMgr, Diags);
    Sema Actions(Diags);
    Parser P(Lex, Actions, Diags);

    DeclList Functions = P.parse();

    if (Diags.numErrors() > 0) {
        llvm::outs() << Diags.numErrors() << " error(s) found.\n";
        return 1;
    }

    std::string Input = argv[1];
    std::string Stem = Input;
    auto DotPos = Input.rfind('.');
    if (DotPos != std::string::npos)
        Stem = Input.substr(0, DotPos);
    std::string OutputPath = Stem + "-output.ast";

    std::error_code EC;
    llvm::raw_fd_ostream Out(OutputPath, EC, llvm::sys::fs::OF_None);
    if (EC) {
        llvm::errs() << "Error writing " << OutputPath << ": " << EC.message() << "\n";
        return 1;
    }

    for (Decl *D : Functions)
        dumpDecl(D, Out);

    llvm::outs() << "Parsed successfully. AST written to " << OutputPath << "\n";

    std::string IRPath = Stem + ".ll";
    CodeGenerator CG(argv[1]);
    // classes must be emitted before functions (struct types + ctors used by CodeGen)
    tinyjs::emitClasses(CG.getModule(), Functions);
    CG.run(Functions);

    std::error_code EC2;
    llvm::raw_fd_ostream IROut(IRPath, EC2, llvm::sys::fs::OF_None);
    if (EC2) {
        llvm::errs() << "Error writing " << IRPath << ": " << EC2.message() << "\n";
        return 1;
    }
    CG.getModule()->print(IROut, nullptr);
    llvm::outs() << "IR written to " << IRPath << "\n";

    return 0;
}
