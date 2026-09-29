# Qt graphics validation

Validated on Linux with Qt 6.10.2, OSG 3.6.5 and Mesa software OpenGL under
Xvfb. This completes the texture/model/lighting improvement pass while retaining
the Qt layout and controls requested by the user. It does not claim full wx/OFI
feature parity or Windows/macOS validation.

## Evidence

| Requirement | Current evidence |
| --- | --- |
| Use existing implementations/assets as references | wx `OrbitViewCanvas.cpp` and `ModelObject.cpp` informed model configuration and normalized display scale. OFI's Qt adapter was inspected; its separate viewer and controls were not adopted. Existing GMAT planet maps and Aura model/atlas are used. |
| Textured bodies with correct depth | Native texture-color and near/far marker tests at display scales 1 and 2; real orbit screenshot. |
| Body orientation follows epoch | First/last body-fixed surface point compared with independent coordinate conversion. |
| Spacecraft models/materials | Actual Aura 3DS close-up and orbit screenshot; synthetic OBJ/material/texture test. Missing models fall back to markers. |
| Spacecraft attitude and Sun illumination | Numerical attitude inverse/composition and Sun ephemeris checks; native day/night, camera-light and replay-frame tests. |
| Retain camera/replay behavior | Native real-mission plot tests cover reversible zoom, retained history, close during run, reopen and rerun. Fit includes model display size. |
| Preserve existing Qt/PR behavior | Nine tests pass, including file/editor, resource/mission editing, real-engine plots and launch checks. Integration baseline `0ef9a8a` remains an ancestor. |
| Render actual Linux missions | Both fixtures propagate 12000 seconds. `orbit.png` shows Earth plus Aura. `spacecraft.png` uses a spacecraft-centered coordinate system to show the model materials. |
| Reliable overlays/export | Native tests compare displayed X11 caption pixels with framebuffer export. Three repeated full-mission X11 captures had identical caption and legend regions. |

`check-qt.txt` records the final nine-test result. The two screenshots are actual
X11 window captures from the final build. Their fixtures require the local Aura
model in `application/data/vehicle/models/aura.3ds` and the accompanying atlas;
see that asset directory's provenance notes. Synthetic automated model tests do
not depend on this optional downloaded asset.

```sh
cmake --build build/linux-gui --target check-qt --parallel 6
application/bin/GmatQt doc/DevelopersDocs/Qt6ModelValidation/orbit.script --run
application/bin/GmatQt doc/DevelopersDocs/Qt6ModelValidation/spacecraft.script --run
```

The Sunlight toolbar button switches between the recorded Sun direction and
camera-facing inspection light. Spacecraft model size is exaggerated using the
wx convention; it is not the physical spacecraft radius. Illumination includes
ambient fill and does not simulate eclipses. Star fields, reference planes,
non-spherical body models and scripted camera tracking remain unsupported.
The Qt offscreen/minimal fallback still draws untextured body disks.
