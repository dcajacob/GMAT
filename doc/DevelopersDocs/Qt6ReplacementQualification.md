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
| `src/gui/hardware/ThrusterCoefficientDialog.hpp` | Grouped chemical/electric coefficient controls implemented; finite-burn execution and electric configuration round trips covered; broader electric operating modes pending |
| `src/gui/hardware/BurnThrusterPanel.hpp` | Pending audit |
| `src/gui/hardware/ThrusterConfigPanel.hpp` | Pending audit |
| `src/gui/hardware/PowerSystemConfigPanel.hpp` | wx field inventory audited; grouped general/bus/solar/shadow controls and shadow-body picker covered. List reconstruction, invalid-body rollback and Undo tested. Epoch-format conversion, failed conversion recovery and paired Apply/Undo covered; GUI-configured nuclear and unshadowed solar report execution covered; eclipse attenuation and decay cases pending. |
| `src/gui/hardware/TankAndMixDialog.hpp` | Combined tank/ratio editor, paired Apply, round trips and two-tank chemical burn covered. Broader electric tank combinations remain pending. |
| `src/gui/event/EventLocatorPanel.hpp` | Pending audit |
| `src/gui/command/TogglePanel.hpp` | Subscriber checklist and On/Off dropdown; empty selection, Cancel, filtering and dual-report suppression/resumption after save/reopen tested. Plot/ephemeris and solver-loop Toggle combinations pending. |
| `src/gui/command/GmatCommandPanel.hpp` | Pending audit |
| `src/gui/command/ManeuverPanel.hpp` | Typed impulsive-burn and spacecraft selectors; Cancel, label/comment preservation, save/reopen and inertial delta-V execution tested. Backprop checkbox and reverse inertial delta-V tested; other frames and mass-decrement cases pending. |
| `src/gui/command/ScriptEventPanel.hpp` | Pending audit |
| `src/gui/command/NonlinearConstraintPanel.hpp` | Optimizer selector and single-parameter left/right operand browser provided. Solver selector tested; constraint operand execution combinations pending. |
| `src/gui/command/AchievePanel.hpp` | Boundary-value solver selector plus single-parameter goal/value browser. Selected target variable, Cancel, save/reopen and solved result tested; Omitted tolerance can be added from the engine default and edited; reopened solve covered. Broader tolerance/property combinations pending. |
| `src/gui/command/ManageObjectPanel.hpp` | Global/Clear/Save object checklists added. Global automatic-resource filtering, Clear Cancel and Save export/reopen/recovery tested. Global/Clear runtime scope semantics pending. |
| `src/gui/command/BeginFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; selected ten-second constant-thrust burn, analytic fuel consumption and save/reopen tested. Other thruster/tank models pending. |
| `src/gui/command/OptimizePanel.hpp` | Optimizer selector, SolveMode/ExitMode dropdowns and progress checkbox provided. Partial-option insertion preserves pending solver/name/options in tests; full optimizer mode combinations pending. |
| `src/gui/command/TargetPanel.hpp` | Boundary-value solver selector, SolveMode/ExitMode dropdowns, progress checkbox and omitted-default insertion tested. Initial-guess execution, Undo and later solve verified; Stop/Discard and other combinations pending. |
| `src/gui/command/VaryPanel.hpp` | Solver selector offers boundary-value solvers and optimizers; writable numeric variable picker added. Selected Vary variable and Achieve target survive save/reopen and solve correctly. Missing option controls can be inserted from engine defaults without losing pending edits; edited bounds/step and reopened solve covered. Solver capability flags now control field enabling; DC/Yukon switching, Cancel and unknown-solver recovery preserve pending values. Other plugin-specific variable cases pending. |
| `src/gui/command/FindEventsPanel.hpp` | Event-locator selector and Append controls covered; manual EclipseLocator replace/append execution and round trips tested. Other locator types and failure modes remain pending. |
| `src/gui/command/PropagatePanel.hpp` | Single parameter/value stop selection and source-preserving controls covered; periapsis/apoapsis selectors and execution covered; direction/tolerance and multiple-stop editing covered; multi-propagator assignment editing and synchronized two-spacecraft execution covered; STM/A-matrix controls and two-spacecraft execution covered; covariance controls and broader formation/mode combinations pending. |
| `src/gui/command/AssignmentPanel.hpp` | CommandForm destination/expression controls plus writable destination picker (including user strings/arrays); source-preservation tests. Picker filtering tested; destination-specific execution and complex syntax audit pending. |
| `src/gui/command/CallFunctionPanel.hpp` | Function resource selector plus ordered input/output argument browsers provided. Cancel, quoted/nested comma preservation, invalid numeric outputs, reordering and multi-output execution after save/reopen covered. Broader object/string/array signature execution pending. |
| `src/gui/command/EndFiniteBurnPanel.hpp` | Typed finite-burn/spacecraft selectors; fuel remains constant during coast after selected EndFiniteBurn. Other thruster/tank models pending. |
| `src/gui/command/MinimizePanel.hpp` | Optimizer selector and single-parameter objective browser provided. Solver selector tested; objective-browser execution combinations pending. |
| `src/gui/command/ReportPanel.hpp` | Configured report-file picker and shared ordered parameter dialog: add/remove/reorder, numeric array indices, Cancel, labels/comments and numerical output tested. Object/property and coordinate/central-body browsing implemented; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types pending. |
| `src/gui/function/MatlabFunctionSetupPanel.hpp` | Pending audit |
| `src/gui/function/FunctionSetupPanel.hpp` | In-app GMAT function-file editing, Save/Cancel and find/replace provided. BOM/CRLF preservation, external-change protection and edited-function execution after mission save/reopen covered. Save As, pending path Apply and execution from the copy covered. New template-based file creation, Cancel, path Apply and reopened execution covered; broader multi-output/function-signature cases remain pending. |
| `src/gui/solver/SolverVariablesPanel.hpp` | Pending audit |
| `src/gui/solver/SolverSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverGoalsPanel.hpp` | Pending audit |
| `src/gui/solver/DCSetupPanel.hpp` | Six wx fields audited; algorithm/derivative dropdowns and report output picker corrected. Pending Apply, chooser Cancel, Broyden/CentralDifference save/reopen solve and report writing covered. Other algorithm/derivative combinations remain to qualify. |
| `src/gui/solver/SolverCreatePanel.hpp` | Pending audit |
| `src/gui/solver/SQPSetupPanel.hpp` | Pending audit |
| `src/gui/controllogic/ForPanel.hpp` | CommandForm index/start/step/end with variable-only index selector and shared bound parameter browsers; Cancel, filtering, selected numeric bounds, execution and Undo tested. Broader parameter-valued loop execution pending. |
| `src/gui/controllogic/ConditionPanel.hpp` | Comparison-row builder with numeric/parameter/array operands, six relations, AND/OR, add/remove and shared operand browser. Cancel, incomplete rows, header-only replacement, real If execution and Undo tested; grouped/expression syntax stays in text. Broader parameter selection and While execution cases pending. |
| `src/gui/spacecraft/OrbitPanel.hpp` | Spacecraft epoch format conversion, invalid-date recovery, Apply and save/reopen propagation covered. Cartesian/Keplerian pending-state conversion and label refresh covered; remaining representations and coordinate-selection workflows pending audit. |
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
| `src/gui/spacecraft/SpicePanel.hpp` | Ordered SPK/CK/SCLK/FK file-list controls added. SPK add/duplicate/order/Cancel/Apply, Undo/Redo, save/reopen, clear and missing-file recovery tested with bundled kernel copies. Mars Express SPK/CK/SCLK execution, Qt trajectory/attitude capture and clock-file recovery tested. FK runtime use and other NAIF combinations pending. |
| `src/gui/spacecraft/FormationSetupPanel.hpp` | ResourceEditor Add list and spacecraft picker; invalid member rejection, reordering, save/reopen and two-member propagation tested. Remaining wx-specific operations under audit. |
| `src/gui/foundation/GmatBaseSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatDialog.hpp` | Pending audit |
| `src/gui/foundation/ParameterCreateDialog.hpp` | Pending audit |
| `src/gui/foundation/ParameterSelectDialog.hpp` | Pending audit |
| `src/gui/foundation/SinglePathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatPanel.hpp` | Pending audit |
| `src/gui/foundation/MultiPathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatColorPanel.hpp` | COLOR_TYPE resource fields and visual picker/swatch added. Spacecraft orbit/target Cancel, pending Apply, Undo/Redo, invalid RGB rollback, save/reopen and published trajectory color tested. Per-view override controls and other resource types pending. |
| `src/gui/foundation/ArraySetupDialog.hpp` | Shared numeric grid; see ArraySetupPanel. Full wx dialog audit pending. |
| `src/gui/foundation/ShowScriptDialog.hpp` | Pending audit |
| `src/gui/foundation/GmatSavePanel.hpp` | Pending audit |
| `src/gui/foundation/ParameterSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/ArraySetupPanel.hpp` | ResourceEditor resizeable numeric grid plus separate mission-start expression grid; retained/new cells, dependent formulas, Cancel, rollback, Undo/Redo and save/reopen tested. Combined numeric/expression Apply, pending resize dimensions, shrink cleanup, atomic Undo and rollback covered; arbitrary existing assignments remain outside the grid workflow. |
| `src/gui/foundation/ShowSummaryDialog.hpp` | Pending audit |
| `src/gui/propagator/PropagationConfigPanel.hpp` | Owned propagator settings exposed; numerical/TLE step edits and serialization tested. Specialized layout and remaining settings pending. |
| `src/gui/propagator/PropagatorSelectDialog.hpp` | Pending audit |
| `src/gui/asset/GroundStationPanel.hpp` | Pending audit |
| `src/gui/debugger/InspectorPanel.hpp` | Pending audit |
| `src/gui/forcemodel/DragInputsDialog.hpp` | Pending audit |
| `src/gui/coordsystem/CoordSysCreateDialog.hpp` | Basic creation plus dedicated Axes dialog tested; MOEEq epoch and constrained-frame edits checked. Remaining origin and specialized-mode cases pending. |
| `src/gui/coordsystem/CoordSystemConfigPanel.hpp` | Axis replacement, dependent field exposure, protected built-ins, failed-edit rollback, Undo and save/reopen tested. Broader modes pending. |
| `src/gui/coordsystem/CoordPanel.hpp` | ObjectReferenced radial frame, MOEEq epoch edits and Sun-aligned LocalAlignedConstrained transforms checked, including save/reopen. Other modes and dependency cases pending. |
| `src/gui/output/ReportFilePanel.hpp` | Read-only, unwrapped report text, full path in title, text selection, close and unavailable-file handling tested. Standard Qt copy controls provided; large reports now have bounded paging, navigation, page-local search and reload recovery. Case-sensitive full-file search added; richer full-file search options remain pending. |
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
| `src/gui/subscriber/OrbitViewPanel.hpp` | Drawing options audited; live Qt display controls covered, camera controls partly covered. Full resource workflow and remaining view modes pending. |
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
| `../plugins/libEphemPropagator` | Mars Express SPK configured through Qt kernel lists, converted viewer, exact round trips, report/view agreement and missing-clock recovery tested. Other ephemeris formats and coverage-boundary cases pending. |
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
At this checkpoint, reports still had a 16 MiB preview limit; the later paging
milestone below removes that access limit. Clipboard contents are not changed by tests.

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

## Writable mission parameter selection

Vary and assignment destinations now use writable modes of the shared browser.
Configured user Variable/Array/String resources are handled separately from
system-parameter IsSettable metadata. Vary additionally restricts configured
parameters to numeric results (and arrays for element construction), and property
browsing to settable/plottable types, following wx's Vary selector. Assignment
allows user strings and whole arrays. Known read-only typed references and numeric
literals cannot be accepted as writable destinations; direct text editing remains
available for unsupported assignment forms.

MissionTests verifies Vary offers x/Choice but not a String, offers spacecraft X
but not ElapsedSecs, and rejects typed read-only references/numeric literals. It
checks assignment string/whole-array availability. The targeting fixture initially
varies the wrong variable; the actual Vary picker selects x, then the existing
Achieve picker selects goalValue. Save/reopen and subsequent solver runs must still
achieve x=8 within 1e-6. Broader hardware/plugin parameter and destination execution
cases remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 26.98 seconds. Evidence:
`Qt6ParityValidation/check-writable-parameters.txt`.

## Spacecraft SPICE file-list editor

SpacePoint's four spacecraft kernel arrays were excluded by the generic list
allowlist. They are now exposed as ordered file lists rather than resource-name
lists. The dialog supports adding existing files, removing selected entries,
drag reordering and Cancel. Serialization quotes paths, preserves commas/spaces,
replaces the entire list and allows clearing. Apostrophes, semicolons and embedded
line breaks are explicitly rejected in this GUI path; unrestricted filename
round trips remain an outstanding edge case.

WorkflowTests uses two copies of the bundled GEOSat.bsp, including filenames with
spaces and a comma. It adds through the real picker, checks duplicate prevention,
reorders, cancels removal, applies and verifies engine order. It then checks
Undo/Redo, exact save/reopen, failed-edit rollback for a nonexistent kernel and
clearing. All four kernel-list controls are present, but this does not qualify
CK/SCLK/FK loading or SPICE propagation/attitude execution.

GmatQt was rebuilt; all 11 Qt checks passed in 27.83 seconds. Evidence:
`Qt6ParityValidation/check-kernel-lists.txt`.

## SPICE orbit/attitude execution and file recovery

CompatibilityTests adapts Ex_SPICEOrbitAndAttitudePropagation.script to a 60-second
run, converts its two OF views through MainWindow's Qt converter, and configures
the bundled SPK, CK and SCLK files through the kernel-list dialogs. The sample's
referenced MarsExpress_MEX_V10.TF is absent from this checkout; the fixture omits
that FK and does not claim unmodified-sample or FK qualification. SpiceAttitude
works with the available files and explicit spacecraft/frame NAIF IDs in this case.

After exact save/Save As/reopen and invalid-build recovery, the mission must produce
at least 13 report/view samples and reach 60 seconds. Plotted Mars-frame positions
match report values within 1e-5 km. Recorded body-to-view orientation matches the
engine's transposed inertial-to-body matrix within 1e-10 and is orthonormal within
1e-8. This short fixture's attitude is effectively stationary (change about 1.6e-15),
so it qualifies orientation capture, not visibly changing SPICE attitude playback.

Removing a copied clock file after a successful run reproduced a recovery defect:
CSPICE may reread remaining text kernels while unloading another kernel. A missing
file left its pool and GMAT's loaded-file map inconsistent, so restoring the file
was insufficient. Shared SpiceInterface cleanup now clears the entire pool using
[KEEPER reset](https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/kclear_c.html)
for UnloadAllKernels. A failed individual unload rebuilds the available remaining
kernels and their cache, leaving unavailable files eligible for later reload.
This is shared engine cleanup and also affects non-Qt callers; propagation math is
unchanged. Restoring the clock must recover and produce an identical report, as
must a further repeat run. Full FK, coverage-boundary and changing-attitude cases
remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 27.54 seconds. Evidence:
`Qt6ParityValidation/check-spice-runtime.txt`; the reproduced failure is retained
in `Qt6ParityValidation/spice-recovery-before.txt`.

## Spacecraft visualization colors

The resource-property adapter previously skipped COLOR_TYPE fields. It now reads
and applies GMAT color strings, preserving names when unchanged and accepting RGB
triples via engine validation. ResourceEditor provides a color picker and preview
swatch; spacecraft OrbitColor/TargetColor are placed in Visualization. SPICE
classification also now includes every field containing Spice, so the four kernel
lists appear together before attitude-specific grouping.

PlotTests selects orbit/target colors, checks Cancel and deferred Apply, exercises
Undo/Redo, rejects an out-of-range RGB component without source mutation, then
saves/reopens/runs. The selected orbit color must reach both the viewer's curve
and final trajectory sample, and the target color must remain in engine state.
This qualifies spacecraft resource colors; solver-iteration target rendering,
per-view overrides and other resource color types remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 30.89 seconds. Evidence:
`Qt6ParityValidation/check-resource-colors.txt`.

## Solver option controls

Audited TargetPanel/OptimizePanel mode dropdowns and progress checkbox, plus
FindEventsPanel's Append checkbox. Qt uses engine-provided solver mode choices
and typed controls for existing options. An explicit Add default options action
adds only missing solver options using fresh command defaults; opening the form
does not rewrite the script. Existing values (including unfamiliar values) and
branch text remain preserved.

MissionTests checks full and partial insertion, pending solver-name edits, branch
labels/comments/bodies, dropdown/checkbox source updates and Append preservation.
The real Target workflow selects RunInitialGuess, verifies x remains at 1, undoes
the edit and continues through the existing saved/reopened Solve case reaching 8.
This does not qualify every Optimize mode or Stop/Discard combination, FindEvents omission was subsequently closed below.

Switching modes exposed stale progress windows: engine listener keys include the
command's generated source, so Initial Guess and Solve could leave separate
windows and the earlier unconverged result remained visible. Qt now tracks which
listeners were used by the current run and removes inactive windows when that run
ends. Listener objects remain alive for engine-held pointers. The mode-switch test
requires exactly one current progress window and successful convergence; existing
repeat-run, close/recreate and user column-width checks remain in place.

GmatQt was rebuilt; all 11 Qt checks passed in 30.94 seconds. Evidence:
`Qt6ParityValidation/check-solver-options.txt`.

## Thruster coefficient editor

Audited wx ThrusterCoefficientDialog: two coefficient tables with read-only names
and units. Qt now supplies matching chemical C1–C16 / K1–K16 and electric
ThrustCoeff1–5 / MassFlowCoeff1–5 tables, deriving units and row counts from engine
metadata. Columns remain adjustable. Values come from the resource panel's pending
state; Cancel is local, all entries must be finite before OK, and engine changes
wait for the parent Apply.

CompatibilityTests checks pending-value initialization, Cancel, unit protection,
resizing, overflow rejection without partial copying and deferred engine updates.
The chemical fixture sets C1/K1 through the dialog before save/Save As, failed-build
recovery, reopen and the existing finite-burn fuel-use/coasting assertion. Electric
fifth coefficients are changed through both tabs, applied and checked after the
same round trip. This electric case qualifies configuration persistence, not
execution of every thrust model or power-dependent operating mode.

GmatQt was rebuilt; all 11 Qt checks passed in 29.68 seconds. Evidence:
`Qt6ParityValidation/check-thruster-coefficients.txt`.

## FindEvents omitted options and report behavior

wx FindEventsPanel always exposes Append. Qt previously only exposed it when
already written in source. Add default options now also handles event searches,
reading the default from FindEvents itself. The resulting checkbox retains the
existing source-preserving edit behavior. MissionTests checks omitted insertion,
pending locator edits, label/comment preservation and no duplicate insertion.

CompatibilityTests extends the EclipseLocator plugin case with Manual mode and a
FindEvents command. It selects replacement through the form, saves/Save As,
recovers from failed interpretation, reopens and runs to a report containing
shadow intervals. Selecting Append through the form and repeating that lifecycle
must preserve the previous report as a prefix and add new output. Clearing Append
and rerunning must restore the original report exactly. This qualifies these
manual EclipseLocator report modes, not all ContactLocator/IntrusionLocator
options or file-error recovery cases.

GmatQt was rebuilt; all 11 Qt checks passed in 33.68 seconds. Evidence:
`Qt6ParityValidation/check-event-options.txt`.

## Camera controls in narrow viewers

Moved the embedded camera selector/projection/FOV widgets from the crowded plot
toolbar into a nonmodal Camera panel. This avoids relying on toolbar widget
availability at tiled widths. Keep projection is in the same panel and delegates
to the existing undoable script edit; changes still preview live. Close explicitly
dismisses the panel without reverting preview settings.

PlotTests checks access from a 330-pixel viewer, panel bounds, reopen persistence,
disabled script persistence for standalone plots, and the existing projection,
FOV and named-camera rendered-image differences. WorkflowTests now invokes the
panel's Keep projection button for unchanged-source preservation, selected-view
metadata, undo and unbuilt-script rejection. This improves camera-control access;
trajectory/segment-relative conversion and broader framing behavior remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 39.09 seconds. Evidence:
`Qt6ParityValidation/check-camera-panel.txt`. An isolated Xvfb/native render run
also passed; `Qt6ParityValidation/camera-panel.png` was visually inspected for
label/control fit. This is not a hardware-driver qualification claim.

## Live orbit display controls

Compared wx OrbitViewPanel drawing options with Qt model/renderer fields. The Qt
viewer now exposes axes, grid, labels, legend, XY/ecliptic planes, wireframe and
origin–Sun line in a nonmodal Display panel. These are live viewer overrides;
persistent resource changes still use the resource editor. The panel explicitly
states this distinction and resynchronizes its checkboxes on reopening.

PlotTests checks each control's initial state and model binding, unchanged replay
frame/camera history, a combined rendered-image change and exact restoration of
the original image after clearing the options. It also checks reopening after a
model value changes. This does not individually qualify every overlay's geometry
or replace the pending full OrbitView resource audit.

GmatQt was rebuilt; all 11 Qt checks passed in 38.90 seconds. Evidence:
`Qt6ParityValidation/check-display-controls.txt`. An isolated native/Xvfb run
also passed, and `Qt6ParityValidation/display-panel.png` was visually inspected.
Hardware-specific viewer qualification remains separate.

## Paired tank and mixture Apply

Auditing wx TankAndMixDialog exposed a Qt Apply ordering defect: changing Tank and
MixRatio together applied ratios against the old list first, then remapped those
new values by old tank name. A reorder could silently assign different ratios
than the user entered; a resized list could reject otherwise valid new ratios.
The Qt adapter now treats explicit simultaneous ratios as positional in the new
ordered list, validating one finite positive value per tank before interpretation.
A Tank-only edit continues to preserve existing ratios by tank name.

WorkflowTests covers simultaneous reorder/new ratios, count mismatch, zero,
negative and overflow rejection with unchanged source, removal/addition with ratio
resizing, and save/reopen preserving the resulting tank/value pairs. The existing
Tank-only GUI picker test still checks association preservation. This fixes the
Apply layer; a dedicated combined tank/mixture selector and multi-tank fuel-use
execution remain pending, so the wx dialog row is not marked complete.

GmatQt was rebuilt; all 11 Qt checks passed in 34.30 seconds. Evidence:
`Qt6ParityValidation/check-paired-mixtures.txt`.

## Combined tank and mixture editor

Qt now offers Tanks and mixtures with named, read-only tank rows, editable positive
ratios, available-tank selection, Add/Remove and Up/Down controls. Moving a row
moves its ratio with it; duplicate additions are disabled and columns remain
resizable. Cancel is local and OK defers engine changes until Apply. Explicit
paired edits submit both fields even if the numeric vector happens to equal the
original vector while tank order has changed.

WorkflowTests covers Cancel, candidate filtering, duplicate prevention, remove/
re-add, default ratio, paired movement, zero rejection, pending engine state,
column resizing and the unchanged-vector/reordered-tanks save/reopen case.
CompatibilityTests adds a second tank through this dialog and sets ratios 3:2.
After save/Save As, failed-build recovery and reopen, the finite-burn fixture must
consume fuel in the corresponding 60/40 split, with both masses unchanged after
EndFiniteBurn during coasting. This qualifies the chemical two-tank GUI path;
broader electric operating modes remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 41.78 seconds. Evidence:
`Qt6ParityValidation/check-tank-mixture-dialog.txt`.

## Perspective Fit in narrow windows

The OpenFrames View::resetView audit found that automatic framing uses the
minimum horizontal/vertical half-angle. Qt's Fit distance used only the vertical
angle, allowing horizontal clipping in portrait windows. OrbitCamera now accepts
the viewport aspect ratio and uses the smaller half-angle for perspective Fit;
both native and QPainter paths pass their actual drawing-area aspect. Explicit
scripted distances with Fit disabled remain unchanged.

NativeOrbit tests render a green sphere in 240×600, 600×240 and 400×400 viewports,
require visible body pixels and a margin on all four sides, at normal and high DPI.
This qualifies perspective Fit containment, not equivalence of all OF automatic
bounding-sphere centers or trajectory/segment framing.

Further trajectory audit: OFScene::GetTrajectoryFrameByName returns
OFSpaceObject::WholeTrajectory (mDrawTraj), while moving-object views use
FrameOnWholeTrajectory (mViewRefFrame). Thus simply disabling body orientation
while continuing to track object position is not a full trajectory-view mapping.
Stored whole-trajectory conversion is addressed in the following milestone;
automatic trajectory and segment framing remain open requirements.

GmatQt was rebuilt; all 11 Qt checks passed in 37.66 seconds. Evidence:
`Qt6ParityValidation/check-perspective-fit.txt`.

## Stored whole-trajectory conversion

The OFSpaceObject source confirms mDrawTraj is attached directly to the primary
plot frame, while mViewRefFrame has the position/attitude follower. Conversion now
maps stored whole-trajectory views to the plot-frame origin rather than tracking
the named spacecraft. Stored eye, center, up, FOV and existing two-frame look-at
handling remain in place. Secondary named trajectory presets likewise use an
empty object reference. Unknown trajectory names are rejected unless represented
in the plot Add list; CoordinateSystem is accepted directly.

WorkflowTests checks primary/secondary mapping, scientific command preservation,
unknown-object rejection, automatic-framing and segment-specific diagnostics.
A converted example is saved/reopened/run; its spacecraft must move more than
1 km while every camera eye/target sample remains fixed, with the stored eye
coordinate retained. The earlier generic trajectory warning is replaced by a
specific preservation note for supported stored views. Unsupported automatic
trajectory and segment cases now stop conversion with actionable errors rather
than substituting moving-object tracking. Their implementation remains pending;
this milestone does not claim complete trajectory camera parity.

GmatQt was rebuilt; all 11 Qt checks passed in 36.87 seconds. Evidence:
`Qt6ParityValidation/check-trajectory-camera.txt`.

## Paged report viewer

Replaced the first-16-MiB-only preview with ReportViewer: approximately 1 MiB per
page, First/Previous/Next/Last, page-number jump, reload and existing read-only
Find dialog scoped explicitly to the current page. All file regions are reachable
without accumulating previously viewed text. UTF-8 codepoints and CRLF pairs are
kept on the same side of a boundary. Long rows may span pages, which the UI states.
Read errors retain the prior display. Reload reopens the path so replaced/shrunk
files are handled, and page navigation clamps to the current file size.

FileTests reconstructs a four-page report containing three- and four-byte UTF-8
characters and a CRLF at boundaries, exercises backward/jump navigation, retains
content after a missing-file error, reaches a marker beyond 17 MiB, then reloads
a shorter replacement. Existing Compatibility/Plot report checks retain exact
small-report text, path-in-title, read-only/no-wrap, selection and close behavior.
Case-sensitive full-file search follows below; wx report-comparison workflows remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 31.05 seconds. Evidence:
`Qt6ParityValidation/check-report-paging.txt`.

## Complete report search

ReportViewer now supports a separate case-sensitive literal search across the
complete file, next non-overlapping match and cancellation. A zero-interval Qt
timer scans one MiB per callback with bounded overlap for cross-chunk matches;
closing the viewer destroys the timer/file objects. Navigation/reload cancels an
active scan. Results navigate to the matching page, highlight its visible portion
and explicitly identify a result continuing onto the next page. Search file
restarts from the beginning; exhaustion does not wrap silently.

FileTests covers a match spanning a page boundary, a Unicode character deferred
to the next page, advancing matches, cancellation, no-match feedback and finding
a marker beyond 17 MiB. Page-local Find retains its existing richer options.
Case-insensitive/whole-word full-file modes and report comparison are not claimed
by this milestone.

GmatQt was rebuilt; all 11 Qt checks passed in 33.34 seconds. Evidence:
`Qt6ParityValidation/check-report-search.txt`.

## Propagate parameter stop controls

The wx audit found a parameter/value stop grid and parameter browser beyond elapsed
seconds/days. Qt PropagationForm now represents one scalar parameter/value stop,
adds single-parameter pickers for both operands, and preserves source spans when
editing. Labels, comments and existing BackProp are retained. Seconds/Days remain
quick selections and follow spacecraft changes. Broader Propagate structures are
left in the source editor without being simplified.

WorkflowTests checks exact labeled/commented BackProp source preservation and
fallback for event-only conditions. The real command editor selects QtSat.A1ModJulian
through the browser, sets an epoch 0.01 days after the initial state, verifies Cancel
on the goal picker, applies, saves/reopens and runs. The resulting epoch must be
864 seconds after the initial epoch. Multi-propagator/multi-stop structures,
event-only controls, StopTolerance and other advanced modes remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 33.11 seconds. Evidence:
`Qt6ParityValidation/check-propagation-stops.txt`.

## Periapsis and apoapsis stop controls

PropagationForm now handles scalar stops and the goal-free Periapsis/Apoapsis
syntax. A dedicated parameter-browser mode includes these event parameters even
when they are not reportable; report and writable-parameter modes remain unchanged.
Selecting an event disables the numeric goal and its picker. Source edits retain
labels, comments and modifiers, and switching back to a scalar restores editable
goal controls. Other unrepresented propagation structures remain in source.

WorkflowTests checks event syntax without a spurious equality/goal.
CompatibilityTests selects both event types through the object/property/central-
body browser, applies, saves/Save As, recovers from failed interpretation, reopens
and executes. A two-body ellipse with SMA 10000 km and eccentricity 0.1 must stop
at radius 9000 km for periapsis and 11000 km for apoapsis, within 0.01 km. These
checks validate GUI choices against the unchanged propagation engine. Multiple
stops, synchronized propagators and StopTolerance controls remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 32.56 seconds. Evidence:
`Qt6ParityValidation/check-apsis-controls.txt`.

## Propagate direction and tolerance

Added a backward-propagation checkbox and optional StopTolerance field to the
single-stop form. The placeholder reads the default from a fresh engine Propagate
command. Empty input preserves omission or removes an explicit option; existing
tolerance values are replaced by span so formatting is retained. BackProp edits
also preserve labels/comments and existing spacing when unchanged.

WorkflowTests covers inserting/removing tolerance, preserving existing compact
option formatting and toggling an existing BackProp modifier. CompatibilityTests
extends the selected apsis workflow: zero tolerance must fail without changing
source, a positive tolerance must apply, and saved/reopened periapsis propagation
must move backward in epoch while apoapsis moves forward. Both retain the expected
orbital radius checks. These changes do not implement multiple stopping conditions,
synchronized propagators or other advanced Propagate options.

GmatQt was rebuilt; all 11 Qt checks passed in 34.38 seconds. Evidence:
`Qt6ParityValidation/check-propagation-options.txt`.

## Multiple stopping-condition table

CommandEditor now offers Stopping conditions for representable Propagate stop
blocks, independently of the single-stop quick form. The two-column dialog adds,
removes and reorders paired parameter/goal rows; shared parameter browsers support
both scalar and apsis conditions. Apsis goals are blank/read-only, blank rows cannot
be accepted, and source-only expressions remain protected by parsing checks.
StopTolerance/OrbitColor options, the propagation header, labels and comments are
retained. OK makes one source undo step, with engine validation deferred to Apply.

WorkflowTests covers the multiple-stop button, Cancel, editable column widths,
blank-row rejection, paired movement/removal, apsis goal protection and source Undo.
CompatibilityTests adds an elapsed-days stop through the new table to a 600-second
command with a tolerance and comment. After save/Save As, failed-build recovery and
reopen, execution must end at 86.4 seconds (0.001 days), the earlier condition.
This qualifies multi-stop editing; synchronized/multiple-propagator configuration
and broader stopping-expression combinations remain pending.

GmatQt was rebuilt; all 11 Qt checks passed in 54.61 seconds. Evidence:
`Qt6ParityValidation/check-multiple-stops.txt`.

## Propagator assignment editor

CommandEditor now offers Propagators and spacecraft for representable Propagate
headers. The dialog provides propagator selectors, editable object lists, an
ordered spacecraft/formation picker, add/remove rows, engine-provided propagation
modes and backward propagation. Empty or duplicate object assignments cannot be
accepted. Formations are supplied separately from the simple spacecraft form so
they cannot generate an invalid formation elapsed-time parameter there. Stop
blocks, options, labels and comments are preserved when changing assignments.

WorkflowTests covers modifier changes, multiple objects, a formation assignment,
exact preservation of the command suffix, and source fallback for STM flags.
CompatibilityTests exercises Cancel, invalid empty/duplicate rows, the nested
object picker, and two distinct propagators in Synchronized mode. After Apply,
save/Save As, failed-build recovery and reopen, both spacecraft advance to the
same epoch, stopping at the earlier of the configured conditions (86.4 seconds).
The second spacecraft also has a changed position. This is not full formation
execution qualification or coverage of all mode/propagator combinations.
Variational propagation flags still require the source editor.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 41.68 seconds
under the isolated test environment. Evidence:
`Qt6ParityValidation/check-propagation-groups.txt`.

## STM and A-matrix propagation controls

The propagator assignment dialog now includes Propagate STM and Compute A-matrix,
matching the command-wide checkboxes in wx PropagatePanel. Quoted and unquoted
flags are parsed separately from spacecraft names. Selecting or clearing a flag
updates the command while retaining the stop block, label and comment. The engine
applies these flags to all groups; serialization places selected flags in the
first group. Covariance flags continue to use source editing without a lossy
structured rewrite.

WorkflowTests loads both flags and independently clears STM while retaining
A-matrix. CompatibilityTests enables both through the dialog before save/Save As,
failed-build recovery and reopen, then runs the synchronized two-propagator
fixture. Both spacecraft reach the expected stop epoch and have nontrivial
FullSTM and FullAMatrix values. This verifies GUI-to-engine option delivery, not
an independent mathematical validation of GMAT's matrix implementation.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 34.45 seconds.
Native rendering tests ran under isolated Xvfb, not the user's live display.
Evidence: `Qt6ParityValidation/check-propagation-variational.txt`.

## For-loop selection controls

Audited wx ForPanel's four selectors and input restrictions. Qt now provides a
Variable-only Index selector and the shared parameter browser for Start, Step
and End, preserving free-text numeric entry. MissionTests checks index filtering
and Cancel, opens each bound browser, confirms a configured variable is available,
selects numeric bounds, applies the header edit, executes the changed loop and
undoes it to recover the original result. The branch body is preserved by the
existing source-span editing. This does not yet qualify every parameter-valued
loop-bound combination.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 34.57 seconds,
with native rendering under isolated Xvfb. Evidence:
`Qt6ParityValidation/check-for-pickers.txt`.

## Combined array values and formulas

Resource Apply now assembles numeric array settings and managed expression cells
into one candidate script and validates it once. Formula metadata bypasses the
ordinary engine property serializer. Expression indices are checked against the
proposed dimensions, including numeric-only resizes, so a shrink cannot silently
discard a managed formula cell. Existing mission commands remain preserved.

WorkflowTests changes a numeric value and a dependent formula in the same resource
panel, submits both with its Apply button, and checks the resulting calculation.
One Undo restores the original script and result. Invalid combined formulas and
shrinks past a formula cell leave source and runtime behavior unchanged. The
expression grid still uses dimensions from panel opening; entering formulas into
newly added cells before applying a resize remains a follow-up workflow gap.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.34 seconds.
Native display checks ran under isolated Xvfb. Evidence:
`Qt6ParityValidation/check-array-combined.txt`.

## Expression editing with pending array dimensions

The expression editor now derives its dimensions from pending numeric values.
New rows and columns are available without an intermediate Apply. If a pending
shrink excludes an existing formula, the grid keeps that cell visible, highlights
it and disables OK until the formula is cleared. Cancel preserves all formulas;
users can enlarge the numeric grid instead. Engine validation still handles
invalid numeric rows and formula references at Apply.

WorkflowTests grows a 1x2 array to 2x3, adds a dependent formula in the new bottom
right cell, and verifies the result is 21 after the combined Apply. It also opens
the expression editor after a pending shrink, checks that removed cells remain
visible and OK is disabled, clears them to enable OK, then cancels. Restoring the
larger numeric grid and applying proves the cancelled cleanup preserved formulas.
The existing single-Undo and invalid-formula rollback checks remain in place.

Validation: rebuilt the user's GmatQt and passed all 11 Qt tests in 34.47 seconds.
After adding explicit shrink-cleanup/Cancel assertions, the updated Workflow test
passed in 8.20 seconds. Native checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-array-pending-size.txt` and
`Qt6ParityValidation/check-array-shrink-ui.txt`.

