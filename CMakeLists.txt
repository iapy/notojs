cmake_minimum_required(VERSION 3.12)
project(notojs LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wno-c99-designator)
option(ENABLE_DEBUG_LOGGING "Enable debug logging" OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS filesystem iostreams program_options url)
find_package(OpenSSL)
find_package(libzip QUIET)
find_package(ZLIB QUIET)

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
    find_package(Boost CONFIG REQUIRED COMPONENTS process)
endif()

include_directories(${CMAKE_SOURCE_DIR}/api ${CMAKE_SOURCE_DIR}/lib ${Boost_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "Release flags" FORCE)

add_subdirectory(app)
add_subdirectory(ext)
add_subdirectory(lib)

add_subdirectory(test)

install(PROGRAMS ${PROJECT_BINARY_DIR}/app/notojs DESTINATION bin)
install(FILES ${PROJECT_SOURCE_DIR}/app/notojs.ini DESTINATION etc)
