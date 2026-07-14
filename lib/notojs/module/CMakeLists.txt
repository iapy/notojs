cmake_minimum_required(VERSION 3.10)

set(SRC
    assert.cpp
    core.cpp
    db.cpp
    doc.cpp
    dom.cpp
    fs.cpp)

if(libzip_FOUND)
list(APPEND SRC zip.cpp)
message(STATUS "libzip found: noto:zip will be included")
else()
message(STATUS "libzip not found: noto:zip will not be included")
endif()

if(ZLIB_FOUND)
list(APPEND SRC gzip.cpp)
message(STATUS "libz found: noto:gzip will be included")
else()
message(STATUS "libz not found: noto:gzip will not be included")
endif()

add_library(module STATIC ${SRC})
target_link_libraries(module ${QUICKJS_LIBRARY} yaml-cpp::yaml-cpp dom)
target_compile_options(module PRIVATE ${PUGIXML_FLAGS} -DNOTOJS_INTERNAL_MODULE)
target_include_directories(module PRIVATE 
    ${CMAKE_SOURCE_DIR}/lib/lmdb/libraries/liblmdb
    ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)

if(ZLIB_FOUND)
target_link_libraries(module ZLIB::ZLIB)
endif()
if(libzip_FOUND)
target_link_libraries(module libzip::zip Boost::iostreams)
endif()

add_subdirectory(doc)
add_subdirectory(dom)
