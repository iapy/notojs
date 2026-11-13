set(SRC plugin.cpp)

add_library(-cron ${SRC})
target_include_directories(-cron PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(-cron PROPERTIES POSITION_INDEPENDENT_CODE ON)

notojs_plugin(cron LINK -cron)
