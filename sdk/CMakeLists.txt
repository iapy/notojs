set(SDK_CONFIGURED_HEADERS
    global.hpp
    module.hpp
)

install(CODE "
set(NOTOJS_VERSION [[${CMAKE_PROJECT_VERSION}]])
set(NOTOJS_VERSION_NS [[${NOTOJS_VERSION_NS}]])
set(SDK_CONFIGURED_HEADERS [[${SDK_CONFIGURED_HEADERS}]])

configure_file(
    \"${CMAKE_CURRENT_SOURCE_DIR}/notojs.cmake\"
    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/notojs/devkit/notojs.cmake\"
    @ONLY
)

foreach(header IN LISTS SDK_CONFIGURED_HEADERS)
    configure_file(
        \"${CMAKE_CURRENT_SOURCE_DIR}/\${header}\"
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lib/notojs/devkit/include/\${header}\"
        @ONLY
    )
endforeach()
")

file(GLOB SDK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(TRANSFORM SDK_CONFIGURED_HEADERS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(REMOVE_ITEM SDK_HEADERS ${SDK_CONFIGURED_HEADERS})

install(
    FILES ${SDK_HEADERS}
    DESTINATION lib/notojs/devkit/include
)

install(
    FILES ${PROJECT_SOURCE_DIR}/lib/quickjs/quickjs.h
    DESTINATION lib/notojs/devkit/include/quickjs
)

install(
    FILES ${PROJECT_SOURCE_DIR}/lib/lmdb/libraries/liblmdb/lmdb.h
    DESTINATION lib/notojs/devkit/include
)

install(
    FILES ${PROJECT_SOURCE_DIR}/lib/lmdbxx/lmdb++.h
    DESTINATION lib/notojs/devkit/include/lmdbxx
)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/lib/rapidjson/include/rapidjson
    DESTINATION lib/notojs/devkit/include
)

add_subdirectory(app)
