cmake_minimum_required(VERSION 3.10)

file(GLOB SRC 
    console.cpp
    dollar.cpp)

add_library(script STATIC ${SRC})
target_link_libraries(script ${QUICKJS_LIBRARY})
target_compile_options(script PRIVATE ${PUGIXML_FLAGS} -DNOTOJS_INTERNAL_MODULE)
target_include_directories(script PRIVATE ${PUGIXML_HEADERS})
target_include_directories(script PRIVATE ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)
