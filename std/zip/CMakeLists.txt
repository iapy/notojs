if(libzip_FOUND)
message(STATUS "libzip found: zip.so will be included in STD")

add_library(zip SHARED zip.cpp)
target_link_libraries(zip libzip::zip Boost::iostreams)

set_target_properties(zip PROPERTIES
    OUTPUT_NAME zip PREFIX "" SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${NOTOJS_STD_OUTPUT}"
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(zip PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

add_custom_command(TARGET zip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/zip.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:zip>" "${PROJECT_BINARY_DIR}/test/zip.so"
    VERBATIM
)

install(TARGETS zip DESTINATION lib/notojs/stdlib)
else()
message(STATUS "libzip not found: zip.so will not be included in STD")
endif()
