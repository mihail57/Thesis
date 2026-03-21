
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
    std::string value;
    std::string type;

    ConstNode(const std::string& v, const std::string& t, const SourceLoc& loc);
};


struct VarNode : AstNode {
    std::string var;

    VarNode(const std::string& n, const SourceLoc& loc);
};


struct FuncNode : AstNode { //λx.e
    std::shared_ptr<VarNode> param; //x
    std::shared_ptr<AstNode> body; //e

    FuncNode(const std::shared_ptr<VarNode>& p, const std::shared_ptr<AstNode>& b, const SourceLoc& loc);
};


struct AppNode : AstNode { //e1 e2
    std::shared_ptr<AstNode> func; //e1
    std::shared_ptr<AstNode> arg; //e2

    AppNode(const std::shared_ptr<AstNode>& f, const std::shared_ptr<AstNode>& a, const SourceLoc& loc);
};


struct LetNode : AstNode { //let x = e1 in e2
    std::shared_ptr<VarNode> bind_var; //x
    std::shared_ptr<AstNode> bind_value; //e1
    std::shared_ptr<AstNode> ret_value; //e2

    LetNode(const std::shared_ptr<VarNode>& b_var, 
            const std::shared_ptr<AstNode>& b_val, 
            const std::shared_ptr<AstNode>& r_val, 
            const SourceLoc& loc);
};


struct BranchNode : AstNode { //if e1 then e2 else e3
    std::shared_ptr<AstNode> cond_expr; //e1
    std::shared_ptr<AstNode> true_expr; //e2
    std::shared_ptr<AstNode> false_expr; //e3

    BranchNode(const std::shared_ptr<AstNode>& cond, 
            const std::shared_ptr<AstNode>& tr_e, 
            const std::shared_ptr<AstNode>& f_e, 
            const SourceLoc& loc);
};


struct FixNode : AstNode { //fix e
    std::shared_ptr<AstNode> func; //e

    FixNode(const std::shared_ptr<AstNode>& f, const SourceLoc& loc);
};

#endif