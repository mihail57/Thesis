
#include "hm_def/substitution.h"
#include "error.h"

using SubstitutionOrError = std::variant<Substitution, TypingError>;

bool is_error(const SubstitutionOrError& t) { return std::holds_alternative<TypingError>(t); }
Substitution unwrap(const SubstitutionOrError& t) { return std::get<Substitution>(t); }