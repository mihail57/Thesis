
add_executable(inference)

include_directories(${INFRASTRUCTURE_DIR})

target_sources(inference PRIVATE main.cpp)

target_link_libraries(inference PRIVATE app)

set_target_properties(inference PROPERTIES
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
)