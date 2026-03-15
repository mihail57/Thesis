

#include "command_handler.h"


Type::base_ptr get_type(const Result& v) { return std::get<0>(v); }

bool is_error(const ResultOrError& v) { return std::holds_alternative<TypingError>(v); }
TypingError get_error(const ResultOrError& v) { return std::get<TypingError>(v); }
Result unwrap(const ResultOrError& v) { return std::get<Result>(v); }


bool InferenceManager::run_algorithm(bool detailed) {
    this->detailed = detailed;

    auto tctx = make_basic_context();

    NodeOrError parsed;
    switch (state.input_type)
    {
    case InputType::Lambda:
        parsed = parse(state.input);
        break;
    
    default:
        state.result = "Ошибка: выбран неизвестный способ ввода!";
        return false;
    }
    
    if(is_error(parsed)) {
        state.result = print_error(get_error(parsed), state.input);
        return false;
    }
    auto root_node = unwrap(parsed);
    
    switch (state.algorithm)
    {
    case AlgorithmKind::W:
    {
        auto result = W(tctx, root_node);
        if(is_error(result)) {
            std::ostringstream ss;
            ss << "Ошибка при выводе типа: " << print_error(get_error(result), state.input);

            state.result = ss.str();
            return false;
        }
        state.result = get_type(unwrap(result))->to_str();
        
        break;
    }
    case AlgorithmKind::M:
    {
        auto type = TypeVar::generate_fresh();
        auto result = M(tctx, root_node, type);
        if(is_error(result)) {
            std::ostringstream ss;
            ss << "Ошибка при выводе типа: " << print_error(std::get<TypingError>(result), state.input);

            state.result = ss.str();
            return false;
        }
        state.result = unwrap(result).apply_to(type)->to_str();
        break;
    }
    default:
        state.result = "Ошибка: выбран неизвестный алгоритм!";
        return false;
    }

    return true;
}

void InferenceManager::change_input(const std::string& new_input) {
    reset();
    state.input = new_input;
}

void InferenceManager::change_input_type(InputType new_input_type) {
    reset();
    state.input_type = new_input_type;
}

void InferenceManager::change_algorithm(AlgorithmKind new_algorithm) {
    reset();
    state.algorithm = new_algorithm;
}

void InferenceManager::reset() {
    state.alg_state.steps.clear();
    state.result.clear();
}

