
#include "app.h"
#include "event_buffer.h"
#include "command_buffer.h"
#include "../flag_parser.hpp"

#include <iostream>
#include <functional>
#include <filesystem>
#include <vector>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#endif

namespace {

std::filesystem::path get_current_executable_dir() {
#ifdef _WIN32
    char exe_path[MAX_PATH] = {0};
    auto len = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return {};
    }
    return std::filesystem::path(exe_path).parent_path();
#else
    char exe_path[PATH_MAX] = {0};
    const auto len = readlink("/proc/self/exe", exe_path, PATH_MAX - 1);
    if (len <= 0) {
        return {};
    }
    exe_path[len] = '\0';
    return std::filesystem::path(exe_path).parent_path();
#endif
}

template<typename Flags>
std::vector<std::string> make_gui_forwarded_args(const Flags& flags) {
    std::vector<std::string> args;

    if (flags.template is_flag_set<'W'>()) args.emplace_back("-W");
    if (flags.template is_flag_set<'M'>()) args.emplace_back("-M");
    if (flags.template is_flag_set<'l'>()) args.emplace_back("-l");
    if (flags.template is_flag_set<'h'>()) args.emplace_back("-h");

    for (const auto& positional : flags.get_positional()) {
        args.emplace_back(positional);
    }

    return args;
}

#ifdef _WIN32
std::string quote_windows_arg(const std::string& arg) {
    if (arg.find_first_of(" \t\"") == std::string::npos) {
        return arg;
    }

    std::string out;
    out.reserve(arg.size() + 2);
    out.push_back('"');
    for (char ch : arg) {
        if (ch == '"') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}
#endif

template<typename Flags>
bool launch_gui_from_same_dir(const Flags& flags) {
    const auto exe_dir = get_current_executable_dir();
    if (exe_dir.empty()) {
        return false;
    }

    const auto passthrough_args = make_gui_forwarded_args(flags);

#ifdef _WIN32
    const auto gui_path = exe_dir / "core-gui.exe";
    if (!std::filesystem::exists(gui_path)) {
        return false;
    }

    std::string cmd_line = quote_windows_arg(gui_path.string());
    for (const auto& arg : passthrough_args) {
        cmd_line += ' ';
        cmd_line += quote_windows_arg(arg);
    }
    std::vector<char> cmd_line_buffer(cmd_line.begin(), cmd_line.end());
    cmd_line_buffer.push_back('\0');

    STARTUPINFOA startup_info{};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info{};

    const BOOL started = CreateProcessA(
        nullptr,
        cmd_line_buffer.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        exe_dir.string().c_str(),
        &startup_info,
        &process_info
    );

    if (!started) {
        return false;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return true;
#else
    const auto gui_path = exe_dir / "core-gui";
    if (!std::filesystem::exists(gui_path)) {
        return false;
    }

    std::vector<std::string> exec_args;
    exec_args.reserve(passthrough_args.size() + 1);
    exec_args.push_back(gui_path.string());
    for (const auto& arg : passthrough_args) {
        exec_args.push_back(arg);
    }

    std::vector<char*> exec_argv;
    exec_argv.reserve(exec_args.size() + 1);
    for (auto& arg : exec_args) {
        exec_argv.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        execv(gui_path.string().c_str(), exec_argv.data());
        _exit(127);
    }

    return true;
#endif
}

} // namespace

class Main {
    CommandBuffer cmd_buf;
    EventBuffer event_buf;
    App app;

    template<typename ConsoleArgs>
    auto console_args_to_params(const ConsoleArgs& args) {
        auto positional_container = args.get_positional();     
        std::string input;
        if(positional_container.size() == 0)
            std::getline(std::cin, input);
        else input = positional_container[0];

        auto input_type = args.template is_flag_set<"haskell">() ? InputType::Haskell : InputType::Lambda;
        auto algorithm = args.template is_flag_set<"algorithm_m">() ? AlgorithmKind::M : AlgorithmKind::W;
        return std::make_tuple(input, input_type, algorithm);
    }

public:
    Main() : cmd_buf(), event_buf(), app(cmd_buf, event_buf) {
    }

    template<typename ConsoleArgs>
    bool run(const ConsoleArgs& flags) {
        std::apply(
            [&](auto&&... args) { return app.set_default_properties(std::forward<decltype(args)>(args)...); }, 
            console_args_to_params(flags)
        );
        auto [ok, result] = app.run_algorithm();
        std::cout << result << std::endl;
        return ok;
    }
};

template<typename Flags>
bool check_flags(const Flags& flags) {
    return flags.template is_flag_set<'W'>() && flags.template is_flag_set<'M'>() ||
           flags.template is_flag_set<'l'>() && flags.template is_flag_set<'h'>();
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
        flag_data{'g', "gui", "Открыть интерактивную версию вместо консольной", flag_type::BOOLEAN}
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

    if(parsed_flags.is_flag_set<'g'>()) {
        if(!launch_gui_from_same_dir(parsed_flags)) {
            std::cerr << "Не удалось запустить core-gui из директории текущего исполняемого файла." << std::endl;
            return -1;
        }
        return 0;
    }

    Main main_app;
    if(!main_app.run(parsed_flags)) return -1;
}