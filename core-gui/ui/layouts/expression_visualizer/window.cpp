
#include "state.h"
#include "window.h"

#include "ast_def.h"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>


constexpr float k_ast_horizontal_spacing = 28.0f;
constexpr float k_ast_vertical_spacing = 44.0f;
constexpr float k_ast_canvas_padding = 16.0f;
constexpr float k_ast_detail_zoom_threshold = 0.65f;

void ExpressionVisualizerState::reset() {
	ast_root.reset();
	selected_ast_node = -1;
	highlight_start = 0;
	highlight_end = 0;
	expression = std::nullopt;
}

void ExpressionVisualizerState::set_expression(std::string_view expr) {
	expression = expr;
}

void ExpressionVisualizerState::request_source_highlight(const SourceLoc& loc) {
	const int text_len = expression.has_value() ? static_cast<int>(expression->size()) : 0;
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
}


struct AstVisualNode {
	AstNode::ptr_t source_node;
	std::string title;
	std::vector<std::string> details;
	std::vector<int> children;
	std::vector<std::string> edge_labels;
	ImVec2 size = ImVec2(140.0f, 56.0f);
	float subtree_width = 0.0f;
	ImU32 fill_color = IM_COL32(128, 128, 128, 60);
	ImU32 border_color = IM_COL32(128, 128, 128, 200);
};

static int build_ast_visualization_data(const AstNode::ptr_t& node, std::vector<AstVisualNode>& out_nodes) {
	AstVisualNode visual_node;
	visual_node.source_node = node;
	std::vector<AstNode::ptr_t> child_nodes;

	if (!node) {
		visual_node.title = "Null";
		visual_node.fill_color = IM_COL32(200, 50, 50, 60);
		visual_node.border_color = IM_COL32(200, 50, 50, 200);
	} else if (auto const_node = std::dynamic_pointer_cast<ConstNode>(node)) {
		visual_node.title = "Con";
		visual_node.details.push_back(const_node->value + ": " + const_node->type);
		visual_node.fill_color = IM_COL32(50, 200, 120, 60);
		visual_node.border_color = IM_COL32(50, 200, 120, 200);
	} else if (auto var_node = std::dynamic_pointer_cast<VarNode>(node)) {
		visual_node.title = "Var";
		visual_node.details.push_back(var_node->var);
		visual_node.fill_color = IM_COL32(50, 120, 200, 60);
		visual_node.border_color = IM_COL32(50, 120, 200, 200);
	} else if (auto func_node = std::dynamic_pointer_cast<FuncNode>(node)) {
		visual_node.title = "Abs";
		visual_node.edge_labels = {"Параметр", "Тело"};
		visual_node.fill_color = IM_COL32(150, 80, 200, 60);
		visual_node.border_color = IM_COL32(150, 80, 200, 200);
		child_nodes.push_back(func_node->param);
		child_nodes.push_back(func_node->body);
	} else if (auto app_node = std::dynamic_pointer_cast<AppNode>(node)) {
		visual_node.title = "App";
		visual_node.edge_labels = {"Лямбда", "Аргумент"};
		visual_node.fill_color = IM_COL32(200, 150, 50, 60);
		visual_node.border_color = IM_COL32(200, 150, 50, 200);
		child_nodes.push_back(app_node->func);
		child_nodes.push_back(app_node->arg);
	} else if (auto let_node = std::dynamic_pointer_cast<LetNode>(node)) {
		visual_node.title = "Let";
		visual_node.edge_labels = {"Переменная", "Значение", "В выражении"};
		visual_node.fill_color = IM_COL32(100, 200, 50, 60);
		visual_node.border_color = IM_COL32(100, 200, 50, 200);
		child_nodes.push_back(let_node->bind_var);
		child_nodes.push_back(let_node->bind_value);
		child_nodes.push_back(let_node->ret_value);
	} else if (auto branch_node = std::dynamic_pointer_cast<BranchNode>(node)) {
		visual_node.title = "Branch";
		visual_node.edge_labels = {"Условие", "Если истинно", "Если ложно"};
		visual_node.fill_color = IM_COL32(200, 100, 50, 60);
		visual_node.border_color = IM_COL32(200, 100, 50, 200);
		child_nodes.push_back(branch_node->cond_expr);
		child_nodes.push_back(branch_node->true_expr);
		child_nodes.push_back(branch_node->false_expr);
	} else if (auto fix_node = std::dynamic_pointer_cast<FixNode>(node)) {
		visual_node.title = "Fix";
		visual_node.edge_labels = {"Лямбда"};
		visual_node.fill_color = IM_COL32(180, 80, 150, 60);
		visual_node.border_color = IM_COL32(180, 80, 150, 200);
		child_nodes.push_back(fix_node->func);
	} else {
		visual_node.title = "Неизвестный узел";
		visual_node.fill_color = IM_COL32(67, 56, 56, 240);
		visual_node.border_color = IM_COL32(210, 120, 120, 255);
	}

	const int node_index = static_cast<int>(out_nodes.size());
	out_nodes.push_back(std::move(visual_node));

	for (const AstNode::ptr_t& child : child_nodes) {
		const int child_index = build_ast_visualization_data(child, out_nodes);
		out_nodes[node_index].children.push_back(child_index);
	}

	if (out_nodes[node_index].edge_labels.size() < out_nodes[node_index].children.size()) {
		out_nodes[node_index].edge_labels.resize(out_nodes[node_index].children.size());
	}

	return node_index;
}

