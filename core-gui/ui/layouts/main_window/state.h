
#ifndef MAIN_WINDOW_STATE_H
#define MAIN_WINDOW_STATE_H

#include "../expression_visualizer/state.h"
#include "../step_visualizer/state.h"
#include "app_state.h"
#include <array>
#include <optional>

enum class MainInputMode {
	Text,
	File
};

struct MainWindowState
{
	MainInputMode input_mode = MainInputMode::Text;
	std::array<char, 2048> text_input{};
	std::array<char, 512> file_path{};

	InputType input_type;
	AlgorithmKind algorithm;
	std::optional<std::string_view> result = std::nullopt;

	ExpressionVisualizerState expr_vis_state;
	StepVisualizerState step_vis_state;
	
	struct ImFont* monospace_font = nullptr;

	MainWindowState(const AppState& app_state);
};

#endif