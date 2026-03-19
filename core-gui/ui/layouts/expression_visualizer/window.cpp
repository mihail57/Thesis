
#include "state.h"
#include "window.h"

#include "ast_def.h"

#include "string_to_buffer.hpp"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>


constexpr float k_ast_horizontal_spacing = 28.0f;
constexpr float k_ast_vertical_spacing = 44.0f;
constexpr float k_ast_canvas_padding = 16.0f;

void ExpressionVisualizerState::reset() {
	root.reset();
	selected_ast_node = -1;
	pending_source_highlight = false;
	clear_source_selection_only = false;
	highlight_start = 0;
	highlight_end = 0;
	CopyStringToBuffer(ast_source_copy, "");
}

void ExpressionVisualizerState::set_expression(const std::string& expr) {
	CopyStringToBuffer(ast_source_copy, expr);
}


struct AstVisualNode {
	std::shared_ptr<AstNode> source_node;
	std::string title;
	std::vector<std::string> details;
	std::vector<int> children;
	std::vector<std::string> edge_labels;
	ImVec2 size = ImVec2(140.0f, 56.0f);
	float subtree_width = 0.0f;
	ImU32 fill_color = IM_COL32(52, 56, 68, 240);
	ImU32 border_color = IM_COL32(120, 130, 170, 255);
	ImU32 title_color = IM_COL32(220, 225, 245, 255);
};

int BuildAstVisualTree(const std::shared_ptr<AstNode>& node, std::vector<AstVisualNode>& out_nodes) {
	AstVisualNode visual_node;
	visual_node.source_node = node;
	std::vector<std::shared_ptr<AstNode>> child_nodes;

	if (!node) {
		visual_node.title = "Null";
		visual_node.fill_color = IM_COL32(70, 46, 46, 240);
		visual_node.border_color = IM_COL32(180, 90, 90, 255);
	} else if (auto const_node = std::dynamic_pointer_cast<ConstNode>(node)) {
		visual_node.title = "Con";
		visual_node.details.push_back(const_node->value + ": " + const_node->type);
		visual_node.fill_color = IM_COL32(45, 63, 56, 240);
		visual_node.border_color = IM_COL32(90, 190, 150, 255);
	} else if (auto var_node = std::dynamic_pointer_cast<VarNode>(node)) {
		visual_node.title = "Var";
		visual_node.details.push_back(var_node->var);
		visual_node.fill_color = IM_COL32(49, 57, 71, 240);
		visual_node.border_color = IM_COL32(113, 151, 214, 255);
	} else if (auto func_node = std::dynamic_pointer_cast<FuncNode>(node)) {
		visual_node.title = "Abs";
		visual_node.edge_labels = {"Параметр", "Тело"};
		visual_node.fill_color = IM_COL32(63, 53, 79, 240);
		visual_node.border_color = IM_COL32(171, 136, 232, 255);
		child_nodes.push_back(func_node->param);
		child_nodes.push_back(func_node->body);
	} else if (auto app_node = std::dynamic_pointer_cast<AppNode>(node)) {
		visual_node.title = "App";
		visual_node.edge_labels = {"Лямбда", "Аргумент"};
		visual_node.fill_color = IM_COL32(66, 58, 44, 240);
		visual_node.border_color = IM_COL32(210, 168, 90, 255);
		child_nodes.push_back(app_node->func);
		child_nodes.push_back(app_node->arg);
	} else if (auto let_node = std::dynamic_pointer_cast<LetNode>(node)) {
		visual_node.title = "Let";
		visual_node.edge_labels = {"Переменная", "Значение", "В выражении"};
		visual_node.fill_color = IM_COL32(52, 63, 45, 240);
		visual_node.border_color = IM_COL32(134, 201, 110, 255);
		child_nodes.push_back(let_node->bind_var);
		child_nodes.push_back(let_node->bind_value);
		child_nodes.push_back(let_node->ret_value);
	} else if (auto branch_node = std::dynamic_pointer_cast<BranchNode>(node)) {
		visual_node.title = "Branch";
		visual_node.edge_labels = {"Условие", "Если истинно", "Если ложно"};
		visual_node.fill_color = IM_COL32(60, 49, 41, 240);
		visual_node.border_color = IM_COL32(220, 135, 95, 255);
		child_nodes.push_back(branch_node->cond_expr);
		child_nodes.push_back(branch_node->true_expr);
		child_nodes.push_back(branch_node->false_expr);
	} else if (auto fix_node = std::dynamic_pointer_cast<FixNode>(node)) {
		visual_node.title = "Fix";
		visual_node.edge_labels = {"Лямбда"};
		visual_node.fill_color = IM_COL32(56, 48, 66, 240);
		visual_node.border_color = IM_COL32(198, 133, 233, 255);
		child_nodes.push_back(fix_node->func);
	} else {
		visual_node.title = "Неизвестный узел";
		visual_node.fill_color = IM_COL32(67, 56, 56, 240);
		visual_node.border_color = IM_COL32(210, 120, 120, 255);
	}

	const int node_index = static_cast<int>(out_nodes.size());
	out_nodes.push_back(std::move(visual_node));

	for (const std::shared_ptr<AstNode>& child : child_nodes) {
		const int child_index = BuildAstVisualTree(child, out_nodes);
		out_nodes[node_index].children.push_back(child_index);
	}

	if (out_nodes[node_index].edge_labels.size() < out_nodes[node_index].children.size()) {
		out_nodes[node_index].edge_labels.resize(out_nodes[node_index].children.size());
	}

	return node_index;
}

