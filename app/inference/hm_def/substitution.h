
#include "inference_context.h"

struct Substitution {
    std::map<std::shared_ptr<TypeVar>, std::shared_ptr<Type>> subs;

    std::shared_ptr<Type> apply_to(const std::shared_ptr<Type>& to) const;
    TypingContext apply_to(const TypingContext& to) const;
    Substitution operator+(const Substitution& other) const;

    static Substitution make_identity();
};