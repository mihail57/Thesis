

#include <tuple>
#include <vector>
#include <exception>
#include <variant>

#include "substitution_or_error.h"
#include "type_or_error.h"
#include "temporary/lambda_parser.h"


SubstitutionOrError MGU(const std::shared_ptr<Type>& first, const std::shared_ptr<Type>& second) {
    if (auto f_var = std::dynamic_pointer_cast<TypeVar>(first)) {
        if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
            if (f_var->name == s_var->name) {
                return {};
            }
        }
        if (occurs_check(f_var, second)) {
            return TypingError{.text = "occurs check failed: infinite type with " + f_var->to_str() + " and " + second->to_str(), .at = SourceLoc()};
        }
        Substitution sub;
        sub.subs[f_var->name] = second;
        return sub;
    } else if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
        if (occurs_check(s_var, first)) {
            return TypingError{.text = "occurs check failed: infinite type with " + f_var->to_str() + " and " + second->to_str(), .at = SourceLoc()};
        }
        Substitution sub;
        sub.subs[s_var->name] = first;
        return sub;
    } else if (auto f_const = std::dynamic_pointer_cast<ConstType>(first)) {
        if (auto s_const = std::dynamic_pointer_cast<ConstType>(second)) {
            if (f_const->name == s_const->name) {
                return {};
            }
        }
        return TypingError{.text = "types do not match: " + first->to_str() + " and " + second->to_str(), .at = SourceLoc()};
    } else if (auto f_func = std::dynamic_pointer_cast<FuncType>(first)) {
        if (auto s_func = std::dynamic_pointer_cast<FuncType>(second)) {
            auto s1 = MGU(f_func->arg_type, s_func->arg_type);
            if(is_error(s1)) {
                auto error = std::get<TypingError>(s1);
                error.text += "\non unifying types " + f_func->to_str() + " and " + s_func->to_str();
                return error;
            }
            auto s1_uw = unwrap(s1);

            auto new_f_ret = apply_substitutions(s1_uw, f_func->ret_type);
            auto new_s_ret = apply_substitutions(s1_uw, s_func->ret_type);

            auto s2 = MGU(new_f_ret, new_s_ret);            
            if(is_error(s2)) {
                auto error = std::get<TypingError>(s2);
                error.text += "\non unifying types " + f_func->to_str() + " and " + s_func->to_str();
                return error;
            }

            return unwrap(s2) + s1_uw;
        }
        return TypingError{.text = "types do not match: " + first->to_str() + " and " + second->to_str(), .at = SourceLoc()};
    } else if(auto f_constructor = std::dynamic_pointer_cast<TypeConstructor>(first)) {
        if(auto s_constructor = std::dynamic_pointer_cast<TypeConstructor>(second)) {
            if(s_constructor->name->name == f_constructor->name->name 
                && s_constructor->args.size() == f_constructor->args.size()
            ) {
                Substitution s;
                for(int i = 0; i < f_constructor->args.size(); i++) {
                    auto res = MGU(f_constructor->args[i], s_constructor->args[i]);
                    if(is_error(res)) {
                        auto error = std::get<TypingError>(res);
                        error.text += "\non unifying types " + f_constructor->to_str() + " and " + s_constructor->to_str() + " (given and expected types of " + f_constructor->to_str() + " )";
                        return error;
                    }
                    s = s + unwrap(res);
                }

                return s;
            }
        }
        return TypingError{.text = "types do not match: " + first->to_str() + " and " + second->to_str(), .at = SourceLoc()};
    }
    return TypingError{.text = "unknown type in MGU", .at = SourceLoc()};
}

