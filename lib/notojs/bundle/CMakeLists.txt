cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
set(CMAKE_CXX_STANDARD 17)
set(BUNDLER_STANDALONE ON)
project(bundler LANGUAGES C CXX)

find_package(Boost CONFIG REQUIRED COMPONENTS filesystem iostreams program_options url)
if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
    find_package(Boost CONFIG REQUIRED COMPONENTS process)
endif()
find_package(OpenSSL)
else()

find_program(BASH NAMES bash)
find_program(CURL NAMES curl)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/esbuild
    COMMAND ${BASH} -c
        "${CURL} -fsSL https://esbuild.github.io/dl/latest \| ${BASH} -"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM)
endif()

add_executable(bundler ${CMAKE_CURRENT_SOURCE_DIR}/bundle.cpp)
target_compile_features(bundler PUBLIC cxx_std_17)
if(${NOTOJS_STRIP_BINARIES})
    add_custom_command(TARGET bundler POST_BUILD
        COMMAND ${NOTOJS_STRIP} $<TARGET_FILE:bundler>)
endif()

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(bundler PRIVATE
    Boost::process)
endif()
target_link_libraries(bundler PRIVATE
    Boost::program_options Boost::filesystem Boost::url OpenSSL::SSL)

if(NOT DEFINED BUNDLER_STANDALONE)
file(GLOB JS *.js)
file(GLOB ICO *.ico)
file(GLOB CSS *.css)
file(GLOB HTML *.html)
file(GLOB PNG *.png)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bundle.cpp
    COMMAND $<TARGET_FILE:bundler> ${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini
        ${JS}
        ${ICO}
        ${CSS}
        ${HTML}
        ${PNG}
        ${CMAKE_CURRENT_SOURCE_DIR}/keymap.ini
        ${CMAKE_CURRENT_SOURCE_DIR}/site.webmanifest
    DEPENDS
        bundler
        ${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini
        ${JS}
        ${CSS}
        ${HTML}
        ${CMAKE_CURRENT_SOURCE_DIR}/keymap.ini
        ${CMAKE_CURRENT_BINARY_DIR}/esbuild
    VERBATIM)

add_library(bundle ${CMAKE_CURRENT_BINARY_DIR}/bundle.cpp)
add_subdirectory(render)
target_compile_definitions(bundler PRIVATE
    -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
else()
target_compile_definitions(bundler PRIVATE
    -DSTANDALONE)
endif()
