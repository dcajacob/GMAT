# Qt star-field validation

Star fields use the existing startup `STAR_FILE` catalog, with RA/declination
converted to inertial unit directions and entries sorted by magnitude.
`EnableStars` and `StarCount` are honored; invalid lines are skipped with a
diagnostic, and absent/empty catalogs leave the mission usable without stars.
Constellation lines remain unsupported and are reported separately.

The sky has a fixed 50-degree vertical field of view. It rotates with the camera
and the recorded inertial-to-view transformation, but does not translate or
change angular scale during orthographic pan/zoom. Background stars do not write
depth and are covered by bodies and spacecraft. Five magnitude bands determine
point brightness and size. This follows catalog positions rather than drawing
random decorative points; proper motion and photometric exposure are not modeled.

## Evidence

- `check-qt.txt`: all nine Linux tests pass, including native rendering at scale
  1 and 2, actual mission workflows, and existing Qt regressions.
- Native checks exercise catalog parsing, invalid rows, missing files,
  magnitude ordering and brightness, star count, disable/zero count, back-sky
  rejection, planet occlusion, camera rotation, pan/zoom invariance and replay.
- The real-engine fixture includes an Earth-fixed OrbitView with StarCount 1234.
  Its first/last inertial-to-view matrices are checked against independent
  coordinate conversions; the catalog resolves to over 40000 valid entries.
- `stars.png`: final-build X11 capture of `stars.script` after 12000 seconds of
  propagation, with 12000 catalog entries selected before view culling.
  The fixture also uses the existing Aura model/atlas; asset provenance is in
  `application/data/vehicle/models/README.md`.

Reproduce from the repository root:

```sh
cmake --build build/linux-gui --target check-qt --parallel 6
application/bin/GmatQt doc/DevelopersDocs/Qt6StarValidation/stars.script --run
```

Validation used Qt 6.10.2, OSG 3.6.5 and Mesa software OpenGL under Xvfb on Linux.
The offscreen/minimal CPU fallback does not render stars. No Windows/macOS build
was performed for this change. Rendering and replay hold numeric state only;
no live engine objects are consulted while drawing the stars.
