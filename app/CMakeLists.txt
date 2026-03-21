
add_library(app STATIC)
if(WIN32)
    set_target_properties(app PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(app PUBLIC include)

include_directories(${INFRASTRUCTURE_DIR})

set(sources
    app.cpp
    inference/inference_manager.cpp
    inference/ast.cpp
    inference/hm_def/inference_context.cpp
    inference/hm_def/type.cpp
    inference/hm_def/substitution.cpp
    inference/hm_def/type_scheme.cpp
    inference/lambda_parser/lambda_parser.cpp
    inference/lambda_parser/token_buffer.cpp
)

target_sources(app PRIVATE ${sources})