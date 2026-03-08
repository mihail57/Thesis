
#include "error.h"
#include "hm_def/type.h"

using TypeOrError = std::variant<std::shared_ptr<Type>, TypingError>;

bool is_error(const TypeOrError& t) { return std::holds_alternative<TypingError>(t); }
TypingError get_error(const TypeOrError& t) { return std::get<TypingError>(t); }
Type::base_ptr unwrap(const TypeOrError& t) { return std::get<Type::base_ptr>(t); }