## Power-system field audit and shadow-body selector

Audited wx PowerSystemConfigPanel's general settings (EpochFormat, InitialEpoch,
InitialMaxPower, AnnualDecayRate, Margin), three bus coefficients, five solar
coefficients, ShadowModel and ShadowBodies. Qt now groups these settings in
General, Bus coefficients, Solar coefficients and Shadow tabs as applicable.
The audit found ShadowBodies was omitted because STRINGARRAY_TYPE fields require
explicit list support. It now uses the ordered resource selector populated with
celestial bodies, and list Apply rejects non-body references.

WorkflowTests checks that each wx field appears in its matching section, unrelated
fields are hidden, and nuclear systems have no solar-only tabs. It exercises the
shadow picker, pending selection, two-body reconstruction, invalid-body rollback
and Undo. The wx behavior that converts InitialEpoch when EpochFormat changes is
not yet implemented in Qt; power output and eclipse/shadow runtime qualification
also remain pending. These tests establish configuration workflow coverage only.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 34.53 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-controls.txt`.

## Power-system epoch-format conversion

The power resource panel now offers the engine's valid time representations in an
EpochFormat dropdown. Changing it converts the current pending InitialEpoch via
TimeSystemConverter, preserving the represented instant. If conversion fails, the
format selection is restored and the date text is retained for correction. The
configured resource is unchanged until Apply.

WorkflowTests exercises solar and nuclear panels, converting a known UTC date to
TAI modified Julian time (including the 32-second offset at that date), converting
back, and rejecting an invalid pending date without changing either field. Paired
Apply reconstructs both settings through the script and Undo restores the source.
Runtime power and shadow calculations remain a separate qualification gap.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.41 seconds.
The subsequently extended Workflow test, including paired Apply/Undo, passed in
18.51 seconds. Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-epoch.txt` and
`Qt6ParityValidation/check-power-epoch-apply.txt`.

