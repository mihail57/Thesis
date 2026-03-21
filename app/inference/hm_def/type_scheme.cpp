
#include "type_scheme.h"

#include <sstream>


TypeScheme::~TypeScheme() {};


PolyTypeScheme::PolyTypeScheme(const std::vector<std::shared_ptr<TypeVar>>& binded, const std::shared_ptr<Type>& body)
: TypeScheme(), binded_type_vars(binded), type_body(body) {};


std::shared_ptr<Type> PolyTypeScheme::instantiate() const {        
    auto new_binded = std::vector<std::shared_ptr<TypeVar>>(binded_type_vars.size());
    auto new_type_body = type_body->replace_vars(binded_type_vars, new_binded);

    return std::shared_ptr<Type>(new_type_body);
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
    


MonoTypeScheme::MonoTypeScheme(std::shared_ptr<Type>& t) : TypeScheme(), type(t) {};


std::shared_ptr<Type> MonoTypeScheme::instantiate() const {
    return type;
}

std::string MonoTypeScheme::to_str() const {
    return type->to_str(false);
}

MonoTypeScheme::~MonoTypeScheme() {};