cmake_minimum_required(VERSION 3.12)

set(SRC
    assert.cpp
    core.cpp
    db.cpp
    doc.cpp
    dom.cpp
    fs.cpp
    noto.cpp)

add_library(module STATIC ${SRC})
target_link_libraries(module ${QUICKJS_LIBRARY} yaml-cpp::yaml-cpp dom)
target_compile_options(module PRIVATE ${PUGIXML_FLAGS} -DNOTOJS_INTERNAL_MODULE)
target_include_directories(module PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/lmdb/libraries/liblmdb
    ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)

add_subdirectory(doc)
add_subdirectory(dom)
