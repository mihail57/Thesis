
#include "state.h"
#include "event_handler.h"
#include "window.h"

#include "imgui.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr float k_ast_horizontal_spacing = 28.0f;
constexpr float k_ast_vertical_spacing = 44.0f;
constexpr float k_ast_canvas_padding = 16.0f;

template <size_t N>
void CopyStringToBuffer(std::array<char, N>& buffer, const std::string& text) {
	const size_t copy_len = std::min(text.size(), N > 0 ? N - 1 : 0);
	if (copy_len > 0) {
		std::memcpy(buffer.data(), text.data(), copy_len);
	}
	if (N > 0) {
		buffer[copy_len] = '\0';
	}
}

void SyncAstSourceCopyFromInferenceState(MainWindowState& main_window) {
	CopyStringToBuffer(main_window.top_panel.ast_source_copy, main_window.inf_mgr_state.input);
}

int InputTypeToComboIndex(InputType input_type) {
	return input_type == InputType::Lambda ? 0 : 1;
}

InputType ComboIndexToInputType(int idx) {
	return idx == 0 ? InputType::Lambda : InputType::Haskell;
}

int AlgorithmKindToComboIndex(AlgorithmKind algorithm) {
	return algorithm == AlgorithmKind::W ? 0 : 1;
}

AlgorithmKind ComboIndexToAlgorithmKind(int idx) {
	return idx == 0 ? AlgorithmKind::W : AlgorithmKind::M;
}

