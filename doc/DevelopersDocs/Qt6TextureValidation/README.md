# Qt textured orbit validation

Linux validation of the initial native textured-body renderer, using Qt 6.10.2,
OSG 3.6 and Mesa software OpenGL under Xvfb. No Windows/macOS validation was
performed for this rendering change.

- `orbit.script` propagates a real spacecraft for 12000 seconds.
- `orbit.png` is the actual GmatQt window after running that script. It shows
  the configured Earth map, labels and depth-tested trajectory from an oblique
  view. The fixture chooses the camera vector; the viewer controls are unchanged.
- `check-qt.txt` records eight passing tests, including native rendering at
  display scales 1 and 2, synthetic depth/texture/lifecycle tests, independent
  body-fixed orientation comparisons, and existing engine/UI regressions.
- `native-mission-tests.txt` records the real-engine plot suite run with
  `QT_QPA_PLATFORM=xcb` and `LIBGL_ALWAYS_SOFTWARE=1`, covering native mixed plots,
  close during run, reopen/rerun, replay retention and reversible zoom.

Reproduce from the repository root:

```sh
cmake --build build/linux-gui --target check-qt --parallel 6
xvfb-run -a env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 \
  application/bin/GmatQt \
  doc/DevelopersDocs/Qt6TextureValidation/orbit.script \
  --settings-dir /tmp/gmat-texture-check --run --screenshot /tmp/gmat-orbit.png
```

This milestone uses viewer-relative lighting and spherical bodies. Spacecraft
models, physical Sun-directed illumination and further graphics parity are
still outstanding. The offscreen/minimal platform fallback remains untextured.
