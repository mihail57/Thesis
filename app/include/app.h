
#include "../../infrastructure/app_state.h"
#include "../../infrastructure/command_buffer.h"
#include "../../infrastructure/event_buffer.h"
#include <optional>

struct AppInternal;

class App {
    AppInternal* internal;
    AppState app_state;
    std::optional<std::reference_wrapper<CommandBuffer>> cmd_buf;
    std::optional<std::reference_wrapper<EventBuffer>> event_buf;

public:
    App();
    ~App();

    void bind_buffers(CommandBuffer& cmd_buf, EventBuffer& event_buf);

    void set_default_properties(std::string input, InputType input_type, AlgorithmKind algorithm);
    std::tuple<bool, std::string> run_algorithm();
    void execute_commands();
    AppState& get_app_state();
};