void ExpressionVisualizerState::request_source_highlight(const SourceLoc& loc) {
	const int text_len = static_cast<int>(std::strlen(ast_source_copy.data()));
	int start_idx = std::clamp(static_cast<int>(loc.start), 0, text_len);
	int end_idx = std::clamp(static_cast<int>(loc.end), 0, text_len);
	if (start_idx > end_idx) {
		std::swap(start_idx, end_idx);
	}
	if (start_idx == end_idx && end_idx < text_len) {
		++end_idx;
	}

	highlight_start = start_idx;
	highlight_end = end_idx;
	pending_source_highlight = true;
	clear_source_selection_only = false;
}

void ExpressionVisualizerState::clear_source_highlight() {
	pending_source_highlight = true;
	clear_source_selection_only = true;
}


void CalculateAstNodeSize(AstVisualNode& node) {
	const float line_height = ImGui::GetTextLineHeight();
	float max_text_width = ImGui::CalcTextSize(node.title.c_str()).x;
	for (const std::string& detail : node.details) {
		max_text_width = std::max(max_text_width, ImGui::CalcTextSize(detail.c_str()).x);
	}

	const float horizontal_padding = 20.0f;
	const float vertical_padding = 16.0f;
	const float min_width = 120.0f;
	const float min_height = 46.0f;
	const float lines_count = static_cast<float>(1 + node.details.size());

	node.size.x = std::max(min_width, max_text_width + horizontal_padding);
	node.size.y = std::max(min_height, lines_count * line_height + vertical_padding);
}

float CalculateSubtreeWidth(int node_index, std::vector<AstVisualNode>& nodes) {
	AstVisualNode& node = nodes[node_index];
	if (node.children.empty()) {
		node.subtree_width = node.size.x;
		return node.subtree_width;
	}

	float children_width = 0.0f;
	for (size_t i = 0; i < node.children.size(); ++i) {
		children_width += CalculateSubtreeWidth(node.children[i], nodes);
		if (i + 1 < node.children.size()) {
			children_width += k_ast_horizontal_spacing;
		}
	}

	node.subtree_width = std::max(node.size.x, children_width);
	return node.subtree_width;
}

