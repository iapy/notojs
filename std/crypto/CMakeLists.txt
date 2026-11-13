notojs_module(crypto LINK OpenSSL::Crypto)

target_include_directories(crypto PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/lmdb/libraries/liblmdb)

if(${NOTOJS_BUILD_TESTS})
add_custom_command(TARGET crypto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/crypto.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:crypto>" "${PROJECT_BINARY_DIR}/test/crypto.so"
    VERBATIM
)
endif()
