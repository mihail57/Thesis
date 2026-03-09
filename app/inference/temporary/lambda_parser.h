
#include "../ast_def.h"
#include "../error.h"

enum class TokenKind {
    DATA, IDENTIFIER, LAMBDA, DOT, LET, EQUAL, IN, IF, THEN, ELSE, FIX, LPAREN, RPAREN, ERROR, EOF_T
};

struct Token {
    TokenKind kind;
    SourceLoc loc;
    std::shared_ptr<Type> type; // Только для констант
    std::string_view data;

    Token(TokenKind kind, std::string_view data, const SourceLoc& loc) : Token(kind, nullptr, data, loc) {};
    Token(TokenKind kind, std::shared_ptr<Type> type, std::string_view data, const SourceLoc& loc) : kind(kind), data(data), type(type), loc(loc) {};
};


/*

*/

class TokenBuffer {
    std::string_view data;
    int pos = 0; 
    Token current;

    enum class SymbolKind {
        TEXT, NUM, QUOTE, LPAREN, RPAREN, INDENT, LAMBDA, DOT, EQUAL, OP_SYMBOL, ERROR
    };

    struct Symbol {
        SymbolKind kind;
        char data;

        static SymbolKind from_char(char c) {
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

        Symbol() : Symbol('\0') {};
        Symbol(char c) : data(c), kind(from_char(c)) {};
    };

    Symbol NextSymbol() {
        return Symbol(data[pos]);
    }

    void SkipIndent() {
        while (pos < data.size() && NextSymbol().kind == SymbolKind::INDENT)
            pos++;
    }

