
#ifndef RENDERER_H
#define RENDERER_H

struct RendererState;
struct AppState;

class Renderer {
public:
    Renderer(AppState& app_state);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;

    ~Renderer();

    bool InitCheck();

    bool ShouldQuit();

    bool BeforeFrame();
    void AfterFrame();

private:
    RendererState* state;
};

#endif
