
#include "../ast_helpers.h"
#include "token_buffer.h"
#include "lambda_parser.h"

#include <algorithm>


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

    if(result.is_error())
        return result;
    
    if(!expect(lexer, TokenKind::EOF_T))
        return Error{.text = "Синтаксическая ошибка", .at = lexer.front().loc};

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
    if(result.is_error())
        return result;
    auto loc = result.unwrap()->loc;

    while (term_like(buffer))
    {
        auto right = parse_term(buffer);
        if(right.is_error())
            return right;
        loc += right.unwrap()->loc;

        result = upcast(make_app_node(result.unwrap(), right.unwrap(), loc));
    }
    return result;
}

NodeOrError parse_term(TokenBuffer& buffer) {
    auto head = buffer.front();
    switch (head.kind) {    
        case TokenKind::DATA:
            buffer.pop();
            return upcast(make_const_node(std::string(head.data), head.type, head.loc));
        case TokenKind::IDENTIFIER:
            buffer.pop();
            return upcast(make_var_node(std::string(head.data), head.loc));
        case TokenKind::LPAREN:
        {
            auto lparen_loc = buffer.front().loc;
            buffer.pop();
            auto result = parse_expr(buffer);
            if(!expect(buffer, TokenKind::RPAREN))
                return Error{.text = "Синтаксическая ошибка: ожидалось \')\'", .at = buffer.front().loc};
            auto result_uw = result.unwrap();
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
            if(fix_expr.is_error()) {
                return fix_expr;
            }
            auto fix_expr_uw = fix_expr.unwrap();
            return upcast(make_fix_node(fix_expr_uw, loc + fix_expr_uw->loc));
        }
        default:
            return Error{.text = "Синтаксическая ошибка: неожиданный токен", .at = buffer.front().loc};
    }
}

bool expect(TokenBuffer& buffer, TokenKind t) {
    return buffer.front().kind == t; 
}

NodeOrError parse_var(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::IDENTIFIER))
        return Error{.text = "Синтаксическая ошибка: ожидался идентификатор", .at = buffer.front().loc};

    auto _var = buffer.front();
    auto var = make_var_node(std::string(_var.data), _var.loc);
    buffer.pop();
    return upcast(var);
}

NodeOrError parse_lambda(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::LAMBDA))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'\\\'", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto var = parse_var(buffer);
    if(var.is_error())
        return var;
    loc += var.unwrap()->loc;

    if(!expect(buffer, TokenKind::DOT))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'.\'", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto definition = parse_expr(buffer);
    if(definition.is_error())
        return definition;
    loc += definition.unwrap()->loc;

    return upcast(make_func_node(std::dynamic_pointer_cast<VarNode>(var.unwrap()), definition.unwrap(), loc));
}

NodeOrError parse_let(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::LET))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'let\'", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto var = parse_var(buffer);
    if(var.is_error())
        return var;
    loc += var.unwrap()->loc;
    
    if(!expect(buffer, TokenKind::EQUAL))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'=\'", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto bind_expr = parse_expr(buffer);
    if(bind_expr.is_error())
        return bind_expr;
    loc += bind_expr.unwrap()->loc;

    if(!expect(buffer, TokenKind::IN))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'in\'", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto ret_expr = parse_expr(buffer);
    if(ret_expr.is_error())
        return ret_expr;
    loc += ret_expr.unwrap()->loc;

    return upcast(make_let_node(std::dynamic_pointer_cast<VarNode>(var.unwrap()), bind_expr.unwrap(), ret_expr.unwrap(), loc));
}

NodeOrError parse_if_else(TokenBuffer& buffer) {
    if(!expect(buffer, TokenKind::IF))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'if\'", .at = buffer.front().loc};
    auto loc = buffer.front().loc;
    buffer.pop();

    auto cond = parse_expr(buffer);
    if(cond.is_error())
        return cond;
    loc += cond.unwrap()->loc;
    
    if(!expect(buffer, TokenKind::THEN))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'then\'", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto true_expr = parse_expr(buffer);
    if(true_expr.is_error())
        return true_expr;
    loc += true_expr.unwrap()->loc;

    if(!expect(buffer, TokenKind::ELSE))
        return Error{.text = "Синтаксическая ошибка: ожидалось \'else\'", .at = buffer.front().loc};
    loc += buffer.front().loc;
    buffer.pop();

    auto false_expr = parse_expr(buffer);
    if(false_expr.is_error())
        return false_expr;
    loc += false_expr.unwrap()->loc;

    return upcast(make_branch_node(cond.unwrap(), true_expr.unwrap(), false_expr.unwrap(), loc));
}