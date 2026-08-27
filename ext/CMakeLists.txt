add_subdirectory(inotify)

install(CODE [[
  file(MAKE_DIRECTORY
    "${CMAKE_INSTALL_PREFIX}/lib/notojs/plugin"
  )
]])
