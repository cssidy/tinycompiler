#include "Parser.h"

using namespace tinyjs;

Parser::Parser(Lexer &Lex, Sema &Actions, DiagnosticsEngine &Diags)
    : Lex(Lex), Actions(Actions), Diags(Diags) {
    advance();
}

void Parser::advance() {
    Lex.next(Tok);
}

bool Parser::expect(tok::TokenKind Kind) {
    if (Tok.is(Kind)) return false;
    Diags.report(Tok.getLocation(), diag::err_expected_token,
                 tok::TokenKind(Kind));
    return true;
}

bool Parser::consume(tok::TokenKind Kind) {
    if (expect(Kind)) return true;
    advance();
    return false;
}

DeclList Parser::parse() {
    DeclList Decls;
    while (!Tok.is(tok::eof)) {
        if (Tok.is(tok::kw_function)) {
            if (FunctionDecl *FD = parseFunction())
                Decls.push_back(FD);
        } else if (Tok.is(tok::kw_class)) {
            if (ClassDecl *CD = parseClass())
                Decls.push_back(CD);
        } else {
            Diags.report(Tok.getLocation(), diag::err_expected_token, "function or class");
            advance();
        }
    }
    return Decls;
}

ClassDecl *Parser::parseClass() {
    consume(tok::kw_class);
    if (expect(tok::identifier)) return nullptr;
    llvm::SMLoc Loc = Tok.getLocation();
    llvm::StringRef Name = Tok.getText();
    advance();

    llvm::StringRef BaseName;
    if (Tok.is(tok::kw_extends)) {
        advance();
        if (expect(tok::identifier)) return nullptr;
        BaseName = Tok.getText();
        advance();
    }

    ClassDecl *CD = Actions.actOnClassDecl(Loc, Name, BaseName);

    // resolve base class (must be declared before this class)
    if (!BaseName.empty()) {
        ClassDecl *Base = Actions.resolveClass(BaseName);
        if (Base) CD->setBase(Base);
        else Diags.report(Loc, diag::err_sym_undeclared, BaseName);
    }

    if (consume(tok::lbrace)) return CD;
    while (!Tok.is(tok::rbrace) && !Tok.is(tok::eof))
        parseClassMember(CD);
    consume(tok::rbrace);

    // assign vtable indices: inherit base slots, then add new virtual methods
    unsigned Slot = 0;
    if (CD->getBase())
        for (auto *M : CD->getBase()->getMethods())
            if (M->isVirtual()) ++Slot;
    for (auto *M : CD->getMethods())
        if (M->isVirtual()) M->setVTableIndex(Slot++);
        else {
            // check if this overrides a base virtual method (same name)
            if (CD->getBase())
                for (auto *BM : CD->getBase()->getMethods())
                    if (BM->isVirtual() && BM->getName() == M->getName())
                        M->setVTableIndex(BM->getVTableIndex());
        }

    return CD;
}

bool Parser::parseClassMember(ClassDecl *CD) {
    bool IsVirtual = false;
    if (Tok.is(tok::kw_virtual)) {
        IsVirtual = true;
        advance();
    }
    if (expect(tok::identifier)) return true;
    llvm::SMLoc Loc = Tok.getLocation();
    llvm::StringRef Name = Tok.getText();
    advance();

    if (!IsVirtual && Tok.is(tok::equal)) {
        // field: NAME = INTEGER ;
        advance();
        long Default = 0;
        if (Tok.is(tok::integer_literal)) {
            Default = std::stol(Tok.getText().str());
            advance();
        }
        consume(tok::semi);
        CD->addField(Actions.actOnFieldDecl(Loc, Name, Default));
    } else {
        // method: [virtual] NAME ( params ) block
        MethodDecl *MD = Actions.actOnMethodDecl(Loc, Name, IsVirtual);
        Actions.enterScope(MD);
        DeclList Params;
        if (consume(tok::lparen)) { Actions.leaveScope(); return true; }
        if (!Tok.is(tok::rparen))
            parseParamList(Params);
        consume(tok::rparen);
        MD->setParams(Params);
        StmtList Body;
        parseBlock(Body);
        MD->setBody(Body);
        Actions.leaveScope();
        CD->addMethod(MD);
    }
    return false;
}

FunctionDecl *Parser::parseFunction() {
    consume(tok::kw_function);

    if (expect(tok::identifier)) return nullptr;
    llvm::SMLoc Loc = Tok.getLocation();
    llvm::StringRef Name = Tok.getText();
    advance();

    FunctionDecl *FD = Actions.actOnFunctionDecl(Loc, Name);

    // Enter function scope
    Actions.enterScope(FD);

    DeclList Params;
    if (consume(tok::lparen)) { Actions.leaveScope(); return nullptr; }
    if (!Tok.is(tok::rparen))
        parseParamList(Params);
    if (consume(tok::rparen)) { Actions.leaveScope(); return nullptr; }

    StmtList Body;
    parseBlock(Body);

    FD->setParams(Params);
    FD->setBody(Body);
    Actions.leaveScope();
    return FD;
}

