
#include "inference_manager_state.h"

struct AppState
{
    InferenceManagerState& inf_mgr_state;

    AppState(InferenceManagerState& inf_state) : inf_mgr_state(inf_state) {};
};
