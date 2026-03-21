
add_executable(core)

include_directories(${INFRASTRUCTURE_DIR})

target_sources(core PRIVATE main.cpp)

target_link_libraries(core PRIVATE app)