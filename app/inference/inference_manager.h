
#ifndef INFERENCE_MANAGER_H
#define INFERENCE_MANAGER_H

#include <tuple>
#include <vector>
#include <exception>

#include "value_or_error.hpp"
#include "lambda_parser/lambda_parser.h"
#include "hm_def/type.h"
#include "hm_def/substitution.h"
#include "hm_def/type_scheme.h"
#include "hm_def/inference_context.h"

#include "inference_manager_state.h"


using Result = std::tuple<Type::base_ptr_t, Substitution>;

Type::base_ptr_t get_type(const Result& v);

using ResultOrError = ValueOrError<Result>;

using SubstitutionOrError = ValueOrError<Substitution>;


struct InferenceManager {
    InferenceManagerState state;

    bool run_algorithm(bool detailed);
    void change_input(const std::string& new_input);
    void change_input_type(InputType new_input_type);
    void change_algorithm(AlgorithmKind new_algorithm);

private:
    bool detailed = false;

    void reset();

    void add_algorithm_step(const std::string& step_text, const AstNode::ptr_t& at);

    SubstitutionOrError MGU(const Type::base_ptr_t& first, const Type::base_ptr_t& second);

    ResultOrError W(InferenceContext& gamma, ConstNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, VarNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, FuncNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, AppNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, LetNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, BranchNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, FixNode::ptr_t node);
    ResultOrError W(InferenceContext& gamma, AstNode::ptr_t node);

    SubstitutionOrError M(InferenceContext& gamma, ConstNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, VarNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, FuncNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, AppNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, LetNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, BranchNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, FixNode::ptr_t node, Type::base_ptr_t expected);
    SubstitutionOrError M(InferenceContext& gamma, AstNode::ptr_t node, Type::base_ptr_t expected);

    std::string highlight_loc(const std::string& source, const SourceLoc& loc);
    InferenceContext make_basic_context();
    std::string print_error(const Error& error, const std::string& source);
};

#endif