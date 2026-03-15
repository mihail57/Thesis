
#ifndef MAIN_WINDOW_STATE_H
#define MAIN_WINDOW_STATE_H

#include "app_state.h"
#include <array>

enum class MainInputMode {
	Text,
	File
};

struct MainTopPanelState {
	MainInputMode input_mode = MainInputMode::Text;
	std::array<char, 2048> text_input{};
	std::array<char, 2048> ast_source_copy{};
	std::array<char, 512> file_path{};

	float ast_zoom = 1.0f;
	int selected_ast_node = -1;
	bool pending_source_highlight = false;
	bool clear_source_selection_only = false;
	int highlight_start = 0;
	int highlight_end = 0;

	bool right_panel_open = true;
	float right_panel_width = 320.0f;
};

struct MainBottomPanelState {
	float reserved_height = 180.0f;
	const AstNode* hovered_step_node = nullptr;
	int steps_ui_generation = 0;
};

struct MainWindowState
{
	MainTopPanelState top_panel;
	MainBottomPanelState bottom_panel;

	InferenceManagerState inf_mgr_state;

	MainWindowState(const AppState& app_state);
};

#endif