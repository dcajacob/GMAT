$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue'
$cmake='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$wxlib=(Get-ChildItem C:\GMAT-Test\deps\wx\lib -Directory | Where-Object Name -like '*x64*dll*' | Select-Object -First 1).FullName
foreach ($v in 'upstream','patched') {
 $root="C:\GMAT-Test\$v"
 & $cmake -S $root -B "$root\build-win" -G 'Visual Studio 17 2022' -A x64 -DwxWidgets_ROOT_DIR=C:/GMAT-Test/deps/wx "-DwxWidgets_LIB_DIR=$wxlib" -DwxWidgets_CONFIGURATION=mswu -DCSPICE_DIR=C:/GMAT-Test/deps/cspice -DXercesC_INCLUDE_DIR=C:/GMAT-Test/deps/xerces/include -DXercesC_LIBRARY_RELEASE=C:/GMAT-Test/deps/xerces/lib/xerces-c_3.lib '-DGMAT_PYTHON3_VERSIONS=3.12' -DGMAT_PYTHON312_ROOT_DIR=C:/Python312 -DPLUGIN_MATLABINTERFACE=OFF -DPLUGIN_OPENFRAMESINTERFACE=OFF -DGMAT_INCLUDE_API=OFF
 if ($LASTEXITCODE) { throw "$v configuration failed" }
 & $cmake --build "$root\build-win" --config Release --parallel 6
 if ($LASTEXITCODE) { throw "$v build failed" }
 Copy-Item "$wxlib\*.dll" "$root\application\bin" -Force
}
