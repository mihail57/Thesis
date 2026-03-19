
#ifndef INFERENCE_MANAGER_H
#define INFERENCE_MANAGER_H

#include <tuple>
#include <vector>
#include <exception>

#include "value_or_error.hpp"
#include "temporary/lambda_parser.h"
#include "hm_def/type.h"
#include "hm_def/substitution.h"
#include "hm_def/type_scheme.h"
#include "hm_def/inference_context.h"

#include "../include/inference_manager_state.h"


using Result = std::tuple<Type::base_ptr, Substitution>;

Type::base_ptr get_type(const Result& v);

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

    void add_algorithm_step(const std::string& step_text, const std::shared_ptr<AstNode>& at);

    SubstitutionOrError MGU(const std::shared_ptr<Type>& first, const std::shared_ptr<Type>& second);

    ResultOrError W(TypingContext& gamma, std::shared_ptr<ConstNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<VarNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<FuncNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<AppNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<LetNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<BranchNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<FixNode> node);
    ResultOrError W(TypingContext& gamma, std::shared_ptr<AstNode> node);

    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<ConstNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<VarNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<FuncNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<AppNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<LetNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<BranchNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<FixNode> node, Type::base_ptr expected);
    SubstitutionOrError M(TypingContext& gamma, std::shared_ptr<AstNode> node, Type::base_ptr expected);

    std::string highlight_loc(const std::string& source, const SourceLoc& loc);
    TypingContext make_basic_context();
    std::string print_error(const Error& error, const std::string& source);
};

#endif