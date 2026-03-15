
#include "../../event_handler_defs.h"
#include "state.h"

struct MainWindowEventHandler {
    MainWindowState& window;

    MainWindowEventHandler(MainWindowState& window);

    EVENT_HANDLER(AlgorithmFinishedEvent);
    EVENT_HANDLER_IGNORE_OTHERS;
};