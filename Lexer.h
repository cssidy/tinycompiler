#pragma once
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"

namespace tinyjs {

    namespace diag {
        enum {
            err_unknown_char,
            err_sym_declared,
            err_sym_undeclared,
            err_expected_token,
        };
    } // namespace diag

    class DiagnosticsEngine {
        llvm::SourceMgr &SrcMgr;
        unsigned NumErrors = 0;
    public:
        DiagnosticsEngine(llvm::SourceMgr &SrcMgr) : SrcMgr(SrcMgr) {}

        unsigned numErrors() { return NumErrors; }

        template <typename... Args>
        void report(llvm::SMLoc Loc, unsigned DiagID, Args &&...Arguments) {
            static const char *Text[] = {
                "unknown character",
                "symbol '{0}' already declared",
                "symbol '{0}' not declared",
                "expected '{0}'",
            };
            std::string Msg = llvm::formatv(Text[DiagID], std::forward<Args>(Arguments)...).str();
            SrcMgr.PrintMessage(Loc, llvm::SourceMgr::DK_Error, Msg);
            ++NumErrors;
        }
    };

    namespace tok {
        enum TokenKind : unsigned short {
            unknown, eof, identifier, integer_literal,
            plus, minus, star, slash, percent,
            equalequal, notequal, less, greater, equal,
            lparen, rparen, lbrace, rbrace, semi, comma,
            kw_var, kw_function, kw_if, kw_else, kw_while,
            kw_return, kw_true, kw_false,
            kw_class, kw_extends, kw_virtual, kw_new,
            dot, string_literal,
        };
    } // namespace tok

    struct Token {
        tok::TokenKind Kind;
        const char *Ptr;
        size_t Length;

        bool is(tok::TokenKind K) const { return Kind == K; }
        llvm::SMLoc getLocation() const {
            return llvm::SMLoc::getFromPointer(Ptr);
        }
        llvm::StringRef getText() const {
            return llvm::StringRef(Ptr, Length);
        }
    };

    class Lexer {
        llvm::SourceMgr &SrcMgr;
        DiagnosticsEngine &Diags;
        const char *CurPtr;
        llvm::StringRef Buffer;
        llvm::StringMap<tok::TokenKind> Keywords;

        void skipWhitespaceAndComments();

    public:
        Lexer(llvm::SourceMgr &SrcMgr, DiagnosticsEngine &Diags);
        void next(Token &T);
    };

} // namespace tinyjs
