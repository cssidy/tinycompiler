#include "Lexer.h"

using namespace tinyjs;

Lexer::Lexer(llvm::SourceMgr &SrcMgr, DiagnosticsEngine &Diags)
    : SrcMgr(SrcMgr), Diags(Diags) {
    unsigned BufID = SrcMgr.getMainFileID();
    Buffer = SrcMgr.getMemoryBuffer(BufID)->getBuffer();
    CurPtr = Buffer.begin();

    Keywords["var"]      = tok::kw_var;
    Keywords["function"] = tok::kw_function;
    Keywords["if"]       = tok::kw_if;
    Keywords["else"]     = tok::kw_else;
    Keywords["while"]    = tok::kw_while;
    Keywords["return"]   = tok::kw_return;
    Keywords["true"]     = tok::kw_true;
    Keywords["false"]    = tok::kw_false;
    Keywords["class"]    = tok::kw_class;
    Keywords["extends"]  = tok::kw_extends;
    Keywords["virtual"]  = tok::kw_virtual;
    Keywords["new"]      = tok::kw_new;
}

void Lexer::skipWhitespaceAndComments() {
    while (CurPtr < Buffer.end()) {
        if (isspace(*CurPtr)) {
            ++CurPtr;
        } else if (*CurPtr == '/' && *(CurPtr+1) == '/') {
            while (CurPtr < Buffer.end() && *CurPtr != '\n')
                ++CurPtr;
        } else {
            break;
        }
    }
}

void Lexer::next(Token &T) {
    skipWhitespaceAndComments();

    auto form = [&](const char *End, tok::TokenKind Kind) {
        T.Kind = Kind; T.Ptr = CurPtr; T.Length = End - CurPtr; CurPtr = End;
    };

    if (CurPtr >= Buffer.end()) {
        form(CurPtr, tok::eof);
        return;
    }

    const char *Start = CurPtr;

    // identifier or keyword
    if (isalpha(*CurPtr) || *CurPtr == '_') {
        while (CurPtr < Buffer.end() && (isalnum(*CurPtr) || *CurPtr == '_'))
            ++CurPtr;
        llvm::StringRef Name(Start, CurPtr - Start);
        auto It = Keywords.find(Name);
        if (It != Keywords.end())
            form(CurPtr, It->second);
        else {
            T.Kind = tok::identifier;
            T.Ptr = Start;
            T.Length = CurPtr - Start;
        }
        return;
    }

    // string literal  "..."
    if (*CurPtr == '"') {
        ++CurPtr;
        while (CurPtr < Buffer.end() && *CurPtr != '"')
            ++CurPtr;
        T.Kind = tok::string_literal;
        T.Ptr = Start + 1;
        T.Length = CurPtr - Start - 1;
        if (CurPtr < Buffer.end()) ++CurPtr; // skip closing "
        return;
    }

    // integer literal
    if (isdigit(*CurPtr)) {
        while (CurPtr < Buffer.end() && isdigit(*CurPtr))
            ++CurPtr;
        T.Kind = tok::integer_literal;
        T.Ptr = Start;
        T.Length = CurPtr - Start;
        return;
    }

    // punctuators
    switch (*CurPtr) {
    case '+': form(CurPtr+1, tok::plus);    return;
    case '-': form(CurPtr+1, tok::minus);   return;
    case '*': form(CurPtr+1, tok::star);    return;
    case '/': form(CurPtr+1, tok::slash);   return;
    case '%': form(CurPtr+1, tok::percent); return;
    case '(': form(CurPtr+1, tok::lparen);  return;
    case ')': form(CurPtr+1, tok::rparen);  return;
    case '{': form(CurPtr+1, tok::lbrace);  return;
    case '}': form(CurPtr+1, tok::rbrace);  return;
    case ';': form(CurPtr+1, tok::semi);    return;
    case ',': form(CurPtr+1, tok::comma);   return;
    case '.': form(CurPtr+1, tok::dot);     return;
    case '<': form(CurPtr+1, tok::less);    return;
    case '>': form(CurPtr+1, tok::greater); return;
    case '=':
        if (*(CurPtr+1) == '=') form(CurPtr+2, tok::equalequal);
        else                     form(CurPtr+1, tok::equal);
        return;
    case '!':
        if (*(CurPtr+1) == '=') form(CurPtr+2, tok::notequal);
        else {
            Diags.report(llvm::SMLoc::getFromPointer(CurPtr), diag::err_unknown_char);
            form(CurPtr+1, tok::unknown);
        }
        return;
    default:
        Diags.report(llvm::SMLoc::getFromPointer(CurPtr), diag::err_unknown_char);
        form(CurPtr+1, tok::unknown);
        return;
    }
}