bool Parser::parseParamList(DeclList &Params) {
    while (true) {
        if (expect(tok::identifier)) return true;
        llvm::SMLoc Loc = Tok.getLocation();
        llvm::StringRef Name = Tok.getText();
        advance();
        Decl *PD = Actions.actOnParamDecl(Loc, Name);
        if (PD) Params.push_back(PD);
        if (!Tok.is(tok::comma)) break;
        advance();
    }
    return false;
}

bool Parser::parseBlock(StmtList &Stmts) {
    if (consume(tok::lbrace)) return true;
    while (!Tok.is(tok::rbrace) && !Tok.is(tok::eof))
        parseStatement(Stmts);
    return consume(tok::rbrace);
}

bool Parser::parseStatement(StmtList &Stmts) {
    if (Tok.is(tok::kw_var))
        return parseVarDecl(Stmts);
    if (Tok.is(tok::kw_if))
        return parseIfStmt(Stmts);
    if (Tok.is(tok::kw_while))
        return parseWhileStmt(Stmts);
    if (Tok.is(tok::kw_return))
        return parseReturnStmt(Stmts);
    if (Tok.is(tok::identifier))
        return parseAssignOrCall(Stmts);

    Diags.report(Tok.getLocation(), diag::err_expected_token, "statement");
    advance();
    return false;
}

bool Parser::parseVarDecl(StmtList &Stmts) {
    consume(tok::kw_var);
    if (expect(tok::identifier)) return true;
    llvm::SMLoc Loc = Tok.getLocation();
    llvm::StringRef Name = Tok.getText();
    advance();

    Decl *VD = Actions.actOnVarDecl(Loc, Name);

    Expr *Init = nullptr;
    if (Tok.is(tok::equal)) {
        advance();
        Init = parseExpr();
        // track class type for virtual dispatch resolution
        if (auto *NE = llvm::dyn_cast_or_null<NewExpr>(Init))
            if (VD) Actions.recordVarClass(VD, NE->getClass());
    }
    consume(tok::semi);
    if (VD) Stmts.push_back(new VarDeclStmt(VD, Init));
    return false;
}

bool Parser::parseIfStmt(StmtList &Stmts) {
    consume(tok::kw_if);
    if (consume(tok::lparen)) return true;
    Expr *Cond = parseExpr();
    if (consume(tok::rparen)) return true;

    StmtList Then, Else;
    parseBlock(Then);

    if (Tok.is(tok::kw_else)) {
        advance();
        parseBlock(Else);
    }
    Stmts.push_back(new IfStmt(Cond, Then, Else));
    return false;
}

bool Parser::parseWhileStmt(StmtList &Stmts) {
    consume(tok::kw_while);
    if (consume(tok::lparen)) return true;
    Expr *Cond = parseExpr();
    if (consume(tok::rparen)) return true;

    StmtList Body;
    parseBlock(Body);
    Stmts.push_back(new WhileStmt(Cond, Body));
    return false;
}

bool Parser::parseReturnStmt(StmtList &Stmts) {
    consume(tok::kw_return);
    Expr *Val = nullptr;
    if (!Tok.is(tok::semi))
        Val = parseExpr();
    consume(tok::semi);
    Stmts.push_back(new ReturnStmt(Val));
    return false;
}

bool Parser::parseAssignOrCall(StmtList &Stmts) {
    llvm::SMLoc Loc = Tok.getLocation();
    llvm::StringRef Name = Tok.getText();
    advance();

    if (Tok.is(tok::dot)) {
        // obj.method(args) virtual dispatch statement
        advance();
        if (expect(tok::identifier)) return false;
        llvm::StringRef MethodName = Tok.getText();
        advance();
        Decl *ObjDecl = Actions.actOnVarRef(Loc, Name);
        ClassDecl *ObjClass = ObjDecl ? Actions.getVarClass(ObjDecl) : nullptr;
        // find method in class or its base
        MethodDecl *MD = nullptr;
        auto findMethod = [&](ClassDecl *CD) {
            for (auto *M : CD->getMethods())
                if (M->getName() == MethodName) { MD = M; return; }
        };
        if (ObjClass) {
            findMethod(ObjClass);
            if (!MD && ObjClass->getBase()) findMethod(ObjClass->getBase());
        }
        ExprList Args;
        consume(tok::lparen);
        while (!Tok.is(tok::rparen) && !Tok.is(tok::eof)) {
            Args.push_back(parseExpr());
            if (Tok.is(tok::comma)) advance();
        }
        consume(tok::rparen);
        consume(tok::semi);
        if (ObjDecl && ObjClass && MD)
            Stmts.push_back(new MethodCallStmt(ObjDecl, ObjClass, MD, Args));
    } else if (Tok.is(tok::equal)) {
        // Assignment
        advance();
        Expr *Val = parseExpr();
        consume(tok::semi);
        Decl *D = Actions.actOnVarRef(Loc, Name);
        if (D) Stmts.push_back(new AssignStmt(D, Val));
    } else if (Tok.is(tok::lparen)) {
        // Function call statement
        advance();
        ExprList Args;
        while (!Tok.is(tok::rparen) && !Tok.is(tok::eof)) {
            Args.push_back(parseExpr());
            if (Tok.is(tok::comma)) advance();
        }
        consume(tok::rparen);
        consume(tok::semi);
        Decl *D = Actions.actOnFunctionRef(Loc, Name);
        if (D) {
            auto *CE = new CallExpr(D, Args);
            Stmts.push_back(new CallStmt(CE));
        }
    } else {
        Diags.report(Loc, diag::err_expected_token, "= or (");
    }
    return false;
}

