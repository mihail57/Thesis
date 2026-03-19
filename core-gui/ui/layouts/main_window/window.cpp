
#include "../expression_visualizer/window.h"
#include "../step_visualizer/window.h"

#include "state.h"
#include "event_handler.h"
#include "window.h"

#include "string_to_buffer.hpp"

#include "imgui.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>


static void set_ui_state_from_inf_mgr_state(MainWindowState& window, const InferenceManagerState& inf_mgr_state) {
	CopyStringToBuffer(window.text_input, inf_mgr_state.input);
	CopyStringToBuffer(window.expr_vis_state.ast_source_copy, inf_mgr_state.input);
	window.input_type = inf_mgr_state.input_type;
	window.algorithm = inf_mgr_state.algorithm;
	window.expr_vis_state.root = inf_mgr_state.input_parsed;
	window.step_vis_state.alg_state = inf_mgr_state.alg_state;
	window.result = inf_mgr_state.result;
	++window.step_vis_state.steps_ui_generation;
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
	main_window.result = std::nullopt;

	main_window.expr_vis_state.reset();

	main_window.step_vis_state.reset();
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

void DrawTopLeftPanel(CommandBuffer& cmd_buf, MainWindowState& state) {
	int input_type_combo_index = InputTypeToComboIndex(state.input_type);
	int algorithm_combo_index = AlgorithmKindToComboIndex(state.algorithm);

	ImGui::Checkbox("Показать правую панель", &state.expr_vis_state.right_panel_open);
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
			ImVec2(-1.0f, 120.0f)
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
	if (selected_input_type != state.input_type) {
		cmd_buf.push(SetInputTypeCommand{selected_input_type});
		state.input_type = selected_input_type;
		ResetMainWindowUiAfterInferenceReset(state);
	}

	const AlgorithmKind selected_algorithm = ComboIndexToAlgorithmKind(algorithm_combo_index);
	if (selected_algorithm != state.algorithm) {
		cmd_buf.push(SetAlgorithmCommand{selected_algorithm});
		state.algorithm = selected_algorithm;
		ResetMainWindowUiAfterInferenceReset(state);
	}

	if (ImGui::Button("Запустить")) {
		cmd_buf.push(SetInputCommand{std::string(state.text_input.data())});
		cmd_buf.push(RunAlgorithmCommand{true});
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Результат");
	ImGui::BeginChild("TopLeftResult", ImVec2(0.0f, 0.0f), true);
	if (!state.result.has_value()) {
		ImGui::TextDisabled("Пока нет результата");
	} else {
		ImGui::TextWrapped("%s", state.result.value().data());
	}
	ImGui::EndChild();
}

MainWindowState::MainWindowState(const AppState& app_state) {
	set_ui_state_from_inf_mgr_state(*this, app_state.inf_mgr_state);
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
	float bottom_height = std::clamp(main_window.step_vis_state.reserved_height, min_bottom_height, std::max(min_bottom_height, available.y - min_top_height - splitter_thickness));
	float top_height = std::max(min_top_height, available.y - bottom_height - splitter_thickness);

	ImGui::BeginChild("TopPanelRoot", ImVec2(0.0f, top_height), false, ImGuiWindowFlags_NoMove);
	ImVec2 child_available = ImGui::GetContentRegionAvail();
	float panel_height = std::max(child_available.y, 1.0f);

	if (!main_window.expr_vis_state.right_panel_open) {
		ImGui::BeginChild("TopLeftPanel", ImVec2(0.0f, panel_height), true);
		DrawTopLeftPanel(cmd_buf, main_window);
		ImGui::EndChild();
		return;
	}

	constexpr float min_left_width = 260.0f;
	constexpr float min_right_width = 220.0f;

	float right_width = std::clamp(main_window.expr_vis_state.right_panel_width, min_right_width, std::max(min_right_width, available.x - min_left_width - splitter_thickness));
	float left_width = std::max(min_left_width, child_available.x - right_width - splitter_thickness);

	ImGui::BeginChild("TopLeftPanel", ImVec2(left_width, panel_height), true);
	DrawTopLeftPanel(cmd_buf, main_window);
	ImGui::EndChild();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::PushID("TopPanelVerticalSplitter");
	DrawVerticalSplitter(panel_height, splitter_thickness, left_width, right_width, min_left_width, min_right_width);
	ImGui::PopID();

	main_window.expr_vis_state.panel_height = panel_height;
	DrawExpressionVisualizer(main_window.expr_vis_state, cmd_buf);

	main_window.expr_vis_state.right_panel_width = right_width;
	ImGui::EndChild();

	ImGui::PushID("MainWindowHorizontalSplitter");
	DrawHorizontalSplitter(available.x, splitter_thickness, top_height, bottom_height, min_top_height, min_bottom_height);
	ImGui::PopID();

	DrawStepVisualizer(main_window.step_vis_state, cmd_buf);
	main_window.expr_vis_state.step_hovered = main_window.step_vis_state.hovered_step_node;
	if (main_window.step_vis_state.hovered_step_node != nullptr) {
		main_window.expr_vis_state.request_source_highlight(main_window.step_vis_state.hovered_step_node->loc);
	} else if (main_window.expr_vis_state.selected_ast_node < 0) {
		main_window.expr_vis_state.clear_source_highlight();
	}

	main_window.step_vis_state.reserved_height = bottom_height;
	ImGui::End();

}


#define MAIN_WINDOW_EVENT_HANDLER_DEF(event) EVENT_HANDLER_DEF(event, MainWindowEventHandler)

MainWindowEventHandler::MainWindowEventHandler(MainWindowState& window) : window(window) {}

MAIN_WINDOW_EVENT_HANDLER_DEF(AlgorithmFinishedEvent) {
	set_ui_state_from_inf_mgr_state(window, event.new_inf_mgr_state);
}