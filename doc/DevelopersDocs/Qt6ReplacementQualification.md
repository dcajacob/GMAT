# Linux Qt replacement qualification: workflows, viewers, plugins and files

Status: in progress. This is the acceptance checklist for requested areas 3, 4,
and 5. Existing milestone tests do not establish full replacement qualification.
Windows/macOS deployment is deferred. MATLAB is outside the selected Linux
runtime; supported plugins below come from its actual startup configuration.

## Acceptance gates

- [ ] Audit each wx workflow below against Qt; implement missing user operations
      and record executable evidence. Shared generic controls can serve a workflow
      only when all its operations and validation are available.
- [ ] Array resizing and expression-valued cell editing with cancel, rollback,
      undo and calculation-preserving save/reopen.
- [ ] Resource reference selection, dependent settings, specialized spacecraft,
      force-model, propagation, coordinate-system, hardware and output workflows.
- [ ] Mission commands and branch editing through controls, preserving labels,
      comments, nested commands and unsupported syntax.
- [ ] Plotting option/callback audit, native and fallback correctness, replay,
      camera tracking and output lifecycle checks.
- [ ] OF conversion: perspective/FOV, multiple views, trajectory-relative and
      body-relative camera modes; preserve settings or explicitly diagnose an
      unresolved case. A warning alone does not count as implemented parity.
- [ ] Each selected plugin: configuration, execution, expected numerical/report
      result and error recovery through Qt, with fixture requirements recorded.
- [ ] Script round trips preserve calculations, expressions, unknown settings,
      comments and references; save/reopen, Save As, undo and failed-edit recovery.
- [ ] Rebuild the actual application/bin/GmatQt and pass relevant regression tests.

## wx workflow inventory

Every row begins unaudited; a panel's existence is not proof of Qt equivalence.

