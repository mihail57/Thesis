
#ifndef LAMBDA_PARSER_H
#define LAMBDA_PARSER_H

#include "ast_def.h"
#include "../value_or_error.hpp"
#include <variant>

using NodeOrError = ValueOrError<std::shared_ptr<AstNode>>;

NodeOrError parse(const std::string& source);

#endif