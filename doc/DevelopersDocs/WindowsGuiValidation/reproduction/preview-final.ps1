$ErrorActionPreference='Stop'
while (!(Test-Path C:\GMAT-Test\tests-done.txt)) {Start-Sleep -Seconds 5}
C:\Python312\python.exe C:\GMAT-Test\compile-harness.py C:\GMAT-Test\upstream C:\GMAT-Test\patched\src\UnitTests\TestLinuxGui\ModelPreviewRegression.cpp
C:\Python312\python.exe C:\GMAT-Test\compile-harness.py C:\GMAT-Test\upstream C:\GMAT-Test\patched\src\UnitTests\TestLinuxGui\ModelPreviewRegression.cpp C:\GMAT-Test\isolated\model-preview-context\VisualModelCanvas.cpp
C:\Python312\python.exe C:\GMAT-Test\compile-harness.py C:\GMAT-Test\patched C:\GMAT-Test\patched\src\UnitTests\TestLinuxGui\ModelPreviewRegression.cpp
C:\Python312\python.exe C:\GMAT-Test\preview-rerun.py
Remove-Item C:\GMAT-Test\tests-done.txt
Start-ScheduledTask -TaskName 'GMAT GUI validation'
