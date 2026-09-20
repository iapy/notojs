cmake_minimum_required(VERSION 3.20)
project(notojs-tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NOTOJS_BUILD_DIR "" CACHE STRING "NotoJS build dir")
# Prefer normal discovery, including explicit LLVM_DIR/CMAKE_PREFIX_PATH.
find_package(LLVM QUIET CONFIG)

if(NOT LLVM_FOUND AND APPLE)
    find_program(HOMEBREW_EXECUTABLE NAMES brew REQUIRED)
    execute_process(
        COMMAND "${HOMEBREW_EXECUTABLE}" --prefix llvm
        OUTPUT_VARIABLE HOMEBREW_LLVM_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_LLVM_PREFIX}")
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG
    HINTS "${LLVM_INSTALL_PREFIX}/lib/cmake/clang"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

set(CLANG_RESOURCE_DIR "" CACHE PATH "Clang resource directory")

if(NOT CLANG_RESOURCE_DIR)
    # Match the resource headers to the LLVM installation used for linking.
    find_program(NOTOJS_CLANG_EXECUTABLE
        NAMES clang
        PATHS "${LLVM_TOOLS_BINARY_DIR}"
        NO_DEFAULT_PATH
        REQUIRED
    )
    execute_process(
        COMMAND "${NOTOJS_CLANG_EXECUTABLE}" -print-resource-dir
        OUTPUT_VARIABLE CLANG_RESOURCE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(NOTOJS_DOC_CLANG_ARGS
    "--extra-arg-before=-resource-dir=${CLANG_RESOURCE_DIR}"
)

if(APPLE)
    set(XCODE_SDK_PATH "" CACHE PATH "macOS SDK path")

    if(NOT XCODE_SDK_PATH)
        if(CMAKE_OSX_SYSROOT AND IS_DIRECTORY "${CMAKE_OSX_SYSROOT}")
            set(XCODE_SDK_PATH "${CMAKE_OSX_SYSROOT}")
        else()
            find_program(XCRUN_EXECUTABLE NAMES xcrun REQUIRED)
            execute_process(
                COMMAND "${XCRUN_EXECUTABLE}" --sdk macosx --show-sdk-path
                OUTPUT_VARIABLE XCODE_SDK_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                COMMAND_ERROR_IS_FATAL ANY
            )
        endif()
    endif()

    list(APPEND NOTOJS_DOC_CLANG_ARGS
        "--extra-arg-before=-isysroot"
        "--extra-arg-before=${XCODE_SDK_PATH}"
    )
endif()

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

get_filename_component(NOTOJS_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
add_subdirectory(
    ${NOTOJS_DIR}/lib/yaml
    ${CMAKE_CURRENT_BINARY_DIR}/yaml)

add_library(notojs notojs.cpp)
target_include_directories(notojs PRIVATE
  ${NOTOJS_DIR}/lib/yaml/include
  ${CLANG_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
)

add_executable(notojs-release notojs-release.cpp)
target_link_libraries(notojs-release PRIVATE PkgConfig::LIBGIT2)

add_executable(notojs-doc-gen notojs-doc-gen.cpp)
target_include_directories(notojs-doc-gen PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
)

target_compile_definitions(notojs-doc-gen PRIVATE
  ${LLVM_DEFINITIONS}
)

target_link_libraries(notojs-doc-gen PRIVATE clang-cpp LLVM notojs yaml-cpp::yaml-cpp)
add_custom_target(doc-gen)

macro(doc source)
    get_filename_component(source_name ${source} NAME_WE)
    get_filename_component(source_type ${source} EXT)

    if("${source_type}" STREQUAL ".hpp")
        set(target "${NOTOJS_DIR}/lib/notojs/module/doc/${source_name}.yml")
    else()
        get_filename_component(source_path ${source} DIRECTORY)
        set(target "${source_path}/${source_name}.yml")
    endif()

    add_custom_command(
        OUTPUT ${target}
        COMMAND $<TARGET_FILE:notojs-doc-gen>
            ${NOTOJS_DOC_CLANG_ARGS}
            -p ${NOTOJS_BUILD_DIR} ${source}
            --output ${target}
        DEPENDS $<TARGET_FILE:notojs-doc-gen> ${source}
        VERBATIM)

    add_custom_target(${source_name}.${source_type} DEPENDS ${target})
    add_dependencies(doc-gen ${source_name}.${source_type})
endmacro()

doc(${NOTOJS_DIR}/lib/notojs/global.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/assert.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/db.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/doc.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/dom.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/fs.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/noto.cpp)

doc(${NOTOJS_DIR}/std/crypto/crypto.cpp)
doc(${NOTOJS_DIR}/std/gzip/gzip.cpp)
doc(${NOTOJS_DIR}/std/yaml/yaml.cpp)
doc(${NOTOJS_DIR}/std/zip/zip.cpp)

doc(${NOTOJS_DIR}/sdk/global.hpp)
doc(${NOTOJS_DIR}/sdk/module.hpp)
