
#include "ast_def.h"
#include "algorithm_state.h"

struct StepVisualizerState {
	float reserved_height = 180.0f;
	const AstNode* hovered_step_node = nullptr;
    AlgorithmState alg_state;
	int steps_ui_generation = 0;

    void reset();
};