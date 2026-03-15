
#ifndef EVENT_H
#define EVENT_H

#include "inference_manager_state.h"
#include <variant>
#include <string>

struct AlgorithmFinishedEvent {
    bool ok;
    const InferenceManagerState& new_inf_mgr_state;
};

struct InputChangeErrorEvent {
    std::string msg;
    const InferenceManagerState& new_inf_mgr_state;
};

using Event = std::variant<AlgorithmFinishedEvent, InputChangeErrorEvent>;

#endif