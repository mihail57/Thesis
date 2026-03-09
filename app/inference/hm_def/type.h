
#pragma once

#ifndef HM_TYPE_DEF_H
#define HM_TYPE_DEF_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <variant>

#include "variable.h"


// Типы

struct TypeVar;

struct Type {
    using base_ptr = std::shared_ptr<Type>;

    Type() {};
    virtual ~Type() {};

    virtual std::shared_ptr<Type> clone() const = 0;
    virtual std::shared_ptr<Type> clone_fresh(
        const std::vector<std::shared_ptr<TypeVar>>& old_binded, 
        std::vector<std::shared_ptr<TypeVar>>& new_binded
    ) = 0;

    virtual std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) = 0;

    virtual void add_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) = 0;

    virtual std::string to_str() const = 0;

    virtual bool operator=(Type* b) { 
        return true; 
    }
};


struct ConstType : Type, public std::enable_shared_from_this<ConstType> {
    using ptr = std::shared_ptr<ConstType>;

    std::string name;

    ConstType(const std::string& n) : Type(), name(n) {};
    std::shared_ptr<Type> clone() const { return std::make_shared<ConstType>(*this); }
    std::shared_ptr<Type> clone_fresh(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
        // Не создаём новый объект
        return shared_from_this();
    }
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
        // Ничего не требуется
        return std::static_pointer_cast<Type>(shared_from_this());
    }

    virtual void add_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {};

    virtual std::string to_str() const {
        return name;
    }

    bool operator=(Type* b) override {
        if(auto b_cast = dynamic_cast<const ConstType*>(b)) {
            return name == b_cast->name;
        }
        return false;
    }
};

Type::base_ptr make_const_type(const std::string& name) {
    return std::shared_ptr<Type>(new ConstType(name));
}


struct TypeVar : Type, public std::enable_shared_from_this<TypeVar> {
    using ptr = std::shared_ptr<TypeVar>;

    std::string name;

    TypeVar(const std::string& n) : Type(), name(n) {};
    std::shared_ptr<Type> clone() const { return std::make_shared<TypeVar>(*this); }
    std::shared_ptr<Type> clone_fresh(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
        auto val = std::find_if(old_binded.cbegin(), old_binded.cend(), [this](const std::shared_ptr<TypeVar>& o) { return o->name == this->name; });
        if(val == old_binded.end()) 
            return std::make_shared<TypeVar>(*this); //Создание копии

        auto& lookup = new_binded.at(std::distance(old_binded.cbegin(), val));
        if(!lookup) {
            lookup = TypeVar::generate_fresh();
        }
        return lookup;
    }
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
        if(name == target->name)
            return value;

        return shared_from_this();
    }

    virtual void add_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
        if(std::find_if(
            type_vars.cbegin(), type_vars.cend(), 
            [this](const std::shared_ptr<TypeVar>& t) { return t->name == this->name; }
        ) == type_vars.cend()) {
            auto _this = std::dynamic_pointer_cast<TypeVar>(shared_from_this());
            if(!_this)
                throw std::runtime_error("wtf");

            type_vars.push_back(_this);
        }
    };

    virtual std::string to_str() const {
        return name;
    }

    static std::shared_ptr<TypeVar> generate_fresh() {
        static int generator = 0;
        
        std::stringstream ss;
        ss << "<t" << generator++ << '>';

        return std::shared_ptr<TypeVar>(new TypeVar(ss.str()));
    }
};

TypeVar::ptr _make_type_var(const std::string& name) {
    return std::shared_ptr<TypeVar>(new TypeVar(name));
}

Type::base_ptr make_type_var(const std::string& name) {
    return std::shared_ptr<Type>(new TypeVar(name));
}


struct FuncType : Type, public std::enable_shared_from_this<FuncType> {
    using ptr = std::shared_ptr<FuncType>;
    
    std::shared_ptr<Type> arg_type;
    std::shared_ptr<Type> ret_type;

