
#ifndef UI_H
#define UI_H

#include "ui_init_struct.h"
#include "event_buffer.h"
#include "command_buffer.h"

struct UiState;

class Ui {
public:
    Ui(const UiInitStruct& app_state, CommandBuffer& cmd_buf, EventBuffer& event_buf);

    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;

    Ui(Ui&& other) noexcept;
    Ui& operator=(Ui&& other) noexcept;

    ~Ui();

    void draw_ui();
    void handle_events();

private:
    UiState* state;
    EventBuffer& event_buf;
    CommandBuffer& cmd_buf;
};

#endif