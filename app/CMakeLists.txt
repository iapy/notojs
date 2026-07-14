cmake_minimum_required(VERSION 3.12)

add_executable(notojs notojs.cpp)
target_compile_features(notojs PUBLIC cxx_std_17)

add_dependencies(notojs quickjs)
target_link_options(notojs PRIVATE -rdynamic)
target_link_libraries(notojs PUBLIC -notojs module script)

add_custom_command(TARGET notojs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/notojs.ini
        $<TARGET_FILE_DIR:notojs>/notojs.ini
    VERBATIM)

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(notojs PRIVATE Boost::process)
endif()
