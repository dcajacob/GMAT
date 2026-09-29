# Qt R2026a parity work — Linux milestones

This records the implementation and Linux acceptance evidence for priorities
1–5. It is not a claim that every wx dialog or optional plugin has identical
coverage. Windows/macOS and clean-machine packaging remain deferred.

## Resource milestone

Array creation (bounded dimensions in the dialog), array/vector/matrix cell
editing, supported hardware/force-body lists, tank-name-preserving mixture
ratios, and resource sections are implemented. The real-engine workflow test
checks dimensions, finite-number validation, Cancel, pending values, invalid
references, serialization/reinterpretation, zero assignments and undo.
Force-model sections also expose owned gravity, drag and radiation-pressure
settings through their canonical engine property names. Tests change gravity
degree/order, propagate, and undo. Spacecraft attitude fields follow the current
representation; changing representations refreshes the editable field set.

## Commands and solver feedback

Source-preserving forms cover common Maneuver, finite-burn, Vary, Achieve,
Minimize, constraint, Report, FindEvents and solver-branch statements. Tests
exercise reordered options, labels/comments, repeated field changes and branch
body preservation. A real DifferentialCorrector fixture changes an Achieve
goal through the form, solves for the expected value, verifies the progress
table and final residual, closes the table, and reruns successfully.
Function-call forms edit inputs and outputs without replacing surrounding
source. The compatibility suite runs a changed GMAT-function cross product,
the shipped Yukon algebraic optimization (X1 and X2 both approach 2), and an
automatic eclipse search. Event settings are editable and reports appear in
Output, alongside solver reports.

`hohmann-qt.script` copies the shipped `application/samples/Ex_HohmannTransfer.script`
with only its OFI viewer/view definitions replaced by an OrbitView. Spacecraft,
force model, propagation, burns and targeting statements are unchanged. The
original sample fails to build without OFI; this adaptation is not evidence of
transparent OFI compatibility. The explicit conversion workflow below now
runs the original sample without this manual adaptation.

The adapted mission completed under Xvfb/xcb with software OpenGL. The captured
native window shows convergence with:

- `DefaultSC.Earth.RMAG`: 42165.05419499055 km, goal 42165, residual
  0.0541949905527872 km (tolerance 0.1).
- `DefaultSC.ECC`: 5.030953519506678e-7, goal 0 (tolerance 0.1).
- Burn variables: TOI.Element1 2.240283977353204 and GOI.Element1 1.432966302001916.

![Native Qt solver result](hohmann-solver.png)

`check-qt.txt` records all nine Qt Linux tests passing after this milestone,
including native/HiDPI rendering, files, workflow, mission, plots and launch.

## Scripted camera milestone

Object/vector reference points, viewpoint objects/vectors, target objects/vectors,
scale and signed view-up axes are recorded at publication time. A shared camera
basis serves the native renderer and headless fallback. Tracking continues under
manual orbit/pan/zoom offsets; Script view resets the offsets and Fit frames the
mission. Projection remains orthographic by design. Replay uses retained numeric
camera states, and clearing solver data also clears its camera snapshots.

The real-engine plot tests compare both ends of moving-object camera histories
and independently convert an inertial X up axis into EarthFixed. Native normal
and high-DPI checks cover target centering, reversed up axes, scale, manual zoom
and exact replay restoration. `tracking.script` supplies a real Aura-model
camera centered on a propagated spacecraft in EarthMJ2000Eq (optional Aura
asset required, as in the earlier model validation).

![Native spacecraft tracking view](tracking.png)

The native mission completed successfully. `check-camera.txt` records all nine
Linux Qt tests passing after the camera changes.

## Drawing and chart milestone

Native constellation outlines, XY/ecliptic reference grids, wireframe, Sun
direction lines and non-spherical body meshes are implemented. Native tests
check catalog units/ranges, forward/rear hemisphere behavior, pan/zoom
invariance, guide toggles, wireframe restoration, unexaggerated celestial mesh
dimensions and missing-mesh sphere fallback. The real-engine plot fixture
verifies scripted drawing flags and reads the bundled constellation catalog.

