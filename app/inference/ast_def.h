
#ifndef AST_DEF_H
#define AST_DEF_H

#include "hm_def/type.h"
#include "source_loc.h"
#include <string>


struct AstNode {
    SourceLoc loc;

    AstNode(const SourceLoc& loc) : loc(loc) {};

    virtual std::string to_str() const = 0;
};


struct ConstNode : AstNode {
    std::string value;
    std::shared_ptr<Type> type;

    virtual std::string to_str() const { return value; }

    ConstNode(const std::string& v, const std::shared_ptr<Type>& t, const SourceLoc& loc) 
        : AstNode(loc), value(v), type(t) {};
};

std::shared_ptr<AstNode> make_const_node(const std::string& value, const std::shared_ptr<Type>& type, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new ConstNode(value, type, loc));
}


struct VarNode : AstNode {
    Variable var;

    virtual std::string to_str() const { return var.name; }

    VarNode(const std::string& n, const SourceLoc& loc) : AstNode(loc), var(n) {};
};

std::shared_ptr<VarNode> _make_var_node(const std::string& name, const SourceLoc& loc) {
    return std::shared_ptr<VarNode>(new VarNode(name, loc));
}

std::shared_ptr<AstNode> make_var_node(const std::string& name, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new VarNode(name, loc));
}


struct FuncNode : AstNode { //λx.e
    std::shared_ptr<VarNode> param; //x
    std::shared_ptr<AstNode> body; //e

    virtual std::string to_str() const { return "(λ" + param->to_str() + ". " + body->to_str() + ")"; }

    FuncNode(const std::shared_ptr<VarNode>& p, const std::shared_ptr<AstNode>& b, const SourceLoc& loc)
        : AstNode(loc), param(p), body(b)
    {};
};

std::shared_ptr<AstNode> make_func_node(const std::shared_ptr<VarNode>& param, const std::shared_ptr<AstNode>& body, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new FuncNode(param, body, loc));
}


struct AppNode : AstNode { //e1 e2
    std::shared_ptr<AstNode> func; //e1
    std::shared_ptr<AstNode> arg; //e2

    virtual std::string to_str() const { return "(" + func->to_str() + " " + arg->to_str() + ")"; }

    AppNode(const std::shared_ptr<AstNode>& f, const std::shared_ptr<AstNode>& a, const SourceLoc& loc)
        : AstNode(loc), func(f), arg(a)
    {};
};

std::shared_ptr<AstNode> make_app_node(const std::shared_ptr<AstNode>& func, const std::shared_ptr<AstNode>& arg, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new AppNode(func, arg, loc));
}


struct LetNode : AstNode { //let x = e1 in e2
    std::shared_ptr<VarNode> bind_var; //x
    std::shared_ptr<AstNode> bind_value; //e1
    std::shared_ptr<AstNode> ret_value; //e2

    virtual std::string to_str() const { return "let " + bind_var->to_str() + " = " + bind_value->to_str() + " in " + ret_value->to_str(); }

    LetNode(const std::shared_ptr<VarNode>& b_var, 
            const std::shared_ptr<AstNode>& b_val, 
            const std::shared_ptr<AstNode>& r_val, 
            const SourceLoc& loc)
        : AstNode(loc), bind_var(b_var), bind_value(b_val), ret_value(r_val)
    {};
};

std::shared_ptr<AstNode> make_let_node(
    const std::shared_ptr<VarNode>& bind_var, 
    const std::shared_ptr<AstNode>& bind_value,
    const std::shared_ptr<AstNode>& ret_value, 
    const SourceLoc& loc) 
{
    return std::shared_ptr<AstNode>(new LetNode(bind_var, bind_value, ret_value, loc));
}

#endif