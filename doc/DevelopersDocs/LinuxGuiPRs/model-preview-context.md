# Defer spacecraft preview OpenGL calls until its context is current

Opening spacecraft properties after native plots were closed could terminate GMAT with X11 BadDrawable. The preview constructor issued OpenGL calls against the previous drawable. Remove those redundant calls; OnPaint already performs them after setting the current context.

Validation: Run/rerun, close/recreate plots, Stop/recovery and opening spacecraft properties repeatedly, with Xvfb software rendering and the Intel Iris Xe desktop. The original reproduction failed before this change and passes afterward. A clean native build tested this branch independently with the DE-header, CMake PATH, Linux layout, ground-track and viewport prerequisites. The combined suite reports 30 passing checks. Windows/macOS and native Wayland are not validated.

The production-only branch is `pr/model-preview-context`. Supplemental tests and full evidence live on `linux-gui-integration` in `LinuxGuiAudit.md`. No PR has been opened.