| wx source | Qt mapping / missing operations / evidence |
| --- | --- |
| `src/gui/hardware/ThrusterCoefficientDialog.hpp` | Pending audit |
| `src/gui/hardware/BurnThrusterPanel.hpp` | Pending audit |
| `src/gui/hardware/ThrusterConfigPanel.hpp` | Pending audit |
| `src/gui/hardware/PowerSystemConfigPanel.hpp` | Pending audit |
| `src/gui/hardware/TankAndMixDialog.hpp` | Pending audit |
| `src/gui/event/EventLocatorPanel.hpp` | Pending audit |
| `src/gui/command/TogglePanel.hpp` | Subscriber checklist and On/Off dropdown; empty selection, Cancel, filtering and dual-report suppression/resumption after save/reopen tested. Plot/ephemeris and solver-loop Toggle combinations pending. |
| `src/gui/command/GmatCommandPanel.hpp` | Pending audit |
| `src/gui/command/ManeuverPanel.hpp` | Typed impulsive-burn and spacecraft selectors; Cancel, label/comment preservation, save/reopen and inertial delta-V execution tested. Backprop checkbox and reverse inertial delta-V tested; other frames and mass-decrement cases pending. |
| `src/gui/command/ScriptEventPanel.hpp` | Pending audit |
| `src/gui/command/NonlinearConstraintPanel.hpp` | Optimizer selector and single-parameter left/right operand browser provided. Solver selector tested; constraint operand execution combinations pending. |
| `src/gui/command/AchievePanel.hpp` | Boundary-value solver selector plus single-parameter goal/value browser. Selected target variable, Cancel, save/reopen and solved result tested; full tolerance/property combinations pending. |
| `src/gui/command/ManageObjectPanel.hpp` | Global/Clear/Save object checklists added. Global automatic-resource filtering, Clear Cancel and Save export/reopen/recovery tested. Global/Clear runtime scope semantics pending. |
| `src/gui/command/BeginFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; selected ten-second constant-thrust burn, analytic fuel consumption and save/reopen tested. Other thruster/tank models pending. |
| `src/gui/command/OptimizePanel.hpp` | Optimizer selector excludes boundary-value solvers; selector tested. Broader Optimize workflow pending. |
| `src/gui/command/TargetPanel.hpp` | Boundary-value solver selector excludes optimizers; selector tested. Broader Target workflow pending. |
| `src/gui/command/VaryPanel.hpp` | Solver selector offers boundary-value solvers and optimizers; tested with DC and Yukon. Full variable/options workflow pending. |
| `src/gui/command/FindEventsPanel.hpp` | Event-locator selector tested with EclipseLocator; full execution/options workflow pending. |
| `src/gui/command/PropagatePanel.hpp` | Pending audit |
| `src/gui/command/AssignmentPanel.hpp` | CommandForm destination/expression controls; source-preservation tests. Parameter chooser and complex syntax audit pending. |
| `src/gui/command/CallFunctionPanel.hpp` | Function resource selector added alongside existing input/output controls. Selector-specific and broader function workflow qualification pending. |
| `src/gui/command/EndFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; fuel remains constant during coast after selected EndFiniteBurn. Other thruster/tank models pending. |
| `src/gui/command/MinimizePanel.hpp` | Optimizer selector and single-parameter objective browser provided. Solver selector tested; objective-browser execution combinations pending. |
| `src/gui/command/ReportPanel.hpp` | Configured report-file picker and shared ordered parameter dialog: add/remove/reorder, numeric array indices, Cancel, labels/comments and numerical output tested. Object/property and coordinate/central-body browsing implemented; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types pending. |
| `src/gui/function/MatlabFunctionSetupPanel.hpp` | Pending audit |
| `src/gui/function/FunctionSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverVariablesPanel.hpp` | Pending audit |
| `src/gui/solver/SolverSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverGoalsPanel.hpp` | Pending audit |
| `src/gui/solver/DCSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverCreatePanel.hpp` | Pending audit |
| `src/gui/solver/SQPSetupPanel.hpp` | Pending audit |
| `src/gui/controllogic/ForPanel.hpp` | CommandForm index/start/step/end; real-engine step edit and Undo tested. Parameter selection audit pending. |
| `src/gui/controllogic/ConditionPanel.hpp` | Comparison-row builder with numeric/parameter/array operands, six relations, AND/OR, add/remove and shared operand browser. Cancel, incomplete rows, header-only replacement, real If execution and Undo tested; grouped/expression syntax stays in text. Broader parameter selection and While execution cases pending. |
| `src/gui/spacecraft/OrbitPanel.hpp` | Pending audit |
| `src/gui/spacecraft/PowerSystemPanel.hpp` | Pending audit |
| `src/gui/spacecraft/BallisticsMassPanel.hpp` | Pending audit |
| `src/gui/spacecraft/TankPanel.hpp` | Pending audit |
| `src/gui/spacecraft/OrbitDesignerDialog.hpp` | Pending audit |
| `src/gui/spacecraft/VisualModelPanel.hpp` | Pending audit |
| `src/gui/spacecraft/AttitudePanel.hpp` | Pending audit |
| `src/gui/spacecraft/SpaceObjectSelectDialog.hpp` | ResourceEditor engine-typed reference picker. Ordered tank selection, Cancel, pending state and mixture-preserving Apply tested; all object-specific uses still need audit. |
| `src/gui/spacecraft/OrbitSummaryDialog.hpp` | Pending audit |
| `src/gui/spacecraft/SpacecraftPanel.hpp` | Pending audit |
| `src/gui/spacecraft/ThrusterPanel.hpp` | Pending audit |
| `src/gui/spacecraft/SpicePanel.hpp` | Pending audit |
| `src/gui/spacecraft/FormationSetupPanel.hpp` | ResourceEditor Add list and spacecraft picker; invalid member rejection, reordering, save/reopen and two-member propagation tested. Remaining wx-specific operations under audit. |
| `src/gui/foundation/GmatBaseSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatDialog.hpp` | Pending audit |
| `src/gui/foundation/ParameterCreateDialog.hpp` | Pending audit |
| `src/gui/foundation/ParameterSelectDialog.hpp` | Pending audit |
| `src/gui/foundation/SinglePathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatPanel.hpp` | Pending audit |
| `src/gui/foundation/MultiPathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatColorPanel.hpp` | Pending audit |
| `src/gui/foundation/ArraySetupDialog.hpp` | Shared numeric grid; see ArraySetupPanel. Full wx dialog audit pending. |
| `src/gui/foundation/ShowScriptDialog.hpp` | Pending audit |
| `src/gui/foundation/GmatSavePanel.hpp` | Pending audit |
| `src/gui/foundation/ParameterSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/ArraySetupPanel.hpp` | ResourceEditor resizeable numeric grid; retained/new cells, cancel, ragged/nonfinite rejection, reconstruction and Undo tested. Expression workflow pending. |
| `src/gui/foundation/ShowSummaryDialog.hpp` | Pending audit |
| `src/gui/propagator/PropagationConfigPanel.hpp` | Owned propagator settings exposed; numerical/TLE step edits and serialization tested. Specialized layout and remaining settings pending. |
| `src/gui/propagator/PropagatorSelectDialog.hpp` | Pending audit |
| `src/gui/asset/GroundStationPanel.hpp` | Pending audit |
| `src/gui/debugger/InspectorPanel.hpp` | Pending audit |
| `src/gui/forcemodel/DragInputsDialog.hpp` | Pending audit |
| `src/gui/coordsystem/CoordSysCreateDialog.hpp` | Basic creation plus dedicated Axes dialog tested; MOEEq epoch and constrained-frame edits checked. Remaining origin and specialized-mode cases pending. |
| `src/gui/coordsystem/CoordSystemConfigPanel.hpp` | Axis replacement, dependent field exposure, protected built-ins, failed-edit rollback, Undo and save/reopen tested. Broader modes pending. |
| `src/gui/coordsystem/CoordPanel.hpp` | ObjectReferenced radial frame, MOEEq epoch edits and Sun-aligned LocalAlignedConstrained transforms checked, including save/reopen. Other modes and dependency cases pending. |
| `src/gui/output/ReportFilePanel.hpp` | Read-only, unwrapped report text, full path in title, text selection, close and unavailable-file handling tested. Standard Qt copy controls provided; large reports are limited to a 16 MiB preview. |
| `src/gui/output/EventFilePanel.hpp` | Pending audit |
| `src/gui/output/CompareReportPanel.hpp` | Pending audit |
| `src/gui/mission/UndockedMissionPanel.hpp` | Pending audit |
| `src/gui/mission/TreeViewOptionDialog.hpp` | Pending audit |
| `src/gui/view/ViewTextDialog.hpp` | Pending audit |
| `src/gui/view/FindReplaceDialog.hpp` | Nonmodal Find/Replace with next/previous, wrap, session histories, selected replacement and Replace All. Case/whole-word controls, no-match feedback, read-only protection and single-operation Undo tested. |
| `src/gui/solarsys/LibrationPointPanel.hpp` | Pending audit |
| `src/gui/view/EditorPanel.hpp` | Pending audit |
| `src/gui/app/CompareFilesDialog.hpp` | Pending audit |
| `src/gui/app/ScriptPanel.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyOrientationPanel.hpp` | Pending audit |
| `src/gui/solarsys/UniversePanel.hpp` | Pending audit |
| `src/gui/solarsys/CelesBodySelectDialog.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyPanel.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyVisualizationPanel.hpp` | Pending audit |
| `src/gui/subscriber/GroundTrackPlotPanel.hpp` | Pending audit |
| `src/gui/subscriber/XyPlotSetupPanel.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyPropertiesPanel.hpp` | Pending audit |
| `src/gui/solarsys/BarycenterPanel.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyOrbitPanel.hpp` | Pending audit |
| `src/gui/app/CompareTextDialog.hpp` | Pending audit |
| `src/gui/subscriber/EphemerisFilePanel.hpp` | Pending audit |
| `src/gui/burn/FiniteBurnSetupPanel.hpp` | Pending audit |
| `src/gui/burn/ImpulsiveBurnSetupPanel.hpp` | Pending audit |
| `src/gui/app/FileUpdateDialog.hpp` | Pending audit |
| `src/gui/app/TextEphemFileDialog.hpp` | Pending audit |
| `src/gui/subscriber/TsPlotOptionsDialog.hpp` | PlotWidget Style dialog: per-curve visibility, lines/markers, widths, marker sizes/shapes, line styles, colors and error bars; plot grid/legend. Cancel, existing-point styling, curve isolation and rendered differences tested. Remaining axes/range options pending audit. |
| `src/gui/subscriber/OrbitViewPanel.hpp` | Pending audit |
| `src/gui/app/RunScriptFolderDialog.hpp` | Pending audit |
| `src/gui/subscriber/OpenGlOptionDialog.hpp` | Pending audit |
| `src/gui/subscriber/SubscriberSetupPanel.hpp` | Pending audit |
| `src/gui/subscriber/DynamicDataDisplaySetupPanel.hpp` | Pending audit |
| `src/gui/subscriber/ReportFileSetupPanel.hpp` | Report parameter lists accept numeric array elements; readable delimiter selector, precision/width rejection, exact save/reopen and explicit/automatic report values tested. Append across repeat runs, fixed-width headers, left/right alignment and zero fill tested. Shared ordered parameter selector added; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types and solver-iteration combinations pending. |
| `src/gui/subscriber/DynamicDataSettingsDialog.hpp` | Pending audit |
| `src/gui/app/WelcomePanel.hpp` | Pending audit |
| `src/gui/app/AboutDialog.hpp` | Pending audit |
| `src/gui/app/InteractiveMatlabDialog.hpp` | Pending audit |
| `src/gui/app/SetPathDialog.hpp` | Pending audit |

