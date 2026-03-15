
#include "ast_def.h"


AstNode::AstNode(const SourceLoc& loc) : loc(loc) {}


ConstNode::ConstNode(const std::string& v, const std::string& t, const SourceLoc& loc) 
    : AstNode(loc), value(v), type(t) {}

std::shared_ptr<AstNode> make_const_node(const std::string& value, const std::string& type, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new ConstNode(value, type, loc));
}


VarNode::VarNode(const std::string& n, const SourceLoc& loc) : AstNode(loc), var(n) {}

std::shared_ptr<VarNode> _make_var_node(const std::string& name, const SourceLoc& loc) {
    return std::shared_ptr<VarNode>(new VarNode(name, loc));
}

std::shared_ptr<AstNode> make_var_node(const std::string& name, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new VarNode(name, loc));
}


FuncNode::FuncNode(const std::shared_ptr<VarNode>& p, const std::shared_ptr<AstNode>& b, const SourceLoc& loc)
    : AstNode(loc), param(p), body(b) {}

std::shared_ptr<AstNode> make_func_node(const std::shared_ptr<VarNode>& param, const std::shared_ptr<AstNode>& body, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new FuncNode(param, body, loc));
}


AppNode::AppNode(const std::shared_ptr<AstNode>& f, const std::shared_ptr<AstNode>& a, const SourceLoc& loc)
    : AstNode(loc), func(f), arg(a) {}

std::shared_ptr<AstNode> make_app_node(const std::shared_ptr<AstNode>& func, const std::shared_ptr<AstNode>& arg, const SourceLoc& loc) {
    return std::shared_ptr<AstNode>(new AppNode(func, arg, loc));
}


LetNode::LetNode(const std::shared_ptr<VarNode>& b_var, 
        const std::shared_ptr<AstNode>& b_val, 
        const std::shared_ptr<AstNode>& r_val, 
        const SourceLoc& loc)
    : AstNode(loc), bind_var(b_var), bind_value(b_val), ret_value(r_val) {}

std::shared_ptr<AstNode> make_let_node(
    const std::shared_ptr<VarNode>& bind_var, 
    const std::shared_ptr<AstNode>& bind_value,
    const std::shared_ptr<AstNode>& ret_value, 
    const SourceLoc& loc) 
{
    return std::shared_ptr<AstNode>(new LetNode(bind_var, bind_value, ret_value, loc));
}


BranchNode::BranchNode(const std::shared_ptr<AstNode>& cond, 
        const std::shared_ptr<AstNode>& tr_e, 
        const std::shared_ptr<AstNode>& f_e, 
        const SourceLoc& loc)
    : AstNode(loc), cond_expr(cond), true_expr(tr_e), false_expr(f_e) {}

std::shared_ptr<AstNode> make_branch_node(
    const std::shared_ptr<AstNode>& cond_expr, 
    const std::shared_ptr<AstNode>& true_expr,
    const std::shared_ptr<AstNode>& false_expr, 
    const SourceLoc& loc) 
{
    return std::shared_ptr<AstNode>(new BranchNode(cond_expr, true_expr, false_expr, loc));
}


FixNode::FixNode(const std::shared_ptr<AstNode>& f, const SourceLoc& loc)
    : AstNode(loc), func(f) {}

std::shared_ptr<AstNode> make_fix_node(const std::shared_ptr<AstNode>& func, const SourceLoc& loc) 
{
    return std::shared_ptr<AstNode>(new FixNode(func, loc));
}