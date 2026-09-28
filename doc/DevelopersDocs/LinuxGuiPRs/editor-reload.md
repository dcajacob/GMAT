# Preserve script edits and undo history when reloading fails

Reloading an unavailable script cleared unsaved text and undo history while returning success. Preserve the existing buffer, filename and modified state unless loading succeeds.

Validation: Failed-load return value, text, dirty state, actual Undo/Redo, retained filename, a disappeared file and successful recovery. The original reproduction failed before this change and passes afterward. A clean native build tested this branch independently with the DE-header, CMake PATH, Linux layout, ground-track and viewport prerequisites. The combined suite reports 30 passing checks. Windows/macOS and native Wayland are not validated.

The production-only branch is `pr/editor-reload`. Supplemental tests and full evidence live on `linux-gui-integration` in `LinuxGuiAudit.md`. No PR has been opened.
