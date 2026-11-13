include(ExternalProject)

ExternalProject_Add(sdk.bundler
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/lib/notojs/bundle"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/bundler"
    CMAKE_CACHE_ARGS
        "-DNOTOJS_STRIP:STRING=${NOTOJS_STRIP}"
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DNOTOJS_STRIP_BINARIES:BOOL=${NOTOJS_STRIP_BINARIES}"
    INSTALL_COMMAND ""
)

install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bundler/bundler
    DESTINATION lib/notojs/devkit/bin)

install(
    PROGRAMS ${PROJECT_BINARY_DIR}/lib/notojs/bundle/esbuild
    DESTINATION lib/notojs/devkit/bin)

find_package(Boost CONFIG REQUIRED COMPONENTS
    unit_test_framework)

add_executable(testkit testkit.cpp)
target_link_libraries(testkit PRIVATE
    -notojs module script
    Boost::unit_test_framework)

target_compile_definitions(testkit PRIVATE BOOST_TEST_ALTERNATIVE_INIT_API)
target_link_options(testkit PRIVATE -rdynamic)

install(TARGETS testkit DESTINATION lib/notojs/devkit/bin)
