
#ifndef SUBSTITUTION_OR_ERROR_H
#define SUBSTITUTION_OR_ERROR_H

#include "hm_def/substitution.h"
#include "error.h"

using SubstitutionOrError = std::variant<Substitution, TypingError>;

bool is_error(const SubstitutionOrError& t);
Substitution unwrap(const SubstitutionOrError& t);

#endif