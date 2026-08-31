cmake_minimum_required(VERSION 3.12)

add_library(yaml SHARED yaml.cpp)
target_link_libraries(yaml yaml-cpp::yaml-cpp)

set_target_properties(yaml PROPERTIES
    OUTPUT_NAME yaml PREFIX "" SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${NOTOJS_STD_OUTPUT}"
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(yaml PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

add_custom_command(TARGET yaml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/yaml.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:yaml>" "${PROJECT_BINARY_DIR}/test/yaml.so"
    VERBATIM
)

install(TARGETS yaml DESTINATION lib/notojs/stdlib)
