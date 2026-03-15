
#ifndef RENDERER_H
#define RENDERER_H

struct RendererState;

class Renderer {
public:
    Renderer();

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
