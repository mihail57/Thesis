
add_library(ui STATIC)

include_directories(${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
include_directories(${dds_image_SOURCE_DIR} ${dds_image_SOURCE_DIR}/include)
# Freetype via FetchContent usually exposes includes via its target, but we can add:
include_directories(${freetype_SOURCE_DIR}/include)

set(LIBRARIES "freetype")


target_include_directories(ui PUBLIC include)

set(sources 
    ui.cpp
    layouts/main_window/window.cpp
)


target_sources(ui PRIVATE ${sources} ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_link_libraries(ui PRIVATE app)
target_compile_definitions(ui PRIVATE IMGUI_ENABLE_FREETYPE)