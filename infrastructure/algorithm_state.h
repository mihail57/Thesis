
#ifndef ALGORITHM_STATE_H
#define ALGORITHM_STATE_H

#include <vector>
#include <string>
#include "ast_def.h"

struct AlgorithmStep {
    std::string data;
    AstNode::ptr_t at;
};

struct AlgorithmState {
    std::vector<AlgorithmStep> steps;
};

#endif