

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
        sub.subs[f_var] = second;
        return sub;
    } else if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
        if (occurs_check(s_var, first)) {
            return TypingError{.text = "occurs check failed: infinite type with " + f_var->to_str() + " and " + second->to_str(), .at = SourceLoc()};
        }
        Substitution sub;
        sub.subs[s_var] = first;
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


using Result = std::tuple<Type::base_ptr, Substitution>;
Type::base_ptr get_type(const Result& v) { return std::get<0>(v); }
using ResultOrError = std::variant<Result, TypingError>;
bool is_error(const ResultOrError& v) { return std::holds_alternative<TypingError>(v); }
TypingError get_error(const ResultOrError& v) { return std::get<TypingError>(v); }
Result unwrap(const ResultOrError& v) { return std::get<Result>(v); }

ResultOrError W(TypingContext& gamma, std::shared_ptr<AstNode> node,
    std::vector<SourceLoc>& trace);

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<ConstNode> node,
    std::vector<SourceLoc>& trace
) {
    return Result{ std::shared_ptr<Type>(node->type), Substitution::make_identity() };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<VarNode> node,
    std::vector<SourceLoc>& trace
) {
    auto sub = Substitution::make_identity();
    if(!gamma.has(node->var)) {
        return TypingError{.text = "unknown variable " + node->to_str(), .at = node->loc};
    }
        
    std::shared_ptr<TypeScheme> scheme = gamma.get(node->var).second;
    return Result{ scheme->instantiate(), sub };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<FuncNode> node,
    std::vector<SourceLoc>& trace
) {
    auto beta = TypeVar::generate_fresh();
    auto beta_upcasted = std::static_pointer_cast<Type>(beta);
    auto res_1 = W(
        TypingContext(gamma).with(
            node->param->var,
            std::shared_ptr<TypeScheme>(new PolyTypeScheme(
                std::vector<std::shared_ptr<TypeVar>> { },
                beta_upcasted
            ))
        ),
        node->body, trace
    );

    if(is_error(res_1)) {
        trace.push_back(node->loc);
        return res_1;
    }

    auto [t_1, s_1] = unwrap(res_1);

    // resulting_sub.print();

    return Result{ std::shared_ptr<Type>(new FuncType(
        s_1.apply_to(beta_upcasted),
        unwrap(t_1)
    )), s_1 };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<AppNode> node,
    std::vector<SourceLoc>& trace
) {
    auto res_1 = W(gamma, node->func, trace);

    if(is_error(res_1)) {
        trace.push_back(node->loc);
        return res_1;
    }

    auto [t_1, s_1] = unwrap(res_1);

    TypingContext new_ctx = s_1.apply_to(gamma);
    auto res_2 = W(new_ctx, node->arg, trace);

    if(is_error(res_2)) {
        trace.push_back(node->loc);
        return res_2;
    }

    auto [t_2, s_2] = unwrap(res_2);

    auto beta = TypeVar::generate_fresh();
    auto res_3 = MGU(
        s_2.apply_to(unwrap(t_1)), 
        make_func_type(unwrap(t_2), beta)
    );

    if(is_error(res_3)) {
        auto error = std::get<TypingError>(res_3);
        error.at = node->loc;
        trace.push_back(node->loc);
        return error;
    }

    auto s_3 = unwrap(res_3);
    return Result{ s_3.apply_to(beta), s_3 + s_2 + s_1 };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<LetNode> node,
    std::vector<SourceLoc>& trace
) {
    auto res_1 = W(gamma, node->bind_value, trace);

    if(is_error(res_1)) {
        trace.push_back(node->loc);
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    TypingContext new_ctx = s_1.apply_to(gamma);
    auto res_2 = W(new_ctx.with(node->bind_var->var, new_ctx.generalize(t_1)), node->ret_value, trace);

    if(is_error(res_2)) {
        trace.push_back(node->loc);
        return res_2;
    }

    auto [t_2, s_2] = unwrap(res_2);
    return Result{ t_2, s_2 + s_1 };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<BranchNode> node,
    std::vector<SourceLoc>& trace
) {
    auto res_1 = W(gamma, node->cond_expr, trace);

    if(is_error(res_1)) {
        trace.push_back(node->loc);
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    auto res_2 = MGU(s_1.apply_to(t_1), make_const_type("Bool"));
    if(is_error(res_2)) {
        auto error = std::get<TypingError>(res_2);
        error.at = node->loc;
        trace.push_back(node->loc);
        return error;
    }
    auto s_2 = unwrap(res_2);
    auto s_2_1 = s_2 + s_1;

    TypingContext new_ctx = s_2_1.apply_to(gamma);
    auto res_3 = W(new_ctx, node->true_expr, trace);
    if(is_error(res_3)) {
        trace.push_back(node->loc);
        return res_3;
    }
    auto [t_2, s_3] = unwrap(res_3);

    auto s_3_2_1 = s_3 + s_2 + s_1;
    new_ctx = s_3_2_1.apply_to(gamma);
    auto res_4 = W(new_ctx, node->false_expr, trace);
    if(is_error(res_4)) {
        trace.push_back(node->loc);
        return res_4;
    }
    auto [t_3, s_4] = unwrap(res_4);

    auto res_5 = MGU(s_4.apply_to(t_2), t_3);
    if(is_error(res_5)) {
        auto error = std::get<TypingError>(res_5);
        error.at = node->loc;
        trace.push_back(node->loc);
        return error;
    }
    auto s_5 = unwrap(res_5);

    return Result{ s_5.apply_to(t_3), s_5 + s_4 + s_3 + s_2 + s_1 };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<FixNode> node,
    std::vector<SourceLoc>& trace
) {
    auto res_1 = W(gamma, node->func, trace);

    if(is_error(res_1)) {
        trace.push_back(node->loc);
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    auto beta = TypeVar::generate_fresh();
    auto res_2 = MGU(s_1.apply_to(t_1), make_func_type(beta, beta));
    if(is_error(res_2)) {
        auto error = std::get<TypingError>(res_2);
        error.at = node->loc;
        trace.push_back(node->loc);
        return error;
    }
    auto s_2 = unwrap(res_2);

    return Result{ s_2.apply_to(beta), s_2 };
}

ResultOrError W(
    TypingContext& gamma,
    std::shared_ptr<AstNode> node,
    std::vector<SourceLoc>& trace
) {
    if(auto const_node = std::dynamic_pointer_cast<ConstNode>(node))
        return W(gamma, const_node, trace);
    if(auto var_node = std::dynamic_pointer_cast<VarNode>(node))
        return W(gamma, var_node, trace);
    if(auto func_node = std::dynamic_pointer_cast<FuncNode>(node))
        return W(gamma, func_node, trace);
    if(auto app_node = std::dynamic_pointer_cast<AppNode>(node))
        return W(gamma, app_node, trace);
    if(auto let_node = std::dynamic_pointer_cast<LetNode>(node))
        return W(gamma, let_node, trace);
    if(auto branch_node = std::dynamic_pointer_cast<BranchNode>(node))
        return W(gamma, branch_node, trace);
    if(auto fix_node = std::dynamic_pointer_cast<FixNode>(node))
        return W(gamma, fix_node, trace);
    
    trace.push_back(node->loc);
    return TypingError{.text = "unknown node type", .at = node->loc};
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
    auto list_identifier = _make_type_var("List");
    auto param = TypeVar::generate_fresh();
    auto list = make_type_constructor(list_identifier, { param });
    auto nil = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { param },
        list
    )); //∀α. List α
    auto cons = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { param },
        make_func_type(param, make_func_type(list, list))
    )); //∀α. α -> List α -> List α
    auto bool_type = make_const_type("Bool");
    auto equal = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { param },
        make_func_type(param, make_func_type(param, bool_type))
    )); //∀α. α -> α -> Bool

    auto hd = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { param },
        make_func_type(list, param)
    )); //∀α. List α -> α
    auto tl = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
        { param },
        make_func_type(list, list)
    )); //∀α. List α -> List α

    auto nil_id = _make_var_node("[]", SourceLoc());
    auto cons_id = _make_var_node("::", SourceLoc());
    auto equal_id = _make_var_node("equal", SourceLoc());
    auto hd_id = _make_var_node("hd", SourceLoc());
    auto tl_id = _make_var_node("tl", SourceLoc());
    TypingContext tctx;
    tctx.set(nil_id->var, nil);
    tctx.set(cons_id->var, cons);
    tctx.set(equal_id->var, equal);
    tctx.set(hd_id->var, hd);
    tctx.set(tl_id->var, tl);

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
    return get_type(unwrap(result))->to_str();
}
