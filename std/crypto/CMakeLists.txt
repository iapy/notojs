cmake_minimum_required(VERSION 3.12)

add_library(crypto SHARED crypto.cpp)
target_link_libraries(crypto OpenSSL::Crypto)

set_target_properties(crypto PROPERTIES
    OUTPUT_NAME crypto PREFIX "" SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${NOTOJS_STD_OUTPUT}"
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(crypto PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/lmdb/libraries/liblmdb)

if(APPLE)
    set_target_properties(crypto PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

add_custom_command(TARGET crypto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/crypto.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:crypto>" "${PROJECT_BINARY_DIR}/test/crypto.so"
    VERBATIM
)

install(TARGETS crypto DESTINATION lib/notojs/stdlib)