## Power-model selection and report execution

Solar ShadowModel now offers None and DualCone, matching the engine's accepted
values and the wx selector. CompatibilityTests edits nuclear and solar resources
through ResourceEditor, including maximum power, decay, margin, bus coefficients,
and solar coefficients/model selection. It saves, saves under a second name,
recovers from a failed interpretation, reopens and runs each mission.

The generated report must show the selected constant bus demand, configured
margin applied to the generated power, and exactly 10 kW nuclear output. The
unshadowed solar case uses inverse-square generation coefficients and checks a
bounded near-Earth output; this is not an exact independent solar-model check.
Eclipse attenuation, time-dependent decay, and electric-thruster coupling remain
separate qualification cases.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.43 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-power-runtime.txt`.

## Spacecraft epoch display conversion

The spacecraft DateFormat selector now shares the pending-epoch conversion
behavior with power systems. A successful change updates Epoch through GMAT's
time converter; failed conversion restores the prior selector value and retains
the invalid text for correction. This addresses a wx OrbitPanel interaction gap
without changing the propagation engine.

WorkflowTests converts a spacecraft epoch through UTC Gregorian and TAI modified
Julian formats, checks invalid-date recovery, applies the pair, and compares the
configured epoch with the original instant. After save/reopen, the mission must
finish at the original epoch plus the requested 600 seconds. Other OrbitPanel
state-representation and coordinate-system interactions remain under audit.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 58.11 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-spacecraft-epoch.txt`.

