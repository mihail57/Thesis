
#pragma once

#ifndef HM_TYPE_DEF_H
#define HM_TYPE_DEF_H

#include <string>
#include <vector>
#include <map>
#include <memory>


// Типы

struct TypeStringifier {
    std::map<std::string, std::string> mapping;
    int next_id = 0;
    std::string get_name(const std::string& orig_name);
};

struct TypeVar;

struct Type {
    using base_ptr = std::shared_ptr<Type>;

    Type() {};
    virtual ~Type() = default;

    virtual std::shared_ptr<Type> clone() const = 0;
    virtual std::shared_ptr<Type> replace_vars(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    ) = 0;

    virtual std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) = 0;

    virtual void get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) = 0;

    std::string to_str(bool pretty_print = true) const;
    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const = 0;

    virtual bool operator=(Type* b) { 
        return true; 
    }
};


struct ConstType : Type, public std::enable_shared_from_this<ConstType> {
    using ptr = std::shared_ptr<ConstType>;

    std::string name;

    ConstType(const std::string& n);
    std::shared_ptr<Type> clone() const;
    std::shared_ptr<Type> replace_vars(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    );    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value);
    virtual void get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars);
    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

Type::base_ptr make_const_type(const std::string& name);


struct TypeVar : Type, public std::enable_shared_from_this<TypeVar> {
    using ptr = std::shared_ptr<TypeVar>;

    std::string name;
    bool generated = false;

    TypeVar(const std::string& n);
    std::shared_ptr<Type> clone() const;
    std::shared_ptr<Type> replace_vars(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    );
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value);
    virtual void get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;

    static std::shared_ptr<TypeVar> generate_fresh();
};

TypeVar::ptr _make_type_var(const std::string& name);

Type::base_ptr make_type_var(const std::string& name);


struct FuncType : Type, public std::enable_shared_from_this<FuncType> {
    using ptr = std::shared_ptr<FuncType>;
    
    std::shared_ptr<Type> arg_type;
    std::shared_ptr<Type> ret_type;

    FuncType(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& r);

    std::shared_ptr<Type> clone() const;
    std::shared_ptr<Type> replace_vars(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    );
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value);

    virtual void get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

Type::base_ptr make_func_type(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b);


struct TypeConstructor : Type, public std::enable_shared_from_this<TypeConstructor> {
    using ptr = std::shared_ptr<TypeConstructor>;

    std::shared_ptr<TypeVar> name;
    std::vector<std::shared_ptr<Type>> args;

    TypeConstructor(const std::shared_ptr<TypeVar>& n, const std::vector<std::shared_ptr<Type>>& a);

    std::shared_ptr<Type> clone() const;
    std::shared_ptr<Type> replace_vars(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    );
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value);

    virtual void get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars);

    virtual std::string to_str_impl(TypeStringifier& ctx, bool pretty_print = true) const;
};

std::shared_ptr<Type> make_type_constructor(const std::shared_ptr<TypeVar>& name, const std::vector<std::shared_ptr<Type>>& args);

#endif