    FuncType(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& r) 
        : Type(), arg_type(a), ret_type(r) 
    {};

    std::shared_ptr<Type> clone() const { return std::make_shared<FuncType>(*this); }
    std::shared_ptr<Type> clone_fresh(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
        auto a = arg_type->clone_fresh(old_binded, new_binded);
        auto r = ret_type->clone_fresh(old_binded, new_binded);
        if(a != arg_type || r != ret_type) 
            return std::shared_ptr<FuncType>(new FuncType(a, r));
        return shared_from_this();
    }
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
        auto a = arg_type->make_sub(target, value);
        auto r = ret_type->make_sub(target, value);
        if(a != arg_type || r != ret_type) 
            return std::shared_ptr<FuncType>(new FuncType(a, r));
        return std::static_pointer_cast<Type>(shared_from_this());
    }

    virtual void add_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
        arg_type->add_vars(type_vars);
        ret_type->add_vars(type_vars);
    };

    virtual std::string to_str() const {
        if(auto arg_func = std::dynamic_pointer_cast<FuncType>(arg_type))
            return "(" + arg_func->to_str() + ")->" + ret_type->to_str();
        return arg_type->to_str() + "->" + ret_type->to_str();
    }
};

Type::base_ptr make_func_type(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b) {
    return std::shared_ptr<Type>(new FuncType(a, b));
}


struct TypeConstructor : Type, public std::enable_shared_from_this<TypeConstructor> {
    using ptr = std::shared_ptr<TypeConstructor>;

    std::shared_ptr<TypeVar> name;
    std::vector<std::shared_ptr<Type>> args;

    TypeConstructor(const std::shared_ptr<TypeVar>& n, const std::vector<std::shared_ptr<Type>>& a) 
        : Type(), name(n), args(a) 
    {};

    std::shared_ptr<Type> clone() const { return std::make_shared<TypeConstructor>(*this); }
    std::shared_ptr<Type> clone_fresh(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
        bool should_clone = false;
        std::vector<std::shared_ptr<Type>> new_args;
        new_args.reserve(args.size());

        for(auto& arg : args) {
            auto new_arg = arg->clone_fresh(old_binded, new_binded);
            if(arg != new_arg) {
                should_clone = true;
            }
            new_args.push_back(new_arg);
        }

        // Имя конструктора неизменно!
        //auto n = std::dynamic_pointer_cast<TypeVar>(name->clone_fresh(old_binded, new_binded));
        if(should_clone) 
            return std::shared_ptr<TypeConstructor>(new TypeConstructor(name, new_args));
        return shared_from_this();
    }
    
    std::shared_ptr<Type> make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
        bool should_clone = false;
        std::vector<std::shared_ptr<Type>> new_args;
        new_args.reserve(args.size());

        for(auto& arg : args) {
            auto new_arg = arg->make_sub(target, value);
            if(arg != new_arg) {
                should_clone = true;
            }
            new_args.push_back(new_arg);
        }

        // Имя конструктора неизменно!
        //auto n = std::dynamic_pointer_cast<TypeVar>(name->make_sub(target, value));
        if(should_clone) 
            return std::shared_ptr<TypeConstructor>(new TypeConstructor(name, new_args));
        return shared_from_this();
    }

    virtual void add_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
        name->add_vars(type_vars);
        for(const auto& arg : args)
            arg->add_vars(type_vars);
    };

    virtual std::string to_str() const {
        std::ostringstream ss;
        ss << name->to_str() << "(" << args[0]->to_str();
        for(int i = 1; i < args.size(); i++)
            ss << ", " << args[i]->to_str();
        ss << ")";
        return ss.str();
    }
};

std::shared_ptr<Type> make_type_constructor(const std::shared_ptr<TypeVar>& name, const std::vector<std::shared_ptr<Type>>& args) {
    return std::shared_ptr<Type>(new TypeConstructor(name, args));
}

#endif