## Pending spacecraft state representation

DisplayStateType choices now come from StateConversionUtil, filtered using wx's
celestial-body-origin and fixed-coordinate requirements. The selector converts a
cloned spacecraft with the six pending element values, then updates all six
labels, values and units only after successful conversion. Failures restore the
previous selector and keep the inputs intact. Pending coordinate, epoch or anomaly
changes must be applied first so conversion uses the intended reference settings.

WorkflowTests switches Cartesian to Keplerian, checks SMA against the configured
state, rejects invalid input without losing it, edits SMA, switches back to
Cartesian and then Keplerian, and applies the result. The configured spacecraft
remains unchanged until Apply; the reconstructed state retains the SMA edit and
Undo restores the original script. Other representations and combined coordinate
changes still need dedicated qualification.

The audit also exposed a generic state-editing failure: spacecraft element aliases
resolve to IDs outside the normal property range, where read-only validation
throws. Resource assignment now resolves current display labels to their six
writable Element IDs. Apply establishes DisplayStateType before assigning
elements, avoiding alphabetical ordering that could apply AOP before the new
representation. Both fixes are exercised by the pending-state conversion test.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 50.20 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-state-representation.txt`.

## Enumeration versus color controls

The shared resource-property reader incorrectly fell through from ENUMERATION_TYPE
into COLOR_TYPE, marking enumerations as colors. Fields whose engine enum list
was empty could consequently receive a color button, including spacecraft
AnomalyType and extra controls beside date/state settings. Enumeration handling
now reads its string value and finishes without setting the color flag.

WorkflowTests verifies that DateFormat, DisplayStateType and AnomalyType do not
offer color buttons or color metadata, while OrbitColor and TargetColor retain
their pickers. Existing PlotTests still exercise actual color selection, Cancel,
Apply, rollback and rendered trajectory color.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 35.00 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-enum-controls.txt`.

