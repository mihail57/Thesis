
#ifndef EVENT_BUFFER_H
#define EVENT_BUFFER_H

#include "event.h"

#include <vector>

struct EventBuffer {
    std::vector<Event> items;

    auto begin() const { return items.begin(); }
    auto end() const   { return items.end(); }

    void push(const Event& event) { items.push_back(event); }

    void clear() { items.clear(); }
};

#endif