## Selected runtime plugin inventory

Every row requires real-engine evidence, not just registration.

| Plugin | Configuration / execution / reports / recovery evidence |
| --- | --- |
| `../plugins/libDataInterface` | Pending qualification |
| `../plugins/libEphemPropagator` | Pending qualification |
| `../plugins/libEKF` | Pending qualification |
| `../plugins/libGmatEstimation` | Pending qualification |
| `../plugins/libEventLocator` | CompatibilityTests: edited eclipse lists, exact save/Save As/reopen, invalid-type build recovery, eclipse intervals and Output report access. Contact and remaining locator workflows pending. |
| `../plugins/libExternalForceModel_py314` | Pending qualification |
| `../plugins/libExtraPropagators` | BulirschStoer: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Remaining cases pending. |
| `../plugins/libFormation` | CompatibilityTests: Add editing/reordering, non-spacecraft rejection, exact save/Save As/reopen and failed-build recovery, both members propagate 60 seconds. Remaining settings/output coverage pending. |
| `../plugins/libGmatFunction` | CompatibilityTests: edited cross-product arguments, exact save/Save As/reopen, invalid-type build recovery, expected numerical cross product. Broader function and report audit pending. |
| `../plugins/libMsise00` | Pending qualification |
| `../plugins/libNewParameters` | Pending qualification |
| `../plugins/libPolyhedronGravity` | Pending qualification |
| `../plugins/libProductionPropagators` | PrinceDormand853: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Remaining cases pending. |
| `../plugins/libPythonInterface_py314` | Shipped Python example: exact save/Save As/reopen and failed-build recovery, independently computed cross-product result and report. Remaining call types and runtime-error recovery pending. |
| `../plugins/libSaveCommand` | CompatibilityTests: edited object list, exact save/Save As/reopen, runtime spacecraft/variable export and reload, repeat-run replacement, loop snapshots, bad-path and disk-write recovery. Remaining resource types and multi-snapshot reimport pending. |
| `../plugins/libScriptTools` | Pending qualification |
| `../plugins/libStation` | Pending qualification |
| `../plugins/libThrustFile` | Pending qualification |
| `../plugins/thinksys/libTLEPropagator` | Shipped example: step edit, exact save/Save As/reopen, report epoch/state, sampling invariance, invalid-build and missing-file recovery. Broader settings/reference ephemeris comparison pending. |
| `../plugins/libYukonOptimizer` | CompatibilityTests: shipped algebraic optimization, exact save/Save As/reopen, invalid-type build recovery, analytic optimum and report. Additional settings/error modes pending. |

## Implementation checkpoint 1

Added command controls for For, If/While, assignments, Toggle and Global/Clear.
Header edits preserve nested source. Actual engine-normalized If headers omit
semicolons; both forms are covered. Tests execute changed loop/condition values
and verify Undo restores the original numerical result.

Array grids now resize: retained cells survive, new cells are zero, shrinking
removes cropped cells only from the dialog until accepted. Apply reconstructs
and validates the mission; Undo restores dimensions and values. Non-array
vector/matrix dimensions remain fixed. Expression cells are still pending.

All 11 Qt tests passed (18.70 seconds), including the new workflow assertions;
see `Qt6ParityValidation/check-qualification-1.txt`. This checkpoint does not
close any of the three requested qualification areas.

## Plugin/file checkpoint

The function, optimizer and eclipse fixtures now save, Save As to a Unicode
filename, deliberately fail interpretation, reopen and rebuild the saved script,
and only then run their numerical/report checks. Both saved files must match
the edited script exactly. The targeted compatibility test passed; see
`Qt6ParityValidation/plugin-roundtrip.txt`. This establishes these workflows,
not qualification of the remaining plugin inventory.

## Reference-picker and plugin checkpoint

Resource properties now offer file browsing and engine-typed scalar/list
reference selection. Ordered lists retain current members first (including
existing names absent from the offered set), support drag reordering and do not
apply until Apply. Tests cover tank order/mixture ratios, Cancel, pending state,
EarthFixed selection and choosing a new output filename without creating it.
Free text remains available for parameter expressions and plugin-defined types.

Qualification exposed the missing Formation.Add editor. It now accepts only
existing spacecraft and supports reconstruction through the normal undoable
Apply path. The compatibility test verifies member reordering, rejection of a
propagator as a member, exact file round trips and actual 60-second propagation
of both spacecraft. The shipped Python interface example also runs after file
round trips/recovery, with an independently calculated cross product and report.
These results expand evidence; the remaining plugin and viewer gates stay open.

The rebuilt Linux application passed all 11 tests in 19.07 seconds; evidence:
`Qt6ParityValidation/check-qualification-2.txt`.

## XY viewer control checkpoint

Qt now exposes its implemented XY styles through a Style dialog. Each curve has
its own controls, and changing the selected curve retains other pending edits.
OK applies; Cancel changes neither model nor pixels. Color/marker changes update
existing samples as well as the curve defaults. Styles are session-local and
are reset by rebuilding the mission; the dialog states that scope explicitly.

