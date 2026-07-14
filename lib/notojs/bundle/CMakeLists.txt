cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED Boost_VERSION)
set(CMAKE_CXX_STANDARD 17)
set(BUNDLER_STANDALONE ON)
project(bundler LANGUAGES C CXX)

find_package(Boost CONFIG REQUIRED COMPONENTS url iostreams program_options)
if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
    find_package(Boost CONFIG REQUIRED COMPONENTS process)
endif()
find_package(OpenSSL)
endif()

find_program(BASH NAMES bash)
find_program(CURL NAMES curl)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/esbuild
    COMMAND ${BASH} -c "${CURL} -fsSL https://esbuild.github.io/dl/latest \| ${BASH} -"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM)

add_executable(bundler ${CMAKE_CURRENT_SOURCE_DIR}/bundle.cpp)
target_compile_features(bundler PUBLIC cxx_std_17)

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(bundler PRIVATE Boost::process)
endif()
target_link_libraries(bundler PRIVATE Boost::program_options Boost::filesystem Boost::url OpenSSL::SSL)

file(GLOB JS *.js)
file(GLOB ICO *.ico)
file(GLOB CSS *.css)
file(GLOB HTML *.html)
file(GLOB PNG *.png)

if(NOT DEFINED BUNDLER_STANDALONE)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bundle.cpp
    COMMAND $<TARGET_FILE:bundler> ${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini ${JS} ${ICO} ${CSS} ${HTML} ${PNG} ${CMAKE_CURRENT_SOURCE_DIR}/keymap.ini ${CMAKE_CURRENT_SOURCE_DIR}/site.webmanifest
    DEPENDS bundler ${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini ${JS} ${CSS} ${HTML} ${CMAKE_CURRENT_SOURCE_DIR}/keymap.ini ${CMAKE_CURRENT_BINARY_DIR}/esbuild
    VERBATIM)

add_library(bundle ${CMAKE_CURRENT_BINARY_DIR}/bundle.cpp)
add_subdirectory(render)
target_compile_definitions(bundler PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
else()
target_compile_definitions(bundler PRIVATE -DSTANDALONE)
endif()