Expr *Parser::parseExpr() {
    return parseComparison();
}

Expr *Parser::parseComparison() {
    Expr *Left = parseTerm();
    while (Tok.is(tok::equalequal) || Tok.is(tok::notequal) ||
           Tok.is(tok::less) || Tok.is(tok::greater)) {
        BinOpExpr::Op Op;
        if (Tok.is(tok::equalequal))     Op = BinOpExpr::Eq;
        else if (Tok.is(tok::notequal))  Op = BinOpExpr::Neq;
        else if (Tok.is(tok::less))      Op = BinOpExpr::Lt;
        else                             Op = BinOpExpr::Gt;
        advance();
        Expr *Right = parseTerm();
        Left = new BinOpExpr(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm() {
    Expr *Left = parseFactor();
    while (Tok.is(tok::plus) || Tok.is(tok::minus)) {
        BinOpExpr::Op Op = Tok.is(tok::plus) ? BinOpExpr::Add : BinOpExpr::Sub;
        advance();
        Expr *Right = parseFactor();
        Left = new BinOpExpr(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parsePrimary() {
    if (Tok.is(tok::kw_new)) {
        advance();
        if (expect(tok::identifier)) return nullptr;
        llvm::SMLoc Loc = Tok.getLocation();
        llvm::StringRef ClassName = Tok.getText();
        advance();
        ClassDecl *CD = Actions.resolveClass(ClassName);
        if (!CD) Diags.report(Loc, diag::err_sym_undeclared, ClassName);
        ExprList Args;
        consume(tok::lparen);
        while (!Tok.is(tok::rparen) && !Tok.is(tok::eof)) {
            Args.push_back(parseExpr());
            if (Tok.is(tok::comma)) advance();
        }
        consume(tok::rparen);
        return CD ? new NewExpr(CD, Args) : nullptr;
    }
    if (Tok.is(tok::string_literal)) {
        llvm::StringRef Val = Tok.getText();
        advance();
        return new StringLiteralExpr(Val);
    }
    if (Tok.is(tok::integer_literal)) {
        long Val = std::stol(Tok.getText().str());
        advance();
        return new IntLiteralExpr(Val);
    }
    if (Tok.is(tok::kw_true))  { advance(); return new BoolLiteralExpr(true); }
    if (Tok.is(tok::kw_false)) { advance(); return new BoolLiteralExpr(false); }
    if (Tok.is(tok::lparen)) {
        advance();
        Expr *E = parseExpr();
        consume(tok::rparen);
        return E;
    }
    if (Tok.is(tok::identifier)) {
        llvm::SMLoc Loc = Tok.getLocation();
        llvm::StringRef Name = Tok.getText();
        advance();
        if (Tok.is(tok::lparen)) {
            advance();
            ExprList Args;
            while (!Tok.is(tok::rparen) && !Tok.is(tok::eof)) {
                Args.push_back(parseExpr());
                if (Tok.is(tok::comma)) advance();
            }
            consume(tok::rparen);
            Decl *D = Actions.actOnFunctionRef(Loc, Name);
            return D ? new CallExpr(D, Args) : nullptr;
        }
        Decl *D = Actions.actOnVarRef(Loc, Name);
        return D ? new VarExpr(D) : nullptr;
    }
    Diags.report(Tok.getLocation(), diag::err_expected_token, "expression");
    advance();
    return nullptr;
}

Expr *Parser::parseFactor() {
    Expr *Left = parsePrimary();
    while (Tok.is(tok::star) || Tok.is(tok::slash) || Tok.is(tok::percent)) {
        BinOpExpr::Op Op;
        if (Tok.is(tok::star))       Op = BinOpExpr::Mul;
        else if (Tok.is(tok::slash)) Op = BinOpExpr::Div;
        else                         Op = BinOpExpr::Mod;
        advance();
        Expr *Right = parsePrimary();
        Left = new BinOpExpr(Op, Left, Right);
    }
    return Left;
}