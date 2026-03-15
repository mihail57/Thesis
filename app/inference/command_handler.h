
#include "../command_handler_defs.h"
#include "inference_manager.h"

struct CommandHandler {
    InferenceManager& mgr;

    CommandHandler(InferenceManager& mgr);

    COMMAND_HANDLER(RunAlgorithmCommand);
    COMMAND_HANDLER(SetAlgorithmCommand);
    COMMAND_HANDLER(SetInputCommand);
    COMMAND_HANDLER(SetInputTypeCommand);
    COMMAND_HANDLER_IGNORE_OTHERS;
};