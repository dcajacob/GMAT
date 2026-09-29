# Qt 6 Windows validation

Validated 2026-09-28 in the existing `gmat-win11-test` KVM VM: Windows 11,
Visual Studio 2022 Build Tools, x64 Release, Qt 6.10.2 MSVC 2022, Python 3.12.
Source was an archive of `f7b5d5e`, with the accompanying Qt CMake changes
(`NOMINMAX` and `deploy-qt`) copied into the isolated test checkout.
The prior wx validation checkouts were not modified.

The initial Qt compilation failed where Windows `min`/`max` macros expanded
C++ functions in MissionModel and ResourceProperties. Defining `NOMINMAX` on
GmatQtFrontend and its consumers resolved those errors. All six CTest checks
then passed; [ctest.txt](ctest.txt) records the native result. The Linux suite
also passed after these CMake changes (six tests, 8.15 seconds).

## Build configuration

The test checkout was `C:/GMAT-Test/qt6-f7b5d5e`, with build directory `build-qt`.
CMake used Visual Studio 17 2022, x64, and these overrides:

```
-DGMAT_INCLUDE_GUI=OFF
-DGMAT_INCLUDE_QT_GUI=ON
-DGMAT_QT_BUILD_TESTS=ON
-DCMAKE_PREFIX_PATH=C:/GMAT-Test/deps/Qt/6.10.2/msvc2022_64
-DCSPICE_DIR=C:/GMAT-Test/deps/cspice
-DXercesC_INCLUDE_DIR=C:/GMAT-Test/deps/xerces/include
-DXercesC_LIBRARY_RELEASE=C:/GMAT-Test/deps/xerces/lib/xerces-c_3.lib
-DGMAT_PYTHON3_VERSIONS=3.12
-DGMAT_PYTHON312_ROOT_DIR=C:/Python312
-DPython3_EXECUTABLE=C:/Python312/python.exe
-DPLUGIN_MATLABINTERFACE=OFF
-DPLUGIN_OPENFRAMESINTERFACE=OFF
-DGMAT_INCLUDE_API=OFF
```

Run `cmake --build build-qt --target check-qt --config Release --parallel 6`
for the tests, then the same build command with `--target deploy-qt` to copy
Qt's runtime libraries and plugins beside the application. The deployment
target completed successfully, including `platforms/qwindows.dll` and JPEG
image support. Qt was installed only in the VM's dependency directory, using
aqtinstall and the official Qt `qtbase` binary archive.

## Native desktop execution

An interactive scheduled task launched `application/bin/GmatQt.exe` in the
logged-in user's session (session 1), from `C:/Windows/Temp`. It supplied the
absolute `src/qtgui/tests/plots.script` path, `--run`, `--screenshot`, and an
isolated `--settings-dir`. No startup override was used. QT_QPA_PLATFORM and
QT_PLUGIN_PATH were unset, and PATH contained only Python, Windows system
directories and the user's WindowsApps directory; no Qt SDK directory.

The process exited 0 and captured [native-window.png](native-window.png).
Visual inspection confirmed the Resources/Mission/Output navigation, MDI
workspace, message pane, rendered world map/ground track, XY plot and dynamic
data table. The mission completed at 12,000 seconds. The Qt runtime was
resolved from the deployed build directory, rather than a Qt entry on PATH.

This verifies native build, the offscreen regression suite, build-directory
deployment and a real Windows desktop launch. It does not establish macOS
support, standalone installer completeness, or full wx feature parity.
