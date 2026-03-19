
#ifndef VALUE_OR_ERROR_HPP
#define VALUE_OR_ERROR_HPP

#include "source_loc.h"

#include <string>
#include <variant>

struct Error {
    std::string text;
    SourceLoc at;
};

template<typename T>
struct ValueOrError {
    ValueOrError() {}
    ValueOrError(const T& value) : _union(value) {}
    ValueOrError(const Error& error) : _union(error) {}

    bool is_error() const { return std::holds_alternative<Error>(_union); }
    Error get_error() const { return std::get<Error>(_union); }
    T unwrap() const { return std::get<T>(_union); }

private:
    std::variant<T, Error> _union;
};

#endif