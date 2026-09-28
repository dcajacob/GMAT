# Preserve PATH when wxWidgets_ROOT_DIR is supplied

Configure a fresh Linux build with wxWidgets_ROOT_DIR explicitly supplied and SYS_PATH absent. The original condition fails to save the system PATH before wxWidgets discovery.

Initialize the internal saved PATH independently of wxWidgets_ROOT_DIR. Fresh Linux configuration with an explicit wxWidgets root otherwise loses the system search path.

## Validation

Fresh Release CMake/Ninja configuration passed with wxWidgets_ROOT_DIR=/usr/bin and no SYS_PATH override.

Checked with system wxGTK 3.2.9 on Linux. Other toolchains are not claimed tested.

Prepared branch: `pr/wxwidgets-path`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
