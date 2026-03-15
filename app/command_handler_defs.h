
#include "include/command.h"
#include "include/event_buffer.h"

#define COMMAND_HANDLER(cmd) void operator()(const cmd& command, EventBuffer& event_buf) const
#define COMMAND_HANDLER_DEF(cmd, handler_class) void handler_class::operator()(const cmd& command, EventBuffer& event_buf) const
#define COMMAND_HANDLER_IGNORE_OTHERS void operator()(auto&&, EventBuffer&) const {}