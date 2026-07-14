cmake_minimum_required(VERSION 3.20)
project(notojs-tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NOTOJS_BUILD_DIR "" CACHE STRING "NotoJS build dir")
set(CLANG_RESOURCE_DIR "/opt/homebrew/opt/llvm/lib/clang/22" CACHE STRING "CLang resource dir")
set(XCODE_SDK_PATH "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" CACHE STRING "XCode SDK path")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

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
add_custom_target(generate)

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
            --extra-arg-before=-resource-dir
            --extra-arg-before=${CLANG_RESOURCE_DIR}
            --extra-arg-before=-isysroot
            --extra-arg-before=${XCODE_SDK_PATH}
            -p ${NOTOJS_BUILD_DIR} ${source}
            --output ${target}
        DEPENDS $<TARGET_FILE:notojs-doc-gen> ${source}
        VERBATIM)

    add_custom_target(${source_name}.${source_type} DEPENDS ${target})
    add_dependencies(generate ${source_name}.${source_type})
endmacro()

doc(${NOTOJS_DIR}/lib/notojs/global.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/db.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/doc.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/dom.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/fs.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/zip.cpp)
doc(${NOTOJS_DIR}/lib/notojs/module/gzip.cpp)

doc(${NOTOJS_DIR}/api/global.hpp)
doc(${NOTOJS_DIR}/api/module.hpp)
