
#include "token_buffer.h"

#include <algorithm>


Token::Token(TokenKind kind, std::string_view data, const SourceLoc& loc) : Token(kind, "", data, loc) {}
Token::Token(TokenKind kind, std::string type, std::string_view data, const SourceLoc& loc) 
    : kind(kind), data(data), type(type), loc(loc) {}



TokenBuffer::SymbolKind TokenBuffer::Symbol::from_char(char c) {
    if(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
        return SymbolKind::TEXT;
    else if(c == ' ' || c == '\n' || c == '\r' || c == '\t')
        return SymbolKind::INDENT;
    else if(c >= '0' && c <= '9') return SymbolKind::NUM;
    else if(c == '\\') return SymbolKind::LAMBDA;
    else if(c == '.') return SymbolKind::DOT;
    else if(c == '=') return SymbolKind::EQUAL;
    else if(c == '(') return SymbolKind::LPAREN;
    else if(c == ')') return SymbolKind::RPAREN;
    else if(c == '"' || c == '\'') return SymbolKind::QUOTE;
    else if(c == '[' || c == ']' || c == ':') return SymbolKind::OP_SYMBOL;
    else return SymbolKind::ERROR;
}

TokenBuffer::Symbol::Symbol() : Symbol('\0') {};
TokenBuffer::Symbol::Symbol(char c) : data(c), kind(from_char(c)) {};

TokenBuffer::Symbol TokenBuffer::next_symbol() {
    return Symbol(data[pos]);
}

void TokenBuffer::skip_indent() {
    while (pos < data.size() && next_symbol().kind == SymbolKind::INDENT)
        pos++;
}

Token TokenBuffer::next_identifier() {
    auto start = pos;

    while (pos < data.size())
    {
        auto c = next_symbol();
        if(c.kind != SymbolKind::TEXT && c.kind != SymbolKind::NUM)
            break;

        pos++;
    }

    SourceLoc loc(start, pos);
    
    auto identifier = data.substr(start, pos - start);

    std::string id_ignore_case;
    id_ignore_case.resize(identifier.length());
    std::transform(identifier.begin(), identifier.end(), id_ignore_case.begin(), tolower);

    if(id_ignore_case == "if") return Token(TokenKind::IF, identifier, loc);
    if(id_ignore_case == "then") return Token(TokenKind::THEN, identifier, loc);
    if(id_ignore_case == "else") return Token(TokenKind::ELSE, identifier, loc);
    if(id_ignore_case == "let") return Token(TokenKind::LET, identifier, loc);
    if(id_ignore_case == "in") return Token(TokenKind::IN, identifier, loc);
    if(id_ignore_case == "fix") return Token(TokenKind::FIX, identifier, loc);
    
    return Token(TokenKind::IDENTIFIER, identifier, loc);
}

Token TokenBuffer::next_quoted() {
    auto start = pos++;
    Symbol c;

    while (pos < data.size())
    {
        c = next_symbol();
        if(c.kind == SymbolKind::QUOTE)
            break;

        pos++;
    }

    SourceLoc loc(start, pos + 1);
    
    auto text = data.substr(start + 1, pos - start - 1);

    if(pos == data.size() && c.kind != SymbolKind::QUOTE)
        return Token(TokenKind::ERROR, text, loc);

    pos++;
    
    return Token(TokenKind::DATA, "Str", text, loc);
}

Token TokenBuffer::next_num() {
    auto start = pos;

    while (pos < data.size())
    {
        auto c = next_symbol();
        if(c.kind != SymbolKind::NUM)
            break;

        pos++;
    }

    SourceLoc loc(start, pos);
    
    auto num = data.substr(start, pos - start);
    
    return Token(TokenKind::DATA, "Int", num, loc);
}

Token TokenBuffer::next_op() {
    auto start = pos;

    while (pos < data.size())
    {
        auto c = next_symbol();
        if(c.kind != SymbolKind::OP_SYMBOL)
            break;

        pos++;
    }

    SourceLoc loc(start, pos);
    
    auto op = data.substr(start, pos - start);

    if(op == "::") return Token(TokenKind::IDENTIFIER, op, loc);
    if(op == "[]") return Token(TokenKind::IDENTIFIER, op, loc);
    
    return Token(TokenKind::ERROR, op, loc);
}

Token TokenBuffer::next_token() {
    skip_indent();

    if (pos >= data.size())
        return Token(TokenKind::EOF_T, "", SourceLoc(data.size(), data.size() + 1));

    auto c = next_symbol();

    switch (c.kind)
    {
        case SymbolKind::TEXT      : return next_identifier();
        case SymbolKind::QUOTE     : return next_quoted();
        case SymbolKind::NUM       : return next_num();
        case SymbolKind::OP_SYMBOL : return next_op();
        default: break;
    }
    
    SourceLoc loc(pos, pos + 1);
    pos++;
    switch (c.kind) {
        case SymbolKind::LAMBDA : return Token(TokenKind::LAMBDA, "λ", loc);
        case SymbolKind::EQUAL  : return Token(TokenKind::EQUAL, "=", loc);
        case SymbolKind::DOT    : return Token(TokenKind::DOT, ".", loc);
        case SymbolKind::LPAREN : return Token(TokenKind::LPAREN, "(", loc);
        case SymbolKind::RPAREN : return Token(TokenKind::RPAREN, ")", loc);
        default: return Token(TokenKind::ERROR, c.data + "", loc);
    }
}

TokenBuffer::TokenBuffer(const std::string& target) 
    : data(target), pos(0), current(next_token()) {}
    
const Token& TokenBuffer::front() { return current; }
void TokenBuffer::pop() { current = next_token(); }