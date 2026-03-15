
#ifndef LAMBDA_PARSER_H
#define LAMBDA_PARSER_H

#include "../ast_def.h"
#include "../error.h"

using NodeOrError = std::variant<std::shared_ptr<AstNode>, TypingError>;

bool is_error(const NodeOrError& n);
TypingError get_error(const NodeOrError& n);
std::shared_ptr<AstNode> unwrap(const NodeOrError& n);

NodeOrError parse(const std::string& source);

#endif