PlotTests verifies canceled edits preserve the captured image, accepted edits
change the image and selected curve, and another curve stays unchanged. The
same fixture runs under software-native and HiDPI configurations. Source audit
also confirmed wx ignores the index argument for ChangeWidth/ChangeStyle
(`GuiPlotReceiver.cpp`); Qt's whole-curve behavior matches that baseline.
Camera projection, multiple views and OF-relative camera conversion remain open.

All 11 tests passed in 19.53 seconds and GmatQt was rebuilt; evidence:
`Qt6ParityValidation/check-xy-style.txt`.

## Perspective camera checkpoint

Added interactive perspective projection and vertical FOV controls while keeping
orthographic as the default. Native rendering uses a perspective frustum;
fallback projection applies depth scaling and rejects objects behind the eye.
The fallback pan offset uses the camera plane, and native labels use clip-space
bounds rather than drawing labels behind the camera. Perspective sky geometry
uses the camera FOV/zoom and remains translation invariant.

Native tests cover FOV framing, depth-dependent body size, behind-camera
clipping, exact orthographic restoration and star translation/zoom behavior.
PlotTests exercises the actual projection/FOV controls and rendered changes in
both fallback and native/HiDPI configurations. OF FOV import, persistent camera
settings, multiple views and relative camera modes are still open; interactive
projection alone does not satisfy those conversion requirements.

All 11 tests passed in 20.90 seconds; GmatQt was rebuilt. Evidence:
`Qt6ParityValidation/check-perspective.txt`.

## OpenFrames projection import checkpoint

The converter now imports the first selected OF view's perspective projection
and FOVy (default 45 degrees), preserving fractional angles. Settings are stored
in a `% GMAT-Qt-Camera` JSON comment, so the base engine can still interpret the
converted calculations. Qt validates plot names, booleans, finite FOV from 1 to
150 degrees and duplicate directives before using them. Invalid or unsupported
FOV is rejected rather than clamped during conversion.

Receiver-created orbit models get these settings before their widgets are
created. GUI reconstruction retains directives when engine serialization omits
them. Tests cover defaults, fractional FOV, invalid values, duplicate directives,
the real Hohmann renderer model, resource edit/Undo, and save/reopen/run. This
closes basic OF perspective/FOV import, but not multiple-view selection, relative
orientation, or saving camera changes made with the interactive controls.

Rebuilt GmatQt and passed all 11 tests in 20.59 seconds; evidence:
`Qt6ParityValidation/check-fov-import.txt`.

## Interactive projection persistence checkpoint

OrbitView now has Keep projection. It adds/replaces that plot's validated camera
comment as one undoable editor change and updates the built-source snapshot,
without interpreting the mission or deleting the viewer during its callback.
The file is not written until the normal Save action. Running missions, unbuilt
script edits, pending panel edits and missing/non-OrbitView resources are rejected.

WorkflowTests exercises the actual toolbar action, fractional FOV, switching to
orthographic, repeated clicks without duplicate directives, unchanged mission
sequence, unchanged engine object identity, Undo/Redo, protection of unbuilt
edits, and save/reopen/run restoration. This closes projection/FOV persistence;
manual orbit angles/pan/zoom, multiple views and relative camera modes are still
outstanding, alongside the other workflow and plugin qualification gates.

GmatQt rebuilt; all 11 tests passed in 20.17 seconds. Evidence:
`Qt6ParityValidation/check-keep-projection.txt`.

## Expression-cell checkpoint

Added an Expressions grid to Array resources. General formulas are GMAT mission
commands, so new formula blocks initialize cells at mission start in row order;
the dialog states this timing explicitly. Numeric initial values stay separate.
Formula blocks preserve text across Apply, Undo/Redo and save/reopen. Existing
mission assignments remain untouched. Unrecognized statements inside a managed
block are refused instead of erased; invalid cells/formulas restore the previous
mission. Resource editing now preserves the original mission section verbatim.

WorkflowTests opens the grid, exercises Cancel and Apply, evaluates dependent
cells to an independently expected result, verifies Undo/Redo, unrelated resource
editing, save/reopen and reopened grid text, clears formulas and undoes that,
and verifies extra commands/unknown references are rejected without changing
source or runtime results. Numeric and expression edits currently require
separate Apply operations; arbitrary existing assignments use Mission editing.
The remaining specialized-editor, multi-camera and plugin gates stay open.

GmatQt rebuilt; all 11 tests passed in 21.53 seconds. Evidence:
`Qt6ParityValidation/check-array-expressions.txt`.


## Propagator plugin checkpoint

Resource panels now enumerate writable fields of a PropSetup's owned propagator,
including plugin settings hidden by the parent forwarding table. Edits target the
owned object. Resource serialization excludes the separately written force model
when matching propagator blocks; previously numerical-propagator Apply could fail
with the specialized-editor message despite having a supported property.

CompatibilityTests runs the shipped TLE mission, checks a one-day report span,
finite low-Earth-orbit state and matching spacecraft epoch, edits sampling from
300 to 120 seconds, verifies endpoint invariance, and recovers from a missing TLE
file without changing the result. BulirschStoer and PrinceDormand853 each undergo
step editing, exact save/Save As/reopen and invalid-build recovery, then propagate
a circular point-mass Earth orbit against an analytic endpoint within 0.1 metre.
Both produce reports. These checks qualify these scenarios, not every plugin
option or propagation regime. Specialized workflows and other inventory gaps
remain open.

The user's GmatQt executable was rebuilt. All 11 Qt tests passed in 21.91 seconds.
Evidence: `Qt6ParityValidation/check-plugin-propagators.txt`.


## Camera roll checkpoint

OF conversion now preserves the selected view's arbitrary up vector in validated
Qt camera metadata. The receiver applies it in ViewUpCoordinateSystem to every
camera-history sample, so playback uses the same roll as propagation. A nearest
standard axis remains in the script as a base-viewer fallback. Invalid and zero
vectors are rejected instead of approximated. Keep projection preserves the
vector; an explicit resource-panel ViewUpAxis edit removes that override.

WorkflowTests covers exact non-axis conversion, invalid metadata, real Hohmann
propagation with an arbitrary up vector at the first and last camera frames,
resource-edit retention, save/reopen, Keep projection retention, explicit axis
override, and Undo restoring the imported vector. Multiple-view switching and
body-/trajectory-relative orientation remain pending; this closes the arbitrary
up-vector approximation only.

