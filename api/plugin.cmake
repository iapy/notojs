cmake_minimum_required(VERSION 3.10)

get_filename_component(NOTOJS_API_DIR ${CMAKE_CURRENT_LIST_DIR} REALPATH)
get_filename_component(NOTOJS_SRC_DIR ${NOTOJS_API_DIR} DIRECTORY)
add_compile_options(-Wno-c99-designator)

macro(plugin name)

add_library(${name} SHARED ${name}.cpp)
target_link_libraries(${name} PRIVATE Boost::boost)
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/api)
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/lib)
target_include_directories(${name} PRIVATE ${NOTOJS_SRC_DIR}/lib/rapidjson/include)

set_target_properties(${name} PROPERTIES
    OUTPUT_NAME "${name}" PREFIX "" SUFFIX ".so"
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
