
add_library(ui STATIC)

include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
include_directories(${dds_image_SOURCE_DIR} ${dds_image_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/misc)


target_include_directories(ui PUBLIC include)

set(sources 
    ui.cpp
    layouts/main_window/window.cpp
    layouts/expression_visualizer/window.cpp
    layouts/step_visualizer/window.cpp
)


target_sources(ui PRIVATE ${sources})
target_link_libraries(ui PRIVATE app)