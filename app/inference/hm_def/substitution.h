
#include "inference_context.h"

struct Substitution {
    std::map<TypeVar::ptr_t, Type::base_ptr_t> subs;

    Type::base_ptr_t apply_to(const Type::base_ptr_t& to) const;
    InferenceContext apply_to(const InferenceContext& to) const;
    Substitution operator+(const Substitution& other) const;
    std::string to_str() const;

    static Substitution make_identity();
};