void ResetMainWindowUiAfterInferenceReset(MainWindowState& main_window) {
	main_window.inf_mgr_state.alg_state.steps.clear();
	main_window.inf_mgr_state.result.clear();
	main_window.inf_mgr_state.input_parsed.reset();

	main_window.top_panel.selected_ast_node = -1;
	main_window.top_panel.pending_source_highlight = false;
	main_window.top_panel.clear_source_selection_only = false;
	main_window.top_panel.highlight_start = 0;
	main_window.top_panel.highlight_end = 0;
	CopyStringToBuffer(main_window.top_panel.ast_source_copy, "");

	main_window.bottom_panel.hovered_step_node = nullptr;
	++main_window.bottom_panel.steps_ui_generation;
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
		visual_node.details.push_back("empty");
		visual_node.fill_color = IM_COL32(70, 46, 46, 240);
		visual_node.border_color = IM_COL32(180, 90, 90, 255);
	} else if (auto const_node = std::dynamic_pointer_cast<ConstNode>(node)) {
		visual_node.title = "Const";
		visual_node.details.push_back(const_node->value + ": " + const_node->type);
		visual_node.fill_color = IM_COL32(45, 63, 56, 240);
		visual_node.border_color = IM_COL32(90, 190, 150, 255);
	} else if (auto var_node = std::dynamic_pointer_cast<VarNode>(node)) {
		visual_node.title = "Var";
		visual_node.details.push_back(var_node->var);
		visual_node.fill_color = IM_COL32(49, 57, 71, 240);
		visual_node.border_color = IM_COL32(113, 151, 214, 255);
	} else if (auto func_node = std::dynamic_pointer_cast<FuncNode>(node)) {
		visual_node.title = "Lambda";
		visual_node.details.push_back("param");
		visual_node.details.push_back("body");
		visual_node.edge_labels = {"param", "body"};
		visual_node.fill_color = IM_COL32(63, 53, 79, 240);
		visual_node.border_color = IM_COL32(171, 136, 232, 255);
		child_nodes.push_back(func_node->param);
		child_nodes.push_back(func_node->body);
	} else if (auto app_node = std::dynamic_pointer_cast<AppNode>(node)) {
		visual_node.title = "Apply";
		visual_node.details.push_back("func");
		visual_node.details.push_back("arg");
		visual_node.edge_labels = {"func", "arg"};
		visual_node.fill_color = IM_COL32(66, 58, 44, 240);
		visual_node.border_color = IM_COL32(210, 168, 90, 255);
		child_nodes.push_back(app_node->func);
		child_nodes.push_back(app_node->arg);
	} else if (auto let_node = std::dynamic_pointer_cast<LetNode>(node)) {
		visual_node.title = "Let";
		visual_node.details.push_back("bind");
		visual_node.details.push_back("in");
		visual_node.edge_labels = {"bind_var", "bind_value", "ret_value"};
		visual_node.fill_color = IM_COL32(52, 63, 45, 240);
		visual_node.border_color = IM_COL32(134, 201, 110, 255);
		child_nodes.push_back(let_node->bind_var);
		child_nodes.push_back(let_node->bind_value);
		child_nodes.push_back(let_node->ret_value);
	} else if (auto branch_node = std::dynamic_pointer_cast<BranchNode>(node)) {
		visual_node.title = "If";
		visual_node.details.push_back("cond");
		visual_node.details.push_back("then/else");
		visual_node.edge_labels = {"cond", "then", "else"};
		visual_node.fill_color = IM_COL32(60, 49, 41, 240);
		visual_node.border_color = IM_COL32(220, 135, 95, 255);
		child_nodes.push_back(branch_node->cond_expr);
		child_nodes.push_back(branch_node->true_expr);
		child_nodes.push_back(branch_node->false_expr);
	} else if (auto fix_node = std::dynamic_pointer_cast<FixNode>(node)) {
		visual_node.title = "Fix";
		visual_node.details.push_back("func");
		visual_node.edge_labels = {"func"};
		visual_node.fill_color = IM_COL32(56, 48, 66, 240);
		visual_node.border_color = IM_COL32(198, 133, 233, 255);
		child_nodes.push_back(fix_node->func);
	} else {
		visual_node.title = "Unknown";
		visual_node.details.push_back("unsupported node type");
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

void RequestSourceHighlight(MainWindowState& main_window, const SourceLoc& loc) {
	const int text_len = static_cast<int>(std::strlen(main_window.top_panel.ast_source_copy.data()));
	int start_idx = std::clamp(static_cast<int>(loc.start), 0, text_len);
	int end_idx = std::clamp(static_cast<int>(loc.end), 0, text_len);
	if (start_idx > end_idx) {
		std::swap(start_idx, end_idx);
	}
	if (start_idx == end_idx && end_idx < text_len) {
		++end_idx;
	}

	main_window.top_panel.highlight_start = start_idx;
	main_window.top_panel.highlight_end = end_idx;
	main_window.top_panel.pending_source_highlight = true;
	main_window.top_panel.clear_source_selection_only = false;
}

void ClearSourceHighlight(MainWindowState& main_window) {
	main_window.top_panel.pending_source_highlight = true;
	main_window.top_panel.clear_source_selection_only = true;
}

void DrawSourceHoverPreview(const MainTopPanelState& state, bool highlight_active) {
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

int ApplyInputHighlightCallback(ImGuiInputTextCallbackData* data) {
	auto* top_panel = static_cast<MainTopPanelState*>(data->UserData);
	if (top_panel == nullptr || !top_panel->pending_source_highlight) {
		return 0;
	}

	if (top_panel->clear_source_selection_only) {
		data->SelectionStart = data->CursorPos;
		data->SelectionEnd = data->CursorPos;
		top_panel->pending_source_highlight = false;
		top_panel->clear_source_selection_only = false;
		return 0;
	}

	int start = std::clamp(top_panel->highlight_start, 0, data->BufTextLen);
	int end = std::clamp(top_panel->highlight_end, 0, data->BufTextLen);
	if (start > end) {
		std::swap(start, end);
	}

	data->SelectionStart = start;
	data->SelectionEnd = end;
	data->CursorPos = end;
	top_panel->pending_source_highlight = false;
	return 0;
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

void DrawAstGraph(MainWindowState& main_window, const std::shared_ptr<AstNode>& root_node) {
	if (!root_node) {
		ImGui::TextDisabled("AST пока не построено");
		return;
	}

	float zoom = main_window.top_panel.ast_zoom;
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
			main_window.top_panel.ast_zoom = zoom;
		}
	}

	const int selected_index = main_window.top_panel.selected_ast_node;
	const AstNode* step_hovered_node = main_window.bottom_panel.hovered_step_node;
	int step_hovered_index = -1;
	if (step_hovered_node != nullptr) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			if (nodes[i].source_node.get() == step_hovered_node) {
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
					(parent_bottom.x + child_top.x) * 0.5f - label_size.x * 0.5f,
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
			main_window.top_panel.selected_ast_node = static_cast<int>(i);
			hovered_node_index = static_cast<int>(i);
			const std::shared_ptr<AstNode>& selected_ast_node = nodes[i].source_node;
			if (selected_ast_node) {
				RequestSourceHighlight(main_window, selected_ast_node->loc);
			}
		}
		ImGui::PopID();
	}

	const bool canvas_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	const bool need_clear_hover_selection = hovered_node_index < 0 && (!canvas_hovered || !ImGui::IsAnyItemHovered());
	if (need_clear_hover_selection && main_window.top_panel.selected_ast_node >= 0 && step_hovered_node == nullptr) {
		main_window.top_panel.selected_ast_node = -1;
		ClearSourceHighlight(main_window);
	}

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ImGui::GetIO().KeyCtrl && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
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

void DrawVerticalSplitter(float height, float thickness, float& left_width, float& right_width, float min_left, float min_right) {
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.45f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.45f, 0.45f, 0.65f));

	ImGui::InvisibleButton("##vertical_splitter", ImVec2(thickness, height));
	const bool is_active = ImGui::IsItemActive();
	if (ImGui::IsItemHovered() || is_active) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	if (is_active) {
		const float delta = ImGui::GetIO().MouseDelta.x;
		if (delta != 0.0f) {
			const float new_left = std::clamp(left_width + delta, min_left, left_width + right_width - min_right);
			const float applied_delta = new_left - left_width;
			left_width = new_left;
			right_width -= applied_delta;
		}
	}

	ImGui::PopStyleColor(3);
}

