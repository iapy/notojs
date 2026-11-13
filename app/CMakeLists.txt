add_executable(notojs notojs.cpp)
target_compile_features(notojs PUBLIC cxx_std_17)

add_dependencies(notojs quickjs)
target_link_options(notojs PRIVATE -rdynamic)
target_link_libraries(notojs PUBLIC -notojs module script)

if(${NOTOJS_STRIP_BINARIES})
add_custom_command(TARGET notojs POST_BUILD
    COMMAND ${NOTOJS_STRIP} $<TARGET_FILE:notojs>)
endif()

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(notojs PRIVATE Boost::process)
endif()

install(TARGETS notojs DESTINATION bin)

install(CODE "
    set(NOTOJS_CONFIG \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/etc/notojs.ini\")

    if(NOT EXISTS \"\${NOTOJS_CONFIG}\")
        file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/etc\")
        configure_file(
            \"${CMAKE_CURRENT_SOURCE_DIR}/notojs.ini.in\"
            \"\${NOTOJS_CONFIG}\"
            @ONLY
        )
    endif()
")
