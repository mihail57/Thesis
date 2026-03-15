
add_library(app SHARED)
if(WIN32)
    set_target_properties(app PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(app PUBLIC include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(sources
    app.cpp
    inference/inference_manager.cpp
    inference/ast.cpp
    inference/error.cpp
    inference/hm_def/inference_context.cpp
    inference/hm_def/type.cpp
    inference/temporary/lambda_parser.cpp
)

target_sources(app PRIVATE ${sources})