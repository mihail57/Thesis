
#ifndef MAIN_WINDOW_STATE_H
#define MAIN_WINDOW_STATE_H

#include "../expression_visualizer/state.h"
#include "../step_visualizer/state.h"
#include "ui_init_struct.h"
#include <array>
#include <optional>

struct MainWindowState
{
	std::array<char, 2048> expression_input{};
	std::array<char, 512> file_path{};

	InputType input_type;
	AlgorithmKind algorithm;
	std::optional<std::string_view> result = std::nullopt;

	ExpressionVisualizerState expr_vis_state;
	StepVisualizerState step_vis_state;
	
	struct ImFont* monospace_font = nullptr;

	MainWindowState(const UiInitStruct& app_state);
};

#endif