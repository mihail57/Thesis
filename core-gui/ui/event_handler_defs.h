
#include "event.h"

#define EVENT_HANDLER(e) void operator()(const e& event) const
#define EVENT_HANDLER_DEF(e, handler_class) void handler_class::operator()(const e& event) const
#define EVENT_HANDLER_IGNORE_OTHERS void operator()(auto&&) const {}