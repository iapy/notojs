cmake_minimum_required(VERSION 3.18)
notojs_module(yaml LINK yaml-cpp::yaml-cpp)

if(${NOTOJS_BUILD_TESTS})
add_custom_command(TARGET yaml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/yaml.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:yaml>" "${PROJECT_BINARY_DIR}/test/yaml.so"
    VERBATIM
)
endif()