void DrawHorizontalSplitter(float width, float thickness, float& top_height, float& bottom_height, float min_top, float min_bottom) {
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.45f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.45f, 0.45f, 0.65f));

	ImGui::InvisibleButton("##horizontal_splitter", ImVec2(width, thickness));
	const bool is_active = ImGui::IsItemActive();
	if (ImGui::IsItemHovered() || is_active) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}

	if (is_active) {
		const float delta = ImGui::GetIO().MouseDelta.y;
		if (delta != 0.0f) {
			const float new_top = std::clamp(top_height + delta, min_top, top_height + bottom_height - min_bottom);
			const float applied_delta = new_top - top_height;
			top_height = new_top;
			bottom_height -= applied_delta;
		}
	}

	ImGui::PopStyleColor(3);
}

void DrawTopLeftPanel(CommandBuffer& cmd_buf, MainWindowState& main_window) {
	MainTopPanelState& state = main_window.top_panel;
	InferenceManagerState& inf_mgr_state = main_window.inf_mgr_state;
	int input_type_combo_index = InputTypeToComboIndex(inf_mgr_state.input_type);
	int algorithm_combo_index = AlgorithmKindToComboIndex(inf_mgr_state.algorithm);

	ImGui::Checkbox("Показать правую панель", &state.right_panel_open);
	ImGui::Separator();

	const bool text_selected = state.input_mode == MainInputMode::Text;
	if (ImGui::RadioButton("Текстовый ввод", text_selected)) {
		state.input_mode = MainInputMode::Text;
	}
	ImGui::SameLine();
	const bool file_selected = state.input_mode == MainInputMode::File;
	if (ImGui::RadioButton("Выбор файла", file_selected)) {
		state.input_mode = MainInputMode::File;
	}

	if (state.input_mode == MainInputMode::Text) {
		ImGui::InputTextMultiline(
			"##text_input",
			state.text_input.data(),
			state.text_input.size(),
			ImVec2(-1.0f, 120.0f),
			ImGuiInputTextFlags_CallbackAlways,
			ApplyInputHighlightCallback,
			&state
		);
	} else {
		ImGui::InputText("Файл", state.file_path.data(), state.file_path.size());
		ImGui::SameLine();
		ImGui::Button("...");
	}

	const char* first_combo_items[] = {"Lambda", "Haskell"};
	const char* second_combo_items[] = {"W", "M"};
	ImGui::Combo("Тип ввода", &input_type_combo_index, first_combo_items, IM_ARRAYSIZE(first_combo_items));
	ImGui::Combo("Алгоритм", &algorithm_combo_index, second_combo_items, IM_ARRAYSIZE(second_combo_items));

	const InputType selected_input_type = ComboIndexToInputType(input_type_combo_index);
	if (selected_input_type != inf_mgr_state.input_type) {
		cmd_buf.push(SetInputTypeCommand{selected_input_type});
		inf_mgr_state.input_type = selected_input_type;
		ResetMainWindowUiAfterInferenceReset(main_window);
	}

	const AlgorithmKind selected_algorithm = ComboIndexToAlgorithmKind(algorithm_combo_index);
	if (selected_algorithm != inf_mgr_state.algorithm) {
		cmd_buf.push(SetAlgorithmCommand{selected_algorithm});
		inf_mgr_state.algorithm = selected_algorithm;
		ResetMainWindowUiAfterInferenceReset(main_window);
	}

	if (ImGui::Button("Запустить")) {
		cmd_buf.push(SetInputCommand{std::string(state.text_input.data())});
		cmd_buf.push(RunAlgorithmCommand{true});
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Результат");
	ImGui::BeginChild("TopLeftResult", ImVec2(0.0f, 0.0f), true);
	if (inf_mgr_state.result.empty()) {
		ImGui::TextDisabled("Пока нет результата");
	} else {
		ImGui::TextWrapped("%s", inf_mgr_state.result.c_str());
	}
	ImGui::EndChild();
}

void DrawTopRightPanel(MainWindowState& main_window) {
	MainTopPanelState& state = main_window.top_panel;
	const InferenceManagerState& inf_mgr_state = main_window.inf_mgr_state;
	const bool has_active_source_hover = state.selected_ast_node >= 0 || main_window.bottom_panel.hovered_step_node != nullptr;

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
	DrawAstGraph(main_window, inf_mgr_state.input_parsed);
}

void DrawBottomPanel(MainWindowState& main_window) {
	const AlgorithmState& alg_state = main_window.inf_mgr_state.alg_state;
	const AstNode* hovered_step_node = nullptr;

	if (alg_state.steps.empty()) {
		main_window.bottom_panel.hovered_step_node = nullptr;
		ImGui::TextDisabled("Шаги алгоритма отсутствуют");
		return;
	}

	ImGui::PushID(main_window.bottom_panel.steps_ui_generation);

	for (size_t i = 0; i < alg_state.steps.size(); ++i) {
		const AlgorithmStep& step = alg_state.steps[i];
		ImGui::PushID(static_cast<int>(i));

		if (ImGui::CollapsingHeader(("Шаг " + std::to_string(i + 1)).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && step.at) {
				hovered_step_node = step.at.get();
			}
			if (step.data.empty()) {
				ImGui::TextDisabled("Пустой шаг");
			} else {
				const float content_width = std::max(1.0f, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x * 2.0f);
				const ImVec2 wrapped_text_size = ImGui::CalcTextSize(step.data.c_str(), nullptr, false, content_width);
				const float content_height = wrapped_text_size.y + ImGui::GetStyle().WindowPadding.y * 2.0f + ImGui::GetStyle().ItemSpacing.y + 4.0f;

				ImGui::BeginChild(
					"StepContent",
					ImVec2(0.0f, content_height),
					true,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
				);
				ImGui::PushTextWrapPos();
				ImGui::TextUnformatted(step.data.c_str());
				ImGui::PopTextWrapPos();
				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && step.at) {
					hovered_step_node = step.at.get();
				}
				ImGui::EndChild();
			}
		} else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && step.at) {
			hovered_step_node = step.at.get();
		}

		ImGui::PopID();
	}

	ImGui::PopID();

	main_window.bottom_panel.hovered_step_node = hovered_step_node;
	if (hovered_step_node != nullptr) {
		RequestSourceHighlight(main_window, hovered_step_node->loc);
	} else if (main_window.top_panel.selected_ast_node < 0) {
		ClearSourceHighlight(main_window);
	}
}

