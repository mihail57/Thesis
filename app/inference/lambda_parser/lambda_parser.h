
#ifndef LAMBDA_PARSER_H
#define LAMBDA_PARSER_H

#include "ast_def.h"
#include "../value_or_error.hpp"
#include "token_buffer.h"

using NodeOrError = ValueOrError<AstNode::ptr_t>;

struct LambdaParser {
    TokenBuffer lexer;

    LambdaParser(const std::string_view& expression);

    NodeOrError parse();
};

#endif