SubstitutionOrError InferenceManager::MGU(const std::shared_ptr<Type>& first, const std::shared_ptr<Type>& second) {
    TypeStringifier ctx;
    if (auto f_var = std::dynamic_pointer_cast<TypeVar>(first)) {
        if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
            if (f_var->name == s_var->name) {
                return {};
            }
        }
        if (occurs_check(f_var, second)) {
            return TypingError{.text = "бесконечный тип с " + f_var->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
        }
        Substitution sub;
        sub.subs[f_var] = second;
        return sub;
    } else if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
        if (occurs_check(s_var, first)) {
            return TypingError{.text = "бесконечный тип с " + s_var->to_str_impl(ctx) + " и " + first->to_str_impl(ctx), .at = SourceLoc()};
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
        return TypingError{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
    } else if (auto f_func = std::dynamic_pointer_cast<FuncType>(first)) {
        if (auto s_func = std::dynamic_pointer_cast<FuncType>(second)) {
            auto s1 = MGU(f_func->arg_type, s_func->arg_type);
            if(is_error(s1)) {
                auto error = std::get<TypingError>(s1);
                error.text += "\nпри унификации " + f_func->to_str_impl(ctx) + " и " + s_func->to_str_impl(ctx);
                return error;
            }
            auto s1_uw = unwrap(s1);

            auto new_f_ret = s1_uw.apply_to(f_func->ret_type);
            auto new_s_ret = s1_uw.apply_to(s_func->ret_type);

            auto s2 = MGU(new_f_ret, new_s_ret);            
            if(is_error(s2)) {
                auto error = std::get<TypingError>(s2);
                error.text += "\nпри унификации " + f_func->to_str_impl(ctx) + " и " + s_func->to_str_impl(ctx);
                return error;
            }

            return unwrap(s2) + s1_uw;
        }
        return TypingError{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
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
                        error.text += "\nпри унификации " + f_constructor->to_str_impl(ctx) + " и " + s_constructor->to_str_impl(ctx) + " (given and expected types of " + f_constructor->to_str_impl(ctx) + " )";
                        return error;
                    }
                    s = s + unwrap(res);
                }

                return s;
            }
        }
        return TypingError{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
    }
    return TypingError{.text = "неизвестный тип в MGU", .at = SourceLoc()};
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<ConstNode> node
) {
    return Result{ std::shared_ptr<Type>(node->type), Substitution::make_identity() };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<VarNode> node
) {
    auto sub = Substitution::make_identity();
    if(!gamma.has(node->var)) {
        return TypingError{.text = "неизвестная типовая переменная " + node->to_str(), .at = node->loc};
    }
        
    std::shared_ptr<TypeScheme> scheme = gamma.get(node->var).second;
    return Result{ scheme->instantiate(), sub };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<FuncNode> node
) {
    auto beta = TypeVar::generate_fresh();
    auto beta_upcasted = std::static_pointer_cast<Type>(beta);
    auto new_ctx = TypingContext(gamma).with(
        node->param->var,
        std::shared_ptr<TypeScheme>(new PolyTypeScheme(
            std::vector<std::shared_ptr<TypeVar>> { },
            beta_upcasted
        ))
    );
    auto res_1 = W(new_ctx, node->body);

    if(is_error(res_1)) {
        return res_1;
    }

    auto [t_1, s_1] = unwrap(res_1);

    return Result{ std::shared_ptr<Type>(new FuncType(
        s_1.apply_to(beta_upcasted),
        unwrap(t_1)
    )), s_1 };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<AppNode> node
) {
    auto res_1 = W(gamma, node->func);

    if(is_error(res_1)) {
        return res_1;
    }

    auto [t_1, s_1] = unwrap(res_1);

    TypingContext new_ctx = s_1.apply_to(gamma);
    auto res_2 = W(new_ctx, node->arg);

    if(is_error(res_2)) {
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
        return error;
    }

    auto s_3 = unwrap(res_3);
    return Result{ s_3.apply_to(beta), s_3 + s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<LetNode> node
) {
    auto res_1 = W(gamma, node->bind_value);

    if(is_error(res_1)) {
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    TypingContext new_ctx = s_1.apply_to(gamma);
    new_ctx.set(node->bind_var->var, new_ctx.generalize(t_1));
    auto res_2 = W(new_ctx, node->ret_value);

    if(is_error(res_2)) {
        return res_2;
    }

    auto [t_2, s_2] = unwrap(res_2);
    return Result{ t_2, s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<BranchNode> node
) {
    auto res_1 = W(gamma, node->cond_expr);

    if(is_error(res_1)) {
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    auto res_2 = MGU(s_1.apply_to(t_1), make_const_type("Bool"));
    if(is_error(res_2)) {
        auto error = std::get<TypingError>(res_2);
        error.at = node->loc;
        return error;
    }
    auto s_2 = unwrap(res_2);
    auto s_2_1 = s_2 + s_1;

    TypingContext new_ctx = s_2_1.apply_to(gamma);
    auto res_3 = W(new_ctx, node->true_expr);
    if(is_error(res_3)) {
        return res_3;
    }
    auto [t_2, s_3] = unwrap(res_3);

    auto s_3_2_1 = s_3 + s_2 + s_1;
    new_ctx = s_3_2_1.apply_to(gamma);
    auto res_4 = W(new_ctx, node->false_expr);
    if(is_error(res_4)) {
        return res_4;
    }
    auto [t_3, s_4] = unwrap(res_4);

    auto res_5 = MGU(s_4.apply_to(t_2), t_3);
    if(is_error(res_5)) {
        auto error = std::get<TypingError>(res_5);
        error.at = node->loc;
        return error;
    }
    auto s_5 = unwrap(res_5);

    return Result{ s_5.apply_to(t_3), s_5 + s_4 + s_3 + s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<FixNode> node
) {
    auto res_1 = W(gamma, node->func);

    if(is_error(res_1)) {
        return res_1;
    }
    auto [t_1, s_1] = unwrap(res_1);

    auto beta = TypeVar::generate_fresh();
    auto res_2 = MGU(s_1.apply_to(t_1), make_func_type(beta, beta));
    if(is_error(res_2)) {
        auto error = std::get<TypingError>(res_2);
        error.at = node->loc;
        return error;
    }
    auto s_2 = unwrap(res_2);

    return Result{ s_2.apply_to(beta), s_2 };
}

ResultOrError InferenceManager::W(
    TypingContext& gamma,
    std::shared_ptr<AstNode> node
) {
    if(auto const_node = std::dynamic_pointer_cast<ConstNode>(node))
        return W(gamma, const_node);
    if(auto var_node = std::dynamic_pointer_cast<VarNode>(node))
        return W(gamma, var_node);
    if(auto func_node = std::dynamic_pointer_cast<FuncNode>(node))
        return W(gamma, func_node);
    if(auto app_node = std::dynamic_pointer_cast<AppNode>(node))
        return W(gamma, app_node);
    if(auto let_node = std::dynamic_pointer_cast<LetNode>(node))
        return W(gamma, let_node);
    if(auto branch_node = std::dynamic_pointer_cast<BranchNode>(node))
        return W(gamma, branch_node);
    if(auto fix_node = std::dynamic_pointer_cast<FixNode>(node))
        return W(gamma, fix_node);
    
    return TypingError{.text = "неизвестный тип узла АСД", .at = node->loc};
}


SubstitutionOrError InferenceManager::M(
    TypingContext& gamma, 
    std::shared_ptr<ConstNode> node, 
    Type::base_ptr expected
) {
    auto result = MGU(expected, node->type);
    if(is_error(result)) {
        auto error = std::get<TypingError>(result);
        error.at = node->loc;
        return error;
    }
    return result;
}

SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<VarNode> node, Type::base_ptr expected) {
    if(!gamma.has(node->var)) {
        return TypingError{.text = "неизвестная типовая переменная " + node->to_str(), .at = node->loc};
    }
        
    std::shared_ptr<TypeScheme> scheme = gamma.get(node->var).second;
    auto result = MGU(expected, scheme->instantiate());

    if(is_error(result)) {
        auto error = std::get<TypingError>(result);
        error.at = node->loc;
        return error;
    }
    return result;
}

SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<FuncNode> node, Type::base_ptr expected) {
    auto b_1 = TypeVar::generate_fresh(), b_2 = TypeVar::generate_fresh();
    auto func_type = make_func_type(b_1, b_2);
    auto result_1 = MGU(expected, func_type);

    if(is_error(result_1)) {
        auto error = std::get<TypingError>(result_1);
        error.at = node->loc;
        return error;
    }
    auto s_1 = unwrap(result_1);

    auto new_ctx = s_1.apply_to(gamma).with(
        node->param->var, 
        std::shared_ptr<TypeScheme>(new PolyTypeScheme(
            { },
            s_1.apply_to(b_1)
        ))
    );
    auto result_2 = M(new_ctx, node->body, s_1.apply_to(b_2));

    if(is_error(result_2)) {
        return result_2;
    }
    auto s_2 = unwrap(result_2);
    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<AppNode> node, Type::base_ptr expected) {
    auto beta = TypeVar::generate_fresh();
    auto result_1 = M(gamma, node->func, make_func_type(beta, expected));

    if(is_error(result_1))
        return result_1;
    auto s_1 = unwrap(result_1);

    auto new_ctx = s_1.apply_to(gamma);
    auto result_2 = M(new_ctx, node->arg, s_1.apply_to(beta));

    if(is_error(result_2))
        return result_2;
    auto s_2 = unwrap(result_2);

    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<LetNode> node, Type::base_ptr expected) {
    auto beta = TypeVar::generate_fresh();
    auto result_1 = M(gamma, node->bind_value, beta);

    if(is_error(result_1))
        return result_1;
    auto s_1 = unwrap(result_1);

    auto new_ctx = s_1.apply_to(gamma);
    auto s_t_1 = s_1.apply_to(beta);
    new_ctx.set(node->bind_var->var, new_ctx.generalize(s_t_1));

    auto result_2 = M(new_ctx, node->ret_value, s_1.apply_to(expected));

    if(is_error(result_2))
        return result_2;
    auto s_2 = unwrap(result_2);

    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<BranchNode> node, Type::base_ptr expected) {
    return TypingError{.text = "Не имплементированно", .at = node->loc};
}
SubstitutionOrError InferenceManager::M(TypingContext& gamma, std::shared_ptr<FixNode> node, Type::base_ptr expected) {    
    return TypingError{.text = "Не имплементированно", .at = node->loc};
}

SubstitutionOrError InferenceManager::M(
    TypingContext& gamma, 
    std::shared_ptr<AstNode> node, 
    Type::base_ptr expected
) {
    if(auto const_node = std::dynamic_pointer_cast<ConstNode>(node))
        return M(gamma, const_node, expected);
    if(auto var_node = std::dynamic_pointer_cast<VarNode>(node))
        return M(gamma, var_node, expected);
    if(auto func_node = std::dynamic_pointer_cast<FuncNode>(node))
        return M(gamma, func_node, expected);
    if(auto app_node = std::dynamic_pointer_cast<AppNode>(node))
        return M(gamma, app_node, expected);
    if(auto let_node = std::dynamic_pointer_cast<LetNode>(node))
        return M(gamma, let_node, expected);
    if(auto branch_node = std::dynamic_pointer_cast<BranchNode>(node))
        return M(gamma, branch_node, expected);
    if(auto fix_node = std::dynamic_pointer_cast<FixNode>(node))
        return M(gamma, fix_node, expected);
    
    return TypingError{.text = "неизвестный тип узла АСД", .at = node->loc};
}


std::string InferenceManager::highlight_loc(const std::string& source, const SourceLoc& loc) {
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

TypingContext InferenceManager::make_basic_context() {    
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

std::string InferenceManager::print_error(const TypingError& error, const std::string& source) {
    std::ostringstream ss;
    ss << error.text << std::endl;
    ss << highlight_loc(source, error.at) << std::endl;
    return ss.str();
}


#define INFERENCE_COMMAND_HANDLER_DEF(arg) COMMAND_HANDLER_DEF(arg, CommandHandler)

CommandHandler::CommandHandler(InferenceManager& mgr) : mgr(mgr) {}

INFERENCE_COMMAND_HANDLER_DEF(RunAlgorithmCommand) {
    auto ok = mgr.run_algorithm(command.detailed);
    event_buf.push(AlgorithmFinishedEvent{.ok = ok});
}

INFERENCE_COMMAND_HANDLER_DEF(SetAlgorithmCommand) {
    mgr.change_algorithm(command.new_kind);
}

INFERENCE_COMMAND_HANDLER_DEF(SetInputCommand) {
    mgr.change_input(command.new_input);
}

INFERENCE_COMMAND_HANDLER_DEF(SetInputTypeCommand) {
    mgr.change_input_type(command.new_input_type);
}