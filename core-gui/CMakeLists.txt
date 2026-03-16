
add_executable(core-gui)


include(FetchContent)

FetchContent_Declare(
    glfw3
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579 #3.4
)

if(UNIX AND NOT APPLE)
    if(NOT DEFINED GLFW_BUILD_WAYLAND)
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build GLFW Wayland backend" FORCE)
    endif()
    if(NOT DEFINED GLFW_BUILD_X11)
        set(GLFW_BUILD_X11 ON CACHE BOOL "Build GLFW X11 backend" FORCE)
    endif()
endif()

FetchContent_MakeAvailable(glfw3)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG 6d910d5487d11ca567b61c7824b0c78c569d62f0 #1.92.5
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    Freetype
    GIT_REPOSITORY https://github.com/freetype/freetype
    GIT_TAG 526ec5c47b9ebccc4754c85ac0c0cdf7c85a5e9b #2.14.1
)
FetchContent_MakeAvailable(Freetype)

if (WIN32)
    set_property(TARGET core-gui PROPERTY WIN32_EXECUTABLE ON)
endif()

add_subdirectory(renderer)
add_subdirectory(ui)

target_sources(core-gui PRIVATE main.cpp)

target_link_libraries(core-gui PRIVATE ui renderer app)