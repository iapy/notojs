cmake_minimum_required(VERSION 3.10)

add_executable(builder ${CMAKE_CURRENT_SOURCE_DIR}/builder.cpp)
target_link_libraries(builder PRIVATE Boost::program_options yaml-cpp::yaml-cpp)
target_compile_features(builder PUBLIC cxx_std_17)

set(DOCS
    ${PROJECT_SOURCE_DIR}/lib/notojs/global.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/module/core.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/module/db.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/module/doc.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/module/dom.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/module/fs.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/script/console.yml
    ${PROJECT_SOURCE_DIR}/lib/notojs/script/dollar.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/global.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/module.yml
    ${CMAKE_CURRENT_SOURCE_DIR}/api.md
    ${CMAKE_CURRENT_SOURCE_DIR}/api-cpp.md
    ${CMAKE_CURRENT_SOURCE_DIR}/api-http.md
    ${CMAKE_CURRENT_SOURCE_DIR}/api-javascript.md
    ${CMAKE_CURRENT_SOURCE_DIR}/applications.md
    ${CMAKE_CURRENT_SOURCE_DIR}/config.md
    ${CMAKE_CURRENT_SOURCE_DIR}/dom.md
    ${CMAKE_CURRENT_SOURCE_DIR}/extensions.md
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.md
    ${CMAKE_CURRENT_SOURCE_DIR}/gzip.md
    ${CMAKE_CURRENT_SOURCE_DIR}/lmdb.md
    ${CMAKE_CURRENT_SOURCE_DIR}/markdown.md
    ${CMAKE_CURRENT_SOURCE_DIR}/notojs.md
    ${CMAKE_CURRENT_SOURCE_DIR}/packages.md
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline.md
    ${CMAKE_CURRENT_SOURCE_DIR}/selectors.md
    ${CMAKE_CURRENT_SOURCE_DIR}/timers.md
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.md
    ${CMAKE_CURRENT_SOURCE_DIR}/zip.md
)

if(libzip_FOUND)
list(APPEND DOCS ${PROJECT_SOURCE_DIR}/lib/notojs/module/zip.yml)
endif()

if(ZLIB_FOUND)
list(APPEND DOCS ${PROJECT_SOURCE_DIR}/lib/notojs/module/gzip.yml)
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libdoc.cpp
    COMMAND $<TARGET_FILE:builder> --output ${CMAKE_CURRENT_BINARY_DIR}/libdoc.cpp ${DOCS}
    DEPENDS builder ${DOCS}
    VERBATIM)

add_library(doc SHARED ${CMAKE_CURRENT_BINARY_DIR}/libdoc.cpp)
