
#include "ast_def.h"

#include <optional>
#include <string_view>

struct ExpressionVisualizerState {
	std::optional<std::string_view> expression = std::nullopt;

	AstNode::ptr_t ast_root;
	AstNode::ptr_t hovered_node;

	float zoom_factor = 1.0f;
	bool panning_active = false;
	int selected_ast_node = -1;
	int highlight_start = 0;
	int highlight_end = 0;

	float width = 320.0f;
	float height = 320.0f;

	void reset();
	void set_expression(std::string_view expr);
	void request_source_highlight(const SourceLoc& loc);
};