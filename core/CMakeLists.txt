
add_executable(core)

target_sources(core PRIVATE main.cpp)

target_link_libraries(core PRIVATE app)