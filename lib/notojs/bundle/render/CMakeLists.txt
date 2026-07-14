cmake_minimum_required(VERSION 3.10)

get_filename_component(BUNDLER_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)

macro(render name)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.js
    DEPENDS
        ${BUNDLER_DIR}/bundler
        ${BUNDLER_DIR}/esbuild
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.js/bundle.ini
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.js/client.js
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.js/server.js
    COMMAND ${BUNDLER_DIR}/bundler
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.js
        --cache ${BUNDLER_DIR}/cache
        --target ${CMAKE_CURRENT_BINARY_DIR}/${name}.js
        --esbuild ${BUNDLER_DIR}/esbuild
    VERBATIM
)
add_custom_target(${name} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name}.js)
endmacro()

render(charts)
render(tables)
