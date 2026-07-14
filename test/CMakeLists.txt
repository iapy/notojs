cmake_minimum_required(VERSION 3.12)
find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework)

file(GLOB SRC
    jstest.cpp
    test_application.cpp
    test_assert.cpp
    test_async.cpp
    test_client.cpp
    test_compile.cpp
    test_custom.cpp
    test_db.cpp
    test_doc.cpp
    test_dollar.cpp
    test_html.cpp
    test_html_ext.cpp
    test_engine.cpp
    test_errors.cpp
    test_export.cpp
    test_extend.cpp
    test_fs.cpp
    test_getter.cpp
    test_global.cpp
    test_handle.cpp
    test_httpdata.cpp
    test_import.cpp
    test_interface.cpp
    test_intern.cpp
    test_jsdict.cpp
    test_layout.cpp
    test_method.cpp
    test_notojs.cpp
    test_pipeline.cpp
    test_router.cpp
    test_search.cpp
    test_server.cpp
    test_setter.cpp
    test_simple.cpp
    test_struct.cpp
    test_storage.cpp
    test_template.cpp
    test_validate.cpp
    test_vararg.cpp
    test_xml.cpp
    test_xpath.cpp
)

if(libzip_FOUND)
list(APPEND SRC test_zip.cpp)
endif()

if(ZLIB_FOUND)
list(APPEND SRC test_gzip.cpp)
endif()

add_executable(jstest ${SRC})
target_compile_features(jstest PUBLIC cxx_std_17)
target_compile_definitions(jstest PRIVATE -DNOTOJS_TEST)
target_compile_definitions(jstest PRIVATE -DTEST_SERVER_PORT=58342)
target_compile_definitions(jstest PRIVATE -DCMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(jstest PRIVATE -DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_dependencies(jstest quickjs)
target_link_options(jstest PRIVATE -rdynamic)
target_link_libraries(jstest PUBLIC -notojs module script Boost::unit_test_framework)

if(${Boost_VERSION} VERSION_GREATER_EQUAL "1.84")
target_link_libraries(jstest PRIVATE Boost::process)
endif()

add_test(NAME jstest COMMAND $<TARGET_FILE:jstest>)

macro(test_library name)

add_library(${name} SHARED ${name}.cpp)
target_link_libraries(${name} Boost::boost)

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

test_library(async)
test_library(core)
test_library(custom)
test_library(errors)
test_library(extend)
test_library(fs)
test_library(getter)
test_library(global)
test_library(interface)
test_library(intern)
test_library(jsdict)
test_library(method)
test_library(setter)
test_library(simple)
test_library(struct)
test_library(validate)
test_library(vararg)

enable_testing()
