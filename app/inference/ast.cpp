
#include "ast_def.h"
#include "ast_helpers.h"


AstNode::AstNode(const SourceLoc& loc) : loc(loc) {}


ConstNode::ConstNode(const std::string& v, const std::string& t, const SourceLoc& loc) 
    : AstNode(loc), value(v), type(t) {}

ConstNode::ptr_t make_const_node(const std::string& value, const std::string& type, const SourceLoc& loc) {
    return ConstNode::ptr_t(new ConstNode(value, type, loc));
}


VarNode::VarNode(const std::string& n, const SourceLoc& loc) : AstNode(loc), var(n) {}

VarNode::ptr_t make_var_node(const std::string& name, const SourceLoc& loc) {
    return VarNode::ptr_t(new VarNode(name, loc));
}


FuncNode::FuncNode(const VarNode::ptr_t& p, const AstNode::ptr_t& b, const SourceLoc& loc)
    : AstNode(loc), param(p), body(b) {}

FuncNode::ptr_t make_func_node(const VarNode::ptr_t& param, const AstNode::ptr_t& body, const SourceLoc& loc) {
    return FuncNode::ptr_t(new FuncNode(param, body, loc));
}


AppNode::AppNode(const AstNode::ptr_t& f, const AstNode::ptr_t& a, const SourceLoc& loc)
    : AstNode(loc), func(f), arg(a) {}

AppNode::ptr_t make_app_node(const AstNode::ptr_t& func, const AstNode::ptr_t& arg, const SourceLoc& loc) {
    return AppNode::ptr_t(new AppNode(func, arg, loc));
}


LetNode::LetNode(const VarNode::ptr_t& b_var,
        const AstNode::ptr_t& b_val,
        const AstNode::ptr_t& r_val,
        const SourceLoc& loc)
    : AstNode(loc), bind_var(b_var), bind_value(b_val), ret_value(r_val) {}

LetNode::ptr_t make_let_node(
    const VarNode::ptr_t& bind_var,
    const AstNode::ptr_t& bind_value,
    const AstNode::ptr_t& ret_value,
    const SourceLoc& loc) 
{
    return LetNode::ptr_t(new LetNode(bind_var, bind_value, ret_value, loc));
}


BranchNode::BranchNode(const AstNode::ptr_t& cond,
        const AstNode::ptr_t& tr_e,
        const AstNode::ptr_t& f_e,
        const SourceLoc& loc)
    : AstNode(loc), cond_expr(cond), true_expr(tr_e), false_expr(f_e) {}

BranchNode::ptr_t make_branch_node(
    const AstNode::ptr_t& cond_expr,
    const AstNode::ptr_t& true_expr,
    const AstNode::ptr_t& false_expr,
    const SourceLoc& loc) 
{
    return BranchNode::ptr_t(new BranchNode(cond_expr, true_expr, false_expr, loc));
}


FixNode::FixNode(const AstNode::ptr_t& f, const SourceLoc& loc)
    : AstNode(loc), func(f) {}

FixNode::ptr_t make_fix_node(const AstNode::ptr_t& func, const SourceLoc& loc)
{
    return FixNode::ptr_t(new FixNode(func, loc));
}



AstNode::ptr_t upcast(const ConstNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const VarNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const FuncNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const AppNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const LetNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const BranchNode::ptr_t& node) { return node; }
AstNode::ptr_t upcast(const FixNode::ptr_t& node) { return node; }