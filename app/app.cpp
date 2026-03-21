
#include "include/app.h"
#include "inference/inference_manager.h"
#include "command_dispatcher.hpp"
#include "command_buffer.h"

struct AppInternal
{
    InferenceManager inf_mgr;
    std::optional<CommandDispatcher> dispatcher;

    AppInternal() 
        : inf_mgr(), dispatcher(std::nullopt) {}

    void build_dispatcher(EventBuffer& event_buf) {
        dispatcher.emplace(CommandDispatcher(CommandHandler(inf_mgr), event_buf));
    }
};

App::App() 
    : internal(new AppInternal()), app_state(internal->inf_mgr.state) {}

App::~App() { delete internal; }


void App::bind_buffers(CommandBuffer& cmd_buf, EventBuffer& event_buf) {
    this->cmd_buf = cmd_buf;
    this->event_buf = event_buf;
    internal->build_dispatcher(event_buf);
}

void App::execute_commands() {
    for(const auto& command: cmd_buf.value().get()) {
        std::visit(*internal->dispatcher, command);
    }

    cmd_buf.value().get().clear();
}

AppState& App::get_app_state() { return app_state; }

void App::set_default_properties(std::string input, InputType input_type, AlgorithmKind algorithm) {
    auto& inf_mgr = internal->inf_mgr;
    inf_mgr.change_algorithm(algorithm);
    inf_mgr.change_input_type(input_type);
    inf_mgr.change_input(input);
}

std::tuple<bool, std::string> App::run_algorithm() {
    auto& inf_mgr = internal->inf_mgr;
    auto ok = inf_mgr.run_algorithm(false);
    auto result = inf_mgr.state.result;
    inf_mgr.change_input("");
    return { ok, result };
}

