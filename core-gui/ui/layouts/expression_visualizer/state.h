
#include "ast_def.h"

#include <string>
#include <array>

struct ExpressionVisualizerState {
	std::array<char, 2048> ast_source_copy{};

    AstNode::ptr_t root;
    const AstNode* step_hovered;

	float ast_zoom = 1.0f;
	int selected_ast_node = -1;
	bool pending_source_highlight = false;
	bool clear_source_selection_only = false;
	int highlight_start = 0;
	int highlight_end = 0;

	bool right_panel_open = true;
	float right_panel_width = 320.0f;
	float panel_height = 320.0f;

    void reset();
    void set_expression(const std::string& expr);
    void request_source_highlight(const SourceLoc& loc);
    void clear_source_highlight();
};