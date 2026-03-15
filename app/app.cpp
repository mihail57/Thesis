
#include "include/app.h"
#include "inference/inference_manager.h"
#include "command_dispatcher.hpp"
#include "command_buffer.h"

struct AppInternal
{
    InferenceManager inf_mgr;
    CommandDispatcher dispatcher;

    AppInternal(EventBuffer& event_buf) 
        : inf_mgr(), dispatcher(build_dispatcher(event_buf)) {}

private:
    CommandDispatcher build_dispatcher(EventBuffer& event_buf) {
        return CommandDispatcher(CommandHandler(inf_mgr), event_buf);
    }
};

App::App(CommandBuffer& cmd_buf, EventBuffer& event_buf) 
    : cmd_buf(cmd_buf), event_buf(event_buf),
       internal(new AppInternal(event_buf)), app_state(internal->inf_mgr.state) {}

App::~App() { delete internal; }

void App::execute_commands() {
    for(const auto& command: cmd_buf) {
        std::visit(internal->dispatcher, command);
    }

    cmd_buf.clear();
}

AppState App::get_app_state() const { return app_state; }

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

