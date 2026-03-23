
#include "inference_context.h"

#include <unordered_set>
#include <algorithm>


bool InferenceContext::has(const std::string& var) {
    return _context.find(var) != end();
}

InferenceContext::context_pair& InferenceContext::get(const std::string& var) {
    return *_context.find(var);
}

void InferenceContext::set(const context_pair::first_type& var, const context_pair::second_type& scheme) {
    _context.insert(std::make_pair(var, scheme));
}

InferenceContext InferenceContext::with(const context_pair::first_type& var, const context_pair::second_type& scheme) {
    auto new_ctx = InferenceContext(*this);
    new_ctx.set(var, scheme);
    return new_ctx;
}

TypeScheme::ptr_t InferenceContext::generalize(Type::base_ptr_t& type) {
    // 1. FV(t)
    std::vector<TypeVar::ptr_t> type_vars;
    type->get_vars(type_vars);

    // 2. FV(Gamma)
    std::unordered_set<std::string> ctx_free_vars;

    for(const auto& ctx_pair: *this) {
        std::vector<TypeVar::ptr_t> ctx_term_fvs;
        if(auto mono = std::dynamic_pointer_cast<MonoTypeScheme>(ctx_pair.second))
            mono->type->get_vars(ctx_term_fvs);
        else if(auto poly = std::dynamic_pointer_cast<PolyTypeScheme>(ctx_pair.second)){
            std::vector<TypeVar::ptr_t> ctx_term_all_tvs;
            poly->type_body->get_vars(ctx_term_all_tvs);

            ctx_term_fvs.reserve(ctx_term_all_tvs.size());
            for(const auto& ctx_term_tv : ctx_term_all_tvs) {
                if(!std::any_of(
                    poly->binded_type_vars.cbegin(),
                    poly->binded_type_vars.cend(),
                    [ctx_term_tv](const TypeVar::ptr_t& binded_tv) { return ctx_term_tv->name == binded_tv->name; }
                ))
                    ctx_term_fvs.push_back(ctx_term_tv);
            }
        }
        
        for(const auto& ctx_term_fv: ctx_term_fvs)
            ctx_free_vars.insert(ctx_term_fv->name);
    }

    // 3. FV(t) - FV(Gamma)
    std::vector<TypeVar::ptr_t> new_binded;
    new_binded.reserve(type_vars.size());

    for(const auto& tv : type_vars) {
        if(ctx_free_vars.find(tv->name) != ctx_free_vars.end())
            new_binded.push_back(tv);
    }

    return PolyTypeScheme::ptr_t(new PolyTypeScheme(new_binded, type));
}