
#ifndef EVENT_H
#define EVENT_H

#include <variant>
#include <string>

struct AlgorithmFinishedEvent {
    bool ok;
};

struct InputChangeErrorEvent {
    std::string msg;
};

using Event = std::variant<AlgorithmFinishedEvent, InputChangeErrorEvent>;

#endif