## GMAT function-file editor

Audited wx FunctionSetupPanel and added an Edit function file button to Qt
GmatFunction resources. It opens the selected FunctionPath in ScriptEditor with
find/replace, Save and Cancel. Saving uses QSaveFile and checks the current file
against the loaded bytes before writing, preventing silent overwrite of external
edits. Failed reads and invalid UTF-8 disable saving; BOM and uniform CRLF style
are retained. Run already rebuilds the mission before executing functions.

FileTests exercises content changes, BOM/CRLF retention, external-change conflict
and invalid UTF-8. CompatibilityTests runs a temporary GMAT function, edits its
multiplier through the resource button and dialog Save, then saves/reopens the
mission with failed-build recovery and verifies the changed output. wx's separate
function-file Save As workflow remains pending; MATLAB editing is outside the
current Linux/no-MATLAB scope.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 40.50 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-function-editor.txt`.

## Function-file Save As

FunctionFileDialog now provides Save As with a native destination chooser and
overwrite confirmation. Success writes the current editor contents to the new
file, preserving encoding/line-ending behavior, and returns its path to the
resource panel. FunctionPath remains pending until resource Apply rebuilds the
mission. Failed saves do not change the current path; saving to the original path
retains the external-change check.

FileTests covers Unicode/spaced destinations, preservation of unsaved contents,
existing-destination refusal without confirmation, failed destination writes,
and Cancel leaving the original untouched. CompatibilityTests opens the actual
Save As chooser, creates a copy in another directory, checks the old file remains,
checks the engine path is unchanged before Apply, applies the new path, then
saves/reopens the mission and executes the edited function. Creation of a new
empty function file is still outside this existing-file editor workflow.

Validation: rebuilt the user's GmatQt. The full run passed the other 10 Qt checks,
including Compatibility's actual Save As/execution workflow. Files initially
failed because its new assertion expected a trailing newline absent from the
editor contents. After correcting that expectation and adding Cancel coverage,
Files passed in 3.40 seconds. Thus every check has a passing result for the final
implementation, across the full and targeted runs. Native viewers used Xvfb.
Evidence: `Qt6ParityValidation/check-function-save-as.txt` and
`Qt6ParityValidation/check-function-save-as-files.txt`.

## New GMAT function-file creation

ResourceEditor now offers New function file for a GmatFunction resource. It asks
for an unused destination and opens a basic positional input/output template with
the resource's function name. Saving creates the file and leaves FunctionPath
pending until Apply. Cancel creates nothing. If a file appears at the chosen
destination while the template is open, Save refuses to overwrite it.

FileTests covers creation, Cancel and an externally created destination.
CompatibilityTests drives the destination chooser, edits the generated template,
saves, applies the path, saves/reopens the mission with failed-build recovery and
checks the function's resulting output. The resource in this fixture already
exists; combined resource creation and broader function signatures still require
additional qualification.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 41.98 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-new-function.txt`.