    Token next_identifier() {
        auto start = pos;

        while (pos < data.size())
        {
            auto c = NextSymbol();
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

    Token next_quoted() {
        auto start = pos++;
        Symbol c;

        while (pos < data.size())
        {
            c = NextSymbol();
            if(c.kind == SymbolKind::QUOTE)
                break;

            pos++;
        }

        SourceLoc loc(start, pos + 1);
        
        auto text = data.substr(start + 1, pos - start - 1);

        if(pos == data.size() && c.kind != SymbolKind::QUOTE)
            return Token(TokenKind::ERROR, text, loc);

        pos++;
        
        return Token(TokenKind::DATA, make_const_type("Str"), text, loc);
    }

    Token next_num() {
        auto start = pos;

        while (pos < data.size())
        {
            auto c = NextSymbol();
            if(c.kind != SymbolKind::NUM)
                break;

            pos++;
        }

        SourceLoc loc(start, pos);
        
        auto num = data.substr(start, pos - start);
        
        return Token(TokenKind::DATA, make_const_type("Int"), num, loc);
    }

    Token next_op() {
        auto start = pos;

        while (pos < data.size())
        {
            auto c = NextSymbol();
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

    Token NextToken() {
        SkipIndent();

        if (pos >= data.size())
            return Token(TokenKind::EOF_T, "", SourceLoc(data.size() - 1, data.size()));

        auto c = NextSymbol();

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

public:
    TokenBuffer(const std::string& target) : data(target), pos(0), current(NextToken()) {};

    const Token& front() { return current; }
    void pop() { current = NextToken(); }
};


/*
expr = let | if_else | lambda | app | term
let = LET var EQUAL expr IN expr
if_else = IF expr THEN expr ELSE expr
lambda = LAMBDA var DOT expr
app = app term
term = const | var | LPAREN expr RPAREN | FIX expr
const = DATA
var = IDENTIFIER
*/


using NodeOrError = std::variant<std::shared_ptr<AstNode>, TypingError>;

bool is_error(const NodeOrError& n) { return std::holds_alternative<TypingError>(n); }
TypingError get_error(const NodeOrError& n) { return std::get<TypingError>(n); }
std::shared_ptr<AstNode> unwrap(const NodeOrError& n) { return std::get<std::shared_ptr<AstNode>>(n); }


bool expect(TokenBuffer& buffer, TokenKind t);
NodeOrError parse_expr(TokenBuffer& buffer);
NodeOrError parse_app_or_term(TokenBuffer& buffer);
NodeOrError parse_term(TokenBuffer& buffer);
NodeOrError parse_var(TokenBuffer& buffer);
NodeOrError parse_lambda(TokenBuffer& buffer);
NodeOrError parse_let(TokenBuffer& buffer);
NodeOrError parse_if_else(TokenBuffer& buffer);

NodeOrError parse(const std::string& source) {
    TokenBuffer lexer(source);
    auto result = parse_expr(lexer);

    if(is_error(result))
        return result;
    
    if(!expect(lexer, TokenKind::EOF_T))
        return TypingError{.text = "Syntax error", .at = lexer.front().loc};

    return result;
}

NodeOrError parse_expr(TokenBuffer& buffer) {
    auto head = buffer.front();
    switch (head.kind)
    {
        case TokenKind::LET:
            return parse_let(buffer);
        case TokenKind::IF:
            return parse_if_else(buffer);
        case TokenKind::LAMBDA:
            return parse_lambda(buffer);
        default:
            return parse_app_or_term(buffer);
    }
}

bool term_like(TokenBuffer& buffer) {
    auto head = buffer.front();
    return head.kind == TokenKind::DATA || head.kind == TokenKind::IDENTIFIER || head.kind == TokenKind::LPAREN;
}

NodeOrError parse_app_or_term(TokenBuffer& buffer) {
    auto result = parse_term(buffer);
    if(is_error(result))
        return result;
    auto loc = unwrap(result)->loc;

    while (term_like(buffer))
    {
        auto right = parse_term(buffer);
        if(is_error(right))
            return right;
        loc += unwrap(right)->loc;

        result = make_app_node(unwrap(result), unwrap(right), loc);
    }
    return result;
}

NodeOrError parse_term(TokenBuffer& buffer) {
    auto head = buffer.front();
    switch (head.kind) {    
        case TokenKind::DATA:
            buffer.pop();
            return make_const_node(std::string(head.data), head.type, head.loc);
        case TokenKind::IDENTIFIER:
            buffer.pop();
            return make_var_node(std::string(head.data), head.loc);
        case TokenKind::LPAREN:
        {
            auto lparen_loc = buffer.front().loc;
            buffer.pop();
            auto result = parse_expr(buffer);
            if(!expect(buffer, TokenKind::RPAREN))
                return TypingError{.text = "Syntax error: expected )", .at = buffer.front().loc};
            auto result_uw = unwrap(result);
            auto rparen_loc = buffer.front().loc;
            buffer.pop();
            result_uw->loc = lparen_loc + result_uw->loc + rparen_loc;
            return result_uw;
        }
        case TokenKind::FIX:
        {
            auto loc = buffer.front().loc;
            buffer.pop();
            auto fix_expr = parse_expr(buffer);
            if(is_error(fix_expr)) {
                return fix_expr;
            }
            auto fix_expr_uw = unwrap(fix_expr);
            return make_fix_node(fix_expr_uw, loc + fix_expr_uw->loc);
        }
        default:
            return TypingError{.text = "Syntax error: unexpected token", .at = buffer.front().loc};
    }
}

bool expect(TokenBuffer& buffer, TokenKind t) {
    return buffer.front().kind == t; 
}

NodeOrError parse_var(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::IDENTIFIER))
        return TypingError{.text = "Syntax error: expected identifier", .at = buffer.front().loc};

    auto _var = buffer.front();
    auto var = make_var_node(std::string(_var.data), _var.loc);
    buffer.pop();
    return var;
}

NodeOrError parse_lambda(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::LAMBDA))
        return TypingError{.text = "Syntax error: expected lambda", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto var = parse_var(buffer);
    if(is_error(var))
        return var;
    loc += unwrap(var)->loc;

    if(!expect(buffer, TokenKind::DOT))
        return TypingError{.text = "Syntax error: expected .", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto definition = parse_expr(buffer);
    if(is_error(definition))
        return definition;
    loc += unwrap(definition)->loc;

    return make_func_node(std::dynamic_pointer_cast<VarNode>(unwrap(var)), unwrap(definition), loc);
}

NodeOrError parse_let(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::LET))
        return TypingError{.text = "Syntax error: expected let", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto var = parse_var(buffer);
    if(is_error(var))
        return var;
    loc += unwrap(var)->loc;
    
    if(!expect(buffer, TokenKind::EQUAL))
        return TypingError{.text = "Syntax error: expected =", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto bind_expr = parse_expr(buffer);
    if(is_error(bind_expr))
        return bind_expr;
    loc += unwrap(bind_expr)->loc;

    if(!expect(buffer, TokenKind::IN))
        return TypingError{.text = "Syntax error: expected in", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto ret_expr = parse_expr(buffer);
    if(is_error(ret_expr))
        return ret_expr;
    loc += unwrap(ret_expr)->loc;

    return make_let_node(std::dynamic_pointer_cast<VarNode>(unwrap(var)), unwrap(bind_expr), unwrap(ret_expr), loc);
}

NodeOrError parse_if_else(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::IF))
        return TypingError{.text = "Syntax error: expected if", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto cond = parse_expr(buffer);
    if(is_error(cond))
        return cond;
    loc += unwrap(cond)->loc;
    
    if(!expect(buffer, TokenKind::THEN))
        return TypingError{.text = "Syntax error: expected then", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto true_expr = parse_expr(buffer);
    if(is_error(true_expr))
        return true_expr;
    loc += unwrap(true_expr)->loc;

    if(!expect(buffer, TokenKind::ELSE))
        return TypingError{.text = "Syntax error: expected else", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto false_expr = parse_expr(buffer);
    if(is_error(false_expr))
        return false_expr;
    loc += unwrap(false_expr)->loc;

    return make_branch_node(unwrap(cond), unwrap(true_expr), unwrap(false_expr), loc);
}