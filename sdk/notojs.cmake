cmake_minimum_required(VERSION 3.18)
set(NOTOJS_UUID 313960ee-e195-4a1e-a796-e881cd5553c2)

if("@NOTOJS_VERSION@" MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
message(STATUS "NotoJS devkit ${CMAKE_CURRENT_LIST_DIR}")
add_compile_definitions(NOTOJS_VERSION="@NOTOJS_VERSION@")
add_compile_definitions(CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
set(BUNDLER ${CMAKE_CURRENT_LIST_DIR}/bin/bundler)
set(ESBUILD ${CMAKE_CURRENT_LIST_DIR}/bin/esbuild)
set(TESTKIT ${CMAKE_CURRENT_LIST_DIR}/bin/testkit)
else()
set(BUNDLER ${PROJECT_BINARY_DIR}/lib/notojs/bundle/bundler)
set(ESBUILD ${PROJECT_BINARY_DIR}/lib/notojs/bundle/esbuild)
set(TESTKIT ${PROJECT_BINARY_DIR}/lib/notojs/bundle/testkit)
endif()

if((NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    OR CMAKE_BUILD_TYPE STREQUAL "Release")
option(NOTOJS_STRIP_BINARIES "Strip binaries" ON)
else()
option(NOTOJS_STRIP_BINARIES "Strip binaries" OFF)
endif()

if(${NOTOJS_STRIP_BINARIES})
if(APPLE)
    set(NOTOJS_STRIP ${CMAKE_STRIP} -x)
else()
    set(NOTOJS_STRIP ${CMAKE_STRIP} --strip-unneeded)
endif()
message(STATUS "strip ${NOTOJS_STRIP}")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$")
add_compile_options(-Wno-c99-designator)
endif()

macro(_notojs_shared_object name)
set(o_args NOINSTALL)
set(s_args DEST)
set(m_args LINK)

cmake_parse_arguments(ARG "${o_args}" "${s_args}" "${m_args}" ${ARGN})

string(UUID shared_object_uuid
    NAMESPACE "${NOTOJS_UUID}"
    NAME "${ARG_DEST}/${name}"
    TYPE SHA1)

string(REPLACE "-" "_" shared_object_uuid "${shared_object_uuid}")
set(unique_namespace "notojs_${shared_object_uuid}")

add_library("${name}" SHARED "${name}.cpp")
target_link_libraries("${name}" PRIVATE Boost::boost ${ARG_LINK})

set(shared_object_targets "${name}")
foreach(link IN LISTS ARG_LINK)
    if(TARGET "${link}")
        get_target_property(link_target "${link}" ALIASED_TARGET)
        if(link_target)
            list(APPEND shared_object_targets "${link_target}")
        else()
            list(APPEND shared_object_targets "${link}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES shared_object_targets)

foreach(target IN LISTS shared_object_targets)
    get_target_property(target_imported "${target}" IMPORTED)
    if(target_imported)
        continue()
    endif()

    get_target_property(target_type "${target}" TYPE)
    if(NOT target_type MATCHES
        "^(STATIC|SHARED|MODULE|OBJECT)_LIBRARY$|^EXECUTABLE$")
        continue()
    endif()

    get_target_property(target_source_dir "${target}" SOURCE_DIR)
    get_directory_property(target_project_source_dir
        DIRECTORY "${target_source_dir}" DEFINITION PROJECT_SOURCE_DIR)
    if(NOT "${target_project_source_dir}" STREQUAL "${PROJECT_SOURCE_DIR}")
        continue()
    endif()

    get_property(namespace_set TARGET "${target}"
        PROPERTY NOTOJS_UNIQUE_NAMESPACE SET)
    if(namespace_set)
        get_target_property(target_namespace "${target}"
            NOTOJS_UNIQUE_NAMESPACE)
        if(NOT "${target_namespace}" STREQUAL "${unique_namespace}")
            message(FATAL_ERROR
                "Target ${target} "
                "is assigned to multiple NotoJS namespaces: "
                "${target_namespace} and ${unique_namespace}")
        endif()
    else()
        set_property(TARGET "${target}" PROPERTY
            NOTOJS_UNIQUE_NAMESPACE "${unique_namespace}")
        target_compile_definitions("${target}" PRIVATE
            NOTOJS_UNIQUE_NAMESPACE=${unique_namespace})
    endif()
endforeach()
set_target_properties("${name}" PROPERTIES
    OUTPUT_NAME "${PARSED_NAME}" PREFIX "" SUFFIX ".so"
    POSITION_INDEPENDENT_CODE ON)
if(APPLE)
    set_target_properties("${name}" PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup")
endif()
if(NOT ARG_NOINSTALL)
    install(TARGETS "${name}" DESTINATION "${ARG_DEST}")
endif()
if(${NOTOJS_STRIP_BINARIES})
    add_custom_command(TARGET "${name}" POST_BUILD
        COMMAND ${NOTOJS_STRIP} "$<TARGET_FILE:${name}>")
endif()
endmacro()

macro(notojs_plugin name)
_notojs_shared_object(${name} ${ARGN} DEST lib/notojs/plugin)
endmacro()

macro(notojs_module name)
_notojs_shared_object(${name} ${ARGN} DEST lib/notojs/stdlib)
endmacro()

macro(notojs_script name)
get_filename_component(e ${name} LAST_EXT)
get_filename_component(n ${name} NAME_WE)

if(NOT ${e} STREQUAL ".js")
    message(FATAL_ERROR
        "notojs_script(${name}): should end with .js")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini")
    message(FATAL_ERROR
        "notojs_script(${name}): missing bundle.ini")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    message(FATAL_ERROR
        "notojs_script(${name}): missing script")
endif()
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    message(FATAL_ERROR
        "notojs_script(${name}): must be a file")
endif()

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/${name}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/bundle.ini
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}
    COMMAND ${BUNDLER}
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}
        --cache ${CMAKE_CURRENT_BINARY_DIR}/cache
        --target ${CMAKE_CURRENT_BINARY_DIR}
        --esbuild ${ESBUILD}
    VERBATIM
)
add_custom_target(${n} ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/${name})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}
    DESTINATION lib/notojs/script)
