cmake_minimum_required(VERSION 3.10)

add_subdirectory(bundle)
add_subdirectory(module)
add_subdirectory(script)

file(GLOB SRC *.cpp)
add_library(-notojs STATIC ${SRC})

target_link_libraries(-notojs ${QUICKJS_LIBRARY} ${LMDB_LIBRARY} Boost::url OpenSSL::SSL bundle)
target_compile_options(-notojs PRIVATE -DNOTOJS_INTERNAL_MODULE)

if(libzip_FOUND)
target_compile_options(-notojs PRIVATE -DWITH_LIBZIP)
endif()

if(ZLIB_FOUND)
target_compile_options(-notojs PRIVATE -DWITH_ZLIB)
endif()

if(DEBUG_LOGGING)
target_compile_options(-notojs PRIVATE -DDEBUG_LOGGING)
endif()

target_include_directories(-notojs PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/lmdb/libraries/liblmdb
    ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)
