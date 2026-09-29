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
| `src/gui/command/TogglePanel.hpp` | CommandForm subscriber list and state controls; form coverage in MissionTests. Dedicated selection checklist pending. |
| `src/gui/command/GmatCommandPanel.hpp` | Pending audit |
| `src/gui/command/ManeuverPanel.hpp` | Pending audit |
| `src/gui/command/ScriptEventPanel.hpp` | Pending audit |
| `src/gui/command/NonlinearConstraintPanel.hpp` | Pending audit |
| `src/gui/command/AchievePanel.hpp` | Pending audit |
| `src/gui/command/ManageObjectPanel.hpp` | CommandForm Global/Clear object controls; form coverage in MissionTests. Reference-selection workflow pending. |
| `src/gui/command/BeginFiniteBurnPanel.hpp` | Pending audit |
| `src/gui/command/OptimizePanel.hpp` | Pending audit |
| `src/gui/command/TargetPanel.hpp` | Pending audit |
| `src/gui/command/VaryPanel.hpp` | Pending audit |
| `src/gui/command/FindEventsPanel.hpp` | Pending audit |
| `src/gui/command/PropagatePanel.hpp` | Pending audit |
| `src/gui/command/AssignmentPanel.hpp` | CommandForm destination/expression controls; source-preservation tests. Parameter chooser and complex syntax audit pending. |
| `src/gui/command/CallFunctionPanel.hpp` | Pending audit |
| `src/gui/command/EndFiniteBurnPanel.hpp` | Pending audit |
| `src/gui/command/MinimizePanel.hpp` | Pending audit |
| `src/gui/command/ReportPanel.hpp` | Pending audit |
| `src/gui/function/MatlabFunctionSetupPanel.hpp` | Pending audit |
| `src/gui/function/FunctionSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverVariablesPanel.hpp` | Pending audit |
| `src/gui/solver/SolverSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverGoalsPanel.hpp` | Pending audit |
| `src/gui/solver/DCSetupPanel.hpp` | Pending audit |
| `src/gui/solver/SolverCreatePanel.hpp` | Pending audit |
| `src/gui/solver/SQPSetupPanel.hpp` | Pending audit |
| `src/gui/controllogic/ForPanel.hpp` | CommandForm index/start/step/end; real-engine step edit and Undo tested. Parameter selection audit pending. |
| `src/gui/controllogic/ConditionPanel.hpp` | CommandForm If/While condition; real-engine branch result and Undo tested. Structured compound-condition builder pending. |
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
| `src/gui/propagator/PropagationConfigPanel.hpp` | Pending audit |
| `src/gui/propagator/PropagatorSelectDialog.hpp` | Pending audit |
| `src/gui/asset/GroundStationPanel.hpp` | Pending audit |
| `src/gui/debugger/InspectorPanel.hpp` | Pending audit |
| `src/gui/forcemodel/DragInputsDialog.hpp` | Pending audit |
| `src/gui/coordsystem/CoordSysCreateDialog.hpp` | Pending audit |
| `src/gui/coordsystem/CoordSystemConfigPanel.hpp` | Pending audit |
| `src/gui/coordsystem/CoordPanel.hpp` | Pending audit |
| `src/gui/output/ReportFilePanel.hpp` | Pending audit |
| `src/gui/output/EventFilePanel.hpp` | Pending audit |
| `src/gui/output/CompareReportPanel.hpp` | Pending audit |
| `src/gui/mission/UndockedMissionPanel.hpp` | Pending audit |
| `src/gui/mission/TreeViewOptionDialog.hpp` | Pending audit |
| `src/gui/view/ViewTextDialog.hpp` | Pending audit |
| `src/gui/view/FindReplaceDialog.hpp` | Pending audit |
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
| `src/gui/subscriber/ReportFileSetupPanel.hpp` | Pending audit |
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
| `../plugins/libExtraPropagators` | Pending qualification |
| `../plugins/libFormation` | CompatibilityTests: Add editing/reordering, non-spacecraft rejection, exact save/Save As/reopen and failed-build recovery, both members propagate 60 seconds. Remaining settings/output coverage pending. |
| `../plugins/libGmatFunction` | CompatibilityTests: edited cross-product arguments, exact save/Save As/reopen, invalid-type build recovery, expected numerical cross product. Broader function and report audit pending. |
| `../plugins/libMsise00` | Pending qualification |
| `../plugins/libNewParameters` | Pending qualification |
| `../plugins/libPolyhedronGravity` | Pending qualification |
| `../plugins/libProductionPropagators` | Pending qualification |
| `../plugins/libPythonInterface_py314` | Shipped Python example: exact save/Save As/reopen and failed-build recovery, independently computed cross-product result and report. Remaining call types and runtime-error recovery pending. |
| `../plugins/libSaveCommand` | Pending qualification |
| `../plugins/libScriptTools` | Pending qualification |
| `../plugins/libStation` | Pending qualification |
| `../plugins/libThrustFile` | Pending qualification |
| `../plugins/thinksys/libTLEPropagator` | Pending qualification |
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
