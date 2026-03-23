
#include "ui.h"
#include "ui_event_dispatcher.hpp"

#include "layouts/main_window/window.h"
#include "layouts/main_window/state.h"

#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

#include <algorithm>
#include <cmath>
#include "imgui.h"

struct UiState
{
    MainWindowState main_window_state;

    std::optional<UiEventDispatcher> dispatcher;

    UiState(const UiInitStruct& app_state) : main_window_state(app_state) {}

    void BuildDispatcher() {
        dispatcher.emplace(
            UiEventDispatcher{
                .main_window_handler = MainWindowEventHandler(main_window_state)
            }
        );
    }
};

Ui::Ui(const UiInitStruct& app_state, CommandBuffer& cmd_buf, EventBuffer& event_buf) : cmd_buf(cmd_buf), event_buf(event_buf) {    
    state = new UiState(app_state);
    state->BuildDispatcher();
}

void Ui::draw_ui() {
    ImGui::NewFrame();

    draw_main_window(cmd_buf, state->main_window_state);
    // static bool show = true;
    // ImGui::ShowDemoWindow(&show);
    
    ImGui::Render();
}

void Ui::handle_events() {
    for(const auto& event : event_buf)
        std::visit(*state->dispatcher, event);

    event_buf.clear();
}

Ui::~Ui() {
    delete state;
}

Ui::Ui(Ui&& other) noexcept : state(other.state), cmd_buf(other.cmd_buf), event_buf(other.event_buf) {
    other.state = nullptr;
}

Ui& Ui::operator=(Ui&& other) noexcept {
    std::swap(state, other.state);
    return *this;
}