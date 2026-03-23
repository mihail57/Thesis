
#ifndef AST_HELPERS_H
#define AST_HELPERS_H

#include "ast_def.h"

ConstNode::ptr_t make_const_node(const std::string& value, const std::string& type, const SourceLoc& loc);
VarNode::ptr_t make_var_node(const std::string& name, const SourceLoc& loc);
FuncNode::ptr_t make_func_node(const VarNode::ptr_t& param, const AstNode::ptr_t& body, const SourceLoc& loc);
AppNode::ptr_t make_app_node(const AstNode::ptr_t& func, const AstNode::ptr_t& arg, const SourceLoc& loc);
LetNode::ptr_t make_let_node(
    const VarNode::ptr_t& bind_var,
    const AstNode::ptr_t& bind_value,
    const AstNode::ptr_t& ret_value,
    const SourceLoc& loc);
BranchNode::ptr_t make_branch_node(
    const AstNode::ptr_t& cond_expr,
    const AstNode::ptr_t& true_expr,
    const AstNode::ptr_t& false_expr,
    const SourceLoc& loc);
FixNode::ptr_t make_fix_node(const AstNode::ptr_t& func, const SourceLoc& loc);

AstNode::ptr_t upcast(const ConstNode::ptr_t& node);
AstNode::ptr_t upcast(const VarNode::ptr_t& node);
AstNode::ptr_t upcast(const FuncNode::ptr_t& node);
AstNode::ptr_t upcast(const AppNode::ptr_t& node);
AstNode::ptr_t upcast(const LetNode::ptr_t& node);
AstNode::ptr_t upcast(const BranchNode::ptr_t& node);
AstNode::ptr_t upcast(const FixNode::ptr_t& node);

#endif