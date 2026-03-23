
#ifndef AST_DEF_H
#define AST_DEF_H

#include "source_loc.h"
#include <string>
#include <memory>


struct AstNode {
    using ptr_t = std::shared_ptr<AstNode>;
    
    SourceLoc loc;

    AstNode(const SourceLoc& loc);
    virtual ~AstNode() = default;
};


struct ConstNode : AstNode {
    using ptr_t = std::shared_ptr<ConstNode>;

    std::string value;
    std::string type;

    ConstNode(const std::string& v, const std::string& t, const SourceLoc& loc);
};


struct VarNode : AstNode {
    using ptr_t = std::shared_ptr<VarNode>;

    std::string var;

    VarNode(const std::string& n, const SourceLoc& loc);
};


struct FuncNode : AstNode { //λx.e
    using ptr_t = std::shared_ptr<FuncNode>;

    VarNode::ptr_t param; //x
    AstNode::ptr_t body; //e

    FuncNode(const VarNode::ptr_t& p, const AstNode::ptr_t& b, const SourceLoc& loc);
};


struct AppNode : AstNode { //e1 e2
    using ptr_t = std::shared_ptr<AppNode>;

    AstNode::ptr_t func; //e1
    AstNode::ptr_t arg; //e2

    AppNode(const AstNode::ptr_t& f, const AstNode::ptr_t& a, const SourceLoc& loc);
};


struct LetNode : AstNode { //let x = e1 in e2
    using ptr_t = std::shared_ptr<LetNode>;

    VarNode::ptr_t bind_var; //x
    AstNode::ptr_t bind_value; //e1
    AstNode::ptr_t ret_value; //e2

    LetNode(const VarNode::ptr_t& b_var,
            const AstNode::ptr_t& b_val,
            const AstNode::ptr_t& r_val,
            const SourceLoc& loc);
};


struct BranchNode : AstNode { //if e1 then e2 else e3
    using ptr_t = std::shared_ptr<BranchNode>;

    AstNode::ptr_t cond_expr; //e1
    AstNode::ptr_t true_expr; //e2
    AstNode::ptr_t false_expr; //e3

    BranchNode(const AstNode::ptr_t& cond,
            const AstNode::ptr_t& tr_e,
            const AstNode::ptr_t& f_e,
            const SourceLoc& loc);
};


struct FixNode : AstNode { //fix e
    using ptr_t = std::shared_ptr<FixNode>;

    AstNode::ptr_t func; //e

    FixNode(const AstNode::ptr_t& f, const SourceLoc& loc);
};

#endif