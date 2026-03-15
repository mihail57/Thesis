
#ifndef ALGORITHM_STATE_H
#define ALGORITHM_STATE_H

#include <vector>
#include <string>
#include "ast_def.h"

struct InferenceContextData {
    // Пары переменная - значение
    std::vector<std::pair<std::string, std::string>> data;
};

struct AlgorithmStep {
    std::string data;
    std::shared_ptr<AstNode> at;
};

struct AlgorithmState {
    std::vector<AlgorithmStep> steps;
    InferenceContextData ctx;
};

#endif