## Function-call input and output selection

CommandForm now offers ordered argument selectors for function inputs and outputs,
using shared parameter browsing plus whole space-point/impulsive-burn objects and
strings. Output browsing filters writable parameters and rejects numeric output
destinations. Inputs retain free text for literals and other function syntax.
Argument splitting respects quoted strings and nested parentheses/brackets;
command source spans preserve the function name and trailing comment.

CompatibilityTests checks Cancel, ordering, invalid output recovery, and a real
two-input/two-output function after Apply, save/Save As, failed-build recovery and
reopen. Swapping both lists must produce the expected sum/difference in the
selected output variables. A separate source-preservation check covers quoted
commas and array-index commas; it does not claim execution coverage for those
string/array signatures. Broader object arguments and zero-output calls remain
qualification cases.

Validation: rebuilt the user's GmatQt. The full suite passed ten checks; the
compatibility check initially expected a change notification when accepting an
unchanged argument list. The corrected test retains the original source as its
initial value and checks any emitted replacement against it. The targeted
compatibility rerun passed in 9.54 seconds. No production code changed between
the full run and that rerun. Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-function-arguments.txt` and
`Qt6ParityValidation/check-function-arguments-compat.txt`.

## Omitted solver-command option controls

The wx Vary and Achieve panels expose settings even when the script omits them.
Qt now offers Add default options for these commands, using Vary/Achieve engine
prototypes for defaults. Existing option order and values remain intact; missing
values are inserted inside the command argument list. Labels, comments and
pending field edits survive insertion. Added controls remain editable.

MissionTests checks partially specified Vary options, completely omitted options
for both commands, preservation of pending solver/value edits, and prevention of
duplicate insertion. The targeting workflow applies newly exposed bounds, maximum
step and tolerance, saves/reopens, and converges to the selected goal. This does
not yet qualify solver-dependent enable/disable behavior or every plugin option.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 63.63 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-solver-option-controls.txt`.

