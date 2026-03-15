
#ifndef TYPE_SCHEME_H
#define TYPE_SCHEME_H

#include "type.h"

struct TypeScheme {
    virtual std::shared_ptr<Type> instantiate() const = 0;
    virtual ~TypeScheme() {};
};

struct PolyTypeScheme : TypeScheme {
    std::vector<std::shared_ptr<TypeVar>> binded_type_vars;
    std::shared_ptr<Type> type_body;

    PolyTypeScheme(const std::vector<std::shared_ptr<TypeVar>>& binded, const std::shared_ptr<Type>& body)
    : TypeScheme(), binded_type_vars(binded), type_body(body) {};
   

    virtual std::shared_ptr<Type> instantiate() const {        
        auto new_binded = std::vector<std::shared_ptr<TypeVar>>(binded_type_vars.size());
        auto new_type_body = type_body->replace_vars(binded_type_vars, new_binded);

        return std::shared_ptr<Type>(new_type_body);
    }

    ~PolyTypeScheme() {};
};

struct MonoTypeScheme : TypeScheme {
    std::shared_ptr<Type> type;

    MonoTypeScheme(std::shared_ptr<Type>& t) : TypeScheme(), type(t) {};
   

    virtual std::shared_ptr<Type> instantiate() const {
        return type;
    }
    ~MonoTypeScheme() {};
};

#endif