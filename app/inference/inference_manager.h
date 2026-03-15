
#ifndef INFERENCE_MANAGER_H
#define INFERENCE_MANAGER_H

#include <tuple>
#include <vector>
#include <exception>
#include <variant>

#include "substitution_or_error.h"
#include "type_or_error.h"
#include "temporary/lambda_parser.h"

#include "../include/inference_manager_state.h"


using Result = std::tuple<Type::base_ptr, Substitution>;

Type::base_ptr get_type(const Result& v);

using ResultOrError = std::variant<Result, TypingError>;

bool is_error(const ResultOrError& v);
TypingError get_error(const ResultOrError& v);
Result unwrap(const ResultOrError& v);


struct InferenceManager {
    InferenceManagerState state;

    bool run_algorithm(bool detailed);
    void change_input(const std::string& new_input);
    void change_input_type(InputType new_input_type);
    void change_algorithm(AlgorithmKind new_algorithm);

private:
    bool detailed = false;

    void reset();

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
    std::string print_error(const TypingError& error, const std::string& source);
};

#endif