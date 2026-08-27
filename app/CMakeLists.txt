cmake_minimum_required(VERSION 3.12)

add_executable(notojs notojs.cpp)
target_compile_features(notojs PUBLIC cxx_std_17)

add_dependencies(notojs quickjs)
target_link_options(notojs PRIVATE -rdynamic)
target_link_libraries(notojs PUBLIC -notojs module script)

if(${STRIP_NOTOJS_BINARY})
if(APPLE)
  set(STRIP_CMD ${CMAKE_STRIP} -x)
  message(STATUS "${CMAKE_STRIP} -x app/notojs")
else()
  set(STRIP_CMD ${CMAKE_STRIP} --strip-unneeded)
  message(STATUS "${CMAKE_STRIP} --strip-unneeded app/notojs")
endif()
add_custom_command(TARGET notojs POST_BUILD
    COMMAND ${STRIP_CMD} $<TARGET_FILE:notojs>)
endif()

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(notojs PRIVATE Boost::process)
endif()

install(TARGETS notojs DESTINATION bin)

install(CODE "
    file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/etc\")
    configure_file(
        \"${CMAKE_CURRENT_SOURCE_DIR}/notojs.ini.in\"
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/etc/notojs.ini\"
        @ONLY
    )
")