## Solver-dependent Vary controls

Audited wx VaryPanel::SetControlEnabling against the engine capability flags.
Qt now enables perturbation, bounds, maximum step and scale-factor fields only
when the selected configured solver supports them. Unsupported fields retain
their values and explain why they are disabled. An unknown solver disables these
fields until a valid solver is selected; source validation remains with Apply.

MissionTests switches from DifferentialCorrector to Yukon and back, verifies
Yukon's disabled bounds and enabled supported settings, and checks that pending
values, labels and comments survive. Unknown-solver recovery and Cancel are
covered. The existing targeting test still verifies Apply, save/reopen and solve.
The scale/step/perturbation-disabled combinations of other plugins remain outside
this runtime qualification; MATLAB remains deferred.

The form also handles construction before engine initialization by leaving
capability-dependent fields disabled without querying the configuration manager.
The initial run exposed this case; after guarding the lookup, all 11 Qt tests
passed in 70.23 seconds and the user's GmatQt was rebuilt. Native viewer checks
used isolated Xvfb. Evidence: `Qt6ParityValidation/check-solver-capabilities.txt`.

## DifferentialCorrector resource editor

Audited DCSetupPanel's six fields: MaximumIterations, ReportStyle, ReportFile,
ShowProgress, Algorithm and DerivativeMethod. Qt already exposes the iteration,
report-style and progress controls in Convergence/Output sections. Algorithm and
derivative method were free text because the engine marks them as enumerations
without supplying their choice lists. Qt now supplies the same three choices for
each as wx and the engine setters. Solver ReportFile now uses an output chooser.

MissionTests configures Broyden, CentralDifference, Verbose report output and an
iteration limit through ResourceEditor. It checks chooser Cancel and that pending
settings do not alter the configured engine, applies them, saves/reopens, solves
the existing target, and verifies a nonempty report at the selected path. This
qualifies that combination; it does not establish all algorithm/derivative pairs.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 64.67 seconds.
Native viewer checks used isolated Xvfb. Evidence:
`Qt6ParityValidation/check-dc-controls.txt`.

## Centered scene Fit

Qt Fit previously used radius from the coordinate origin, making translated
scenes unnecessarily small and leaving the camera aimed away from their center.
Fit now centers the camera on the bounding box of visible curve history and uses
a sphere enclosing that box. Native bounds include loaded model extents, scale
and offsets, or body radii; hidden objects do not add body radius, and invisible
curves do not contribute. The software renderer uses the same camera calculation
with its displayed body radii. Script view retains the stored camera target and
distance until the user selects Fit. Framing uses all retained history for stable
replay, rather than changing with the playback frame.

