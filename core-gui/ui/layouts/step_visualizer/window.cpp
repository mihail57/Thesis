
#include "state.h"
#include "window.h"

#include "imgui.h"

void StepVisualizerState::reset() {
	hovered_step_node.reset();
	alg_state.steps.clear();
	++steps_ui_generation;
}

void draw_step_visualizer(StepVisualizerState& step_vis_state, CommandBuffer& cmd_buf) {
	ImGui::BeginChild("BottomPanelRoot", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
	AstNode::ptr_t hovered_step_node;

	if (step_vis_state.alg_state.steps.empty()) {
		step_vis_state.hovered_step_node.reset();
		ImGui::TextDisabled("Шаги алгоритма отсутствуют");
	    ImGui::EndChild();
		return;
	}

	ImGui::PushID(step_vis_state.steps_ui_generation);

	for (size_t i = 0; i < step_vis_state.alg_state.steps.size(); ++i) {
		const AlgorithmStep& step = step_vis_state.alg_state.steps[i];
		ImGui::PushID(static_cast<int>(i));

		if (ImGui::CollapsingHeader(("Шаг " + std::to_string(i + 1)).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && step.at) {
				hovered_step_node = step.at;
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
					hovered_step_node = step.at;
				}
				ImGui::EndChild();
			}
		} else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && step.at) {
			hovered_step_node = step.at;
		}

		ImGui::PopID();
	}

	ImGui::PopID();

	step_vis_state.hovered_step_node = hovered_step_node;
	ImGui::EndChild();
}