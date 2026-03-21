
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
        if (!renderer->InitCheck()) return false;
        ui.emplace(app.get_app_state(), cmd_buf, event_buf);

        while (!renderer->ShouldQuit())
        {
            if(!renderer->BeforeFrame()) continue;
            ui->DrawUi();
            renderer->AfterFrame();

            app.execute_commands();
            ui->HandleEvents();
        }

        return true;
    }
};

template<typename Flags>
bool check_flags(const Flags& flags) {
    return flags.template is_flag_set<'W'>() && flags.template is_flag_set<'M'>() ||
           flags.template is_flag_set<'l'>() && flags.template is_flag_set<'h'>() ||
           flags.template is_flag_set<'f'>() && flags.get_positional().size() != 0;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    constexpr static std::array possible_flags { 
        flag_data{'W', "algorithm_w", "Использовать алгоритм W для вывода типов (по умолчанию)", flag_type::BOOLEAN},
        flag_data{'M', "algorithm_m", "Использовать алгоритм M для вывода типов", flag_type::BOOLEAN},
        flag_data{'l', "lambda", "Использовать ввод в расширенном лямбда-исчислении (по умолчанию)", flag_type::BOOLEAN},
        flag_data{'h', "haskell", "Использовать ввод на Haskell", flag_type::BOOLEAN},
        flag_data{'f', "file", "Режим ввода через файл. В качестве параметра должен передаваться путь. Несовместимо с передачей выражения как параметра", flag_type::VALUE}
    };
    constexpr auto flag_parser = make_flag_parser<possible_flags, 0, 1>();
    auto parse_res = flag_parser.process_args(argc, argv);

    if (!parse_res) {
        std::cerr << "Ошибка при обработке аргументов запуска: " << parse_res.error_message << "\n\n";
        std::cerr << flag_parser.help_string(argv[0]) << std::endl;
        return -1;
    }

    auto& parsed_flags = parse_res.args;
    if(check_flags(parsed_flags)) {
        std::cerr << "Некорректная комбинация аргументов запуска.\n\n" << flag_parser.help_string(argv[0]) << std::endl;
        return -1;
    }

    if(parsed_flags.is_flag_set<'f'>() && !std::filesystem::exists(std::filesystem::path(parsed_flags.get_flag_value<'f'>().value()))) {
        std::cerr << "Указанный файл не найден: " << parsed_flags.get_flag_value<'f'>().value() << ".\n\n" << flag_parser.help_string(argv[0]) << std::endl;
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