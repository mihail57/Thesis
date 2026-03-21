
#ifndef TYPE_SCHEME_H
#define TYPE_SCHEME_H

#include "type.h"

struct TypeScheme {
    virtual std::shared_ptr<Type> instantiate() const = 0;
    virtual std::string to_str() const = 0;
    virtual ~TypeScheme();
};

struct PolyTypeScheme : TypeScheme {
    std::vector<std::shared_ptr<TypeVar>> binded_type_vars;
    std::shared_ptr<Type> type_body;

    PolyTypeScheme(const std::vector<std::shared_ptr<TypeVar>>& binded, const std::shared_ptr<Type>& body);
   
    virtual std::shared_ptr<Type> instantiate() const;
    virtual std::string to_str() const;
    ~PolyTypeScheme();
};

struct MonoTypeScheme : TypeScheme {
    std::shared_ptr<Type> type;

    MonoTypeScheme(std::shared_ptr<Type>& t);
   
    virtual std::shared_ptr<Type> instantiate() const;
    virtual std::string to_str() const;
    ~MonoTypeScheme();
};

#endif