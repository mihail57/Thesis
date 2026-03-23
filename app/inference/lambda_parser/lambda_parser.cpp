
#include "../ast_helpers.h"
#include "lambda_parser.h"

#include <algorithm>


/*
expr = let | if_else | lambda | infix
let = LET var "=" expr IN expr
if_else = IF expr THEN expr ELSE expr
lambda = LAMBDA var DOT expr
infix = prefix (op prefix)*
prefix = ('!' | '-')* app
app = term term*
term = const | var | LPAREN expr RPAREN | FIX expr
const = DATA
var = IDENTIFIER
*/


bool expect(TokenBuffer& buffer, TokenKind t);
NodeOrError parse_expr(TokenBuffer& buffer);
NodeOrError parse_infix(TokenBuffer& buffer, int min_prec = 1);
NodeOrError parse_prefix(TokenBuffer& buffer);
NodeOrError parse_app(TokenBuffer& buffer);
NodeOrError parse_term(TokenBuffer& buffer);
NodeOrError parse_var(TokenBuffer& buffer);
NodeOrError parse_lambda(TokenBuffer& buffer);
NodeOrError parse_let(TokenBuffer& buffer);
NodeOrError parse_if_else(TokenBuffer& buffer);

bool is_binary_infix_op(const Token& token);
bool is_unary_prefix_op(const Token& token);
int get_op_precedence(const Token& token);

LambdaParser::LambdaParser(const std::string_view& expression)
    : lexer(expression) {}

NodeOrError LambdaParser::parse() {
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
            return parse_infix(buffer);
    }
}

bool term_start_like(TokenBuffer& buffer) {
    auto head = buffer.front();
    if(head.kind == TokenKind::DATA || head.kind == TokenKind::LPAREN || head.kind == TokenKind::FIX)
        return true;

    if(head.kind == TokenKind::IDENTIFIER && !is_binary_infix_op(head))
        return true;

    return false;
}

NodeOrError parse_app(TokenBuffer& buffer) {
    auto result = parse_term(buffer);
    if(result.is_error())
        return result;
    auto loc = result.unwrap()->loc;

    while (term_start_like(buffer))
    {
        auto right = parse_term(buffer);
        if(right.is_error())
            return right;
        loc += right.unwrap()->loc;

        result = upcast(make_app_node(result.unwrap(), right.unwrap(), loc));
    }
    return result;
}

NodeOrError parse_prefix(TokenBuffer& buffer) {
    auto head = buffer.front();
    if(!is_unary_prefix_op(head))
        return parse_app(buffer);

    auto op = make_var_node(std::string(head.data), head.loc);
    buffer.pop();

    // Унарные операторы могут быть в скобках: (!) или (-)
    if(expect(buffer, TokenKind::RPAREN))
        return upcast(op);

    auto right = parse_prefix(buffer);
    if(right.is_error())
        return right;

    auto right_uw = right.unwrap();

    // Унарный минус преобразуется в бинарный: -x ==> (-) 0 x
    if(op->var == "-") {
        auto zero = make_const_node("0", "Int", op->loc);
        auto partial = make_app_node(op, zero, op->loc + zero->loc);
        auto loc = partial->loc + right_uw->loc;
        return upcast(make_app_node(partial, right_uw, loc));
    }

    auto loc = op->loc + right_uw->loc;
    return upcast(make_app_node(op, right_uw, loc));
}

NodeOrError parse_infix(TokenBuffer& buffer, int min_prec) {
    auto left = parse_prefix(buffer);
    if(left.is_error())
        return left;

    while (true)
    {
        auto head = buffer.front();
        if(!is_binary_infix_op(head))
            break;

        auto prec = get_op_precedence(head);
        if(prec < min_prec)
            break;

        auto op = make_var_node(std::string(head.data), head.loc);
        buffer.pop();

        auto right = parse_infix(buffer, prec + 1);
        if(right.is_error())
            return right;

        auto left_uw = left.unwrap();
        auto right_uw = right.unwrap();
        auto loc = left_uw->loc + op->loc + right_uw->loc;
        auto partial = make_app_node(op, left_uw, op->loc + left_uw->loc);
        left = upcast(make_app_node(partial, right_uw, loc));
    }

    return left;
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
            else if(result.is_error())
                return result;
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

bool is_binary_infix_op(const Token& token) {
    if(token.kind != TokenKind::IDENTIFIER)
        return false;

    return token.data == "||"
        || token.data == "&&"
        || token.data == "="
        || token.data == "+"
        || token.data == "-"
        || token.data == "*"
        || token.data == "/";
}

bool is_unary_prefix_op(const Token& token) {
    return token.kind == TokenKind::IDENTIFIER && (token.data == "!" || token.data == "-");
}

int get_op_precedence(const Token& token) {
    if(token.data == "||") return 1;
    if(token.data == "&&") return 2;
    if(token.data == "=") return 3;
    if(token.data == "+" || token.data == "-") return 4;
    if(token.data == "*" || token.data == "/") return 5;
    return 0;
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
    
    auto head = buffer.front();
    if(!(head.kind == TokenKind::IDENTIFIER && head.data == "="))
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