#pragma once
#include "Lexer.h"
#include "AST.h"
#include "Sema.h"

namespace tinyjs {

    class Parser {
        Lexer &Lex;
        Sema &Actions;
        DiagnosticsEngine &Diags;
        Token Tok;

        bool expect(tok::TokenKind Kind);
        bool consume(tok::TokenKind Kind);
        void advance();

        // Parsing methods
        FunctionDecl *parseFunction();
        bool parseParamList(DeclList &Params);
        bool parseBlock(StmtList &Stmts);
        bool parseStatement(StmtList &Stmts);
        bool parseVarDecl(StmtList &Stmts);
        bool parseIfStmt(StmtList &Stmts);
        bool parseWhileStmt(StmtList &Stmts);
        bool parseReturnStmt(StmtList &Stmts);
        bool parseAssignOrCall(StmtList &Stmts);
        Expr *parseExpr();
        Expr *parseComparison();
        Expr *parseTerm();
        Expr *parseFactor();
        Expr *parsePrimary();

    public:
        Parser(Lexer &Lex, Sema &Actions, DiagnosticsEngine &Diags);
        DeclList parse();
    };

} // namespace tinyjs