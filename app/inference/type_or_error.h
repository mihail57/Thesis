
#ifndef TYPE_OR_ERROR_H
#define TYPE_OR_ERROR_H

#include "error.h"
#include "hm_def/type.h"

using TypeOrError = std::variant<std::shared_ptr<Type>, TypingError>;

bool is_error(const TypeOrError& t);
TypingError get_error(const TypeOrError& t);
Type::base_ptr unwrap(const TypeOrError& t);

#endif