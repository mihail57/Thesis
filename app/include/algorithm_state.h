
#ifndef ALGORITHM_STATE_H
#define ALGORITHM_STATE_H

#include <vector>
#include <string>

struct InferenceContextData {
    // Пары переменная - значение
    std::vector<std::pair<std::string, std::string>> data;
};

struct AlgorithmStep {
    std::string data;
    InferenceContextData inf_ctx;
};

struct AlgorithmState {
    std::vector<AlgorithmStep> steps;
};

#endif