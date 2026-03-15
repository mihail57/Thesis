
#ifndef APP_STATE_H
#define APP_STATE_H

#include "inference_manager_state.h"

struct AppState
{
    InferenceManagerState& inf_mgr_state;

    AppState(InferenceManagerState& inf_state) : inf_mgr_state(inf_state) {};
};

#endif