
#include "app_state.h"
#include "command_buffer.h"
#include "event_buffer.h"

struct AppInternal;

class App {
    AppInternal* internal;
    AppState app_state;
    CommandBuffer& cmd_buf;
    EventBuffer& event_buf;

public:
    App(CommandBuffer& cmd_buf, EventBuffer& event_buf);
    ~App();

    void set_default_properties(std::string input, InputType input_type, AlgorithmKind algorithm);
    std::tuple<bool, std::string> run_algorithm();
    void execute_commands();
    AppState get_app_state() const;
};