
#ifndef TYPE_SCHEME_H
#define TYPE_SCHEME_H

#include "type.h"

struct TypeScheme {
    using ptr_t = std::shared_ptr<TypeScheme>;

    virtual Type::base_ptr_t instantiate() const = 0;
    virtual std::string to_str() const = 0;
    virtual ~TypeScheme();
};

struct PolyTypeScheme : TypeScheme {
    using ptr_t = std::shared_ptr<PolyTypeScheme>;

    std::vector<TypeVar::ptr_t> binded_type_vars;
    Type::base_ptr_t type_body;

    PolyTypeScheme(const std::vector<TypeVar::ptr_t>& binded, const Type::base_ptr_t& body);
   
    virtual Type::base_ptr_t instantiate() const;
    virtual std::string to_str() const;
    ~PolyTypeScheme();
};

struct MonoTypeScheme : TypeScheme {
    using ptr_t = std::shared_ptr<MonoTypeScheme>;

    Type::base_ptr_t type;

    MonoTypeScheme(Type::base_ptr_t& t);
   
    virtual Type::base_ptr_t instantiate() const;
    virtual std::string to_str() const;
    ~MonoTypeScheme();
};

#endif