static void calculate_node_size(AstVisualNode& node, bool show_details) {
	const float line_height = ImGui::GetTextLineHeight();
	float max_text_width = ImGui::CalcTextSize(node.title.c_str()).x;
	if (show_details) {
		for (const std::string& detail : node.details) {
			max_text_width = std::max(max_text_width, ImGui::CalcTextSize(detail.c_str()).x);
		}
	}

	const float horizontal_padding = 20.0f;
	const float vertical_padding = 16.0f;
	const float min_width = 120.0f;
	const float min_height = 46.0f;
	const float lines_count = show_details ? static_cast<float>(1 + node.details.size()) : 1.0f;

	node.size.x = std::max(min_width, max_text_width + horizontal_padding);
	node.size.y = std::max(min_height, lines_count * line_height + vertical_padding);
}

static float calculate_subtree_width(int node_index, std::vector<AstVisualNode>& nodes, float h_spacing) {
	AstVisualNode& node = nodes[node_index];
	if (node.children.empty()) {
		node.subtree_width = node.size.x;
		return node.subtree_width;
	}

	float children_width = 0.0f;
	for (size_t i = 0; i < node.children.size(); ++i) {
		children_width += calculate_subtree_width(node.children[i], nodes, h_spacing);
		if (i + 1 < node.children.size()) {
			children_width += h_spacing;
		}
	}

	node.subtree_width = std::max(node.size.x, children_width);
	return node.subtree_width;
}

static void place_nodes(
	int node_index,
	std::vector<AstVisualNode>& nodes,
	std::vector<ImVec2>& positions,
	float left,
	float top,
	float h_spacing,
	float v_spacing
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
			total_children_width += h_spacing;
		}
	}

	float child_left = left + (node.subtree_width - total_children_width) * 0.5f;
	for (const int child_index : node.children) {
		place_nodes(child_index, nodes, positions, child_left, top + node.size.y + v_spacing, h_spacing, v_spacing);
		child_left += nodes[child_index].subtree_width + h_spacing;
	}
}

