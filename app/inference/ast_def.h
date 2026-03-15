
#ifndef AST_DEF_H
#define AST_DEF_H

#include "hm_def/type.h"
#include "source_loc.h"
#include <string>


struct AstNode {
    SourceLoc loc;

    AstNode(const SourceLoc& loc);

    virtual std::string to_str() const = 0;
};


struct ConstNode : AstNode {
    std::string value;
    std::shared_ptr<Type> type;

    virtual std::string to_str() const;

    ConstNode(const std::string& v, const std::shared_ptr<Type>& t, const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_const_node(const std::string& value, const std::shared_ptr<Type>& type, const SourceLoc& loc);


struct VarNode : AstNode {
    std::string var;

    virtual std::string to_str() const;

    VarNode(const std::string& n, const SourceLoc& loc);
};

std::shared_ptr<VarNode> _make_var_node(const std::string& name, const SourceLoc& loc);

std::shared_ptr<AstNode> make_var_node(const std::string& name, const SourceLoc& loc);


struct FuncNode : AstNode { //λx.e
    std::shared_ptr<VarNode> param; //x
    std::shared_ptr<AstNode> body; //e

    virtual std::string to_str() const;

    FuncNode(const std::shared_ptr<VarNode>& p, const std::shared_ptr<AstNode>& b, const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_func_node(const std::shared_ptr<VarNode>& param, const std::shared_ptr<AstNode>& body, const SourceLoc& loc);


struct AppNode : AstNode { //e1 e2
    std::shared_ptr<AstNode> func; //e1
    std::shared_ptr<AstNode> arg; //e2

    virtual std::string to_str() const;

    AppNode(const std::shared_ptr<AstNode>& f, const std::shared_ptr<AstNode>& a, const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_app_node(const std::shared_ptr<AstNode>& func, const std::shared_ptr<AstNode>& arg, const SourceLoc& loc);


struct LetNode : AstNode { //let x = e1 in e2
    std::shared_ptr<VarNode> bind_var; //x
    std::shared_ptr<AstNode> bind_value; //e1
    std::shared_ptr<AstNode> ret_value; //e2

    virtual std::string to_str() const;

    LetNode(const std::shared_ptr<VarNode>& b_var, 
            const std::shared_ptr<AstNode>& b_val, 
            const std::shared_ptr<AstNode>& r_val, 
            const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_let_node(
    const std::shared_ptr<VarNode>& bind_var, 
    const std::shared_ptr<AstNode>& bind_value,
    const std::shared_ptr<AstNode>& ret_value, 
    const SourceLoc& loc);


struct BranchNode : AstNode { //if e1 then e2 else e3
    std::shared_ptr<AstNode> cond_expr; //e1
    std::shared_ptr<AstNode> true_expr; //e2
    std::shared_ptr<AstNode> false_expr; //e3

    virtual std::string to_str() const;

    BranchNode(const std::shared_ptr<AstNode>& cond, 
            const std::shared_ptr<AstNode>& tr_e, 
            const std::shared_ptr<AstNode>& f_e, 
            const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_branch_node(
    const std::shared_ptr<AstNode>& cond_expr, 
    const std::shared_ptr<AstNode>& true_expr,
    const std::shared_ptr<AstNode>& false_expr, 
    const SourceLoc& loc);


struct FixNode : AstNode { //fix e
    std::shared_ptr<AstNode> func; //e

    virtual std::string to_str() const;

    FixNode(const std::shared_ptr<AstNode>& f, const SourceLoc& loc);
};

std::shared_ptr<AstNode> make_fix_node(const std::shared_ptr<AstNode>& func, const SourceLoc& loc);

#endif