GmatQt rebuilt; all 11 Qt tests passed in 31.38 seconds. Evidence:
`Qt6ParityValidation/check-camera-up.txt`.


## Multiple-camera checkpoint

Conversion now retains the full ordered OF View list as a first standard
OrbitView camera plus named Qt camera definitions. Each additional view has
projection/FOV, eye/center/up vectors, reference and target objects. Qt validates
all views and references before execution. The receiver records bounded histories
for each view and clears them on mission/solver reset. The camera selector uses
those histories for replay and does not reinterpret or rerun the mission.
Keep projection edits the selected view only, preserving the others.

WorkflowTests covers stored Current camera vectors, malformed secondary views,
the shipped Hohmann three-view list, per-frame Earth/spacecraft tracking,
replay basis changes, unchanged mission object identity/frame count on selection,
independent projection settings, save/reopen, invalid-reference build rejection
and recovery. PlotTests verifies visible switching and exact restoration in both
fallback and native/HiDPI renderers, plus history cleanup. Existing resource edits
and Undo retain the full camera metadata.

This implements named view switching and translation tracking. Body-relative
rotation, trajectory/segment views and original automatic framing remain open.
The current unstored-location fallback is explicitly reported as 30000 km; it is
not counted as OF automatic-framing parity. Base viewer compatibility retains
the first camera; additional definitions are Qt comments.

The user executable was rebuilt; all 11 Qt tests passed in 22.99 seconds.
Evidence: `Qt6ParityValidation/check-multiple-cameras.txt`.


## Body-relative camera checkpoint

Primary and additional OF views now retain the InertialFrame mode. With that
setting Off and ViewTrajectory Off, their eye offsets, center offsets and up
vectors rotate with the reference object's attitude into the plot coordinate
system. Celestial and spacecraft attitude conventions are handled separately.
InertialFrame On retains position tracking with plot-frame axes. Primary center
offsets are retained, and an explicit ViewDirection edit clears that override.
Validation rejects incompatible primary reference/eye types before execution.

WorkflowTests checks the primary Earth camera against independent EarthFixed
coordinate conversions, including nonzero center and non-axis up vectors at the
first and last samples, both in inertial and rotating plot frames. Additional
Earth and spinning-spacecraft views match the recorded body orientation and
position histories. Save/reopen retains the poses; incompatible reference edits
are rejected and the previous mission reruns unchanged. Conversion also verifies
InertialFrame On and rejects malformed primary stored vectors.

Trajectory/segment views, two-frame look-at ShortestAngle/AZEL orientation and
original automatic framing remain pending. These cases retain explicit conversion
notes and are not counted as completed parity.

The user executable was rebuilt; all 11 Qt tests passed in 24.63 seconds.
Evidence: `Qt6ParityValidation/check-body-cameras.txt`.


## Two-frame look-at checkpoint

LookAtFrame now imports OF's full camera-pose alignment instead of setting only
the target position. ShortestAngle On maps to direct shortest rotation; Off maps
to azimuth then elevation, with OF's singular-case threshold. Both primary and
additional views combine this alignment with their body-relative/absolute frame.
The stored center remains local to the reference frame. Explicit ViewDirection
editing clears the primary alignment; Undo restores it. Mode values and metadata
booleans are validated rather than silently interpreted as Off.

The optional OpenFramesCameraReference probe compares the shared Qt alignment
helper to actual OpenFrames FollowingTrackball transforms: 48 frame/mode/direction
cases, four points each, including coincident origins, near-pole cases and reverse
directions. Maximum point error was 2.06801e-12 scene units. The reference library
is `/home/dan/GIT/OpenFramesInterface/dep/OpenFrames-git/installed/lib/libOpenFrames.so`;
the source checkout HEAD inspected was `037643090d42c3ff804584f69b13b3b6511a8b66`.
This probe adds no OpenFrames dependency to the Qt application.

WorkflowTests exercises primary and additional Earth/spacecraft look-at views
in both modes, checks complete eye/center orientation and perpendicular up vectors
at the first and last samples, confirms the modes produce different roll, and
verifies save/reopen, explicit-direction override and Undo. Trajectory/segment
views and original automatic framing remain pending, along with the other
workflow/plugin qualification gates.

The user executable was rebuilt; all 11 Qt tests passed in 25.27 seconds.
Evidence: `Qt6ParityValidation/check-look-at.txt` and
`Qt6ParityValidation/look-at-reference.txt`.


## Save-command workflow/plugin checkpoint

Qt exposes Save in the command templates when its plugin is registered and edits
its object list through CommandForm. Actual export qualification found two plugin
defects: stream failures were ignored, and copied Save commands left fileArray
uninitialized. Save now initializes copied stream ownership, disposes prior
streams before reinitialization/assignment, checks open/flush/close results, and
closes streams on exceptions. Export restores original object comment flags even
when serialization fails. These fixes apply to the shared Save plugin, including
its use outside Qt.

CompatibilityTests edits a Save command to export a spacecraft and variable,
saves/reopens the mission exactly, executes it, then opens the exported resource
file through Qt and checks current numeric values. Repeated runs replace previous
output; a loop retains both snapshots. A nonexistent output path must fail, then
recover to identical output. On Linux, a temporary symlink to /dev/full verifies
post-open write failure and recovery. The fixture only modifies temporary outputs.
Broader resource-type export and multi-snapshot reimport remain pending.

GmatQt and the Save plugin were rebuilt; all 11 Qt tests passed in 26.55 seconds.
Evidence: `Qt6ParityValidation/check-save-command.txt`.


## Coordinate-system workflow checkpoint

The audit found that CoordinateSystem::SetStringParameter("Axes", ...) accepts
text without replacing the owned axes, and the generic Qt editor omitted owned
axis properties. Qt now creates/replaces the owned AxisSystem on a proposal clone,
exposes its writable fields and retains built-in protection. A dedicated Axes
dialog lets type and dependent fields be edited together before applying; a type
change rebuilds the pending form, and Cancel does not touch the configured model.
Primary/secondary/reference objects get pickers and R/V/N directions get choices.

