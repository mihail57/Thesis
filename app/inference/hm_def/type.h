
#pragma once

#ifndef HM_TYPE_DEF_H
#define HM_TYPE_DEF_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>


// Типы

struct TypeStringifier {
    std::map<std::string, std::string> mapping;
    int next_id = 0;
    std::string get_name(const std::string& orig_name);
};

struct TypeVar;
using TypeVarPtr = std::shared_ptr<TypeVar>;

struct Type {
    using base_ptr_t = std::shared_ptr<Type>;

    Type() {};
    virtual ~Type() = default;

    virtual base_ptr_t clone() const = 0;
    virtual base_ptr_t replace_vars(std::unordered_map<std::string, TypeVarPtr>& replacements) = 0;

    virtual base_ptr_t make_sub(const TypeVarPtr& target, const base_ptr_t& value) = 0;

    virtual void get_vars(std::vector<TypeVarPtr>& type_vars) = 0;

    std::string to_str(bool pretty_print = true) const;
    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const = 0;
};


struct ConstType : Type, public std::enable_shared_from_this<ConstType> {
    using ptr_t = std::shared_ptr<ConstType>;

    std::string name;

    ConstType(const std::string& n);
    base_ptr_t clone() const;
    base_ptr_t replace_vars(std::unordered_map<std::string, TypeVarPtr>& replacements);
    base_ptr_t make_sub(const TypeVarPtr& target, const base_ptr_t& value);
    virtual void get_vars(std::vector<TypeVarPtr>& type_vars);
    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

ConstType::ptr_t make_const_type(const std::string& name);


struct TypeVar : Type, public std::enable_shared_from_this<TypeVar> {
    using ptr_t = TypeVarPtr;

    std::string name;
    bool generated = false;

    TypeVar(const std::string& n);
    base_ptr_t clone() const;
    base_ptr_t replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements);
    
    base_ptr_t make_sub(const TypeVar::ptr_t& target, const base_ptr_t& value);
    virtual void get_vars(std::vector<TypeVar::ptr_t>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;

    static TypeVar::ptr_t generate_fresh();
};

TypeVar::ptr_t make_type_var(const std::string& name);


struct FuncType : Type, public std::enable_shared_from_this<FuncType> {
    using ptr_t = std::shared_ptr<FuncType>;
    
    Type::base_ptr_t arg_type;
    Type::base_ptr_t ret_type;

    FuncType(const Type::base_ptr_t& a, const Type::base_ptr_t& r);

    base_ptr_t clone() const;
    base_ptr_t replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements);
    
    base_ptr_t make_sub(const TypeVar::ptr_t& target, const base_ptr_t& value);

    virtual void get_vars(std::vector<TypeVar::ptr_t>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

FuncType::ptr_t make_func_type(const Type::base_ptr_t& a, const Type::base_ptr_t& b);


struct TypeConstructor : Type, public std::enable_shared_from_this<TypeConstructor> {
    using ptr_t = std::shared_ptr<TypeConstructor>;

    TypeVar::ptr_t name;
    std::vector<Type::base_ptr_t> args;

    TypeConstructor(const TypeVar::ptr_t& n, const std::vector<Type::base_ptr_t>& a);

    base_ptr_t clone() const;
    base_ptr_t replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements);
    
    base_ptr_t make_sub(const TypeVar::ptr_t& target, const base_ptr_t& value);

    virtual void get_vars(std::vector<TypeVar::ptr_t>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

TypeConstructor::ptr_t make_type_constructor(const TypeVar::ptr_t& name, const std::vector<Type::base_ptr_t>& args);

#endif