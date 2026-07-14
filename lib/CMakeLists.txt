cmake_minimum_required(VERSION 3.10)

set(LMDB_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/lmdb/libraries/liblmdb/liblmdb.a CACHE PATH "lib/lmdb.a")
set(QUICKJS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/quickjs/libquickjs.a CACHE PATH "lib/quickjs.a")

add_custom_command(OUTPUT ${LMDB_LIBRARY}
    COMMAND make
    ARGS -C ${CMAKE_CURRENT_SOURCE_DIR}/lmdb/libraries/liblmdb liblmdb.a
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lmdb/libraries/liblmdb
    COMMENT "Building LMDB" VERBATIM)

add_custom_target(clean.lmdb
    COMMAND make clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lmdb/libraries/liblmdb
    COMMENT "Cleaning QuickJS" VERBATIM)

add_custom_target(lmdb
    ALL DEPENDS ${LMDB_LIBRARY})

add_custom_command(OUTPUT ${QUICKJS_LIBRARY}
    COMMAND make
    ARGS -C ${CMAKE_CURRENT_SOURCE_DIR}/quickjs libquickjs.a
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/quickjs
    COMMENT "Building QuickJS" VERBATIM)

add_custom_target(clean.quickjs
    COMMAND make clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/quickjs
    COMMENT "Cleaning QuickJS" VERBATIM)

add_custom_target(quickjs
    ALL DEPENDS ${QUICKJS_LIBRARY})

set(PUGIXML_FLAGS -DPUGIXML_HEADER_ONLY CACHE PATH "lib/pugixml-flags")
set(PUGIXML_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src CACHE PATH "lib/pugixml")

set(LEXBOR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(LEXBOR_BUILD_STATIC ON  CACHE BOOL "" FORCE)

add_subdirectory(yaml)
add_subdirectory(lexbor EXCLUDE_FROM_ALL)
add_subdirectory(notojs)
