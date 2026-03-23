
#ifndef APP_STATE_H
#define APP_STATE_H

#include "inference_manager_state.h"
#include <string>

struct UiInitStruct
{
    InferenceManagerState& inf_mgr_state;
    std::string monospace_font_path;
    float font_size = 16.0f;

    UiInitStruct(InferenceManagerState& inf_state) : inf_mgr_state(inf_state) {};
};

#endif