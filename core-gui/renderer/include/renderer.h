
#ifndef RENDERER_H
#define RENDERER_H

#include <string>

struct RendererInternal;
struct UiInitStruct;

class Renderer {
public:
    Renderer(UiInitStruct& app_state);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;

    ~Renderer();

    bool init_check();

    static void show_error(const std::string& message, const std::string& title = "Ошибка");

    bool should_quit();

    bool before_frame();
    void after_frame();

private:
    RendererInternal* state;
};

#endif