Ground-track footprints follow wx's five-degree reference-circle convention;
polar geometry and the dateline are checked. They do not claim sensor or
horizon coverage. XY markers and line styles have distinct image checks, and
indexed highlights leave unrelated samples unchanged. Repeated current-iteration
clearing is tested against a retained break anchor; the previous implementation
discarded that anchor after the first clear.

## Script and plugin compatibility

**Edit > Convert OpenFrames views for Qt**, or the `--convert-views` launch
option, explicitly converts supported `OpenFramesInterface` / `OpenFramesView`
definitions into Qt OrbitView settings. It preserves mission calculations,
validates the resulting script with the engine, and makes one undoable unsaved
edit. The source file is untouched until Save. Per-object labels/grids collapse
to plot-wide controls; the first selected view is imported. Perspective/FOV,
multiple-view switching, trajectory-relative orientation and body-relative
camera rotation are not equivalent. The converted script and message window
list these differences and every dropped viewer property. Unsupported object
kinds and dynamic viewer statements require manual conversion.

The original shipped Hohmann sample completed with `--convert-views` and the
same convergence values as the manual adaptation above. The workflow test
also checks that the mission-sequence text is unchanged and Undo restores the
original. `ofi-import.png` records the native completed mission.

![Converted original Hohmann sample](ofi-import.png)

**Help > Available engine types…** shows actual registered types from the
running engine. Registration means a type is available to scripts; it does not
promise a dedicated Qt panel or numerical validation of every optional plugin.
wx-only OpenFrames/OVtoOFI startup plugins are rejected with a clear diagnostic.
Qt startup preserves the available native engine plugins, including functions,
Yukon, event location, propagation, estimation and Python components.

| Area | Acceptance evidence |
| --- | --- |
| Resources and compound values | Workflow: numeric grids, hardware references, mixture ratios, owned gravity settings, attitude representation/state, validation, serialization and undo |
| Commands and solvers | Mission: source-preserving forms and actual DC convergence; Compatibility: changed function arguments, Yukon optimum and eclipse report |
| Scripted cameras | Plots: independent frame conversions; NativeOrbit at normal/HiDPI: tracking, up, scale, manual offsets and replay |
| Visualization | NativeOrbit: constellations, grids, wireframe, Sun line and physical body meshes; Plots: ground footprints, XY styles and repeated solver clears |
| Plugin / OFI workflow | Compatibility: real native-plugin calculations; Workflow: explicit conversion, preserved mission text, real shipped Hohmann and undo; native imported Hohmann screenshot |

`check-final.txt` contains the complete ten-test Linux Qt acceptance run.
Native graphics use Xvfb/xcb and software Mesa; headless tests exercise the
separate fallback paths. This does not establish hardware-driver coverage.

Retained design limits: orthographic Qt controls, five-degree wx-style ground
reference footprints (not sensor coverage), constellation outlines without
names/borders, and diffuse lighting without eclipse shadows. Advanced plugin
fields without a Qt control remain accessible in the script editor. Optional
plugins outside the exercised function/optimizer/event paths require their
own scientific acceptance tests before claiming numerical parity.


## Desktop GPU regression

On 2026-09-29, the default mission reproduced Intel Iris Xe (ADL GT2) GPU
hangs/context resets on GNOME Wayland with four-sample widget MSAA. The mission
contained 123 samples, but capture contained no Earth or trajectory pixels and
the test stalled before failing: [before log](desktop-msaa-before.txt).

Using a single-sample framebuffer with hardware acceleration retained, three
consecutive desktop runs passed in 2.7–2.9 seconds, including title-bar minimize,
Output activation, Earth/orbit pixel checks, restore and repeat minimize.
There were no kernel log entries during those runs: [after log](desktop-msaa-after.txt).
The [captured OrbitView](desktop-orbit.png) shows the texture, trajectory,
spacecraft and stars. The rebuilt application and tests use the same renderer;
[all 11 regression tests passed](check-msaa.txt). This covers the observed Intel
configuration, not every hardware/driver combination. The pixel check is now
part of `QtGui.NativeWindows` so a visible but empty view cannot pass it.
