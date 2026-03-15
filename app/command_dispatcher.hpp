
#include "include/command.h"
#include "include/event_buffer.h"
#include "inference/command_handler.h"

struct CommandDispatcher {
    CommandHandler inf_cmd_handler;    
    EventBuffer& event_buf;

    CommandDispatcher(CommandHandler handler, EventBuffer& event_buf) : inf_cmd_handler(handler), event_buf(event_buf) {};

    template<typename E>
    void operator()(const E& command) const {
        /*
        Реализация паттерна Visitor для передачи команд в обработчики.
        */
        
        inf_cmd_handler(command, event_buf);
    }
};