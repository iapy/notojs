cmake_minimum_required(VERSION 3.12)
include(${CMAKE_SOURCE_DIR}/sdk/plugin.cmake)
plugin(inotify)

if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORESERVICES_FRAMEWORK CoreServices REQUIRED)
    target_link_libraries(inotify PRIVATE
        "${COREFOUNDATION_FRAMEWORK}"
        "${CORESERVICES_FRAMEWORK}"
    )
endif()
