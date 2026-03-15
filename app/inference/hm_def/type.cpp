
#include "type.h"


// Типы

std::string TypeStringifier::get_name(const std::string& orig_name) {
    auto it = mapping.find(orig_name);
    if(it != mapping.end()) {
        return it->second;
    }
    
    int id = next_id++;
    std::string new_name;
    int curr = id;
    do {
        new_name = char('a' + (curr % 26)) + new_name;
        curr = (curr / 26) - 1;
    } while (curr >= 0);
    
    mapping[orig_name] = new_name;
    return new_name;
}

std::string Type::to_str(bool pretty_print) const {
    TypeStringifier ctx;
    return to_str_impl(ctx, pretty_print);
}

ConstType::ConstType(const std::string& n) : Type(), name(n) {};
std::shared_ptr<Type> ConstType::clone() const { return std::make_shared<ConstType>(*this); }
std::shared_ptr<Type> ConstType::replace_vars(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
    // Не создаём новый объект
    return shared_from_this();
}

std::shared_ptr<Type> ConstType::make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
    // Ничего не требуется
    return std::static_pointer_cast<Type>(shared_from_this());
}

void ConstType::get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {};

std::string ConstType::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    return name;
}

Type::base_ptr make_const_type(const std::string& name) {
    return std::shared_ptr<Type>(new ConstType(name));
}



TypeVar::TypeVar(const std::string& n) : Type(), name(n) {};
std::shared_ptr<Type> TypeVar::clone() const { return std::make_shared<TypeVar>(*this); }
std::shared_ptr<Type> TypeVar::replace_vars(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
    auto val = std::find_if(old_binded.cbegin(), old_binded.cend(), [this](const std::shared_ptr<TypeVar>& o) { return o->name == this->name; });
    if(val == old_binded.end()) 
        return std::make_shared<TypeVar>(*this); //Создание копии

    auto& lookup = new_binded.at(std::distance(old_binded.cbegin(), val));
    if(!lookup) {
        lookup = TypeVar::generate_fresh();
    }
    return lookup;
}

std::shared_ptr<Type> TypeVar::make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
    if(name == target->name)
        return value;

    return shared_from_this();
}

void TypeVar::get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
    if(std::find_if(
        type_vars.cbegin(), type_vars.cend(), 
        [this](const std::shared_ptr<TypeVar>& t) { return t->name == this->name; }
    ) == type_vars.cend()) {
        auto _this = std::dynamic_pointer_cast<TypeVar>(shared_from_this());

        type_vars.push_back(_this);
    }
};

std::string TypeVar::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    if(pretty_print && generated) return ctx.get_name(name);
    return name;
}

std::shared_ptr<TypeVar> TypeVar::generate_fresh() {
    static int generator = 0;
    
    std::stringstream ss;
    ss << "<t" << generator++ << '>';

    auto result = std::shared_ptr<TypeVar>(new TypeVar(ss.str()));
    result->generated = true;
    return result;
}

TypeVar::ptr _make_type_var(const std::string& name) {
    return std::shared_ptr<TypeVar>(new TypeVar(name));
}

Type::base_ptr make_type_var(const std::string& name) {
    return std::shared_ptr<Type>(new TypeVar(name));
}



FuncType::FuncType(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& r) 
    : Type(), arg_type(a), ret_type(r) 
{};

std::shared_ptr<Type> FuncType::clone() const { return std::make_shared<FuncType>(*this); }
std::shared_ptr<Type> FuncType::replace_vars(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
    auto a = arg_type->replace_vars(old_binded, new_binded);
    auto r = ret_type->replace_vars(old_binded, new_binded);
    if(a != arg_type || r != ret_type) 
        return std::shared_ptr<FuncType>(new FuncType(a, r));
    return shared_from_this();
}

std::shared_ptr<Type> FuncType::make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
    auto a = arg_type->make_sub(target, value);
    auto r = ret_type->make_sub(target, value);
    if(a != arg_type || r != ret_type) 
        return std::shared_ptr<FuncType>(new FuncType(a, r));
    return std::static_pointer_cast<Type>(shared_from_this());
}

void FuncType::get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
    arg_type->get_vars(type_vars);
    ret_type->get_vars(type_vars);
};

std::string FuncType::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    std::string arg_str;
    if(auto arg_func = std::dynamic_pointer_cast<FuncType>(arg_type)) {
        arg_str = "(" + arg_func->to_str_impl(ctx, pretty_print) + ")";
    } else {
        arg_str = arg_type->to_str_impl(ctx, pretty_print);
    }
    std::string ret_str = ret_type->to_str_impl(ctx, pretty_print);
    return arg_str + "->" + ret_str;
}

Type::base_ptr make_func_type(const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b) {
    return std::shared_ptr<Type>(new FuncType(a, b));
}




TypeConstructor::TypeConstructor(const std::shared_ptr<TypeVar>& n, const std::vector<std::shared_ptr<Type>>& a) 
    : Type(), name(n), args(a) 
{};

std::shared_ptr<Type> TypeConstructor::clone() const { return std::make_shared<TypeConstructor>(*this); }
std::shared_ptr<Type> TypeConstructor::replace_vars(const std::vector<std::shared_ptr<TypeVar>>& old_binded, std::vector<std::shared_ptr<TypeVar>>& new_binded) {
    bool should_clone = false;
    std::vector<std::shared_ptr<Type>> new_args;
    new_args.reserve(args.size());

    for(auto& arg : args) {
        auto new_arg = arg->replace_vars(old_binded, new_binded);
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

std::shared_ptr<Type> TypeConstructor::make_sub(const std::shared_ptr<TypeVar>& target, const std::shared_ptr<Type>& value) {
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

void TypeConstructor::get_vars(std::vector<std::shared_ptr<TypeVar>>& type_vars) {
    name->get_vars(type_vars);
    for(const auto& arg : args)
        arg->get_vars(type_vars);
};

std::string TypeConstructor::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    std::string res = name->to_str_impl(ctx, pretty_print) + "(";
    if (!args.empty()) {
        res += args[0]->to_str_impl(ctx, pretty_print);
        for(size_t i = 1; i < args.size(); i++) {
            res += ", " + args[i]->to_str_impl(ctx, pretty_print);
        }
    }
    res += ")";
    return res;
}

std::shared_ptr<Type> make_type_constructor(const std::shared_ptr<TypeVar>& name, const std::vector<std::shared_ptr<Type>>& args) {
    return std::shared_ptr<Type>(new TypeConstructor(name, args));
}