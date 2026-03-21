#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <tuple>
#include <optional>
#include <stdexcept>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

enum class flag_type { VALUE, BOOLEAN };

struct flag_data {
    char flag_short;
    char flag_long[20];
    char msg[300];
    flag_type type;
};

template <size_t N>
struct fixed_string {
    char buf[N];
    constexpr fixed_string(const char (&s)[N]) {
        for (size_t i = 0; i < N; ++i) buf[i] = s[i];
    }
};

template<size_t N, std::array<flag_data, N> Flags>
class console_arguments {
    std::array<bool, N> set_flags{};
    std::array<std::optional<std::string>, N> values{};
    std::vector<std::string> positional;

    static constexpr std::optional<size_t> find_index(char flag_short) {
        for (size_t i = 0; i < N; ++i) {
            if (Flags[i].flag_short != '\0' && Flags[i].flag_short == flag_short) return i;
        }
        return std::nullopt;
    }

    static constexpr std::optional<size_t> find_index_long(std::string_view name) {
        for (size_t i = 0; i < N; ++i) {
            std::string_view flag_long(Flags[i].flag_long);
            if (!flag_long.empty() && flag_long == name) return i;
        }
        return std::nullopt;
    }

public:
    void set(size_t index) {
        set_flags[index] = true;
    }

    void set_value(size_t index, std::string value) {
        set_flags[index] = true;
        values[index] = std::move(value);
    }

    void add_positional(std::string value) {
        positional.push_back(std::move(value));
    }

    template<char Flag>
    constexpr bool is_flag_set() const {
        constexpr auto index = find_index(Flag);
        static_assert(index.has_value(), "Unknown flag");
        return set_flags[*index];
    }

    template<fixed_string FlagName>
    constexpr bool is_flag_set() const {
        constexpr std::string_view name{FlagName.buf, sizeof(FlagName.buf) - 1};
        constexpr auto index = find_index_long(name);
        static_assert(index.has_value(), "Unknown long flag");
        return set_flags[*index];
    }

    template<char Flag>
    auto get_flag_value() const {
        constexpr auto index = find_index(Flag);
        static_assert(index.has_value(), "Unknown flag");
        constexpr auto type = Flags[*index].type;

        if constexpr (type == flag_type::VALUE) {
            return set_flags[*index] ? values[*index] : std::nullopt;
        } else {
            return set_flags[*index];
        }
    }

    template<fixed_string FlagName>
    auto get_flag_value() const {
        constexpr std::string_view name{FlagName.buf, sizeof(FlagName.buf) - 1};
        constexpr auto index = find_index_long(name);
        static_assert(index.has_value(), "Unknown long flag");
        constexpr auto type = Flags[*index].type;

        if constexpr (type == flag_type::VALUE) {
            return set_flags[*index] ? values[*index] : std::nullopt;
        } else {
            return set_flags[*index];
        }
    }

    const std::vector<std::string>& get_positional() const {
        return positional;
    }
};

template<size_t N, std::array<flag_data, N> Flags>
struct parse_result {
    console_arguments<N, Flags> args;
    bool is_ok;
    std::string error_message;

    explicit operator bool() const noexcept { return is_ok; }
};

template<size_t N, std::array<flag_data, N> Flags, size_t MinPosArgs = 0, size_t MaxPosArgs = static_cast<size_t>(-1)>
class flag_parser {
    static constexpr std::optional<size_t> find_short(char f) {
        for(size_t i=0; i<N; ++i) {
            if(Flags[i].flag_short != '\0' && Flags[i].flag_short == f) return i;
        }
        return std::nullopt;
    }
    static constexpr std::optional<size_t> find_long(std::string_view f) {
        for(size_t i=0; i<N; ++i) {
            std::string_view flag_long(Flags[i].flag_long);
            if(!flag_long.empty() && flag_long == f) return i;
        }
        return std::nullopt;
    }
public:
    constexpr flag_parser() = default;

    std::string help_string(std::string_view program_name) const {
        std::ostringstream oss;
        oss << "Использование: " << program_name << " [ОПЦИИ]";
        
        if constexpr (MaxPosArgs == static_cast<size_t>(-1)) {
            if constexpr (MinPosArgs == 0) {
                oss << " [аргументы...]";
            } else {
                for (size_t i = 0; i < MinPosArgs; ++i) oss << " <арг" << (i+1) << ">";
                oss << " [аргументы...]";
            }
        } else {
            for (size_t i = 0; i < MinPosArgs; ++i) oss << " <арг" << (i+1) << ">";
            for (size_t i = MinPosArgs; i < MaxPosArgs; ++i) oss << " [арг" << (i+1) << "]";
        }
        oss << "\n\nОпции:\n";
        for (const auto& flag : Flags) {
            oss << "  ";
            bool has_short = (flag.flag_short != '\0');
            bool has_long = (std::string_view(flag.flag_long) != "");

            if (has_short) {
                oss << "-" << flag.flag_short;
            }
            if (has_short && has_long) {
                oss << ", ";
            }
            if (has_long) {
                oss << "--" << flag.flag_long;
            }

            if (flag.type == flag_type::VALUE) {
                oss << " <значение>";
            }
            oss << "\n      " << flag.msg << "\n";
        }
        return oss.str();
    }

    parse_result<N, Flags> process_args(int argc, char** argv) const noexcept {
        console_arguments<N, Flags> result;
        try {
            for (int i = 1; i < argc; ++i) {
                std::string_view arg = argv[i];
                
                if (arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
                    std::string_view f = arg.substr(2);
                    auto idx = find_long(f);
                    if (!idx) {
                        return {result, false, "Неизвестный флаг: " + std::string(arg)};
                    }
                    
                    if (Flags[*idx].type == flag_type::VALUE) {
                        if (i + 1 < argc && static_cast<std::string_view>(argv[i+1]).find("-") != 0) {
                            result.set_value(*idx, std::string(argv[++i]));
                        } else {
                            return {result, false, "Отсутствует значения флага: " + std::string(arg)};
                        }
                    } else {
                        result.set(*idx);
                    }
                } else if (arg.length() > 1 && arg[0] == '-') {
                    char f = arg[1];
                    auto idx = find_short(f);
                    if (!idx) {
                        return {result, false, "Неизвестный флаг: " + std::string(arg)};
                    }
                    
                    if (Flags[*idx].type == flag_type::VALUE) {
                        if (i + 1 < argc && static_cast<std::string_view>(argv[i+1]).find("-") != 0) {
                            result.set_value(*idx, std::string(argv[++i]));
                        } else {
                            return {result, false, "Отсутствует значение флага: " + std::string(arg)};
                        }
                    } else {
                        result.set(*idx);
                    }
                } else {
                    result.add_positional(std::string(arg));
                }
            }
            
            if (result.get_positional().size() < MinPosArgs) {
                return {result, false, "Недостаточно позиционных аргументов (ожидалось как минимум " + std::to_string(MinPosArgs) + ")"};
            }
            if (result.get_positional().size() > MaxPosArgs) {
                return {result, false, "Слишком много позиционных аргументов (ожидалось не более " + std::to_string(MaxPosArgs) + ")"};
            }
            
            return {result, true, ""};
        } catch (const std::exception& e) {
            return {result, false, "Ошибка при обработке аргументов запуска: " + std::string(e.what())};
        }
    }
};

template<auto Flags, size_t MinPosArgs = 0, size_t MaxPosArgs = static_cast<size_t>(-1)>
constexpr auto make_flag_parser() {
    return flag_parser<Flags.size(), Flags, MinPosArgs, MaxPosArgs>{};
}
