
#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "command.h"

#include <vector>

struct CommandBuffer {
    std::vector<Command> items;

    auto begin() const { return items.begin(); }
    auto end() const   { return items.end(); }

    void push(const Command& cmd) { items.push_back(cmd); }

    void clear() { items.clear(); }
};

#endif