As in the wx panel, ObjectReferenced changes require distinct primary/secondary
objects and exactly two different directions. This is checked before accepting
the edit because engine interpretation alone can defer geometry errors until an
actual coordinate conversion. WorkflowTests covers creation, dialog Cancel,
changing to ObjectReferenced with dependent fields, type-only MJ2000Ec edits,
owned field exposure, invalid directions, protected built-ins and Undo. The edited
R/N frame transforms the propagated spacecraft position to [radius,0,0], and that
numerical result survives save/reopen and another mission run.

The three wx coordinate-panel inventory rows are now partially audited. Additional
origin/dependency combinations and specialized axis modes remain pending and are
not counted as fully qualified.

GmatQt was rebuilt; all 11 Qt tests passed in 25.49 seconds. Evidence:
`Qt6ParityValidation/check-coordinate-axes.txt`.

### Epoch and constrained-frame follow-up

WorkflowTests edits MOEEq's owned Epoch in A1ModJulian, verifies that its orientation
is fixed across evaluation times, preserves vector length, and changes when the
configured epoch changes. Save/reopen preserves the calculated transform.

A LocalAlignedConstrained frame uses Sun as its alignment reference and
EarthMJ2000Eq as its constraint frame. Its transformed X direction matches the
independently calculated Earth-to-Sun unit vector; Z matches inertial Z projected
perpendicular to that direction (component tolerance 1e-10). The same checks pass
after save/reopen and another run. Reference choices are checked as well.

Qt now rejects static degenerate geometry before applying an edit: zero or
nonfinite vectors, parallel alignment/constraint vectors, reference equal to
origin, and self-referencing constraint frames. Rejected edits preserve the script.
The vector thresholds match the engine's 1e-9 geometry tolerance. Other epoch
models, indirect dependency cycles, non-Earth origins and time-varying constraint
singularities remain unqualified.

GmatQt was rebuilt; all 11 Qt tests passed in 26.08 seconds. Evidence:
`Qt6ParityValidation/check-constrained-axes.txt`.

## Report configuration checkpoint

The wx ReportFileSetupPanel audit found a Qt list parsing gap: array elements
were split at their internal comma and rejected. ReportFile Add now accepts
positive numeric array indices and the selection list preserves those entries.
The engine still resolves parameters and checks array bounds during candidate
interpretation. Delimiters now have readable labels, map back to literal
characters, and reject values the engine would silently truncate.

CompatibilityTests verifies array-element list edits and order, no initial dirty
state in the delimiter control, Tab selection, invalid setting rollback, exact
save/Save As/reopen with invalid-build recovery, and numerical output with
precision 6 and comma/tab delimiters. Automatic reporting during propagation
checks that edited Add order and array values reach the file; explicit Report
commands retain their own parameter order. Full parameter selection, append,
fixed-width formatting and solver-iteration combinations remain pending.

GmatQt was rebuilt; all 11 Qt tests passed in 49.81 seconds. Evidence:
`Qt6ParityValidation/check-report-settings.txt`.


### Report formatting and output lifecycle follow-up

CompatibilityTests now exercises fixed-width reports after settings edits and
save/reopen: width 18 plus GMAT's three separator spaces, headers, left/right
alignment, precision 6 and zero-filled significant digits. Checks compare complete
lines, including padding, against expected values. An ordinary second run replaces
the file exactly; enabling AppendToExistingFile preserves the prior file through
configuration/reopen and adds one complete report (including headers) per run.
A missing file is recreated correctly in append mode.

The Output report viewer now includes the full output path in its window title,
as wx does. Tests open the generated report through Output, compare all displayed
text with the file, verify read-only/no-wrap behavior and text selection, close it,
and verify that a missing report produces a diagnostic without an empty viewer.
The existing 16 MiB preview limit remains; whole-file viewing of larger reports
is not qualified as wx-equivalent. Clipboard contents are not changed by tests.

GmatQt was rebuilt; all 11 Qt tests passed in 28.06 seconds. Evidence:
`Qt6ParityValidation/check-report-lifecycle.txt`.


### Shared report parameter selector

The Report command now offers a configured ReportFile picker and an ordered
parameter dialog. ReportFile Add uses the same dialog. It lists configured
reportable parameters, accepts typed references, adds whole arrays or individual
cells with bounds taken from the array, and supports removal, Up/Down and drag
reordering. The dialog edits a local selection; Cancel leaves its caller unchanged.
Command Apply continues to validate references through the engine.

CompatibilityTests exercises the actual command picker and dialog, array bounds,
remove/add/reorder, Cancel, exact label/comment preservation, application to the
mission and numerical results after save/reopen. It also verifies the resource
panel opens the shared dialog without splitting indexed parameters and receives
the reordered list. Property references not already configured can be entered,
but this checkpoint did not yet include object/property/dependency browsing (added
in the follow-up below) or qualify the full ParameterSelectDialog workflow. Drag ordering uses Qt's standard
InternalMove implementation; the executable test uses explicit Up/Down controls.

All 11 Qt checks passed in 27.49 seconds. After wrapping the selector's help text
for a reasonable dialog width, GmatQt was rebuilt and Compatibility passed again
in 2.39 seconds. Evidence: `Qt6ParityValidation/check-report-picker.txt` and
`Qt6ParityValidation/report-picker-final.txt`.


### Report object/property browser

The shared selector now builds object/property choices from ParameterInfo's
reportable types and registered owner objects. Independent properties generate
Owner.Property references. Coordinate-system, central-body and force-model
properties expose the corresponding dependency selection. Coordinate choices
honor metadata requirements for BodyFixed axes and celestial-body origins. Use
reference fills the editable parameter entry; Add parameter commits it only to
the dialog's local list. Browsing does not reconstruct or mutate the mission.

CompatibilityTests selects BrowserSat.EarthMJ2000Eq.X, BrowserSat.Earth.RMAG and
BrowserSat.ElapsedSecs using the browser controls, applies the resulting Report
command and verifies [7000,7000,0] after exact save/Save As/reopen and invalid-build
recovery. PlanetodeticLAT choices include a user-created Earth BodyFixed frame and
exclude EarthMJ2000Eq. Force-model dependency choices are implemented but their
numerical scenarios remain pending, as do owned/attached hardware parameters and
broader plugin-property coverage.

GmatQt was rebuilt; all 11 Qt checks passed in 28.72 seconds. Evidence:
`Qt6ParityValidation/check-report-property-browser.txt`.


### Attached hardware and owned attitude follow-up

