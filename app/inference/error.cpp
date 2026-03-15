
#include "type_or_error.h"
#include "substitution_or_error.h"

bool is_error(const TypeOrError& t) { return std::holds_alternative<TypingError>(t); }
TypingError get_error(const TypeOrError& t) { return std::get<TypingError>(t); }
Type::base_ptr unwrap(const TypeOrError& t) { return std::get<Type::base_ptr>(t); }


bool is_error(const SubstitutionOrError& t) { return std::holds_alternative<TypingError>(t); }
Substitution unwrap(const SubstitutionOrError& t) { return std::get<Substitution>(t); }