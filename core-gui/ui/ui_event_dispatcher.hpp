
#include "layouts/main_window/event_handler.h"

struct UiEventDispatcher {
    MainWindowEventHandler main_window_handler;

    template<typename E>
    void operator()(const E& event) const {        
        main_window_handler(event);
    }
};