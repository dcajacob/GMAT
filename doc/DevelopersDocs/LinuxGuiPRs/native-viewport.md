# Use physical pixel dimensions for native OpenGL viewports

On a display using 3× scaling, run the default mission with native OrbitView (disable OVtoOFI). The scene occupies only the lower-left ninth of its plot. Resizing does not correct it: a 722×536 logical canvas was given a 722×536 viewport instead of 2166×1608 physical pixels.

Scale the three native OpenGL viewport calls by wxWidgets' content scale factor. Shared drawing and both native GL resize handlers use physical pixels; projection and mouse coordinates retain their existing logical units. The patch changes three files with nine additions and three deletions.

Validation: Release GUI rebuilt successfully with wxGTK 3.2.9. The integration harness checks GL_VIEWPORT against physical client dimensions at 1×, 2× and 3×, before and after two resizes each. All nine assertions pass; the unpatched 3× run failed all three. All 25 combined integration checks pass. Manual reproduction: run the native default mission, maximize/resize the window, and confirm Earth remains centered in the full plot at each scale.

Supplemental command, on `linux-gui-integration`:

```sh
python3 src/UnitTests/TestLinuxGui/test_viewport.py build/linux-gui
```

Tests use X11/Xvfb and software OpenGL. Physical GPU, native Wayland, Windows/macOS and live legacy GL GroundTrack rendering remain untested. The proposed branch contains production code only; repeat an isolated build before submission, accounting for the upstream ephemeris startup failure on this host.

Prepared branch: `pr/native-viewport`. No PR has been opened.