TypeOrError _W(TypingContext& gamma, std::shared_ptr<AstNode> node,
    Substitution& resulting_sub, std::vector<SourceLoc>& trace);

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<ConstNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    resulting_sub = make_identity();
    return std::shared_ptr<Type>(node->type);
}

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<VarNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    resulting_sub = make_identity();
    if(!gamma.has(node->var)) {
        return TypingError{.text = "unknown variable " + node->to_str(), .at = node->loc};
    }
        
    std::shared_ptr<TypeScheme> scheme = gamma.get(node->var).second;
    return scheme->instantiate();
}

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<FuncNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    auto beta = TypeVar::generate_fresh();
    auto beta_upcasted = std::static_pointer_cast<Type>(beta);
    auto t_1 = _W(
        TypingContext(gamma).with(
            node->param->var,
            std::shared_ptr<TypeScheme>(new PolyTypeScheme(
                std::vector<std::shared_ptr<TypeVar>> { },
                beta_upcasted
            ))
        ),
        node->body, resulting_sub, trace
    );

    if(is_error(t_1)) {
        trace.push_back(node->loc);
        return t_1;
    }

    // resulting_sub.print();

    return std::shared_ptr<Type>(new FuncType(
        resulting_sub.apply_to(beta_upcasted),
        unwrap(t_1)
    ));
}

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<AppNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    Substitution s_1;
    auto t_1 = _W(gamma, node->func, s_1, trace);

    if(is_error(t_1)) {
        trace.push_back(node->loc);
        return t_1;
    }

    TypingContext new_ctx = s_1.apply_to(gamma);
    Substitution s_2;
    auto t_2 = _W(new_ctx, node->arg, s_2, trace);

    if(is_error(t_2)) {
        trace.push_back(node->loc);
        return t_2;
    }

    auto beta = TypeVar::generate_fresh();
    auto s_3 = MGU(
        s_2.apply_to(unwrap(t_1)), 
        make_func_type(unwrap(t_2), beta)
    );

    if(is_error(s_3)) {
        auto error = std::get<TypingError>(s_3);
        error.at = node->loc;
        trace.push_back(node->loc);
        return error;
    }

    Substitution s_3_uw = unwrap(s_3);
    resulting_sub = s_3_uw + s_2 + s_1;
    return s_3_uw.apply_to(beta);
}

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<LetNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    Substitution s_1;
    auto t_1 = _W(gamma, node->bind_value, s_1, trace);

    if(is_error(t_1)) {
        trace.push_back(node->loc);
        return t_1;
    }
    auto t_1_uw = unwrap(t_1);

    TypingContext new_ctx = s_1.apply_to(gamma);
    Substitution s_2;
    auto t_2 = _W(new_ctx.with(node->bind_var->var, new_ctx.generalize(t_1_uw)), node->ret_value, s_2, trace);

    if(!trace.empty()) {
        trace.push_back(node->loc);
        return t_2;
    }

    resulting_sub = s_2 + s_1;
    return t_2;
}

TypeOrError _W(
    TypingContext& gamma,
    std::shared_ptr<AstNode> node,
    Substitution& resulting_sub, 
    std::vector<SourceLoc>& trace
) {
    if(auto const_node = std::dynamic_pointer_cast<ConstNode>(node))
        return _W(gamma, const_node, resulting_sub, trace);
    if(auto var_node = std::dynamic_pointer_cast<VarNode>(node))
        return _W(gamma, var_node, resulting_sub, trace);
    if(auto func_node = std::dynamic_pointer_cast<FuncNode>(node))
        return _W(gamma, func_node, resulting_sub, trace);
    if(auto app_node = std::dynamic_pointer_cast<AppNode>(node))
        return _W(gamma, app_node, resulting_sub, trace);
    if(auto let_node = std::dynamic_pointer_cast<LetNode>(node))
        return _W(gamma, let_node, resulting_sub, trace);
    
    trace.push_back(node->loc);
    return TypingError{.text = "unknown node type", .at = node->loc};
}

TypeOrError W(const TypingContext& gamma, std::shared_ptr<AstNode> node, std::vector<SourceLoc>& trace) {
    TypingContext tctx(gamma);
    Substitution s;
    return _W(tctx, node, s, trace);
}

std::string highlight_loc(const std::string& source, const SourceLoc& loc) {
    SourceLoc::pos_t source_show_start = 0;
    SourceLoc::pos_t source_show_end   = source.size() - 1;

    std::ostringstream ss;
    ss << source.substr(source_show_start, source_show_end - source_show_start + 1) << std::endl;
    for(int i = source_show_start; i <= source_show_end; i++) { 
        if(i >= loc.start && i < loc.end) ss << '~';
        else ss << ' ';
    }
    return ss.str();
}

TypingContext make_basic_context() {    
    auto list_identifier = _make_type_var("List"), list_param = _make_type_var("α");
    auto list = make_type_constructor(list_identifier, { list_param });
    auto nil = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { list_param },
        list
    )); //∀α. List α
    auto cons = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { list_param },
        make_func_type(list_param, make_func_type(list, list))
    )); //∀α. α -> List α -> List α

    auto nil_id = _make_var_node("[]", SourceLoc());
    auto cons_id = _make_var_node("::", SourceLoc());
    TypingContext tctx;
    tctx.set(nil_id->var, nil);
    tctx.set(cons_id->var, cons);

    return tctx;
}

std::string print_error(const TypingError& error, const std::string& source) {
    std::ostringstream ss;
    ss << error.text << std::endl;
    ss << highlight_loc(source, error.at) << std::endl;
    return ss.str();
}

std::string infer(const std::string& source, TypingContext tctx = make_basic_context()) {
    std::vector<SourceLoc> trace;
    auto parsed = parse(source);
    if(is_error(parsed))
        return print_error(get_error(parsed), source);
    
    auto result = W(tctx, unwrap(parsed), trace);
    if(is_error(result)) {
        std::ostringstream ss;
        ss << "Type error: " << print_error(get_error(result), source);
        ss << "Inference stack trace:" << std::endl;
        for(const auto& st_elem : trace) 
            ss << "\tOn " << source.substr(st_elem.start, st_elem.end - st_elem.start + 1) << std::endl;

        return ss.str();
    }
    return unwrap(result)->to_str();
}
