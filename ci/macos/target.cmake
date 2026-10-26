if(NOT APPLE)
  message(FATAL_ERROR "The native smoke experiment requires macOS")
endif()
add_executable(GmatQtMacOSSmoke "${PROJECT_SOURCE_DIR}/ci/macos/NativeSmoke.cpp")
target_link_libraries(GmatQtMacOSSmoke PRIVATE GmatQtFrontend)
target_include_directories(GmatQtMacOSSmoke PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
add_dependencies(GmatQtMacOSSmoke GmatQtStartup)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/smoke-paths.json" CONTENT
  "{\"app\":\"$<TARGET_FILE:GmatQt>\",\"smoke\":\"$<TARGET_FILE:GmatQtMacOSSmoke>\",\"startup\":\"$<TARGET_FILE_DIR:GmatQt>/gmat_startup_qt.txt\"}\n")
