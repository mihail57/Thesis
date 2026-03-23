
#include "substitution.h"


Type::base_ptr_t Substitution::apply_to(const Type::base_ptr_t& to) const {
    if (subs.empty()) return to;
    
    Type::base_ptr_t result = to;
    for (const auto& pair : subs) {
        auto target = pair.first;
        result = result->make_sub(target, pair.second);
    }
    return result;
}

InferenceContext Substitution::apply_to(const InferenceContext& to) const {        
    InferenceContext new_ctx;

    for(const auto& pair : to) {
        auto polytype = std::dynamic_pointer_cast<PolyTypeScheme>(pair.second);

        if(polytype) {
            std::vector<TypeVar::ptr_t> vars(polytype->binded_type_vars);
            for(auto& var : vars)
                var = std::dynamic_pointer_cast<TypeVar>(this->apply_to(var));
            
            auto body = this->apply_to(polytype->type_body);

            auto new_scheme = PolyTypeScheme::ptr_t(new PolyTypeScheme(vars, body));
            new_ctx.set(pair.first, new_scheme);
        }
        else {
            auto monotype = std::dynamic_pointer_cast<MonoTypeScheme>(pair.second);
            auto body = this->apply_to(monotype->type);

            auto new_scheme = MonoTypeScheme::ptr_t(new MonoTypeScheme(body));
            new_ctx.set(pair.first, new_scheme);
        }
    }

    return new_ctx;
}

Substitution Substitution::operator+(const Substitution& other) const {
    Substitution result;
    
    for (const auto& pair : other.subs) {
        result.subs[pair.first] = this->apply_to(pair.second);
    }
    
    for (const auto& pair : this->subs) {
        if (result.subs.find(pair.first) == result.subs.end()) {
            result.subs[pair.first] = pair.second;
        }
    }
    
    return result;
}    

Substitution Substitution::make_identity() {
    return Substitution {};
}