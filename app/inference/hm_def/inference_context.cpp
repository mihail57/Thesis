
#include "inference_context.h"


bool TypingContext::has(const std::string& var) {
    return _context.find(var) != end();
}

TypingContext::context_pair& TypingContext::get(const std::string& var) {
    return *_context.find(var);
}

void TypingContext::set(const context_pair::first_type& var, const context_pair::second_type& scheme) {
    _context.insert(std::make_pair(var, scheme));
}

TypingContext TypingContext::with(const context_pair::first_type& var, const context_pair::second_type& scheme) {
    auto new_ctx = TypingContext(*this);
    new_ctx.set(var, scheme);
    return new_ctx;
}

std::shared_ptr<TypeScheme> TypingContext::generalize(std::shared_ptr<Type>& type) {
    std::vector<std::shared_ptr<TypeVar>> type_vars;
    type->get_vars(type_vars);

    std::vector<std::shared_ptr<TypeVar>> ctx_free_vars;
    ctx_free_vars.reserve(_context.size());

    for(const auto& ctx_pair: *this) {
        std::vector<std::shared_ptr<TypeVar>> ctx_term_fvs;
        if(auto mono = std::dynamic_pointer_cast<MonoTypeScheme>(ctx_pair.second))
            mono->type->get_vars(ctx_term_fvs);
        else if(auto poly = std::dynamic_pointer_cast<PolyTypeScheme>(ctx_pair.second)){
            std::vector<std::shared_ptr<TypeVar>> ctx_term_all_tvs;
            poly->type_body->get_vars(ctx_term_all_tvs);

            ctx_term_fvs.reserve(ctx_term_all_tvs.size());
            for(const auto& ctx_term_tv : ctx_term_all_tvs) {
                if(!std::any_of(
                    poly->binded_type_vars.cbegin(),
                    poly->binded_type_vars.cend(),
                    [ctx_term_tv](const TypeVar::ptr& binded_tv) { return ctx_term_tv->name == binded_tv->name; }
                ))
                    ctx_term_fvs.push_back(ctx_term_tv);
            }
        }
        
        for(const auto& ctx_term_fv: ctx_term_fvs)
            if(!std::any_of(
                ctx_free_vars.cbegin(), ctx_free_vars.cend(),
                [ctx_term_fv](const TypeVar::ptr& ctx_fv) { return ctx_term_fv->name == ctx_fv->name; }
            ))
                ctx_free_vars.push_back(ctx_term_fv);
    }

    std::vector<std::shared_ptr<TypeVar>> new_binded;
    new_binded.reserve(type_vars.size());

    for(const auto& tv : type_vars) {
        if(!std::any_of(
            ctx_free_vars.cbegin(), ctx_free_vars.cend(),
            [tv](const TypeVar::ptr& s) { return s->name == tv->name; }
        ))
            new_binded.push_back(tv);
    }

    return std::shared_ptr<TypeScheme>(
        new PolyTypeScheme(new_binded, type)
    );
}