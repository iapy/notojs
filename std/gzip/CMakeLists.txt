if(ZLIB_FOUND)
message(STATUS "libz found: gzip.so will be included in STD")

add_library(gzip SHARED gzip.cpp)
target_link_libraries(gzip ZLIB::ZLIB Boost::iostreams)

set_target_properties(gzip PROPERTIES
    OUTPUT_NAME gzip PREFIX "" SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${NOTOJS_STD_OUTPUT}"
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(gzip PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

add_custom_command(TARGET gzip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f "${PROJECT_BINARY_DIR}/test/gzip.so"
    COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:gzip>" "${PROJECT_BINARY_DIR}/test/gzip.so"
    VERBATIM
)

install(TARGETS gzip DESTINATION lib/notojs/stdlib)
else()
message(STATUS "libz not found: gzip.so will not be included in STD")
endif()
