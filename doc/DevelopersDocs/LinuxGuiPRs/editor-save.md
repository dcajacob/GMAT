# Retain script identity and modified state when saving fails

An unsuccessful Save As renamed the script and marked its panel clean; save-and-build could continue despite the write failure. Check save results before changing identity or state and stop the build path on failure. Apply the same handling to the styled and plain-text editors.

Validation: Failed Save As and save-and-build, retained identity/dirty state, retry, persisted contents and the alternate plain-text editor. The original reproduction failed before this change and passes afterward. A clean native build tested this branch independently with the DE-header, CMake PATH, Linux layout, ground-track and viewport prerequisites. The combined suite reports 30 passing checks. Windows/macOS and native Wayland are not validated.

The production-only branch is `pr/editor-save`. Supplemental tests and full evidence live on `linux-gui-integration` in `LinuxGuiAudit.md`. No PR has been opened.
