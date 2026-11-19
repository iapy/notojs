cmake_minimum_required(VERSION 3.18)
project(notojs VERSION 0.5.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NOTOJS_BUILD_TESTS "Build tests" OFF)
option(NOTOJS_DEBUG_LOGGING "Debug logging" OFF)
option(NOTOJS_INSTALL_DEVKIT "Install devkit" ON)

find_package(Boost CONFIG REQUIRED COMPONENTS
    filesystem iostreams program_options url)
find_package(OpenSSL)
find_package(libzip QUIET)
find_package(ZLIB QUIET)

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
    find_package(Boost CONFIG REQUIRED COMPONENTS process)
endif()

include(${PROJECT_SOURCE_DIR}/sdk/notojs.cmake)
include_directories(
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/lib
    ${Boost_INCLUDE_DIRS})

string(REPLACE "." "_" NOTOJS_VERSION_NS "${CMAKE_PROJECT_VERSION}")
string(PREPEND NOTOJS_VERSION_NS version_)

add_compile_definitions(NOTOJS_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(NOTOJS_VERSION_NS=${NOTOJS_VERSION_NS})

set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "Release flags" FORCE)

add_subdirectory(app)
add_subdirectory(lib)
add_subdirectory(std)

if(${NOTOJS_INSTALL_DEVKIT})
add_subdirectory(sdk)
endif()

if(${NOTOJS_BUILD_TESTS})
enable_testing()
add_subdirectory(test)
endif()

notojs_package(MAINTAINER "Alex Polyakov")
