
#ifndef INF_MGR_STATE_H
#define INF_MGR_STATE_H

#include "algorithm_state.h"

enum class AlgorithmKind {
    W, M
};

enum class InputType {
    Lambda, Haskell
};

struct InferenceManagerState {
    std::string input;
    InputType input_type;
    AlgorithmKind algorithm;
    AlgorithmState alg_state;
    std::string result;
};

#endif