if(libzip_FOUND)
message(STATUS "libzip found: zip.so will be included in STD")
notojs_module(zip LINK libzip::zip Boost::iostreams)

if(${NOTOJS_BUILD_TESTS})
add_custom_command(TARGET zip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/zip.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:zip>" "${PROJECT_BINARY_DIR}/test/zip.so"
    VERBATIM
)
endif()

else()
message(STATUS "libzip not found: zip.so will not be included in STD")
endif()
