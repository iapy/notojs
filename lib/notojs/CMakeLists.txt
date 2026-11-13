add_subdirectory(bundle)
add_subdirectory(module)
add_subdirectory(script)

file(GLOB SRC *.cpp parser/*.cpp)
add_library(-notojs STATIC ${SRC})
set_source_files_properties(folder.cpp PROPERTIES COMPILE_DEFINITIONS BOUNDARY="--notojs-${NOTOJS_UUID}")

target_link_libraries(-notojs ${QUICKJS_LIBRARY} ${LMDB_LIBRARY} Boost::url OpenSSL::SSL bundle)
if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(-notojs Boost::process)
endif()
target_compile_options(-notojs PRIVATE -DNOTOJS_INTERNAL_MODULE)

if(DEBUG_LOGGING)
target_compile_options(-notojs PRIVATE -DDEBUG_LOGGING)
endif()

target_include_directories(-notojs PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/lmdb/libraries/liblmdb
    ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)