The property browser now includes attached hardware and owned attitude parameter
types. Attached dependencies are filtered by both engine type and the selected
owner's direct object-reference fields. Transitive references from a thruster do
not establish spacecraft attachment. Missing hardware (or unknown required type)
leaves Use reference disabled. Owned attitude properties generate Owner.Property
without an unnecessary attitude object name.

CompatibilityTests selects BrowserSat.Q4, BrowserSat.BrowserTank.FuelMass and
BrowserSat.BrowserThruster.C1 alongside position/time parameters. It checks the
expected unit scalar quaternion, fuel mass 123.5 and coefficient 12.5 in the
actual report after save/Save As/reopen and invalid-build recovery. Spare tank
and thruster resources are excluded; switching to an unattached spacecraft
clears the dependency list and disables reference generation. Broader attitude
models, power systems, plates, electric thrusters and plugin hardware scenarios
remain unqualified.

All 11 Qt checks passed in 58.32 seconds. After the unknown-hardware-type guard,
GmatQt was rebuilt and Compatibility passed again in 2.25 seconds. Evidence:
`Qt6ParityValidation/check-report-hardware-browser.txt` and
`Qt6ParityValidation/report-hardware-browser-final.txt`.


### Force-model rate browser and engine corrections

Force-model parameter testing exposed an internal/script name mismatch: the
SMADot constructor registered EquinoctialSMADot, while the factory and dependency
metadata use SMADot. The constructor now uses the public factory name, allowing
the browser to offer a valid property and populate force-model choices.

The next numerical check failed because OrbitData returned zero TLONGDot when
perturbing acceleration was exactly zero. The shared engine now initializes that
rate to |r cross v|/r^2 in degrees/second; the existing perturbed calculation remains
in place. This corrects an engine result, not merely Qt display behavior.

CompatibilityTests selects SMADot and TLONGDot with RateForces through the browser,
applies the Report command, saves/reopens with invalid-build recovery and runs an
Earth point-mass mission. At radius 7000 km it checks zero semimajor-axis rate and
true-longitude rate v/r for circular motion, then edits tangential speed to 1.1
times circular speed and repeats the round trip and numerical checks. Rate
component tolerances are 1e-10 for SMADot and 1e-12 degrees/second for TLONGDot.
Perturbed-rate scenarios and other force-model rate components remain pending.

The original zero-rate failure is recorded in
`Qt6ParityValidation/force-rates-before.txt`. GmatQt and the shared engine were
rebuilt; all 11 Qt checks passed in 27.23 seconds. Evidence:
`Qt6ParityValidation/check-force-rates.txt`. This is targeted numerical and Qt
regression evidence, not a complete requalification of the shared engine.


## Typed command resource selectors

CommandForm now provides Select controls for impulsive/finite burns, Maneuver's
spacecraft, solvers, event locators and functions. Choices are filtered by engine
type: Target/Achieve use BoundaryValueSolver, Optimize/Minimize/constraints use
Optimizer, and Vary accepts both. Finite-burn spacecraft fields initially remained
text-only; the follow-up below verifies the engine single-spacecraft restriction
and adds their picker.

MissionTests uses the actual modal pickers to change an impulsive burn and its
spacecraft, checks Cancel plus label/comment preservation, applies the command,
saves/reopens it and verifies a 0.01 km/s inertial X velocity increment on the
selected spacecraft. It checks inclusion/exclusion for finite burns, target and
optimization solvers, Vary, constraints and eclipse locators. These selector
checks do not by themselves qualify every command's options or execution path.

GmatQt was rebuilt; all 11 Qt checks passed in 41.87 seconds. Evidence:
`Qt6ParityValidation/check-command-pickers.txt`.


### Finite-burn spacecraft and Backprop controls

BeginFiniteBurn::SetRefObjectName explicitly rejects multiple spacecraft. Qt now
provides a single-spacecraft selector in both BeginFiniteBurn and EndFiniteBurn,
consistent with that engine restriction. CompatibilityTests selects BurnSat for
both commands through the dialogs, saves/reopens, and runs ten seconds of constant
100 N thrust at Isp 300 seconds followed by ten seconds of coast. Tank mass matches
150 - thrust*time/(Isp*g0) within 1e-7 kg, and changes by less than 1e-10 kg after
EndFiniteBurn. The configured thruster supplies g0 for the independent mass-flow
calculation. Other thruster/tank models and coupled finite-burn workflows remain
pending.

Maneuver now exposes wx's Backprop control. It inserts/removes the keyword in its
own source span, retaining labels, resource fields and surrounding text. Equal
source offsets apply field replacement before keyword insertion. MissionTests
checks forward and backward 0.01 km/s inertial X maneuvers through save/reopen,
recognition of an existing BackProp keyword and disabling it without damaging the
command. Non-inertial frames and mass-decrement/backward-mass behavior remain
unqualified.

GmatQt was rebuilt; all 11 Qt checks passed in 50.28 seconds. Evidence:
`Qt6ParityValidation/check-burn-controls.txt`.


## Toggle output selection and execution

Toggle now provides a subscriber checklist and an On/Off dropdown. Existing
selection/order is retained; additional configured subscribers can be checked and
rows dragged. Non-subscriber resources are excluded, empty selection disables OK,
and Cancel discards pending checklist changes. The dropdown updates only the state
source span, retaining the command's other text.

CompatibilityTests edits both Toggle commands through the controls, selects two
ReportFiles, checks exclusion of spacecraft, empty-selection rejection and Cancel,
then saves/reopens with invalid-build recovery. Both reports remain silent for
the first ten-second propagation and record the same interval after reactivation,
ending at twenty seconds total (epoch tolerance 1e-6 seconds). ElapsedSecs stop
conditions are relative to each Propagate command; the second command requests
ten additional seconds. The finite-burn coast fixture was aligned to the same
semantics so its documented ten-second coast is now exactly what it requests.
Plot/ephemeris subscribers and Toggle inside solver loops remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.66 seconds. Evidence:
`Qt6ParityValidation/check-toggle-controls.txt`.


## Object-management selection

Global, Clear and Save now use the shared command checklist, with configured
objects and user Variable/Array/String resources. Computed system parameters are
not offered as objects. Save/Clear can offer automatic globals; Global excludes
them from new choices and explains why, matching ManageObjectPanel's distinction.
Existing source selections remain available so opening a dialog cannot silently
remove a name. Empty selection disables OK and Cancel preserves prior fields.