void PlaceAstNodes(
	int node_index,
	std::vector<AstVisualNode>& nodes,
	std::vector<ImVec2>& positions,
	float left,
	float top
) {
	AstVisualNode& node = nodes[node_index];
	positions[node_index] = ImVec2(left + (node.subtree_width - node.size.x) * 0.5f, top);

	if (node.children.empty()) {
		return;
	}

	float total_children_width = 0.0f;
	for (size_t i = 0; i < node.children.size(); ++i) {
		total_children_width += nodes[node.children[i]].subtree_width;
		if (i + 1 < node.children.size()) {
			total_children_width += k_ast_horizontal_spacing;
		}
	}

	float child_left = left + (node.subtree_width - total_children_width) * 0.5f;
	for (const int child_index : node.children) {
		PlaceAstNodes(child_index, nodes, positions, child_left, top + node.size.y + k_ast_vertical_spacing);
		child_left += nodes[child_index].subtree_width + k_ast_horizontal_spacing;
	}
}

void DrawAstGraph(ExpressionVisualizerState& main_window, const std::shared_ptr<AstNode>& root_node) {
	if (!root_node) {
		ImGui::TextDisabled("AST пока не построено");
		return;
	}

	float zoom = main_window.ast_zoom;
	zoom = std::clamp(zoom, 0.4f, 1.0f);

	ImGui::TextDisabled("Ctrl + колесо: масштаб");
	ImGui::SameLine();
	ImGui::Text("x%.2f", zoom);
	ImGui::SameLine();
	ImGui::TextDisabled("ЛКМ + перетаскивание: панорама");

	std::vector<AstVisualNode> nodes;
	const int root_index = BuildAstVisualTree(root_node, nodes);
	for (AstVisualNode& node : nodes) {
		CalculateAstNodeSize(node);
		node.size.x *= zoom;
		node.size.y *= zoom;
	}

	CalculateSubtreeWidth(root_index, nodes);

	std::vector<ImVec2> positions(nodes.size());
	PlaceAstNodes(root_index, nodes, positions, k_ast_canvas_padding, k_ast_canvas_padding);

	float canvas_width = 0.0f;
	float canvas_height = 0.0f;
	for (size_t i = 0; i < nodes.size(); ++i) {
		canvas_width = std::max(canvas_width, positions[i].x + nodes[i].size.x + k_ast_canvas_padding);
		canvas_height = std::max(canvas_height, positions[i].y + nodes[i].size.y + k_ast_canvas_padding);
	}

	ImGui::BeginChild("AstCanvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::GetIO().KeyCtrl) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			zoom = std::clamp(zoom + wheel * 0.08f, 0.4f, 1.0f);
			main_window.ast_zoom = zoom;
		}
	}

	const int selected_index = main_window.selected_ast_node;

	int step_hovered_index = -1;
	if (main_window.step_hovered != nullptr) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			if (nodes[i].source_node.get() == main_window.step_hovered) {
				step_hovered_index = static_cast<int>(i);
				break;
			}
		}
	}
	const ImVec2 start_pos = ImGui::GetCursorScreenPos();
	const ImVec2 view_size = ImGui::GetContentRegionAvail();
	const float graph_width = std::max(1.0f, canvas_width - 2.0f * k_ast_canvas_padding);
	const float graph_height = std::max(1.0f, canvas_height - 2.0f * k_ast_canvas_padding);
	const float pan_margin = 120.0f;
	const float content_width = std::max(view_size.x, graph_width + pan_margin * 2.0f);
	const float content_height = std::max(view_size.y, graph_height + pan_margin * 2.0f);
	const float graph_origin_x = pan_margin;
	const float graph_origin_y = pan_margin;

	const ImVec2 pan(graph_origin_x, graph_origin_y);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	int hovered_node_index = -1;

	for (size_t parent_index = 0; parent_index < nodes.size(); ++parent_index) {
		const AstVisualNode& parent = nodes[parent_index];
		const ImVec2 parent_bottom = ImVec2(
			start_pos.x + pan.x + positions[parent_index].x + parent.size.x * 0.5f,
			start_pos.y + pan.y + positions[parent_index].y + parent.size.y
		);

		for (size_t edge_i = 0; edge_i < parent.children.size(); ++edge_i) {
			const int child_index = parent.children[edge_i];
			const AstVisualNode& child = nodes[child_index];
			const ImVec2 child_top = ImVec2(
				start_pos.x + pan.x + positions[child_index].x + child.size.x * 0.5f,
				start_pos.y + pan.y + positions[child_index].y
			);

			const float mid_y = (parent_bottom.y + child_top.y) * 0.5f;
			draw_list->AddLine(parent_bottom, ImVec2(parent_bottom.x, mid_y), IM_COL32(145, 155, 180, 255), 1.4f);
			draw_list->AddLine(ImVec2(parent_bottom.x, mid_y), ImVec2(child_top.x, mid_y), IM_COL32(145, 155, 180, 255), 1.4f);
			draw_list->AddLine(ImVec2(child_top.x, mid_y), child_top, IM_COL32(145, 155, 180, 255), 1.4f);

			if (edge_i < parent.edge_labels.size() && !parent.edge_labels[edge_i].empty()) {
				const std::string& edge_label = parent.edge_labels[edge_i];
				const ImVec2 label_size = ImGui::CalcTextSize(edge_label.c_str());
				const ImVec2 label_pos = ImVec2(
					child_top.x - label_size.x * 0.5f,
					mid_y - label_size.y - 3.0f
				);
				draw_list->AddText(label_pos, IM_COL32(155, 165, 185, 255), edge_label.c_str());
			}
		}
	}

	const float line_height = ImGui::GetTextLineHeight() * zoom;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const AstVisualNode& node = nodes[i];
		const ImVec2 rect_min = ImVec2(start_pos.x + pan.x + positions[i].x, start_pos.y + pan.y + positions[i].y);
		const ImVec2 rect_max = ImVec2(rect_min.x + node.size.x, rect_min.y + node.size.y);
		const bool is_selected = static_cast<int>(i) == selected_index || static_cast<int>(i) == step_hovered_index;
		const ImU32 border_color = is_selected ? IM_COL32(250, 215, 90, 255) : node.border_color;
		const float border_thickness = is_selected ? 2.8f : 1.5f;

		draw_list->AddRectFilled(rect_min, rect_max, node.fill_color, 8.0f);
		draw_list->AddRect(rect_min, rect_max, border_color, 8.0f, 0, border_thickness);

		ImVec2 text_pos = ImVec2(rect_min.x + 10.0f * zoom, rect_min.y + 7.0f * zoom);
		draw_list->AddText(text_pos, node.title_color, node.title.c_str());
		for (const std::string& detail : node.details) {
			text_pos.y += line_height;
			draw_list->AddText(text_pos, IM_COL32(200, 205, 220, 255), detail.c_str());
		}

		ImGui::SetCursorScreenPos(rect_min);
		ImGui::PushID(static_cast<int>(i));
		const bool clicked = ImGui::InvisibleButton("ast_node_select", node.size);
		const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		if (clicked || hovered) {
			main_window.selected_ast_node = static_cast<int>(i);
			hovered_node_index = static_cast<int>(i);
			const std::shared_ptr<AstNode>& selected_ast_node = nodes[i].source_node;
			if (selected_ast_node) {
				main_window.request_source_highlight(selected_ast_node->loc);
			}
		}
		ImGui::PopID();
	}

	const bool canvas_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	const bool need_clear_hover_selection = hovered_node_index < 0 && (!canvas_hovered || !ImGui::IsAnyItemHovered());
	if (need_clear_hover_selection && main_window.selected_ast_node >= 0 && main_window.step_hovered == nullptr) {
		main_window.selected_ast_node = -1;
		main_window.clear_source_highlight();
	}

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AnyWindow) && !ImGui::GetIO().KeyCtrl && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		const float min_visible = 24.0f;
		const float min_scroll_x = std::max(0.0f, graph_origin_x - view_size.x + min_visible);
		const float max_scroll_x = std::min(ImGui::GetScrollMaxX(), graph_origin_x + graph_width - min_visible);
		const float min_scroll_y = std::max(0.0f, graph_origin_y - view_size.y + min_visible);
		const float max_scroll_y = std::min(ImGui::GetScrollMaxY(), graph_origin_y + graph_height - min_visible);

		const float next_scroll_x = std::clamp(ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x, min_scroll_x, max_scroll_x);
		const float next_scroll_y = std::clamp(ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y, min_scroll_y, max_scroll_y);
		ImGui::SetScrollX(next_scroll_x);
		ImGui::SetScrollY(next_scroll_y);
	}

	const ImVec2 visible_region = ImGui::GetContentRegionAvail();
	ImGui::Dummy(ImVec2(std::max(content_width, visible_region.x), std::max(content_height, visible_region.y)));
	ImGui::EndChild();
}

