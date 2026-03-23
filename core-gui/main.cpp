
#include "app.h"
#include "event_buffer.h"
#include "command_buffer.h"
#include "ui.h"
#include "renderer.h"
#include "../flag_parser.hpp"

#include <iostream>
#include <functional>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

class Main {
    CommandBuffer cmd_buf;
    EventBuffer event_buf;
    App app;
    std::optional<Renderer> renderer = std::nullopt;
    std::optional<Ui> ui = std::nullopt;

    template<typename ConsoleArgs>
    auto console_args_to_params(const ConsoleArgs& args) {
        auto positional_container = args.get_positional();     
        std::string input;
        if(args.template is_flag_set<'f'>()) {
            input = args.template get_flag_value<'f'>().value();
        }
        else if(positional_container.size() == 0)
            input = "";
        else input = positional_container[0];

        auto input_type = args.template is_flag_set<'f'>() ? InputType::File : InputType::Text;
        auto algorithm = args.template is_flag_set<"algorithm_m">() ? AlgorithmKind::M : AlgorithmKind::W;
        return std::make_tuple(input, input_type, algorithm);
    }

public:
    Main() : cmd_buf(), event_buf(), app() {
        app.bind_buffers(cmd_buf, event_buf);
    }

    template<typename ConsoleArgs>
    bool run(const ConsoleArgs& flags) {
        std::apply(
            [&](auto&&... args) { return app.set_default_properties(std::forward<decltype(args)>(args)...); },
            console_args_to_params(flags)
        );
        
        renderer.emplace(app.get_app_state());
        if (!renderer->init_check()) return false;
        ui.emplace(app.get_app_state(), cmd_buf, event_buf);

        while (!renderer->should_quit())
        {
            if(!renderer->before_frame()) continue;
            ui->draw_ui();
            renderer->after_frame();

            app.execute_commands();
            ui->handle_events();
        }

        return true;
    }
};

template<typename Flags>
bool check_flags(const Flags& flags) {
    return flags.template is_flag_set<'W'>() && flags.template is_flag_set<'M'>() ||
           flags.template is_flag_set<'f'>() && flags.get_positional().size() != 0;
}

int main(int argc, char** argv) {
    constexpr static std::array possible_flags { 
        FlagData{'W', "algorithm_w", "Использовать алгоритм W для вывода типов (по умолчанию)", FlagType::BOOLEAN},
        FlagData{'M', "algorithm_m", "Использовать алгоритм M для вывода типов", FlagType::BOOLEAN},
        FlagData{'f', "file", "Режим ввода через файл. В качестве параметра должен передаваться путь. Несовместимо с передачей выражения как параметра", FlagType::VALUE}
    };
    constexpr auto flag_parser = make_flag_parser<possible_flags, 0, 1>();
    auto parse_res = flag_parser.process_args(argc, argv);

    if (!parse_res) {
        Renderer::show_error(
            std::string("Ошибка при обработке аргументов запуска: ") + parse_res.error_message + "\n\n" + flag_parser.help_string(argv[0])
        );
        return -1;
    }

    auto& parsed_flags = parse_res.args;
    if(check_flags(parsed_flags)) {
        Renderer::show_error(
            std::string("Некорректная комбинация аргументов запуска.\n\n") + flag_parser.help_string(argv[0])
        );
        return -1;
    }

    if(parsed_flags.is_flag_set<'f'>() && !std::filesystem::exists(std::filesystem::path(parsed_flags.get_flag_value<'f'>().value()))) {
        Renderer::show_error(
            std::string("Указанный файл не найден: ") + parsed_flags.get_flag_value<'f'>().value() + ".\n\n" + flag_parser.help_string(argv[0])
        );
        return -1;
    }

    Main main_app;
    if(!main_app.run(parsed_flags)) return -1;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif