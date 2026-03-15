
#include "layouts/main_window/event_handler.h"

//Helper struct for std::visit, calls all event handlers in hierarchical order.
struct UiEventDispatcher {
    MainWindowEventHandler main_window_handler;

    template<typename E>
    void operator()(const E& event) const {
        /*
        All event handlers return boolean value, showing whether event was consumed or not.
        If it was, nothing to be done next.
        If it wasn't, we should try next one.
        */
        
        main_window_handler(event);
    }
};