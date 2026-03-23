

#include "inference_manager.h"
#include "command_handler.h"
#include "ast_helpers.h"

#include <fstream>
#include <filesystem>
#include <format>


Type::base_ptr_t get_type(const Result& v) { return std::get<0>(v); }


bool InferenceManager::run_algorithm(bool detailed) {
    this->detailed = detailed;

    auto tctx = make_basic_context();

    NodeOrError parsed;
    switch (state.input_type)
    {
    case InputType::File:
    {
        auto ifstream = std::ifstream(state.input);
        state.expression = std::string(std::istreambuf_iterator<char>{ifstream}, {});
        parsed = LambdaParser(state.expression).parse();
        break;
    }

    case InputType::Text:
        state.expression = state.input;
        parsed = LambdaParser(state.expression).parse();
        break;
    
    default:
        state.result = "Ошибка: выбран неизвестный способ ввода!";
        return false;
    }
    
    if(parsed.is_error()) {
        state.result = print_error(parsed.get_error(), state.input);
        return false;
    }
    state.input_parsed = parsed.unwrap();
    
    switch (state.algorithm)
    {
    case AlgorithmKind::W:
    {
        auto result = W(tctx, state.input_parsed);
        if(result.is_error()) {
            std::ostringstream ss;
            ss << "Ошибка при выводе типа: " << print_error(result.get_error(), state.input);

            state.result = ss.str();
            return false;
        }
        state.result = get_type(result.unwrap())->to_str();
        
        break;
    }
    case AlgorithmKind::M:
    {
        auto type = TypeVar::generate_fresh();
        auto result = M(tctx, state.input_parsed, type);
        if(result.is_error()) {
            std::ostringstream ss;
            ss << "Ошибка при выводе типа: " << print_error(result.get_error(), state.input);

            state.result = ss.str();
            return false;
        }
        state.result = result.unwrap().apply_to(type)->to_str();
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
    state.input_parsed.reset();
}

void InferenceManager::add_algorithm_step(const std::string& step_text, const AstNode::ptr_t& at) {
    if(!detailed) return;
    state.alg_state.steps.push_back(AlgorithmStep{
        .data = step_text,
        .at = at
    });
}


static bool occurs_check(const TypeVar::ptr_t& var, const Type::base_ptr_t& type) {
    if (auto t_var = std::dynamic_pointer_cast<TypeVar>(type)) {
        return t_var->name == var->name;
    } else if (auto t_func = std::dynamic_pointer_cast<FuncType>(type)) {
        return occurs_check(var, t_func->arg_type) || occurs_check(var, t_func->ret_type);
    } else if (auto t_cons = std::dynamic_pointer_cast<TypeConstructor>(type)) {
        for(const auto& arg: t_cons->args) {
            if(occurs_check(var, arg)) return true;
        }
    }
    return false;
}

SubstitutionOrError InferenceManager::MGU(const Type::base_ptr_t& first, const Type::base_ptr_t& second) {
    TypeStringifier ctx;
    if (auto f_var = std::dynamic_pointer_cast<TypeVar>(first)) {
        if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
            if (f_var->name == s_var->name) {
                return {};
            }
        }
        if (occurs_check(f_var, second)) {
            return Error{.text = "бесконечный тип с " + f_var->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
        }
        Substitution sub;
        sub.subs[f_var] = second;
        return sub;
    } else if (auto s_var = std::dynamic_pointer_cast<TypeVar>(second)) {
        if (occurs_check(s_var, first)) {
            return Error{.text = "бесконечный тип с " + s_var->to_str_impl(ctx) + " и " + first->to_str_impl(ctx), .at = SourceLoc()};
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
        return Error{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
    } else if (auto f_func = std::dynamic_pointer_cast<FuncType>(first)) {
        if (auto s_func = std::dynamic_pointer_cast<FuncType>(second)) {
            auto s1 = MGU(f_func->arg_type, s_func->arg_type);
            if(s1.is_error()) {
                auto error = s1.get_error();
                error.text += "\nпри унификации " + f_func->to_str_impl(ctx) + " и " + s_func->to_str_impl(ctx);
                return error;
            }
            auto s1_uw = s1.unwrap();

            auto new_f_ret = s1_uw.apply_to(f_func->ret_type);
            auto new_s_ret = s1_uw.apply_to(s_func->ret_type);

            auto s2 = MGU(new_f_ret, new_s_ret);            
            if(s2.is_error()) {
                auto error = s2.get_error();
                error.text += "\nпри унификации " + f_func->to_str_impl(ctx) + " и " + s_func->to_str_impl(ctx);
                return error;
            }

            return s2.unwrap() + s1_uw;
        }
        return Error{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
    } else if(auto f_constructor = std::dynamic_pointer_cast<TypeConstructor>(first)) {
        if(auto s_constructor = std::dynamic_pointer_cast<TypeConstructor>(second)) {
            if(s_constructor->name->name == f_constructor->name->name 
                && s_constructor->args.size() == f_constructor->args.size()
            ) {
                Substitution s;
                for(int i = 0; i < f_constructor->args.size(); i++) {
                    auto res = MGU(f_constructor->args[i], s_constructor->args[i]);
                    if(res.is_error()) {
                        auto error = res.get_error();
                        error.text += "\nпри унификации " + f_constructor->to_str_impl(ctx) + " и " + s_constructor->to_str_impl(ctx) + " (given and expected types of " + f_constructor->to_str_impl(ctx) + " )";
                        return error;
                    }
                    s = s + res.unwrap();
                }

                return s;
            }
        }
        return Error{.text = "несоответствие типов: " + first->to_str_impl(ctx) + " и " + second->to_str_impl(ctx), .at = SourceLoc()};
    }
    return Error{.text = "неизвестный тип в MGU", .at = SourceLoc()};
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    ConstNode::ptr_t node
) {
    add_algorithm_step(
        std::format("Тип константного выражения {}: {}", node->value, node->type),
        node
    );

    return Result{ make_const_type(node->type), Substitution::make_identity() };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    VarNode::ptr_t node
) {
    auto sub = Substitution::make_identity();
    if(!gamma.has(node->var)) {
        add_algorithm_step(
            std::format("Переменная {} отсутствует в контексте, завершаем алгоритм с ошибкой", node->var),
            node
        );

        return Error{.text = "неизвестная типовая переменная " + node->var, .at = node->loc};
    }
        
    TypeScheme::ptr_t scheme = gamma.get(node->var).second;
    auto inst = scheme->instantiate();
    
    add_algorithm_step(
        std::format("Получаем схему типа переменной {} из контекста: {}\n\nПолучаем экземпляр этой схемы: {}", node->var, scheme->to_str(), inst->to_str(false)),
        node
    );

    return Result{ inst, sub };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    FuncNode::ptr_t node
) {
    auto beta = TypeVar::generate_fresh();
    auto beta_upcasted = std::static_pointer_cast<Type>(beta);
    auto beta_ts = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        std::vector<TypeVar::ptr_t> { },
        beta_upcasted
    ));
    auto new_ctx = InferenceContext(gamma).with(
        node->param->var,
        beta_ts
    );
    
    add_algorithm_step(
        std::format(
            "Создаём свежую типовую переменную {}\n"
            "Копируем контекст типизации, добавляем в неё связку {}: {}\n"
            "Вызываем W с копией контекста на теле функции.",
            beta->to_str(false), node->param->var, beta_ts->to_str()),
        node
    );

    auto res_1 = W(new_ctx, node->body);

    if(res_1.is_error()) {
        return res_1;
    }

    auto [t_1, s_1] = res_1.unwrap();

    auto arg_type = s_1.apply_to(beta_upcasted);
    auto body_type = t_1;
    auto result_type = FuncType::ptr_t(new FuncType(arg_type, body_type));

    add_algorithm_step(
        std::format(
            "Применяем полученную подстановку к свежей переменной {}:\n"
            "Результат - {} (тип аргумента лямбды)\n"
            "Тип тела функции (результат вложенного вызова W) - {}\n"
            "Итоговый тип функции: {}.",
            beta->to_str(false),
            arg_type->to_str(false),
            body_type->to_str(false),
            result_type->to_str(false)
        ),
        node
    );

    return Result{ result_type, s_1 };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    AppNode::ptr_t node
) {
    auto res_1 = W(gamma, node->func);

    if(res_1.is_error()) {
        return res_1;
    }

    auto [t_1, s_1] = res_1.unwrap();

    InferenceContext new_ctx = s_1.apply_to(gamma);
    auto res_2 = W(new_ctx, node->arg);

    if(res_2.is_error()) {
        return res_2;
    }

    auto [t_2, s_2] = res_2.unwrap();

    auto beta = TypeVar::generate_fresh();
    auto res_3 = MGU(
        s_2.apply_to(t_1), 
        make_func_type(t_2, beta)
    );

    if(res_3.is_error()) {
        auto error = res_3.get_error();
        error.at = node->loc;
        return error;
    }

    auto s_3 = res_3.unwrap();
    auto result_type = s_3.apply_to(beta);

    add_algorithm_step(
        std::format(
            "Обработка применения (e1 e2):\n"
            "1) Тип функции e1: {}.\n"
            "2) Тип аргумента e2: {}.\n"
            "3) Вводим свежий результат {} и унифицируем тип функции с {} -> {}.\n"
            "4) Итоговый тип узла после унификации: {}.",
            t_1->to_str(false),
            t_2->to_str(false),
            beta->to_str(false),
            t_2->to_str(false),
            beta->to_str(false),
            result_type->to_str(false)
        ),
        node
    );

    return Result{ result_type, s_3 + s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    LetNode::ptr_t node
) {
    add_algorithm_step(
        "Вычислим тип и подстановку для значения привязки",
        node
    );

    auto res_1 = W(gamma, node->bind_value);

    if(res_1.is_error()) {
        return res_1;
    }
    auto [t_1, s_1] = res_1.unwrap();

    InferenceContext new_ctx = s_1.apply_to(gamma);
    auto t_1_gen = new_ctx.generalize(t_1);
    new_ctx.set(node->bind_var->var, t_1_gen);
    
    add_algorithm_step(
        std::format(
            "Тип значения привязки T1: {}\n"
            "Применяем полученную подстановку к контексту типизации\n"
            "Обобщаем тип значения привязки относительно этого контекста.\n"
            "Полученная схема типа: {}\n"
            "Добавляем в контекст эту схему в связке с переменной {}\n"
            "Вызываем W на новом контексте и возвращаемом значении let.",
            t_1->to_str(false),
            t_1_gen->to_str(),
            node->bind_var->var
        ),
        node
    );

    auto res_2 = W(new_ctx, node->ret_value);

    if(res_2.is_error()) {
        return res_2;
    }

    auto [t_2, s_2] = res_2.unwrap();

    add_algorithm_step(
        std::format(
            "Полученный тип возвращаемого значения T2: {}\n"
            "В качестве подстановки возвращаем композицию S2 и S1.",
            t_2->to_str(false)
        ),
        node
    );

    return Result{ t_2, s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    BranchNode::ptr_t node
) {
    auto res_1 = W(gamma, node->cond_expr);

    if(res_1.is_error()) {
        return res_1;
    }
    auto [t_1, s_1] = res_1.unwrap();

    auto res_2 = MGU(s_1.apply_to(t_1), make_const_type("Bool"));
    if(res_2.is_error()) {
        auto error = res_2.get_error();
        error.at = node->loc;
        return error;
    }
    auto s_2 = res_2.unwrap();
    auto s_2_1 = s_2 + s_1;

    InferenceContext new_ctx = s_2_1.apply_to(gamma);
    auto res_3 = W(new_ctx, node->true_expr);
    if(res_3.is_error()) {
        return res_3;
    }
    auto [t_2, s_3] = res_3.unwrap();

    auto s_3_2_1 = s_3 + s_2 + s_1;
    new_ctx = s_3_2_1.apply_to(gamma);
    auto res_4 = W(new_ctx, node->false_expr);
    if(res_4.is_error()) {
        return res_4;
    }
    auto [t_3, s_4] = res_4.unwrap();

    auto res_5 = MGU(s_4.apply_to(t_2), t_3);
    if(res_5.is_error()) {
        auto error = res_5.get_error();
        error.at = node->loc;
        return error;
    }
    auto s_5 = res_5.unwrap();

    auto result_type = s_5.apply_to(t_3);
    add_algorithm_step(
        std::format(
            "Обработка ветвления if-then-else:\n"
            "1) Тип условия: {} (должен унифицироваться с Bool).\n"
            "2) Тип then-ветки: {}.\n"
            "3) Тип else-ветки: {}.\n"
            "4) После унификации then/else итоговый тип узла: {}.",
            t_1->to_str(false),
            t_2->to_str(false),
            t_3->to_str(false),
            result_type->to_str(false)
        ),
        node
    );

    return Result{ result_type, s_5 + s_4 + s_3 + s_2 + s_1 };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    FixNode::ptr_t node
) {
    auto res_1 = W(gamma, node->func);

    if(res_1.is_error()) {
        return res_1;
    }
    auto [t_1, s_1] = res_1.unwrap();

    auto beta = TypeVar::generate_fresh();
    auto res_2 = MGU(s_1.apply_to(t_1), make_func_type(beta, beta));
    if(res_2.is_error()) {
        auto error = res_2.get_error();
        error.at = node->loc;
        return error;
    }
    auto s_2 = res_2.unwrap();

    auto result_type = s_2.apply_to(beta);
    add_algorithm_step(
        std::format(
            "Обработка fix-узла:\n"
            "1) Тип внутреннего выражения: {}.\n"
            "2) Требуем форму {} -> {} и унифицируем.\n"
            "3) Итоговый тип fix-узла: {}.",
            t_1->to_str(false),
            beta->to_str(false),
            beta->to_str(false),
            result_type->to_str(false)
        ),
        node
    );

    return Result{ result_type, s_2 };
}

ResultOrError InferenceManager::W(
    InferenceContext& gamma,
    AstNode::ptr_t node
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
    
    return Error{.text = "неизвестный тип узла АСД", .at = node->loc};
}


SubstitutionOrError InferenceManager::M(
    InferenceContext& gamma, 
    ConstNode::ptr_t node,
    Type::base_ptr_t expected
) {
    auto const_type = make_const_type(node->type);
    auto result = MGU(expected, const_type);
    if(result.is_error()) {
        auto error = result.get_error();
        error.at = node->loc;
        return error;
    }

    add_algorithm_step(
        std::format(
            "M-алгоритм для константы {}:\n"
            "1) Ожидаемый тип: {}.\n"
            "2) Фактический тип константы: {}.\n"
            "3) Унификация успешна.",
            node->value,
            expected->to_str(false),
            const_type->to_str(false)
        ),
        node
    );

    return result;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, VarNode::ptr_t node, Type::base_ptr_t expected) {
    if(!gamma.has(node->var)) {
        return Error{.text = "неизвестная типовая переменная " + node->var, .at = node->loc};
    }
        
    TypeScheme::ptr_t scheme = gamma.get(node->var).second;
    auto inst = scheme->instantiate();
    auto result = MGU(expected, inst);

    if(result.is_error()) {
        auto error = result.get_error();
        error.at = node->loc;
        return error;
    }

    add_algorithm_step(
        std::format(
            "M-алгоритм для переменной {}:\n"
            "1) Ожидаемый тип: {}.\n"
            "2) Схема из контекста: {}.\n"
            "3) Экземпляр схемы: {}.\n"
            "4) Унификация ожидаемого типа и экземпляра успешна.",
            node->var,
            expected->to_str(false),
            scheme->to_str(),
            inst->to_str(false)
        ),
        node
    );

    return result;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, FuncNode::ptr_t node, Type::base_ptr_t expected) {
    auto b_1 = TypeVar::generate_fresh(), b_2 = TypeVar::generate_fresh();
    auto func_type = make_func_type(b_1, b_2);
    auto result_1 = MGU(expected, func_type);

    if(result_1.is_error()) {
        auto error = result_1.get_error();
        error.at = node->loc;
        return error;
    }
    auto s_1 = result_1.unwrap();

    auto s_1_b_1 = s_1.apply_to(b_1);
    auto new_ctx = s_1.apply_to(gamma).with(
        node->param->var, 
        PolyTypeScheme::ptr_t(new PolyTypeScheme(
            { },
            s_1_b_1
        ))
    );
    auto s_1_b_2 = s_1.apply_to(b_2);
    auto result_2 = M(new_ctx, node->body, s_1_b_2);

    if(result_2.is_error()) {
        return result_2;
    }
    auto s_2 = result_2.unwrap();

    auto result_sub = s_2 + s_1;
    add_algorithm_step(
        std::format(
            "M-алгоритм для λ-узла λ{}.body:\n"
            "1) Ожидаемый тип приводим к функциональному виду b1 -> b2.\n"
            "2) Параметру назначаем тип {}.\n"
            "3) Тело проверяем с ожиданием {}.\n"
            "4) Получаем итоговую композицию подстановок для узла.",
            node->param->var,
            s_1_b_1->to_str(false),
            s_1_b_2->to_str(false)
        ),
        node
    );

    return result_sub;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, AppNode::ptr_t node, Type::base_ptr_t expected) {
    auto beta = TypeVar::generate_fresh();
    auto result_1 = M(gamma, node->func, make_func_type(beta, expected));

    if(result_1.is_error())
        return result_1;
    auto s_1 = result_1.unwrap();

    auto new_ctx = s_1.apply_to(gamma);
    auto s_1_beta = s_1.apply_to(beta);
    auto result_2 = M(new_ctx, node->arg, s_1_beta);

    if(result_2.is_error())
        return result_2;
    auto s_2 = result_2.unwrap();

    add_algorithm_step(
        std::format(
            "M-алгоритм для применения (e1 e2):\n"
            "1) Для функции ожидаем тип {} -> {}.\n"
            "2) После первой подстановки проверяем аргумент с ожиданием {}.\n"
            "3) Комбинируем подстановки, получая результат для узла.",
            beta->to_str(false),
            expected->to_str(false),
            s_1_beta->to_str(false)
        ),
        node
    );

    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, LetNode::ptr_t node, Type::base_ptr_t expected) {
    auto beta = TypeVar::generate_fresh();
    auto result_1 = M(gamma, node->bind_value, beta);

    if(result_1.is_error())
        return result_1;
    auto s_1 = result_1.unwrap();

    auto new_ctx = s_1.apply_to(gamma);
    auto s_t_1 = s_1.apply_to(beta);
    new_ctx.set(node->bind_var->var, new_ctx.generalize(s_t_1));

    auto s_1_expected = s_1.apply_to(expected);
    auto result_2 = M(new_ctx, node->ret_value, s_1_expected);

    if(result_2.is_error())
        return result_2;
    auto s_2 = result_2.unwrap();

    add_algorithm_step(
        std::format(
            "M-алгоритм для let-узла let {} = e1 in e2:\n"
            "1) Проверяем e1 против свежего типа {}.\n"
            "2) Обобщаем полученный тип и расширяем контекст.\n"
            "3) Проверяем e2 против ожидаемого типа {} (с учётом подстановки).\n"
            "4) Итог — композиция подстановок текущего узла.",
            node->bind_var->var,
            beta->to_str(false),
            s_1_expected->to_str(false)
        ),
        node
    );

    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, BranchNode::ptr_t node, Type::base_ptr_t expected) {
    auto result_1 = M(gamma, node->cond_expr, make_const_type("Bool"));
    if(result_1.is_error()) {
        return result_1;
    }
    auto s_1 = result_1.unwrap();

    auto new_ctx = s_1.apply_to(gamma);
    auto s_1_expected = s_1.apply_to(expected);

    auto result_2 = M(new_ctx, node->true_expr, s_1_expected);
    if(result_2.is_error()) {
        return result_2;
    }
    auto s_2 = result_2.unwrap();

    auto s_2_1 = s_2 + s_1;
    new_ctx = s_2_1.apply_to(gamma);
    auto s_2_1_expected = s_2_1.apply_to(expected);

    auto result_3 = M(new_ctx, node->false_expr, s_2_1_expected);
    if(result_3.is_error()) {
        return result_3;
    }
    auto s_3 = result_3.unwrap();

    return s_3 + s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(InferenceContext& gamma, FixNode::ptr_t node, Type::base_ptr_t expected) {
    auto beta = TypeVar::generate_fresh();
    auto func = make_func_type(beta, beta);
    
    auto result_1 = M(gamma, node->func, func);
    if(result_1.is_error()) {
        return result_1;
    }
    auto s_1 = result_1.unwrap();

    auto s_1_expected = s_1.apply_to(expected);
    auto s_1_beta = s_1.apply_to(beta);

    auto result_2 = MGU(s_1_expected, s_1_beta);
    if(result_2.is_error()) {
        return result_2;
    }
    auto s_2 = result_2.unwrap();

    return s_2 + s_1;
}

SubstitutionOrError InferenceManager::M(
    InferenceContext& gamma, 
    AstNode::ptr_t node,
    Type::base_ptr_t expected
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
    
    return Error{.text = "неизвестный тип узла АСД", .at = node->loc};
}


std::string InferenceManager::highlight_loc(const std::string& source, const SourceLoc& loc) {
    SourceLoc::pos_t source_show_start = 0;
    SourceLoc::pos_t source_show_end   = source.size();

    std::ostringstream ss;
    ss << source.substr(source_show_start, source_show_end - source_show_start) << std::endl;
    for(auto i = source_show_start; i <= source_show_end; i++) { 
        if(i >= loc.start && i < loc.end) ss << '~';
        else ss << ' ';
    }
    return ss.str();
}

InferenceContext InferenceManager::make_basic_context() {    
    auto list_identifier = make_type_var("List");
    auto param = TypeVar::generate_fresh();
    auto list = make_type_constructor(list_identifier, { param });
    auto int_type = make_const_type("Int");
    auto bool_type = make_const_type("Bool");

    auto nil = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        { param },
        list
    )); //∀α. List α
    auto cons = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        { param },
        make_func_type(param, make_func_type(list, list))
    )); //∀α. α -> List α -> List α

    auto equal = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        { param },
        make_func_type(param, make_func_type(param, bool_type))
    )); //∀α. α -> α -> Bool

    auto hd = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        { param },
        make_func_type(list, param)
    )); //∀α. List α -> α
    auto tl = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        { param },
        make_func_type(list, list)
    )); //∀α. List α -> List α

    auto add = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(int_type, make_func_type(int_type, int_type))
    )); // Int -> Int -> Int
    auto sub = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(int_type, make_func_type(int_type, int_type))
    )); // Int -> Int -> Int
    auto mul = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(int_type, make_func_type(int_type, int_type))
    )); // Int -> Int -> Int
    auto div = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(int_type, make_func_type(int_type, int_type))
    )); // Int -> Int -> Int

    auto and_fn = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, make_func_type(bool_type, bool_type))
    )); // Bool -> Bool -> Bool
    auto or_fn = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, make_func_type(bool_type, bool_type))
    )); // Bool -> Bool -> Bool
    auto not_fn = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, bool_type)
    )); // Bool -> Bool
    auto and_op = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, make_func_type(bool_type, bool_type))
    )); // Bool -> Bool -> Bool
    auto or_op = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, make_func_type(bool_type, bool_type))
    )); // Bool -> Bool -> Bool
    auto not_op = PolyTypeScheme::ptr_t(new PolyTypeScheme(
        {},
        make_func_type(bool_type, bool_type)
    )); // Bool -> Bool

    auto nil_id = make_var_node("[]", SourceLoc());
    auto cons_id = make_var_node("::", SourceLoc());
    auto equal_id = make_var_node("equal", SourceLoc());
    auto eq_op_id = make_var_node("=", SourceLoc());
    auto hd_id = make_var_node("hd", SourceLoc());
    auto tl_id = make_var_node("tl", SourceLoc());
    auto add_id = make_var_node("+", SourceLoc());
    auto sub_id = make_var_node("-", SourceLoc());
    auto mul_id = make_var_node("*", SourceLoc());
    auto div_id = make_var_node("/", SourceLoc());
    auto and_id = make_var_node("and", SourceLoc());
    auto or_id = make_var_node("or", SourceLoc());
    auto not_id = make_var_node("not", SourceLoc());
    auto and_op_id = make_var_node("&&", SourceLoc());
    auto or_op_id = make_var_node("||", SourceLoc());
    auto not_op_id = make_var_node("!", SourceLoc());
    InferenceContext tctx;
    tctx.set(nil_id->var, nil);
    tctx.set(cons_id->var, cons);
    tctx.set(equal_id->var, equal);
    tctx.set(eq_op_id->var, equal);
    tctx.set(hd_id->var, hd);
    tctx.set(tl_id->var, tl);
    tctx.set(add_id->var, add);
    tctx.set(sub_id->var, sub);
    tctx.set(mul_id->var, mul);
    tctx.set(div_id->var, div);
    tctx.set(and_id->var, and_fn);
    tctx.set(or_id->var, or_fn);
    tctx.set(not_id->var, not_fn);
    tctx.set(and_op_id->var, and_op);
    tctx.set(or_op_id->var, or_op);
    tctx.set(not_op_id->var, not_op);

    return tctx;
}

std::string InferenceManager::print_error(const Error& error, const std::string& source) {
    std::ostringstream ss;
    ss << error.text << std::endl;
    ss << highlight_loc(source, error.at) << std::endl;
    return ss.str();
}


#define INFERENCE_COMMAND_HANDLER_DEF(arg) COMMAND_HANDLER_DEF(arg, CommandHandler)

CommandHandler::CommandHandler(InferenceManager& mgr) : mgr(mgr) {}

INFERENCE_COMMAND_HANDLER_DEF(RunAlgorithmCommand) {
    auto ok = mgr.run_algorithm(command.detailed);
    event_buf.push(AlgorithmFinishedEvent{.ok = ok, .new_inf_mgr_state = mgr.state});
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