static void draw_ast_visualizer(ExpressionVisualizerState& state) {
	const auto& root_node = state.ast_root;

	if (!root_node) {
		ImGui::TextDisabled("AST пока не построено");
		return;
	}

	float zoom = state.zoom_factor;
	zoom = std::clamp(zoom, 0.4f, 1.0f);
	const float helper_area_height = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
	const bool show_details = zoom >= k_ast_detail_zoom_threshold;

	std::vector<AstVisualNode> nodes;
	const int root_index = build_ast_visualization_data(root_node, nodes);
	for (AstVisualNode& node : nodes) {
		calculate_node_size(node, show_details);
		node.size.x *= zoom;
		node.size.y *= zoom;
	}

	const float h_spacing = k_ast_horizontal_spacing * zoom;
	const float v_spacing = k_ast_vertical_spacing * zoom;
	const float padding = k_ast_canvas_padding * zoom;

	calculate_subtree_width(root_index, nodes, h_spacing);

	std::vector<ImVec2> positions(nodes.size());
	place_nodes(root_index, nodes, positions, padding, padding, h_spacing, v_spacing);

	float canvas_width = 0.0f;
	float canvas_height = 0.0f;
	for (size_t i = 0; i < nodes.size(); ++i) {
		canvas_width = std::max(canvas_width, positions[i].x + nodes[i].size.x + padding);
		canvas_height = std::max(canvas_height, positions[i].y + nodes[i].size.y + padding);
	}

	const float pan_margin = 120.0f;
	const float graph_width_calc = std::max(1.0f, canvas_width - 2.0f * padding);
	const float graph_height_calc = std::max(1.0f, canvas_height - 2.0f * padding);

	if (state.has_pending_scroll) {
		ImGui::SetNextWindowContentSize(ImVec2(graph_width_calc + pan_margin * 2.0f, graph_height_calc + pan_margin * 2.0f));
		ImGui::SetNextWindowScroll(ImVec2(state.pending_scroll_x, state.pending_scroll_y));
		state.has_pending_scroll = false;
	}

	ImGui::BeginChild("AstCanvas", ImVec2(0.0f, -helper_area_height), true, ImGuiWindowFlags_HorizontalScrollbar);
	
	const ImVec2 start_pos = ImGui::GetCursorScreenPos();
	
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::GetIO().KeyCtrl) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			float new_zoom = std::clamp(zoom + wheel * 0.08f, 0.4f, 1.0f);
			if (new_zoom != zoom) {
				float zoom_ratio = new_zoom / zoom;
				ImVec2 mouse_pos = ImGui::GetMousePos();
				
				float old_scroll_x = ImGui::GetScrollX();
				float old_scroll_y = ImGui::GetScrollY();
				
				float base_x = start_pos.x + old_scroll_x;
				float base_y = start_pos.y + old_scroll_y;
				
				float canvas_pt_x = mouse_pos.x - start_pos.x - pan_margin;
				float canvas_pt_y = mouse_pos.y - start_pos.y - pan_margin;
				
				state.pending_scroll_x = base_x - mouse_pos.x + pan_margin + canvas_pt_x * zoom_ratio;
				state.pending_scroll_y = base_y - mouse_pos.y + pan_margin + canvas_pt_y * zoom_ratio;
				state.has_pending_scroll = true;
				
				state.zoom_factor = new_zoom;
			}
		}
	}

	const int selected_index = state.selected_ast_node;

	int step_hovered_index = -1;
	if (state.hovered_node) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			if (nodes[i].source_node == state.hovered_node) {
				step_hovered_index = static_cast<int>(i);
				break;
			}
		}
	}
	
	const ImVec2 view_size = ImGui::GetContentRegionAvail();
	const float content_width = graph_width_calc + pan_margin * 2.0f;
	const float content_height = graph_height_calc + pan_margin * 2.0f;
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
			ImU32 edge_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
			draw_list->AddLine(parent_bottom, ImVec2(parent_bottom.x, mid_y), edge_color, 1.4f);
			draw_list->AddLine(ImVec2(parent_bottom.x, mid_y), ImVec2(child_top.x, mid_y), edge_color, 1.4f);
			draw_list->AddLine(ImVec2(child_top.x, mid_y), child_top, edge_color, 1.4f);

			if (show_details && edge_i < parent.edge_labels.size() && !parent.edge_labels[edge_i].empty()) {
				const std::string& edge_label = parent.edge_labels[edge_i];
				const ImVec2 label_size = ImGui::CalcTextSize(edge_label.c_str());
				const ImVec2 label_pos = ImVec2(
					child_top.x - label_size.x * 0.5f,
					mid_y - label_size.y - 3.0f
				);
				draw_list->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), edge_label.c_str());
			}
		}
	}

	const float line_height = ImGui::GetTextLineHeight() * zoom;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const AstVisualNode& node = nodes[i];
		const ImVec2 rect_min = ImVec2(start_pos.x + pan.x + positions[i].x, start_pos.y + pan.y + positions[i].y);
		const ImVec2 rect_max = ImVec2(rect_min.x + node.size.x, rect_min.y + node.size.y);
		const bool is_selected = static_cast<int>(i) == selected_index || static_cast<int>(i) == step_hovered_index;
		const ImU32 border_color = is_selected ? ImGui::GetColorU32(ImVec4(0.9f, 0.7f, 0.1f, 1.0f)) : node.border_color;
		const float border_thickness = is_selected ? 2.8f : 1.5f;

		draw_list->AddRectFilled(rect_min, rect_max, ImGui::GetColorU32(ImGuiCol_WindowBg), 8.0f);
		draw_list->AddRectFilled(rect_min, rect_max, node.fill_color, 8.0f);
		draw_list->AddRect(rect_min, rect_max, border_color, 8.0f, 0, border_thickness);

		ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

		if (show_details) {
			ImVec2 text_pos = ImVec2(rect_min.x + 10.0f * zoom, rect_min.y + 7.0f * zoom);
			draw_list->AddText(text_pos, text_col, node.title.c_str());
			for (const std::string& detail : node.details) {
				text_pos.y += line_height;
				draw_list->AddText(text_pos, text_col, detail.c_str());
			}
		} else {
			ImVec2 title_size = ImGui::CalcTextSize(node.title.c_str());
			ImVec2 text_pos = ImVec2(
				rect_min.x + (node.size.x - title_size.x) * 0.5f,
				rect_min.y + (node.size.y - title_size.y) * 0.5f
			);
			draw_list->AddText(text_pos, text_col, node.title.c_str());
		}

		ImGui::SetCursorScreenPos(rect_min);
		ImGui::PushID(static_cast<int>(i));
		ImGui::InvisibleButton("ast_node_select", node.size);
		const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		if (hovered) {
			state.selected_ast_node = static_cast<int>(i);
			hovered_node_index = static_cast<int>(i);
			const AstNode::ptr_t& selected_ast_node = nodes[i].source_node;
			if (selected_ast_node) {
				state.request_source_highlight(selected_ast_node->loc);
			}
		}
		ImGui::PopID();
	}

	const bool canvas_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	ImGuiIO& io = ImGui::GetIO();
	if (canvas_hovered && !io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive()) {
		state.panning_active = true;
	}
	if (state.panning_active && (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || io.KeyCtrl)) {
		state.panning_active = false;
	}

	const bool need_clear_hover_selection = hovered_node_index < 0 && (!canvas_hovered || !ImGui::IsAnyItemHovered());
	if (need_clear_hover_selection && state.selected_ast_node >= 0 && !state.hovered_node) {
		state.selected_ast_node = -1;
	}

	if (state.panning_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		const float min_visible = 24.0f;
		const float min_scroll_x = std::max(0.0f, graph_origin_x - view_size.x + min_visible);
		const float max_scroll_x = std::min(ImGui::GetScrollMaxX(), graph_origin_x + graph_width_calc - min_visible);
		const float min_scroll_y = std::max(0.0f, graph_origin_y - view_size.y + min_visible);
		const float max_scroll_y = std::min(ImGui::GetScrollMaxY(), graph_origin_y + graph_height_calc - min_visible);

		const float next_scroll_x = std::clamp(ImGui::GetScrollX() - io.MouseDelta.x, min_scroll_x, max_scroll_x);
		const float next_scroll_y = std::clamp(ImGui::GetScrollY() - io.MouseDelta.y, min_scroll_y, max_scroll_y);
		ImGui::SetScrollX(next_scroll_x);
		ImGui::SetScrollY(next_scroll_y);
	}

	ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
	ImGui::Dummy(ImVec2(content_width, content_height));
	ImGui::EndChild();

	ImGui::TextDisabled("Ctrl + колесо: масштаб");
	ImGui::SameLine();
	ImGui::Text("x%.2f", zoom);
	ImGui::TextDisabled("ЛКМ + перетаскивание: панорама");
}

