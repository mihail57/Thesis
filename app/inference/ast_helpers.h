
#ifndef AST_HELPERS_H
#define AST_HELPERS_H

#include "ast_def.h"

std::shared_ptr<ConstNode> make_const_node(const std::string& value, const std::string& type, const SourceLoc& loc);
std::shared_ptr<VarNode> make_var_node(const std::string& name, const SourceLoc& loc);
std::shared_ptr<FuncNode> make_func_node(const std::shared_ptr<VarNode>& param, const std::shared_ptr<AstNode>& body, const SourceLoc& loc);
std::shared_ptr<AppNode> make_app_node(const std::shared_ptr<AstNode>& func, const std::shared_ptr<AstNode>& arg, const SourceLoc& loc);
std::shared_ptr<LetNode> make_let_node(
    const std::shared_ptr<VarNode>& bind_var, 
    const std::shared_ptr<AstNode>& bind_value,
    const std::shared_ptr<AstNode>& ret_value, 
    const SourceLoc& loc);
std::shared_ptr<BranchNode> make_branch_node(
    const std::shared_ptr<AstNode>& cond_expr, 
    const std::shared_ptr<AstNode>& true_expr,
    const std::shared_ptr<AstNode>& false_expr, 
    const SourceLoc& loc);
std::shared_ptr<FixNode> make_fix_node(const std::shared_ptr<AstNode>& func, const SourceLoc& loc);

std::shared_ptr<AstNode> upcast(const std::shared_ptr<ConstNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<VarNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<FuncNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<AppNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<LetNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<BranchNode>& node);
std::shared_ptr<AstNode> upcast(const std::shared_ptr<FixNode>& node);

#endif