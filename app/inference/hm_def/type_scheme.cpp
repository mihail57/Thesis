
#include "type_scheme.h"

#include <sstream>
#include <unordered_map>
#include <algorithm>


TypeScheme::~TypeScheme() {};


PolyTypeScheme::PolyTypeScheme(const std::vector<TypeVar::ptr_t>& binded, const Type::base_ptr_t& body)
: TypeScheme(), binded_type_vars(binded), type_body(body) {};


Type::base_ptr_t PolyTypeScheme::instantiate() const {
    std::unordered_map<std::string, TypeVar::ptr_t> replacements;
    replacements.reserve(binded_type_vars.size());
    for (const auto& binded : binded_type_vars) {
        replacements.emplace(binded->name, TypeVar::ptr_t{});
    }

    return type_body->replace_vars(replacements);
}

std::string PolyTypeScheme::to_str() const {
    std::ostringstream ss;
    if(binded_type_vars.size() > 0) {
        for(const auto& binded: binded_type_vars) ss << "AA" << binded->to_str(false);
        ss << ". ";
    }
    ss << type_body->to_str(false);
    return ss.str();
}

void PolyTypeScheme::get_free_vars(std::vector<TypeVar::ptr_t>& vars) const {
    std::vector<TypeVar::ptr_t> ctx_term_all_tvs;
    type_body->get_vars(ctx_term_all_tvs);

    vars.reserve(ctx_term_all_tvs.size());
    for(const auto& ctx_term_tv : ctx_term_all_tvs) {
        if(!std::any_of(
            binded_type_vars.cbegin(),
            binded_type_vars.cend(),
            [ctx_term_tv](const TypeVar::ptr_t& binded_tv) { return ctx_term_tv->name == binded_tv->name; }
        ))
            vars.push_back(ctx_term_tv);
    }
}

PolyTypeScheme::~PolyTypeScheme() {};
    


MonoTypeScheme::MonoTypeScheme(Type::base_ptr_t& t) : TypeScheme(), type(t) {};


Type::base_ptr_t MonoTypeScheme::instantiate() const {
    return type;
}

std::string MonoTypeScheme::to_str() const {
    return type->to_str(false);
}

void MonoTypeScheme::get_free_vars(std::vector<TypeVar::ptr_t>& vars) const {
    type->get_vars(vars);
}

MonoTypeScheme::~MonoTypeScheme() {};