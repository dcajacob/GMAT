# Generate a separate startup file without changing the wx configuration.
if(WIN32)
  set(qt_startup_template "${GMAT_BUILDOUTPUT_DIRECTORY}/bin/gmat_startup_file.txt")
else()
  set(qt_startup_template "${GMAT_BUILDOUTPUT_DIRECTORY}/bin/gmat_startup_file_mac_linux.txt")
endif()
file(STRINGS "${qt_startup_template}" qt_startup_lines)
set(qt_startup "# Generated Qt startup: enabled native plugins; wx OpenFrames adapters excluded.\n")
foreach(line IN LISTS qt_startup_lines)
  if(NOT line MATCHES "^[ \t]*PLUGIN[ \t]*=")
    string(APPEND qt_startup "${line}\n")
  endif()
endforeach()
get_property(qt_plugins GLOBAL PROPERTY GMAT_PLUGIN_TARGETS)
foreach(plugin IN LISTS qt_plugins)
  # These plugins create wx windows and cannot run in a Qt application.
  if(plugin MATCHES "OpenFrames|OVtoOFI")
    continue()
  endif()
  get_target_property(plugin_dir ${plugin} GMAT_PLUGIN_INSTALL_DIR)
  string(APPEND qt_startup "PLUGIN = ../${plugin_dir}/$<TARGET_FILE_PREFIX:${plugin}>$<TARGET_FILE_BASE_NAME:${plugin}>\n")
  add_dependencies(GmatQt ${plugin})
endforeach()
# Optional separately distributed engine plugins (for example VF13ad). These
# libraries are installed by the user and are not bundled by the Qt build.
set(GMAT_QT_EXTERNAL_PLUGINS "" CACHE STRING "Additional engine plugin paths for the Qt startup file, without library extensions")
foreach(plugin IN LISTS GMAT_QT_EXTERNAL_PLUGINS)
  string(APPEND qt_startup "PLUGIN = ${plugin}\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/qt-startup/$<CONFIG>/install/gmat_startup_qt.txt" CONTENT "${qt_startup}")
# Debug binaries live in application/debug/bin, while mission data remains in
# application/data. Installed builds always use the normal bin/data layout.
string(REGEX REPLACE "ROOT_PATH[ \t]*=[ \t]*[^\n]*" "ROOT_PATH = $<IF:$<CONFIG:Debug>,../../,../>" qt_startup "${qt_startup}")
string(REGEX REPLACE "OUTPUT_PATH[ \t]*=[ \t]*[^\n]*" "OUTPUT_PATH = $<IF:$<CONFIG:Debug>,../../output/,../output/>" qt_startup "${qt_startup}")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/qt-startup/$<CONFIG>/gmat_startup_qt.txt" CONTENT "${qt_startup}")
add_custom_target(GmatQtStartup ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GmatQt>"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/qt-startup/$<CONFIG>/gmat_startup_qt.txt"
    "$<TARGET_FILE_DIR:GmatQt>/gmat_startup_qt.txt"
  VERBATIM)
add_dependencies(GmatQt GmatQtStartup)
install(FILES "${CMAKE_BINARY_DIR}/qt-startup/$<CONFIG>/install/gmat_startup_qt.txt" DESTINATION bin)
