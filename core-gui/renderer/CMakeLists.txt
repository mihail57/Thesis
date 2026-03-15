
add_library(renderer STATIC)

include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
include_directories(${freetype_SOURCE_DIR}/include)

include_directories(${APP_STATE_DIR}/include)

find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan;freetype")


target_include_directories(renderer PUBLIC include)

set(sources 
    renderer.cpp
)


target_sources(renderer PRIVATE ${sources} ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_link_libraries(renderer ${LIBRARIES})
target_compile_definitions(renderer PRIVATE IMGUI_ENABLE_FREETYPE)