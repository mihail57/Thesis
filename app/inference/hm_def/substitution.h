
#include "inference_context.h"

struct Substitution {
    std::map<std::shared_ptr<TypeVar>, std::shared_ptr<Type>> subs;

    std::shared_ptr<Type> apply_to(const std::shared_ptr<Type>& to) const {
        if (subs.empty()) return to;
        
        std::shared_ptr<Type> result = to;
        for (const auto& pair : subs) {
            auto target = pair.first;
            result = result->make_sub(target, pair.second);
        }
        return result;
    }

    TypingContext apply_to(const TypingContext& to) const {        
        TypingContext new_ctx;

        for(const auto& pair : to) {
            auto polytype = std::dynamic_pointer_cast<PolyTypeScheme>(pair.second);

            if(polytype) {
                std::vector<std::shared_ptr<TypeVar>> vars(polytype->binded_type_vars);
                for(auto& var : vars)
                    var = std::dynamic_pointer_cast<TypeVar>(this->apply_to(var));
                
                auto body = this->apply_to(polytype->type_body);

                auto new_scheme = std::shared_ptr<TypeScheme>(new PolyTypeScheme(vars, body));
                new_ctx.set(pair.first, new_scheme);
            }
            else {
                auto monotype = std::dynamic_pointer_cast<MonoTypeScheme>(pair.second);
                auto body = this->apply_to(monotype->type);

                auto new_scheme = std::shared_ptr<TypeScheme>(new MonoTypeScheme(body));
                new_ctx.set(pair.first, new_scheme);
            }
        }

        return new_ctx;
    }

    Substitution operator+(const Substitution& other) const {
        Substitution result;
        
        // Apply 'this' substitution to the values of 'other'
        for (const auto& pair : other.subs) {
            result.subs[pair.first] = this->apply_to(pair.second);
        }
        
        // Add mappings from 'this' that are not in 'other'
        for (const auto& pair : this->subs) {
            if (result.subs.find(pair.first) == result.subs.end()) {
                result.subs[pair.first] = pair.second;
            }
        }
        
        return result;
    }    

    static Substitution make_identity() {
        return Substitution {};
    }
};

static bool occurs_check(const std::shared_ptr<TypeVar>& var, const std::shared_ptr<Type>& type) {
    if (auto t_var = std::dynamic_pointer_cast<TypeVar>(type)) {
        return t_var->name == var->name;
    } else if (auto t_func = std::dynamic_pointer_cast<FuncType>(type)) {
        return occurs_check(var, t_func->arg_type) || occurs_check(var, t_func->ret_type);
    } else if (auto t_cons = std::dynamic_pointer_cast<TypeConstructor>(type)) {
        for(const auto& arg: t_cons->args) {
            if(occurs_check(var, arg)) return true;
        }
    }
    return false;
}