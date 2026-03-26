#include "type.h"

#include <algorithm>
#include <sstream>

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
Type::base_ptr_t ConstType::clone() const { return std::make_shared<ConstType>(*this); }
Type::base_ptr_t ConstType::replace_vars(std::unordered_map<std::string, TypeVarPtr>& replacements) {
    return shared_from_this();
}

Type::base_ptr_t ConstType::make_sub(const TypeVarPtr& target, const Type::base_ptr_t& value) {
    return std::static_pointer_cast<Type>(shared_from_this());
}

void ConstType::get_vars(std::vector<TypeVar::ptr_t>& type_vars) {}

std::string ConstType::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    return name;
}

ConstType::ptr_t make_const_type(const std::string& name) {
    return ConstType::ptr_t(new ConstType(name));
}

TypeVar::TypeVar(const std::string& n) : Type(), name(n) {};
Type::base_ptr_t TypeVar::clone() const { return std::make_shared<TypeVar>(*this); }
Type::base_ptr_t TypeVar::replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements) {
    auto val = replacements.find(name);
    if(val == replacements.end())
        return std::static_pointer_cast<Type>(shared_from_this());

    auto& lookup = val->second;
    if(!lookup) {
        lookup = TypeVar::generate_fresh();
    }
    return lookup;
}

Type::base_ptr_t TypeVar::make_sub(const TypeVar::ptr_t& target, const Type::base_ptr_t& value) {
    if(name == target->name)
        return value;

    return shared_from_this();
}

void TypeVar::get_vars(std::vector<TypeVar::ptr_t>& type_vars) {
    if(std::find_if(
        type_vars.cbegin(), type_vars.cend(),
        [this](const TypeVar::ptr_t& t) { return t->name == this->name; }
    ) == type_vars.cend()) {
        auto _this = std::dynamic_pointer_cast<TypeVar>(shared_from_this());
        type_vars.push_back(_this);
    }
}

std::string TypeVar::to_str_impl(TypeStringifier& ctx, bool pretty_print) const {
    if(pretty_print && generated) return ctx.get_name(name);
    return name;
}

TypeVar::ptr_t TypeVar::generate_fresh() {
    static int generator = 0;

    std::stringstream ss;
    ss << "<t" << generator++ << '>';

    auto result = TypeVar::ptr_t(new TypeVar(ss.str()));
    result->generated = true;
    return result;
}

TypeVar::ptr_t make_type_var(const std::string& name) {
    return TypeVar::ptr_t(new TypeVar(name));
}

FuncType::FuncType(const Type::base_ptr_t& a, const Type::base_ptr_t& r)
    : Type(), arg_type(a), ret_type(r)
{};

Type::base_ptr_t FuncType::clone() const { return std::make_shared<FuncType>(*this); }
Type::base_ptr_t FuncType::replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements) {
    auto a = arg_type->replace_vars(replacements);
    auto r = ret_type->replace_vars(replacements);
    if(a != arg_type || r != ret_type)
        return FuncType::ptr_t(new FuncType(a, r));
    return shared_from_this();
}

Type::base_ptr_t FuncType::make_sub(const TypeVar::ptr_t& target, const Type::base_ptr_t& value) {
    auto a = arg_type->make_sub(target, value);
    auto r = ret_type->make_sub(target, value);
    if(a != arg_type || r != ret_type)
        return FuncType::ptr_t(new FuncType(a, r));
    return std::static_pointer_cast<Type>(shared_from_this());
}

void FuncType::get_vars(std::vector<TypeVar::ptr_t>& type_vars) {
    arg_type->get_vars(type_vars);
    ret_type->get_vars(type_vars);
}

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

FuncType::ptr_t make_func_type(const Type::base_ptr_t& a, const Type::base_ptr_t& b) {
    return FuncType::ptr_t(new FuncType(a, b));
}

TypeConstructor::TypeConstructor(const TypeVar::ptr_t& n, const std::vector<Type::base_ptr_t>& a)
    : Type(), name(n), args(a)
{};

Type::base_ptr_t TypeConstructor::clone() const { return std::make_shared<TypeConstructor>(*this); }
Type::base_ptr_t TypeConstructor::replace_vars(std::unordered_map<std::string, TypeVar::ptr_t>& replacements) {
    bool should_clone = false;
    std::vector<Type::base_ptr_t> new_args;
    new_args.reserve(args.size());

    for(auto& arg : args) {
        auto new_arg = arg->replace_vars(replacements);
        if(arg != new_arg) {
            should_clone = true;
        }
        new_args.push_back(new_arg);
    }

    if(should_clone)
        return TypeConstructor::ptr_t(new TypeConstructor(name, new_args));
    return shared_from_this();
}

Type::base_ptr_t TypeConstructor::make_sub(const TypeVar::ptr_t& target, const Type::base_ptr_t& value) {
    bool should_clone = false;
    std::vector<Type::base_ptr_t> new_args;
    new_args.reserve(args.size());

    for(auto& arg : args) {
        auto new_arg = arg->make_sub(target, value);
        if(arg != new_arg) {
            should_clone = true;
        }
        new_args.push_back(new_arg);
    }

    if(should_clone)
        return TypeConstructor::ptr_t(new TypeConstructor(name, new_args));
    return shared_from_this();
}

void TypeConstructor::get_vars(std::vector<TypeVar::ptr_t>& type_vars) {
    for(const auto& arg : args)
        arg->get_vars(type_vars);
}

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

TypeConstructor::ptr_t make_type_constructor(const TypeVar::ptr_t& name, const std::vector<Type::base_ptr_t>& args) {
    return TypeConstructor::ptr_t(new TypeConstructor(name, args));
}
