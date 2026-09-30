find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(OpenSceneGraph 3.6 REQUIRED COMPONENTS osg osgDB osgViewer osgGA osgUtil osgText)
add_library(GmatQtFrontend STATIC MainWindow.cpp MainWindow.hpp
   QtMessageReceiver.cpp QtMessageReceiver.hpp QtInterpreter.cpp QtInterpreter.hpp
   UserParameter.cpp UserParameter.hpp
   AboutDialog.cpp AboutDialog.hpp
   HelpDialog.cpp HelpDialog.hpp
   SolarSystemPanel.cpp SolarSystemPanel.hpp
   CelestialBodyPanel.cpp CelestialBodyPanel.hpp
   PathSettings.cpp PathSettings.hpp PathSettingsDialog.cpp PathSettingsDialog.hpp
   ResourceEditor.cpp ResourceEditor.hpp ResourceProperties.cpp ResourceProperties.hpp
   EditablePanel.cpp EditablePanel.hpp CommandEditor.cpp CommandEditor.hpp MissionModel.cpp MissionModel.hpp
   PropagationForm.cpp PropagationForm.hpp
   CommandForm.cpp CommandForm.hpp
   ConditionDialog.cpp ConditionDialog.hpp
   ReportParameterDialog.cpp ReportParameterDialog.hpp
   QtSolverListener.cpp QtSolverListener.hpp
   StartupCompatibility.cpp StartupCompatibility.hpp
   ScriptCompatibility.cpp ScriptCompatibility.hpp
   BallisticsMassDialog.cpp BallisticsMassDialog.hpp
   VisualModelDialog.cpp VisualModelDialog.hpp
   AttitudeDialog.cpp AttitudeDialog.hpp
   SpacecraftOrbit.cpp SpacecraftOrbit.hpp
   AtmosphereDialog.cpp AtmosphereDialog.hpp
   GroundStationDialog.cpp GroundStationDialog.hpp
   EventLocatorDialog.cpp EventLocatorDialog.hpp
   XYPlotDialog.cpp XYPlotDialog.hpp
   GroundTrackDialog.cpp GroundTrackDialog.hpp
   OrbitViewDialog.cpp OrbitViewDialog.hpp
   ThrusterDialog.cpp ThrusterDialog.hpp
   BurnDialog.cpp BurnDialog.hpp
   EphemerisDialog.cpp EphemerisDialog.hpp
   DynamicDataDialog.cpp DynamicDataDialog.hpp
   TrackingConfigDialog.cpp TrackingConfigDialog.hpp
   EpochIntervalDialog.cpp EpochIntervalDialog.hpp
   ScriptEventDialog.cpp ScriptEventDialog.hpp
   ScriptEditor.cpp ScriptEditor.hpp
   FunctionFileDialog.cpp FunctionFileDialog.hpp
   InspectionDialog.cpp InspectionDialog.hpp
   FindReplaceDialog.cpp FindReplaceDialog.hpp
   ReportViewer.cpp ReportViewer.hpp
   FileComparison.cpp FileComparison.hpp ComparisonPanel.cpp ComparisonPanel.hpp
   PropagationStopsDialog.cpp PropagationStopsDialog.hpp
   PropagationGroupsDialog.cpp PropagationGroupsDialog.hpp
   OrbitRenderer.cpp OrbitRenderer.hpp
   OrbitCamera.hpp
   StarCatalog.cpp StarCatalog.hpp
   PlotModel.cpp PlotModel.hpp PlotWidget.cpp PlotWidget.hpp QtPlotReceiver.cpp QtPlotReceiver.hpp)
