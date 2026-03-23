
#include "source_loc.h"

#include <string>

enum class TokenKind {
    DATA, IDENTIFIER, LAMBDA, DOT, LET, IN, IF, THEN, ELSE, FIX, LPAREN, RPAREN, ERROR, EOF_T
};

struct Token {
    TokenKind kind;
    SourceLoc loc;
    std::string type; // Только для констант
    std::string_view data;

    Token(TokenKind kind, std::string_view data, const SourceLoc& loc);
    Token(TokenKind kind, std::string type, std::string_view data, const SourceLoc& loc);
};

class TokenBuffer {
    std::string_view data;
    int pos = 0; 
    Token current;

    enum class SymbolKind {
        TEXT, NUM, QUOTE, LPAREN, RPAREN, INDENT, LAMBDA, DOT, OP_SYMBOL, ERROR
    };

    struct Symbol {
        SymbolKind kind;
        char data;

        static SymbolKind from_char(char c);

        Symbol();
        Symbol(char c);
    };

    Symbol next_symbol();
    void skip_indent();
    Token next_identifier();
    Token next_quoted();
    Token next_num();
    Token next_op();
    Token next_token();

public:
    TokenBuffer(const std::string_view& target);

    const Token& front();
    void pop();
};