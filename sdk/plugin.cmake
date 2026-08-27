cmake_minimum_required(VERSION 3.12)

get_filename_component(NOTOJS_SDK_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
get_filename_component(NOTOJS_SRC_DIR ${NOTOJS_SDK_DIR} DIRECTORY)
add_compile_options(-Wno-c99-designator)

macro(plugin name)
set(options)
set(oneValueArgs NAME)
set(multiValueArgs LINK)

cmake_parse_arguments(PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
if(NOT PARSED_NAME)
    set(PARSED_NAME ${name})
  endif()

add_library(${name} SHARED ${name}.cpp)
target_link_libraries(${name} PRIVATE Boost::boost ${PARSED_LINK})
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/sdk)
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/lib)
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/lib/rapidjson/include)

set_target_properties(${name} PROPERTIES
    OUTPUT_NAME "${PARSED_NAME}" PREFIX "" SUFFIX ".so"
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(${name} PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()
endmacro()

macro(module name)
plugin(${name})
endmacro()
