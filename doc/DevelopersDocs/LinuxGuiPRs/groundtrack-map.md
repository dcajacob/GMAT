# Resolve ground-track textures and handle failed redraws safely

Launch outside application/bin and run the default mission. The Earth map used a hard-coded relative path, and every repaint retried the failure and could show another modal image error.

Use the configured texture search paths and the central body full path. Cache failed load attempts, report one nonmodal warning, support PNG maps, and guard invalid image sizes and empty/single-point curves.

## Validation

Painted-pixel checks pass for the default mission, a body texture outside TEXTURE_PATH, filename/absolute/script-relative maps, PNG decoding, missing/corrupt image recovery, empty/single-point/stationary curves and 2x scaling.

GUI runtime checks used the integration build, which includes the separate ephemeris startup fix. Some native OrbitView window-creation tests still log GTK notebook gadget warnings. The pixman report is unresolved.

```sh
python3 src/UnitTests/TestLinuxGui/test_groundtrack.py build/linux-gui
```

Prepared branch: `pr/groundtrack-map`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
