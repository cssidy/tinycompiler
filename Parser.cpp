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
    DeclList Functions;
    while (!Tok.is(tok::eof)) {
        if (Tok.is(tok::kw_function)) {
            if (FunctionDecl *FD = parseFunction())
                Functions.push_back(FD);
        } else {
            Diags.report(Tok.getLocation(), diag::err_expected_token, "function");
            advance();
        }
    }
    return Functions;
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

    if (Tok.is(tok::equal)) {
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

// Expression parsing: comparison > additive > term > factor

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