CompatibilityTests selects SavedSat/SavedNumber through the Save dialog and then
runs the existing numerical export, exact save/reopen, export reimport, repeat-run,
loop snapshot, invalid-output-path and disk-write recovery checks. It checks that
Global excludes Earth/EarthMJ2000Eq while allowing a user variable, and that Clear
Cancel preserves its object list. Global/Clear runtime scope/dependency behavior
still requires separate qualification; checklist coverage alone is not counted as
full command parity.

GmatQt was rebuilt; all 11 Qt checks passed in 27.96 seconds. Evidence:
`Qt6ParityValidation/check-object-pickers.txt`.

## Plot playback controls and narrow-window layout

OrbitView and GroundTrack now expose Start, Play/Pause, Latest, a position label,
and 0.25x–4x replay speed. Resuming keeps the paused position; playing at the end
restarts retained history. Scrubbing pauses playback and Latest restores following
incoming data. The timeline has its own full-width row rather than disappearing
into toolbar overflow when plots are tiled. Replay speed is relative to a nominal
three-second sweep of retained history, not simulation-clock playback.

PlotTests verifies pause/resume, rewind/latest, end-of-history stopping, scrubbing,
4x and fractional 0.25x advancement, retained-history eviction/reset, and visible,
in-bounds timeline/speed controls in a 330-pixel-wide plot. These checks run in
native X11 and offscreen/HiDPI configurations. This does not qualify remaining
OpenFrames trajectory/segment cameras or actual Intel/Wayland stability.

GmatQt was rebuilt; all 11 Qt checks passed in 50.01 seconds. Evidence:
`Qt6ParityValidation/check-playback-controls.txt`. A separate native tiled-window
capture confirms the timeline and speed selector remain visible beside the plots.

## Chart text in tiled windows

The XY/ground-track painter now measures legend entries and wraps them into rows,
reserving chart space below. Long titles and horizontal axis labels elide rather
than clipping through window edges. Legend rows are bounded to retain plot area;
when entries exceed that budget, a remaining-entry count replaces the last cell.
Hover text preserves full titles, axis labels and visible curve names.

The native six-window PlotTests capture was inspected at 1280x850: the narrow XY
window shows both complete legend entries on separate rows and a cleanly shortened
title, while orbit and ground-track playback controls remain visible. This is a
chart-layout improvement, not qualification of all plot interaction workflows.
GmatQt was rebuilt and all 11 Qt checks passed in 25.89 seconds. Evidence:
`Qt6ParityValidation/check-chart-layout.txt`.

## Script Find/Replace workflow

Audited wx FindReplaceDialog's next/previous, replacement, Replace All and history
controls. Qt now provides these through the Edit menu and standard shortcuts,
with optional case/whole-word matching. Searching brings the Script window forward
and retains the dialog for subsequent searches. No-match feedback is nonblocking;
replacement checks the script's read-only state before modifying text.

WorkflowTests opens the integrated menu action, verifies forward/backward wrap,
case and word boundaries, selected replacement, history retention, no-match
feedback and read-only protection. Replacing Sat with SatSat proves Replace All
does not repeatedly replace its own insertions; one Undo restores all original
text. Search itself does not normalize or reconstruct script source.

GmatQt was rebuilt and all 11 Qt checks passed in 28.73 seconds. Evidence:
`Qt6ParityValidation/check-find-replace.txt`. The broader editor audit (including
other editing/navigation preferences) remains separate from this completed search
workflow.

## Resource input/output file browsing

The generic filename picker previously used AnyFile for every field. Resource
metadata now distinguishes known input files from output destinations: inputs use
ExistingFile/Open and outputs use AnyFile/Save. Output browsing itself does not
write a file and does not present an overwrite confirmation for an operation it
is not performing. Unclassified plugin fields retain the prior general picker.

WorkflowTests checks the ReportFile output mode and a new path with spaces without
creating it. Spacecraft ModelFile browsing checks existing-input mode, Cancel,
selection of a path with spaces, and deferred Apply (the live spacecraft remains
unchanged). These checks qualify file selection only; function-source editing,
SPICE kernel lists and broader spacecraft model configuration remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.66 seconds. Evidence:
`Qt6ParityValidation/check-file-pickers.txt`.

## Structured mission conditions

Audited wx ConditionPanel's comparison columns and logical joins. Qt now provides
a local condition dialog with left/right operands, relation and AND/OR controls,
add/remove rows and a shared parameter browser. Numeric values (including signed
scientific notation), parameter references and positive numeric array indices are
recognized. Unsupported grouped/expression syntax stays available verbatim in the
text field rather than being silently transformed. Table columns are adjustable.

MissionTests exercises Cancel, removal/addition, incomplete-row rejection, relation
and operand edits, branch/label preservation and non-conversion of grouped syntax.
The real-engine If test now edits through the dialog, executes the changed branch
and restores the original result with Undo. Dedicated While execution, compound
join outcomes and all parameter-picker combinations still need broader coverage.

GmatQt was rebuilt; all 11 Qt checks passed in 28.35 seconds. Evidence:
`Qt6ParityValidation/check-condition-builder.txt`.

## Single-parameter command selection

The shared parameter browser now supports a scalar selection mode for condition
operands, Achieve goals/values, Minimize objectives and nonlinear constraints. It
uses the current entry directly, hides report-list management, and offers array
element construction. Empty text and bare arrays cannot be accepted. The ordered
report mode and its existing tests are retained unchanged.

MissionTests checks initial selection, hidden report-list controls, empty/bare-array
rejection and Choice(2,3) selection. It chooses a configured goalValue variable in
an Achieve command, verifies Cancel, saves/reopens and executes the target to x=8
(tolerance 1e-6). Existing repeated-run and solver-window lifecycle checks also run
with this selected parameter. Minimize/constraint execution combinations and more
property types remain pending; a shared widget is not counted as all-context proof.

The array-element check exposed that Moderator resolves indexed names to the base
array. The browser now distinguishes exact array names from indexed references;
Use element cannot append another index to an already indexed entry. GmatQt was
rebuilt and all 11 Qt checks passed in 38.47 seconds. Evidence:
`Qt6ParityValidation/check-single-parameter.txt`.
