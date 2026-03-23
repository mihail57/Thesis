
#include "ast_def.h"
#include "algorithm_state.h"

struct StepVisualizerState {
	float reserved_height = 180.0f;
	AstNode::ptr_t hovered_step_node;
	AlgorithmState alg_state;
	int steps_ui_generation = 0;

	void reset();
};