static void draw_expression_display_field(const ExpressionVisualizerState& state) {
	bool highlight_active = state.selected_ast_node >= 0 || static_cast<bool>(state.hovered_node);
	ImGui::BeginChild("SourceHoverPreview", ImVec2(0.0f, 78.0f), true);
	if (!state.expression.has_value()) {
		ImGui::TextDisabled("Исходный текст пуст");
		ImGui::EndChild();
		return;
	}

	const std::string_view text = *state.expression;

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

	// Рисуем текст вручную через DrawList с поддержкой переносов и подсветки
	const float line_height = ImGui::GetTextLineHeight();
	const ImVec2 available_size = ImGui::GetContentRegionAvail();
	const float wrap_width = available_size.x - 8.0f; // Отступ от края
	
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 text_start_pos = ImGui::GetCursorScreenPos();
	ImVec2 current_pos = text_start_pos;
	
	ImU32 normal_color = ImGui::GetColorU32(ImGuiCol_Text);
	ImU32 highlight_color = ImGui::GetColorU32(ImVec4(0.95f, 0.82f, 0.35f, 1.0f));
	
	const char* text_begin = text.data();
	const char* text_end = text_begin + text.size();
	const char* line_begin = text_begin;
	
	while (line_begin < text_end) {
		// Находим конец строки
		const char* line_end = line_begin;
		while (line_end < text_end && *line_end != '\n') {
			++line_end;
		}
		
		// Вычисляем размер полной строки
		ImVec2 line_size = ImGui::CalcTextSize(line_begin, line_end);
		
		// Если строка не умещается, переносим на новую строку в рекурсивном стиле
		if (current_pos.x + line_size.x > text_start_pos.x + wrap_width && current_pos.x > text_start_pos.x) {
			current_pos.x = text_start_pos.x;
			current_pos.y += line_height;
		}
		
		// Рисуем строку посимвольно с выделением нужных символов
		int line_start_pos = static_cast<int>(line_begin - text_begin);
		int line_end_pos = static_cast<int>(line_end - text_begin);
		
		for (int i = line_start_pos; i < line_end_pos; ++i) {
			char ch = text_begin[i];
			std::string ch_str(1, ch);
			ImVec2 ch_size = ImGui::CalcTextSize(ch_str.c_str());
			
			// Проверяем, находится ли символ в диапазоне выделения
			ImU32 ch_color = (i >= start && i < end && highlight_active) ? highlight_color : normal_color;
			
			draw_list->AddText(current_pos, ch_color, ch_str.c_str());
			current_pos.x += ch_size.x;
			
			// Если следующий символ не умещается, переносим на новую строку
			if (i + 1 < line_end_pos) {
				ImVec2 next_ch_size = ImGui::CalcTextSize(std::string(1, text_begin[i + 1]).c_str());
				if (current_pos.x + next_ch_size.x > text_start_pos.x + wrap_width) {
					current_pos.x = text_start_pos.x;
					current_pos.y += line_height;
				}
			}
		}
		
		// Переходим на новую строку
		line_begin = line_end + 1;
		if (line_end < text_end && *line_end == '\n') {
			current_pos.x = text_start_pos.x;
			current_pos.y += line_height;
		}
	}
	
	// Резервируем место в ImGui для корректной работы скроллинга и макета
	ImGui::Dummy(ImVec2(0.0f, (current_pos.y - text_start_pos.y) + line_height));
	ImGui::EndChild();
}


void draw_expression_visualizer(ExpressionVisualizerState& expr_vis_state, CommandBuffer& cmd_buf) {    
	ImGui::SameLine(0.0f, 0.0f);

	ImGui::BeginChild("ExpressionVisualizer", ImVec2(0.0f, expr_vis_state.height), true);
	draw_expression_display_field(expr_vis_state);
	ImGui::Spacing();
	draw_ast_visualizer(expr_vis_state);

	ImGui::EndChild();
}