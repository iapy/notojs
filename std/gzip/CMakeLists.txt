if(ZLIB_FOUND)
message(STATUS "libz found: gzip.so will be included in STD")
notojs_module(gzip LINK ZLIB::ZLIB Boost::iostreams)

if(${NOTOJS_BUILD_TESTS})
add_custom_command(TARGET gzip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/gzip.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:gzip>" "${PROJECT_BINARY_DIR}/test/gzip.so"
    VERBATIM
)
endif()

else()
message(STATUS "libz not found: gzip.so will not be included in STD")
endif()