endmacro()

macro(notojs_render name)
get_filename_component(e ${name} LAST_EXT)
get_filename_component(n ${name} NAME_WE)

if(NOT ${e} STREQUAL ".js")
    message(FATAL_ERROR
        "notojs_render(${name}): should end with .js")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}/bundle.ini")
    message(FATAL_ERROR
        "notojs_render(${name}): missing bundle.ini")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}/client.js")
    message(FATAL_ERROR
        "notojs_render(${name}): missing client.js")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${name}/server.js")
    message(FATAL_ERROR
        "notojs_render(${name}): missing server.js")
endif()

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/${name}/client.js
        ${CMAKE_CURRENT_BINARY_DIR}/${name}/server.js
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}/bundle.ini
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}/client.js
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}/server.js
    COMMAND ${BUNDLER}
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}
        --cache ${CMAKE_CURRENT_BINARY_DIR}/cache
        --target ${CMAKE_CURRENT_BINARY_DIR}/${name}
        --esbuild ${ESBUILD}
    VERBATIM
)
add_custom_target(${n} ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/${name}/client.js
    ${CMAKE_CURRENT_BINARY_DIR}/${name}/server.js)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${name}
    DESTINATION lib/notojs/script)
endmacro()

macro(notojs_test name)
set(s_args TESTDIR NAME)
set(m_args OPTIONS)

cmake_parse_arguments(ARG "" "${s_args}" "${m_args}" ${ARGN})
list(TRANSFORM ARG_OPTIONS PREPEND "-D")
add_test(NAME ${name} COMMAND ${TESTKIT}
    $<TARGET_FILE:${name}>
    ${ARG_TESTDIR}
    ${ARG_OPTIONS}
)
if(ARG_NAME)
    add_custom_target(${ARG_NAME} COMMAND ${TESTKIT}
       $<TARGET_FILE:${name}>
       ${ARG_TESTDIR}
       ${ARG_OPTIONS}
       DEPENDS ${name}
       VERBATIM)
endif()
endmacro()

macro(notojs_package)
set(o_args MAINTAINER)
cmake_parse_arguments(ARG "" "${o_args}" "" ${ARGN})

if(NOT ARG_MAINTAINER)
    message(FATAL_ERROR "Maintainer should be specified")
endif()

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${ARG_MAINTAINER}")

if("@NOTOJS_VERSION@" MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
foreach(dependency_var IN ITEMS
    CPACK_DEBIAN_PACKAGE_DEPENDS
    CPACK_RPM_PACKAGE_REQUIRES)
    string(REGEX REPLACE "^[ \\t,;]+" ""
        dependency_value "${${dependency_var}}")
    string(REGEX REPLACE "[ \\t,;]+$" ""
        dependency_value "${dependency_value}")

    if(dependency_value)
        set(dependency_value "${dependency_value}, ")
    endif()

    if(dependency_var STREQUAL "CPACK_DEBIAN_PACKAGE_DEPENDS")
        string(APPEND dependency_value "notojs (= @NOTOJS_VERSION@)")
    else()
        string(APPEND dependency_value "notojs = @NOTOJS_VERSION@")
    endif()
    set(${dependency_var} "${dependency_value}")
endforeach()
endif()
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
include(CPack)
endmacro()