void DrawTopPanel(CommandBuffer& cmd_buf, MainWindowState& main_window, float splitter_thickness) {
	ImVec2 available = ImGui::GetContentRegionAvail();
	float panel_height = std::max(available.y, 1.0f);

	if (!main_window.top_panel.right_panel_open) {
		ImGui::BeginChild("TopLeftPanel", ImVec2(0.0f, panel_height), true);
		DrawTopLeftPanel(cmd_buf, main_window);
		ImGui::EndChild();
		return;
	}

	constexpr float min_left_width = 260.0f;
	constexpr float min_right_width = 220.0f;

	float right_width = std::clamp(main_window.top_panel.right_panel_width, min_right_width, std::max(min_right_width, available.x - min_left_width - splitter_thickness));
	float left_width = std::max(min_left_width, available.x - right_width - splitter_thickness);

	ImGui::BeginChild("TopLeftPanel", ImVec2(left_width, panel_height), true);
	DrawTopLeftPanel(cmd_buf, main_window);
	ImGui::EndChild();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::PushID("TopPanelVerticalSplitter");
	DrawVerticalSplitter(panel_height, splitter_thickness, left_width, right_width, min_left_width, min_right_width);
	ImGui::PopID();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginChild("TopRightPanel", ImVec2(0.0f, panel_height), true);
	DrawTopRightPanel(main_window);
	ImGui::EndChild();

	main_window.top_panel.right_panel_width = right_width;
}

} // namespace