NativeOrbit tests render an origin-centered body and a body translated by ten
million units, with an invisible distant curve, in perspective and orthographic
projections and portrait, landscape and square windows. They require a visible,
centered body with margins. A separate shared-camera check protects stored
scripted target/distance. This improves Qt Fit; automatic OF trajectory/segment
conversion remains a separate open requirement.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 62.63 seconds.
Native checks ran under isolated Xvfb with software OpenGL, not the user's live
Intel/Wayland session. Evidence: `Qt6ParityValidation/check-centered-fit.txt`.

## Automatic named whole-trajectory cameras

Audited OFScene::ProcessOpenFramesView/UpdateSegmentViews and OpenFrames
View::resetView. Automatic trajectory views center a bounding sphere and place
the eye on negative Y with positive Z up; perspective distance uses the smaller
horizontal/vertical half-angle. Qt conversion now retains a named object's
automatic whole-trajectory camera in primary and secondary camera metadata.
Stored camera locations continue to take precedence. Unknown trajectory names
or names absent from OrbitView.Add fail Build with a diagnostic.

The shared camera calculation uses a sphere enclosing the selected curve's
retained path bounds, independently of unrelated bodies and trajectories. Qt
refreshes these bounds as samples arrive and uses complete retained history
during replay. This preserves Qt's live viewer behavior; OF refreshes automatic
trajectory views after completed segments. Fit still frames the whole visible
scene and Script view restores automatic trajectory framing. CoordinateSystem
automatic framing, automatic LookAt combinations and segment-specific cameras
remain unsupported by this conversion and receive explicit errors. Bounding
spheres of OF scene decorations are not claimed to match Qt path bounds.

WorkflowTests covers primary/secondary conversion metadata, unchanged mission
commands, a real converted sample after save/reopen, plot-model delivery, invalid
trajectory recovery and stored-camera precedence. OrbitRendererTests checks
translated path centers and aspect-aware camera distance against explicit
geometric expectations, unrelated-object isolation and named-camera selection.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 39.98 seconds.
After improving the unsupported-LookAt fixture to use valid syntax, the targeted
Workflow rerun passed in 21.85 seconds. Native checks used isolated Xvfb/software
OpenGL. Evidence: `Qt6ParityValidation/check-auto-trajectory.txt` and
`Qt6ParityValidation/check-auto-trajectory-workflow.txt`.

## Automatic trajectory LookAt orientation

Automatic named whole-trajectory conversion now supports LookAtFrame with both
ShortestAngle modes. The shared camera calculation transforms the trajectory
bounding center as well as eye/up orientation, using the published camera frame.
User rotation remains an offset around that transformed center. Primary and
secondary named views retain the mode through save/reopen. CoordinateSystem
automatic framing and segment-relative views remain open.

The optional OpenFramesCameraReference probe now compares complete automatic
world-to-view transforms against the installed OpenFrames library: a translated
bounding sphere, 12 target directions, both DIRECT/AZEL modes and three aspect
ratios (72 cases). Maximum transformed-point difference is 8.92633e-06 scene
units, within the 1e-4 tolerance for the reference's float bounding sphere. The
original 48 stored-frame alignment cases still pass. This adds no OpenFrames
dependency to the Qt application.

WorkflowTests converts primary and named automatic LookAt cameras, saves/reopens
and runs them, switches cameras, verifies target direction and orthogonal up,
and checks that ShortestAngle changes roll.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 54.93 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-look-at.txt` and
`Qt6ParityValidation/auto-look-at-reference.txt`.

## Explicit camera edits after automatic conversion

Explicit OrbitView resource edits now take precedence over imported automatic
trajectory framing. Editing ViewPointReference, ViewPointVector, ViewScaleFactor,
ViewDirection, ViewUpAxis or ViewUpCoordinateSystem clears the primary automatic
mode and its imported LookAt/center override. Explicit up-axis or up-coordinate
edits also remove the imported arbitrary up vector. Named camera presets retain
their independent settings.

WorkflowTests applies a manual primary position/direction, verifies independent
named camera metadata, saves/reopens, runs and checks the resulting camera pose.
It also checks all six individual pose controls and exact-source Undo, then runs
the restored automatic camera. This closes a GUI conflict in which successful
Apply could leave the imported automatic camera overriding explicit user values.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 38.11 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-camera-overrides.txt`.

## Automatic CoordinateSystem cameras

Audited OFScene root creation (hidden root axes, position zero),
ProcessOpenFramesView and ReferenceFrame::getBound/View::resetView. Root bounds
do not include child scene objects. Without LookAt, OFScene supplies twelve
Earth equatorial radii as the automatic bounding radius; with LookAt, the empty
root bound falls back to one unit. Qt now converts both primary and named
CoordinateSystem views without a stored location, regardless of ViewTrajectory.
It retains these radius rules and viewport/FOV scaling. Stored locations retain
precedence; Fit and explicit camera overrides retain their Qt behavior.

The optional OpenFrames reference probe adds 12 root-camera cases: both LookAt
states, both rotation modes and three aspect ratios. All pass with maximum
point difference 0.0103549 scene units. OF constructs its automatic home eye
with a float Vec3; the distant-camera comparison therefore uses a 1e-7 relative
tolerance (minimum 1e-4 absolute), rather than claiming double-precision identity.
The original 48 stored-frame and 72 trajectory LookAt comparisons still pass.

WorkflowTests checks unchanged mission text during conversion, primary/named
metadata, save/reopen/run, origin center and the expected radius/FOV distance
with and without LookAt. Segment-relative cameras and automatic body-bound
framing remain open requirements.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 39.31 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-origin.txt` and
`Qt6ParityValidation/auto-origin-reference.txt`.

## Automatic body and model framing

Audited OFSpaceObject's frame selection and Sphere::getBound: planet views use
the sphere radius; model views use geometry bounds. Converted primary and named
body views without stored locations now retain automaticBody metadata instead
of using a fixed camera distance. The existing body-relative/inertial/LookAt
pose supplies orientation and origin. Native rendering supplies the loaded
model's transformed bounding sphere, including offset, scale and rotation, or
the displayed body's radius. This retains Qt/wx model sizing rather than
changing model size to OF's convention. The software renderer uses its displayed
body radius or unit fallback for a marker; it does not load native mesh bounds.

Camera references must agree with automaticBody and belong to the plot's Add
list. Explicit primary camera pose edits remove automatic framing and imported
body-relative orientation; named views remain independent. Source Undo restores
the imported settings.

NativeOrbit tests frame a translated/rotated textured mesh in a portrait window
and require substantial rendered coverage at normal and high DPI. WorkflowTests
checks converted primary planet/named spacecraft modes, save/reopen execution,
planet radius and center, invalid-reference recovery, manual overrides and Undo.
The preexisting body orientation and LookAt execution checks remain active.
This qualifies Qt-rendered geometry framing, not identical OF model scale or
optional decoration bounds. Segment-relative camera conversion remains open.

Validation: rebuilt the user's GmatQt; all 11 Qt tests passed in 50.58 seconds.
Native checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-auto-body.txt`.