void DrawSourceHoverPreview(const ExpressionVisualizerState& state, bool highlight_active) {
	const std::string text(state.ast_source_copy.data());
	ImGui::BeginChild("SourceHoverPreview", ImVec2(0.0f, 78.0f), true);

	if (text.empty()) {
		ImGui::TextDisabled("Исходный текст пуст");
		ImGui::EndChild();
		return;
	}

	int start = std::clamp(state.highlight_start, 0, static_cast<int>(text.size()));
	int end = std::clamp(state.highlight_end, 0, static_cast<int>(text.size()));
	if (start > end) {
		std::swap(start, end);
	}

	if (start >= end) {
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		return;
	}

	const int snippet_radius = 56;
	const int snippet_start = std::max(0, start - snippet_radius);
	const int snippet_end = std::min(static_cast<int>(text.size()), end + snippet_radius);

	const std::string prefix = text.substr(snippet_start, static_cast<size_t>(start - snippet_start));
	const std::string marked = text.substr(start, static_cast<size_t>(end - start));
	const std::string suffix = text.substr(end, static_cast<size_t>(snippet_end - end));

	ImGui::PushTextWrapPos();
	if (snippet_start > 0) {
		ImGui::TextDisabled("...");
		ImGui::SameLine(0.0f, 0.0f);
	}
	ImGui::TextUnformatted(prefix.c_str());
	ImGui::SameLine(0.0f, 0.0f);
	if (highlight_active) {
		ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.35f, 1.0f), "%s", marked.c_str());
	} else {
		ImGui::TextUnformatted(marked.c_str());
	}
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextUnformatted(suffix.c_str());
	if (snippet_end < static_cast<int>(text.size())) {
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextDisabled("...");
	}
	ImGui::PopTextWrapPos();
	ImGui::EndChild();
}

void DrawTopRightPanel(ExpressionVisualizerState& state) {
	const bool has_active_source_hover = state.selected_ast_node >= 0 || state.step_hovered != nullptr;

	ImGui::Dummy(ImVec2(0.0f, 1.0f));
	ImGui::SameLine(0.0f, 0.0f);
	const float close_button_width = ImGui::GetFrameHeight();
	const float close_button_x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - close_button_width;
	ImGui::SetCursorPosX(std::max(0.0f, close_button_x));
	if (ImGui::Button("x")) {
		state.right_panel_open = false;
	}
	ImGui::Separator();
	DrawSourceHoverPreview(state, has_active_source_hover);
	ImGui::Spacing();
	DrawAstGraph(state, state.root);
}

void DrawExpressionVisualizer(ExpressionVisualizerState& expr_vis_state, CommandBuffer& cmd_buf) {    
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginChild("TopRightPanel", ImVec2(0.0f, expr_vis_state.panel_height), true);
	DrawTopRightPanel(expr_vis_state);
	ImGui::EndChild();
}