target_link_libraries(GmatQtFrontend PUBLIC Qt6::Widgets Qt6::OpenGLWidgets GmatBase GmatUtil ${OPENSCENEGRAPH_LIBRARIES})
target_include_directories(GmatQtFrontend PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
get_filename_component(qt_osg_library_dir "${OSG_LIBRARY_RELEASE}" DIRECTORY)
target_compile_definitions(GmatQtFrontend PRIVATE
   GMAT_QT_OSG_PLUGIN_DIR="${qt_osg_library_dir}/osgPlugins-${OPENSCENEGRAPH_VERSION}")
target_include_directories(GmatQtFrontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
   # Engine headers expose Windows headers; retain the C++ min/max functions.
   target_compile_definitions(GmatQtFrontend PUBLIC NOMINMAX)
endif()
add_executable(GmatQt main.cpp)
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
   # Explicit, like runtime deployment: compiling the GUI does not require Java.
   # The documentation Makefile uses the repository's bundled DocBook tools.
   add_custom_target(build-qt-help
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/BuildHelp.py
      COMMENT "Building and installing the offline GMAT user guide" USES_TERMINAL VERBATIM)
endif()
target_link_libraries(GmatQt PRIVATE GmatQtFrontend)
_SETOUTPUTDIRECTORY(GmatQt bin)
set_target_properties(GmatQt PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
if(WIN32 AND TARGET Qt6::windeployqt)
   # Keep deployment explicit: normal builds do not copy a Qt installation.
   # GMAT's non-Qt dependencies and the MSVC redistributable remain prerequisites.
   add_custom_target(deploy-qt
      COMMAND Qt6::windeployqt
         $<IF:$<CONFIG:Debug>,--debug,--release>
         --no-compiler-runtime --no-translations
         "$<TARGET_FILE:GmatQt>"
      DEPENDS GmatQt
      COMMENT "Deploying Qt runtime libraries and plugins beside GmatQt"
      USES_TERMINAL VERBATIM)
endif()
if(APPLE)
   set_target_properties(GmatQt PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks")
   install(TARGETS GmatQt DESTINATION ${GMAT_MAC_APPBUNDLE_PATH}/MacOS)
else()
   set_target_properties(GmatQt PROPERTIES INSTALL_RPATH "$ORIGIN")
   install(TARGETS GmatQt DESTINATION bin)
endif()

if(GMAT_QT_BUILD_TESTS)
   add_executable(GmatQtOrbitRendererTests tests/OrbitRendererTests.cpp)
   target_link_libraries(GmatQtOrbitRendererTests PRIVATE GmatQtFrontend)
   if(UNIX AND NOT APPLE)
      find_program(QT_XVFB_RUN xvfb-run)
      if(QT_XVFB_RUN)
         add_test(NAME QtGui.NativeOrbit COMMAND ${QT_XVFB_RUN} -a
            ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1
            $<TARGET_FILE:GmatQtOrbitRendererTests>)
         set_tests_properties(QtGui.NativeOrbit PROPERTIES LABELS qt-gui TIMEOUT 60)
         add_test(NAME QtGui.NativeOrbitHiDPI COMMAND ${QT_XVFB_RUN} -a
            ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 QT_SCALE_FACTOR=2
            $<TARGET_FILE:GmatQtOrbitRendererTests>)
         set_tests_properties(QtGui.NativeOrbitHiDPI PROPERTIES LABELS qt-gui TIMEOUT 60)
         add_test(NAME QtGui.NativeOrbitFractionalDPI COMMAND ${QT_XVFB_RUN} -a
            ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 QT_SCALE_FACTOR=1.5
            $<TARGET_FILE:GmatQtOrbitRendererTests>)
         set_tests_properties(QtGui.NativeOrbitFractionalDPI PROPERTIES LABELS qt-gui TIMEOUT 60)
      endif()
   endif()
   add_executable(GmatQtWindowTests tests/WindowTests.cpp)
   target_link_libraries(GmatQtWindowTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtSolverPlotTests tests/SolverPlotTests.cpp)
   target_link_libraries(GmatQtSolverPlotTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtDesktopTests tests/DesktopTests.cpp)
   target_link_libraries(GmatQtDesktopTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtHelpTests tests/HelpTests.cpp)
   target_link_libraries(GmatQtHelpTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtSpiceDiagnosticTests tests/SpiceDiagnosticTests.cpp)
   target_link_libraries(GmatQtSpiceDiagnosticTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtFileTests tests/FileTests.cpp)
   target_link_libraries(GmatQtFileTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtWorkflowTests tests/WorkflowTests.cpp)
   target_link_libraries(GmatQtWorkflowTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtPlotTests tests/PlotTests.cpp)
   target_link_libraries(GmatQtPlotTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtMissionTests tests/MissionTests.cpp)
   target_link_libraries(GmatQtMissionTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtCompatibilityTests tests/CompatibilityTests.cpp)
   target_link_libraries(GmatQtCompatibilityTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtAtmosphereTests tests/AtmosphereTests.cpp)
   target_link_libraries(GmatQtAtmosphereTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtStationTests tests/StationTests.cpp)
   target_link_libraries(GmatQtStationTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtEventLocatorTests tests/EventLocatorTests.cpp)
   target_link_libraries(GmatQtEventLocatorTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtPlotSetupTests tests/PlotSetupTests.cpp)
   target_link_libraries(GmatQtPlotSetupTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtOrbitSetupTests tests/OrbitSetupTests.cpp)
   target_link_libraries(GmatQtOrbitSetupTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtThrusterTests tests/ThrusterTests.cpp)
   target_link_libraries(GmatQtThrusterTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtBurnTests tests/BurnTests.cpp)
   target_link_libraries(GmatQtBurnTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtEphemerisTests tests/EphemerisTests.cpp)
   target_link_libraries(GmatQtEphemerisTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtDynamicDataTests tests/DynamicDataTests.cpp)
   target_link_libraries(GmatQtDynamicDataTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtCalculatedPointTests tests/CalculatedPointTests.cpp)
   target_link_libraries(GmatQtCalculatedPointTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtDataInterfaceTests tests/DataInterfaceTests.cpp)
   target_link_libraries(GmatQtDataInterfaceTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtEstimationTests tests/EstimationTests.cpp)
   target_link_libraries(GmatQtEstimationTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtParameterTests tests/ParameterTests.cpp)
   add_executable(GmatQtPathTests tests/PathTests.cpp)
   add_executable(GmatQtSolarSystemTests tests/SolarSystemTests.cpp)
   add_executable(GmatQtCelestialBodyTests tests/CelestialBodyTests.cpp)
   target_link_libraries(GmatQtCelestialBodyTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtResourceRoundTripTests tests/ResourceRoundTripTests.cpp)
   target_link_libraries(GmatQtResourceRoundTripTests PRIVATE GmatQtFrontend)
   target_link_libraries(GmatQtSolarSystemTests PRIVATE GmatQtFrontend)
   target_link_libraries(GmatQtPathTests PRIVATE GmatQtFrontend)
   add_dependencies(GmatQtPathTests GmatQt)
   target_link_libraries(GmatQtParameterTests PRIVATE GmatQtFrontend)
   add_dependencies(GmatQtParameterTests GmatQt)
   add_executable(GmatQtInspectionTests tests/InspectionTests.cpp)
   target_link_libraries(GmatQtInspectionTests PRIVATE GmatQtFrontend)
   add_dependencies(GmatQtInspectionTests GmatQt)
   add_executable(GmatQtKalmanTests tests/KalmanTests.cpp)
   target_link_libraries(GmatQtKalmanTests PRIVATE GmatQtFrontend)
   add_dependencies(GmatQtKalmanTests GmatQt)
   add_executable(GmatQtComparisonTests tests/ComparisonTests.cpp)
   target_link_libraries(GmatQtComparisonTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtThrustFileTests tests/ThrustFileTests.cpp)
   target_link_libraries(GmatQtThrustFileTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtScriptEditingTests tests/ScriptEditingTests.cpp)
   target_link_libraries(GmatQtScriptEditingTests PRIVATE GmatQtFrontend)
   add_executable(GmatQtPolyhedronTests tests/PolyhedronTests.cpp)
   target_link_libraries(GmatQtPolyhedronTests PRIVATE GmatQtFrontend)
   add_dependencies(GmatQtPolyhedronTests GmatQt)
   add_executable(GmatQtExternalForceTests tests/ExternalForceTests.cpp)
   target_link_libraries(GmatQtExternalForceTests PRIVATE GmatQtFrontend)
   find_package(Python3 REQUIRED COMPONENTS Interpreter)
   set(qt_test_runner "${CMAKE_CURRENT_SOURCE_DIR}/tests/RunTest.py")
   function(add_qt_test name target)
      add_test(NAME ${name} COMMAND "${Python3_EXECUTABLE}" "${qt_test_runner}"
         "$<TARGET_FILE_DIR:GmatQt>" "$<TARGET_FILE_DIR:Qt6::Core>"
         $<TARGET_FILE:${target}> ${ARGN})
      set_tests_properties(${name} PROPERTIES LABELS qt-gui TIMEOUT 60
         RESOURCE_LOCK gmat_qt_engine WORKING_DIRECTORY "$<TARGET_FILE_DIR:GmatQt>")
   endfunction()
   set(qt_test_startup "$<TARGET_FILE_DIR:GmatQt>/gmat_startup_qt.txt")
   if(UNIX AND NOT APPLE AND QT_XVFB_RUN)
      add_test(NAME QtGui.NativeWindows COMMAND ${QT_XVFB_RUN} -a
         ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1
         $<TARGET_FILE:GmatQtWindowTests> "${qt_test_startup}")
      set_tests_properties(QtGui.NativeWindows PROPERTIES LABELS qt-gui TIMEOUT 60 RESOURCE_LOCK gmat_qt_engine)
      add_test(NAME QtGui.NativeSolverPlots COMMAND ${QT_XVFB_RUN} -a
         ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1
         $<TARGET_FILE:GmatQtSolverPlotTests> "${qt_test_startup}")
      set_tests_properties(QtGui.NativeSolverPlots PROPERTIES LABELS qt-gui TIMEOUT 60 RESOURCE_LOCK gmat_qt_engine)
      add_test(NAME QtGui.NativePlots COMMAND ${QT_XVFB_RUN} -a
         ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1
         $<TARGET_FILE:GmatQtPlotTests> "${qt_test_startup}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/plots.script")
      set_tests_properties(QtGui.NativePlots PROPERTIES LABELS qt-gui TIMEOUT 60 RESOURCE_LOCK gmat_qt_engine)
   endif()
   add_qt_test(QtGui.Files GmatQtFileTests)
   add_qt_test(QtGui.Workflow GmatQtWorkflowTests "${qt_test_startup}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/propagate.script")
   add_qt_test(QtGui.Mission GmatQtMissionTests "${qt_test_startup}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/mission.script")
   add_qt_test(QtGui.Compatibility GmatQtCompatibilityTests "${qt_test_startup}" "${PROJECT_SOURCE_DIR}/application/samples")
   add_qt_test(QtGui.Atmosphere GmatQtAtmosphereTests "${qt_test_startup}")
   add_qt_test(QtGui.Stations GmatQtStationTests "${qt_test_startup}")
   add_qt_test(QtGui.EventLocators GmatQtEventLocatorTests "${qt_test_startup}")
   add_qt_test(QtGui.Thrusters GmatQtThrusterTests "${qt_test_startup}")
   add_qt_test(QtGui.Burns GmatQtBurnTests "${qt_test_startup}")
   add_qt_test(QtGui.Ephemeris GmatQtEphemerisTests "${qt_test_startup}")
   add_qt_test(QtGui.DynamicData GmatQtDynamicDataTests "${qt_test_startup}")
   add_qt_test(QtGui.CalculatedPoints GmatQtCalculatedPointTests "${qt_test_startup}")
   add_qt_test(QtGui.DataInterface GmatQtDataInterfaceTests "${qt_test_startup}")
   add_qt_test(QtGui.Estimation GmatQtEstimationTests "${qt_test_startup}")
   add_qt_test(QtGui.Parameters GmatQtParameterTests "${qt_test_startup}")
   add_qt_test(QtGui.Paths GmatQtPathTests "${qt_test_startup}")
   add_qt_test(QtGui.SolarSystem GmatQtSolarSystemTests "${qt_test_startup}")
   add_qt_test(QtGui.CelestialBodies GmatQtCelestialBodyTests "${qt_test_startup}")
   add_qt_test(QtGui.ResourceRoundTrips GmatQtResourceRoundTripTests "${qt_test_startup}")
   add_qt_test(QtGui.Inspection GmatQtInspectionTests "${qt_test_startup}")
   add_qt_test(QtGui.KalmanFilter GmatQtKalmanTests "${qt_test_startup}")
   add_qt_test(QtGui.Comparison GmatQtComparisonTests "${qt_test_startup}")
   add_qt_test(QtGui.ThrustFiles GmatQtThrustFileTests "${qt_test_startup}")
   add_qt_test(QtGui.ScriptEditing GmatQtScriptEditingTests "${qt_test_startup}")
   add_qt_test(QtGui.ExternalForces GmatQtExternalForceTests "${qt_test_startup}")
   add_qt_test(QtGui.Polyhedron GmatQtPolyhedronTests "${qt_test_startup}")
   add_qt_test(QtGui.OrbitSetup GmatQtOrbitSetupTests "${qt_test_startup}")
   add_qt_test(QtGui.SolverPlots GmatQtSolverPlotTests "${qt_test_startup}")
   add_qt_test(QtGui.Desktop GmatQtDesktopTests "${qt_test_startup}")
   add_qt_test(QtGui.Help GmatQtHelpTests "${qt_test_startup}")
   add_qt_test(QtGui.SpiceDiagnostics GmatQtSpiceDiagnosticTests "${qt_test_startup}")
   add_qt_test(QtGui.PlotSetup GmatQtPlotSetupTests "${qt_test_startup}")
   add_qt_test(QtGui.Plots GmatQtPlotTests "${qt_test_startup}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/plots.script")
   add_qt_test(QtGui.PlotsHiDPI GmatQtPlotTests "${qt_test_startup}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/plots.script")
   set_tests_properties(QtGui.PlotsHiDPI PROPERTIES ENVIRONMENT "QT_SCALE_FACTOR=2")
   add_test(NAME QtGui.Launch COMMAND "${Python3_EXECUTABLE}" "${qt_test_runner}"
      "$<TARGET_FILE_DIR:GmatQt>" "$<TARGET_FILE_DIR:Qt6::Core>"
      "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/LaunchTests.py"
      $<TARGET_FILE:GmatQt> "${CMAKE_CURRENT_SOURCE_DIR}/tests/propagate.script")
   set_tests_properties(QtGui.Launch PROPERTIES LABELS qt-gui TIMEOUT 150 RESOURCE_LOCK gmat_qt_engine)
   add_custom_target(check-qt
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG> -L qt-gui
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
      DEPENDS GmatQt GmatQtWindowTests GmatQtFileTests GmatQtWorkflowTests GmatQtMissionTests GmatQtPlotTests GmatQtOrbitRendererTests GmatQtCompatibilityTests GmatQtAtmosphereTests GmatQtStationTests GmatQtEventLocatorTests GmatQtPlotSetupTests GmatQtOrbitSetupTests GmatQtThrusterTests GmatQtBurnTests GmatQtEphemerisTests GmatQtDynamicDataTests GmatQtCalculatedPointTests GmatQtDataInterfaceTests GmatQtEstimationTests GmatQtComparisonTests GmatQtThrustFileTests GmatQtScriptEditingTests GmatQtExternalForceTests GmatQtPolyhedronTests
      USES_TERMINAL VERBATIM)
   add_dependencies(check-qt GmatQtKalmanTests GmatQtInspectionTests GmatQtParameterTests GmatQtPathTests GmatQtSolarSystemTests GmatQtCelestialBodyTests)
   add_dependencies(check-qt GmatQtResourceRoundTripTests)
   add_dependencies(check-qt GmatQtSolverPlotTests)
   add_dependencies(check-qt GmatQtDesktopTests)
   add_dependencies(check-qt GmatQtHelpTests)
   add_dependencies(check-qt GmatQtSpiceDiagnosticTests)
endif()
