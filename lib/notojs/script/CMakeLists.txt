cmake_minimum_required(VERSION 3.12)

file(GLOB SRC
    console.cpp
    crypto.cpp
    dollar.cpp
    dom.cpp
    storage.cpp)

add_library(script STATIC ${SRC})
target_link_libraries(script ${QUICKJS_LIBRARY})
target_compile_options(script PRIVATE ${PUGIXML_FLAGS})
target_include_directories(script PRIVATE ${PUGIXML_HEADERS})
target_include_directories(script PRIVATE ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)
