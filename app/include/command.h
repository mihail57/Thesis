
#ifndef COMMAND_H
#define COMMAND_H

#include "inference_manager_state.h"

#include <variant>
#include <string>

struct RunAlgorithmCommand {
    bool detailed;
};

struct SetAlgorithmCommand {
    AlgorithmKind new_kind;
};

struct SetInputCommand {
    std::string new_input;
};

struct SetInputTypeCommand {
    InputType new_input_type;
};

using Command = std::variant<RunAlgorithmCommand, SetAlgorithmCommand, SetInputCommand, SetInputTypeCommand>;

#endif