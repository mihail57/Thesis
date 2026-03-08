
#include <iostream>
#include "app/inference/inference_manager.h"

void list_test() {
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

    auto result = infer(":: 12 []", tctx);
    std::cout << result;
}

int main() {
    // auto a = make_const_type("a");    
    // auto var_node = _make_var_node("a");
    // auto const_node = make_const_node("12", a);
    // auto func_node = make_func_node(var_node, var_node); //Функция, возвращающая свой аргумент
    // // auto app_node = make_app_node(func_node, const_node); //Применение предыдущей функции к аргументу "12" типа a

    // auto let_var = _make_var_node("x");
    // // auto let_node = make_let_node(let_var, app_node, let_var); //let x = (λa.a 12) in x
    
    // auto x = make_const_type("tx");
    // auto x_val = make_const_node("a", x);
    // // auto func_node_const = make_func_node(var_node, x_val); // λx."a" (тип <t0> -> tx)
    // // auto ret = make_app_node(func_node_const, let_var); // (λb."a" x)
    // // auto let_node_2 = make_let_node(let_var, app_node, ret); //let x = (λa.a 12) in (λb."a" x) (ожидаемый тип - tx)

    // // auto i_var = _make_var_node("i");
    // // auto damas_example_1 = make_let_node(i_var, func_node, make_app_node(i_var, i_var));

    // auto self_application = make_func_node(var_node, make_app_node(var_node, var_node));
    // auto let_self_application = make_let_node(let_var, self_application, let_var);
    // auto non_applicable = make_app_node(make_app_node(func_node, const_node), const_node);
    // auto type_reuse = make_let_node(let_var, func_node, make_app_node(make_app_node(let_var, const_node), make_app_node(let_var, x_val)));

    // auto f = _make_var_node("f"), g = _make_var_node("g");
    // auto something = make_let_node(f, func_node, make_let_node(g, make_app_node(f, f), make_app_node(g, const_node)));
    // auto generalize_check = make_let_node(f, func_node, make_let_node(g, make_app_node(f, const_node), make_app_node(f, x_val)));
    
    // TypingContext tctx;

    // // auto type_scheme_with_type_var = std::shared_ptr<TypeScheme>(new PolyTypeScheme(
    // //     std::vector<std::shared_ptr<TypeVar>>{ },
    // //     make_func_type(c, make_func_type(d, a))
    // // ));
    // // tctx.set(var_node->var, type_scheme_with_type_var);

   

    // std::vector<std::string> trace;
    // auto result = W(tctx, generalize_check, trace);
    // if(result)
    //     std::cout << result->to_str();
    // else {
    //     for(const auto& what : trace)
    //         std::cout << what << std::endl;
    // }

    // list_test();

    std::string test;
    std::getline(std::cin, test);
    auto result = infer(test);
    std::cout << result;
}