MainWindowState::MainWindowState(const AppState& app_state) : inf_mgr_state(app_state.inf_mgr_state) {
	CopyStringToBuffer(top_panel.text_input, inf_mgr_state.input);
	SyncAstSourceCopyFromInferenceState(*this);
}

void DrawMainWindow(CommandBuffer& cmd_buf, MainWindowState& main_window) {
	constexpr float splitter_thickness = 8.0f;
	constexpr float min_top_height = 220.0f;
	constexpr float min_bottom_height = 120.0f;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	constexpr ImGuiWindowFlags main_window_flags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoDecoration;

	if (!ImGui::Begin("Main Window", nullptr, main_window_flags)) {
		ImGui::End();
		return;
	}

	const ImVec2 available = ImGui::GetContentRegionAvail();
	float bottom_height = std::clamp(main_window.bottom_panel.reserved_height, min_bottom_height, std::max(min_bottom_height, available.y - min_top_height - splitter_thickness));
	float top_height = std::max(min_top_height, available.y - bottom_height - splitter_thickness);

	ImGui::BeginChild("TopPanelRoot", ImVec2(0.0f, top_height), false, ImGuiWindowFlags_NoMove);
	DrawTopPanel(cmd_buf, main_window, splitter_thickness);
	ImGui::EndChild();

	ImGui::PushID("MainWindowHorizontalSplitter");
	DrawHorizontalSplitter(available.x, splitter_thickness, top_height, bottom_height, min_top_height, min_bottom_height);
	ImGui::PopID();

	ImGui::BeginChild("BottomPanelRoot", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
	DrawBottomPanel(main_window);
	ImGui::EndChild();

	main_window.bottom_panel.reserved_height = bottom_height;
	ImGui::End();

}


#define MAIN_WINDOW_EVENT_HANDLER_DEF(event) EVENT_HANDLER_DEF(event, MainWindowEventHandler)

MainWindowEventHandler::MainWindowEventHandler(MainWindowState& window) : window(window) {}

MAIN_WINDOW_EVENT_HANDLER_DEF(AlgorithmFinishedEvent) {
	window.inf_mgr_state = InferenceManagerState(event.new_inf_mgr_state);
	SyncAstSourceCopyFromInferenceState(window);
	++window.bottom_panel.steps_ui_generation;
}