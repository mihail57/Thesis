
#include "type_scheme.h"

#include <sstream>
#include <unordered_map>


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
        for(const auto& binded: binded_type_vars) ss << "∀" << binded->to_str(false);
        ss << ". ";
    }
    ss << type_body->to_str(false);
    return ss.str();
}

PolyTypeScheme::~PolyTypeScheme() {};
    


MonoTypeScheme::MonoTypeScheme(Type::base_ptr_t& t) : TypeScheme(), type(t) {};


Type::base_ptr_t MonoTypeScheme::instantiate() const {
    return type;
}

std::string MonoTypeScheme::to_str() const {
    return type->to_str(false);
}

MonoTypeScheme::~MonoTypeScheme() {};