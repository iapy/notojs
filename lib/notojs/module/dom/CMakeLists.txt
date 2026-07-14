cmake_minimum_required(VERSION 3.10)

set(SRC
    backend.cpp
    css_rule_list.cpp
    css_style_properties.cpp
    css_style_sheet.cpp
    dom_string_map.cpp
    dom_token_list.cpp
    element.cpp
    html_collection.cpp
    html_document.cpp
    html_element.cpp
    xml_document.cpp
    lexbor.cpp)

add_library(dom STATIC ${SRC})
target_link_libraries(dom ${QUICKJS_LIBRARY} lexbor_static)
target_compile_options(dom PRIVATE ${PUGIXML_FLAGS})
target_include_directories(dom PUBLIC ${PUGIXML_HEADERS})

target_compile_options(dom PRIVATE -Wno-deprecated-declarations)
