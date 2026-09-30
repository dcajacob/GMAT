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
| `src/gui/hardware/ThrusterCoefficientDialog.hpp` | Grouped chemical/electric coefficient controls implemented; chemical and electric polynomial finite-burn execution, analytic fuel consumption and configuration round trips covered; broader coefficient regimes pending |
| `src/gui/hardware/BurnThrusterPanel.hpp` | Active thruster and impulsive-burn controls audited. Dependent Local frame/axes/origin, direction, scale, duty cycle and gravitational acceleration; ordered typed thruster tanks/mixtures and single impulsive tank; mass-decrement off, clear/restore and failed-burn recovery covered. Impulsive MJ2000Eq/VNB/LVLH/SpacecraftBody/EarthFixed execution, analytic fuel/frame checks and backward restoration covered. Broader frame/operating combinations pending. |
| `src/gui/hardware/ThrusterConfigPanel.hpp` | Active chemical/electric controls audited. Grouped direction/performance setup, electric model dependencies, all three electric models, minimum-power cutoff, maximum clipping, GUI-attached nuclear power, chemical/electric mixtures, finite burn/coast, numerical report equivalence and round trips covered. Shared power, solar/eclipses, broader polynomial/frame regimes pending. |
| `src/gui/hardware/PowerSystemConfigPanel.hpp` | wx field inventory audited; grouped general/bus/solar/shadow controls and shadow-body picker covered. List reconstruction, invalid-body rollback and Undo tested. Epoch-format conversion, failed conversion recovery and paired Apply/Undo covered; GUI-configured nuclear and unshadowed solar report execution covered; eclipse attenuation and decay cases pending. |
| `src/gui/hardware/TankAndMixDialog.hpp` | Combined tank/ratio editor, first-tank addition, type filtering, reorder with paired ratios, paired Apply, round trips and two-tank chemical/electric burns covered. Mass-decrement off, clear-all save/reopen and failed-burn restore covered. Broader tank combinations remain pending. |
| `src/gui/event/EventLocatorPanel.hpp` | Common and Contact/Eclipse/Intrusion-specific controls audited. Grouped Qt editor provides typed targets/bodies/observers/sensors/shadow types, paired epoch conversion, interval/light-time/report dependencies and input/output pickers. Pending Apply/Cancel, validation/rollback, Undo/Redo/save/reopen, bounded contacts, Transmit/Receive corrections, selected detailed reports, eclipse intervals, shipped Mercury transit and failed-output-directory recovery covered. FixedGrid execution, region/spacecraft-observer contacts, additional formats/coverage boundaries and disk-write failures remain pending. |
| `src/gui/command/TogglePanel.hpp` | Subscriber checklist and On/Off dropdown; empty selection, Cancel, filtering and dual-report suppression/resumption after save/reopen tested. Plot/ephemeris and solver-loop Toggle combinations pending. |
| `src/gui/command/GmatCommandPanel.hpp` | Generic editable command text, interpretation/object validation, failure rollback and shared inspection buttons audited. Qt full-mission transactional Apply retains the text fallback; InspectionTests exercises ClearPlot to MarkPoint correction, missing-reference rollback, Unicode save/reopen and report invariance. Other generic command types remain partial. |
| `src/gui/command/ManeuverPanel.hpp` | Typed impulsive-burn and spacecraft selectors; Cancel, label/comment preservation, save/reopen and inertial delta-V execution tested. Backprop checkbox and reverse inertial delta-V tested. BurnTests extends execution to GUI-configured MJ2000Eq/VNB/LVLH/SpacecraftBody/EarthFixed, fuel depletion and backward state/fuel restoration. Broader spacecraft/frame/error combinations pending. |
| `src/gui/command/ScriptEventPanel.hpp` | wx comment/body separation, fixed Begin/End labels, resizable editor areas and pending Save/validation audited. Qt Script event dialog provides separate plain comments and a highlighted, numbered script body with a splitter; preserves named/inline outer boundaries and nested content. MissionTests covers opening without changes, comment-only preservation, Cancel, invalid-command rollback and correction, nested branches/events and quoted marker literals, single pending Undo/Redo, exact mission Undo/Redo, Unicode save/reopen and empty-body execution. Native Wayland layout/execution inspected. Common editor/menu workflows remain under their separate inventory audits. |
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
| `src/gui/spacecraft/OrbitPanel.hpp` | Epoch-format conversion, invalid-date recovery and pending state edits covered. Typed frame/anomaly selectors, dependent representation restrictions, physical-state caching and grouped Apply implemented. All fourteen representation display round trips, Earth-fixed/Moon/barycenter frames, circular/missing-frame recovery, paired epoch/state conversion, exact Undo/Redo and Planetodetic/MA/EA/HA save/reopen/report execution covered. Unrelated Apply/deletion preserve these states. Broader representation editing, singularities and specialized frames remain to qualify. Orbit Designer/Summary are legacy hidden workflows; see the source audit below. |
| `src/gui/spacecraft/PowerSystemPanel.hpp` | Single typed selection plus empty selection audited. Direct dropdown with No power system, nuclear/solar candidates and pending Apply implemented. Returning to None, wrong-type rollback, attachment, save/reopen report execution and detachment covered. Electric propulsion consumes GUI-attached nuclear power in all three thrust models; missing-power failure and attachment recovery covered. Solar/shared-power propulsion cases pending. |
| `src/gui/spacecraft/BallisticsMassPanel.hpp` | All eleven wx controls audited. Focused Spherical/SPAD editor, input file choosers and engine interpolation choices implemented. Cancel, invalid-input recovery, pending edits, paired Apply, exact-source Undo/Redo, save/reopen and GUI-configured SPAD SRP execution/report values covered. SPAD drag force execution with Bilinear/Bicubic interpolation, scale 1.5 and missing-file recovery covered. Broader assets and interpolation/scale combinations remain to qualify. |
| `src/gui/spacecraft/TankPanel.hpp` | wx add/remove/add-all/remove-all attachment operations audited. Typed checklist, ordering and bulk controls implemented. Cancel, pending selection, paired Apply, Undo/Redo, invalid references, save/reopen, chemical two-tank burn/report and complete detachment covered. Electric tank attachment/execution combinations remain to qualify. |
| `src/gui/spacecraft/OrbitDesignerDialog.hpp` | Source audited: launcher explicitly hidden by OrbitPanel (GMT-3383). Retained legacy design tools are not exposed workflows in the base wx GUI; see the audit below. |
| `src/gui/spacecraft/VisualModelPanel.hpp` | File, rotation/translation/scale sliders and numeric fields, recenter/autoscale, new-file pose reset, colors and Earth-reference preview audited. Qt visual editor implemented with pending Apply, engine normalization and model-read diagnostics. Cancel, file/color pickers, invalid input recovery, Undo/Redo and save/reopen propagation covered. Textured OBJ preview/rotation/Earth reference tested at normal, 150% and 200% scaling and directly on Wayland. Broader 3DS asset/material cases remain to qualify. |
| `src/gui/spacecraft/AttitudePanel.hpp` | Model-dependent controls, frame restrictions, Euler/quaternion/MRP/DCM orientation and Euler-rate/angular-velocity selection audited. Focused Qt dialog with pending conversion, per-model edit retention, typed frame/body selectors and AEM input chooser implemented. Cancel, invalid state recovery, exact paired Apply/Undo/Redo, save/reopen and zero-rate Spinner report execution covered. Nonzero-rate propagation, specialized-model execution, other Euler sequences, frame dependencies and AEM/SPICE file cases remain to qualify. |
| `src/gui/spacecraft/SpaceObjectSelectDialog.hpp` | ResourceEditor engine-typed reference picker. Ordered tank selection, Cancel, pending state and mixture-preserving Apply tested; all object-specific uses still need audit. |
| `src/gui/spacecraft/OrbitSummaryDialog.hpp` | Source audited: read-only output of the hidden Orbit Designer, rather than a standalone spacecraft summary action; see the audit below. |
| `src/gui/spacecraft/SpacecraftPanel.hpp` | Wrapper Create/LoadData/SaveData/page-change behavior audited: eight active orbit, attitude, ballistics/mass, tank, power, SPICE, thruster and visual pages share a pending spacecraft clone; disabled sensor page excluded. Qt ResourceEditor and focused dialogs share pending values and one Apply/rebuild with rollback. Individual page evidence is recorded above/below. Broad mixed-page transactions and focus/navigation behavior remain to qualify. |
| `src/gui/spacecraft/ThrusterPanel.hpp` | wx attachment operations audited. Typed checklist with bulk selection/removal covered by Cancel, pending Apply, replacement of a decoy engine, exact-source Undo/Redo, missing-reference rollback and reopened finite-burn execution. Full detachment covered; electric execution with script-attached thrusters and GUI-attached power covered, electric GUI attachment combinations remain to qualify. |
| `src/gui/spacecraft/SpicePanel.hpp` | Ordered SPK/CK/SCLK/FK file-list controls added. SPK add/duplicate/order/Cancel/Apply, Undo/Redo, save/reopen, clear and missing-file recovery tested with bundled kernel copies. Mars Express SPK/CK/SCLK execution, Qt trajectory/attitude capture and clock-file recovery tested. FK runtime use and other NAIF combinations pending. |
| `src/gui/spacecraft/FormationSetupPanel.hpp` | ResourceEditor Add list and spacecraft picker; invalid member rejection, reordering, save/reopen and two-member propagation tested. Remaining wx-specific operations under audit. |
| `src/gui/foundation/GmatBaseSetupPanel.hpp` | Generic writable/visible field generation, cloned validation and INI-based layout/units/help audited. Qt ResourceEditor supplies engine-typed controls, owned properties and atomic script rebuild/rollback; object-specific execution evidence is recorded in individual rows. INI metadata parity, omitted plugin types and all dynamic refresh cases remain pending. |
| `src/gui/foundation/GmatDialog.hpp` | Shared OK/Cancel/reset, validation-before-close and Help contract audited. Qt focused dialogs keep pending values until acceptance; field/dialog Cancel and invalid-input recovery are covered by the corresponding suites. Close/Escape/focus behavior across all dialogs and context Help remain pending. |
| `src/gui/foundation/ParameterCreateDialog.hpp` | Active wx numeric Variable, literal String, Array creation (dimensions 1–1000), name validation and existing-user-parameter operations audited. Qt New resource supplies typed initial values, retains values on type changes, rejects duplicate/reserved names and invalid dimensions, and opens the created resource in the tree. Parameters tests cover actual dialog Cancel, correction, creation Undo/Redo, Unicode String and maximum Array creation, then deletion without changing original report results. Existing parameters open from Resources; wx list/Clear layout and shared Help remain unqualified. |
| `src/gui/foundation/ParameterSelectDialog.hpp` | Pending audit |
| `src/gui/foundation/SinglePathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatPanel.hpp` | Shared Apply/OK/Cancel, dirty-state, resource refresh, Help, Script and Summary contract audited. Qt resource/command panels validate and rebuild atomically, close successful snapshots, reject stale edits and protect pending changes. Read-only applied-script previews and command/mission summaries are now covered by InspectionTests. Context Help and staying open after Apply remain pending. |
| `src/gui/foundation/MultiPathSetupPanel.hpp` | Pending audit |
| `src/gui/foundation/GmatColorPanel.hpp` | COLOR_TYPE resource fields and visual picker/swatch added. Spacecraft orbit/target Cancel, pending Apply, Undo/Redo, invalid RGB rollback, save/reopen and published trajectory color tested. Per-view override controls and other resource types pending. |
| `src/gui/foundation/ArraySetupDialog.hpp` | wx numeric grid, direct row/column selection, Value/Update, finite-value validation and clone/commit audited. Qt numeric grid adds direct Row/Column/Value/Set cell controls and Enter support; selection scrolls to the cell and synchronizes its value. Parameters tests cover actual 1000×1000 creation, last-cell navigation, invalid Set, Cancel, pending acceptance/Apply, adjustable columns and save/reopen. Broader keyboard/focus and shared Help remain unqualified. |
| `src/gui/foundation/ShowScriptDialog.hpp` | Read-only object-generated script, monospaced/unwrapped display and Close audited. Qt resource and command Show script dialogs capture applied configuration; actual MDI controls preserve pending edits/source/undo state. Local Find and Copy are available. Singleton formatting, font zoom and broader object families remain unqualified. |
| `src/gui/foundation/GmatSavePanel.hpp` | Shared Save/Save As, save-build-run, active/dirty status, reload and close contract audited. FileTests, WorkflowTests and ScriptEditingTests cover the single Qt mission document, failure/cancel identity protection, encoding, save-before-run and pending/close protection. Multiple inactive documents, panel-specific reload/status and remaining shared editor cases remain pending. |
| `src/gui/foundation/ParameterSetupPanel.hpp` | Active wx disabled Name and numeric Value/String Expression controls and Apply audited. Qt focused Initial value controls replace the ineffective generic fields; source edits preserve grouped declarations, comments, optional semicolons and subsequent mission assignments. Interpreted-value postconditions reject silent String truncation. Parameters tests cover pending values, applied script previews, invalid correction/rollback, exact Undo/Redo, Unicode save/reopen, native Wayland and independently scripted numeric/text reports. Shared Help and staying open after Apply remain pending. |
| `src/gui/foundation/ArraySetupPanel.hpp` | ResourceEditor resizeable numeric grid plus separate mission-start expression grid; retained/new cells, dependent formulas, Cancel, rollback, Undo/Redo and save/reopen tested. Combined numeric/expression Apply, pending resize dimensions, shrink cleanup, atomic Undo and rollback covered; arbitrary existing assignments remain outside the grid workflow. Numeric dimensions now cover wx 1–1000 per axis; direct cell controls and maximum-array Cancel/Apply/reopen are covered by Parameters tests. |
| `src/gui/foundation/ShowSummaryDialog.hpp` | Captured command state, entire mission/all or physics selection, non-spacecraft-dependent coordinate systems, frame-change error rollback and text export audited. Qt inspection suite compares command states to separate reports in four frames, handles BeginScript via EndScript, skips unexecuted states, rejects stale results and covers Unicode export/source protection, native Wayland, failed/stopped recovery. Broader solver loops, spacecraft hardware fields and font zoom remain unqualified. |
| `src/gui/propagator/PropagationConfigPanel.hpp` | Owned propagator settings exposed; numerical/TLE step edits and serialization tested. Atmosphere/drag controls and selected Earth execution cases covered; other specialized layout and remaining settings pending. |
| `src/gui/propagator/PropagatorSelectDialog.hpp` | Pending audit |
| `src/gui/asset/GroundStationPanel.hpp` | Active wx ID/elevation/body/state/horizon/location controls and colors audited. Grouped Qt station editor, dependent conversion/labels/units, color and horizon-mask pickers implemented. Cancel, pending Apply, paired state/location ordering, Earth/Mars geometry, compact scrolling, exact Undo/Redo/save/reopen, contact intervals, mask execution/clear and missing-mask recovery covered. Station hardware/media/error models and broader bodies/contact cases remain unqualified. |
| `src/gui/debugger/InspectorPanel.hpp` | Pending audit |
| `src/gui/forcemodel/DragInputsDialog.hpp` | Nine wx weather controls audited. Grouped Qt atmosphere/body/shape selection, dependent weather/Schatten controls and input pickers implemented. Earth MSISE90/JacchiaRoberts/NRLMSISE00 and Exponential configuration, validation/Cancel, paired Apply, Undo/Redo, save/reopen, density/trajectory reports and file-error recovery covered. CSSI historic/predicted and selected Schatten prediction covered; broader file contents, coverage boundaries, Schatten modes and non-Earth cases remain to qualify. |
| `src/gui/coordsystem/CoordSysCreateDialog.hpp` | Basic creation plus dedicated Axes dialog tested; MOEEq epoch and constrained-frame edits checked. Remaining origin and specialized-mode cases pending. |
| `src/gui/coordsystem/CoordSystemConfigPanel.hpp` | Axis replacement, dependent field exposure, protected built-ins, failed-edit rollback, Undo and save/reopen tested. Broader modes pending. |
| `src/gui/coordsystem/CoordPanel.hpp` | ObjectReferenced radial frame, MOEEq epoch edits and Sun-aligned LocalAlignedConstrained transforms checked, including save/reopen. Other modes and dependency cases pending. |
| `src/gui/output/ReportFilePanel.hpp` | Read-only, unwrapped report text, full path in title, text selection, close and unavailable-file handling tested. Standard Qt copy controls provided; large reports now have bounded paging, navigation, page-local search and reload recovery. Case-sensitive full-file search added; richer full-file search options remain pending. |
| `src/gui/output/EventFilePanel.hpp` | Pending audit |
| `src/gui/output/CompareReportPanel.hpp` | wx read-only, unwrapped comparison output and Close audited. Qt comparison workspace uses the paged ReportViewer with complete-file search and Close. ComparisonTests covers complete results/export beyond 16 MiB, error summaries, Stop/close and generated-report agreement; native Wayland layout inspected. Broader search/menu and very large directory cases remain under their separate audits. |
| `src/gui/mission/UndockedMissionPanel.hpp` | Pending audit |
| `src/gui/mission/TreeViewOptionDialog.hpp` | Pending audit |
| `src/gui/view/ViewTextDialog.hpp` | Pending audit |
| `src/gui/view/FindReplaceDialog.hpp` | Nonmodal Find/Replace with next/previous, wrap, session histories, selected replacement and Replace All. Case/whole-word controls, no-match feedback, read-only protection and single-operation Undo tested. |
| `src/gui/solarsys/LibrationPointPanel.hpp` | Active primary/secondary, L1–L5 and orbit/target color controls audited. Typed celestial-body/barycenter choices exclude spacecraft, libration points and SSB; paired Apply rejects equal bodies. Pending choices/colors, invalid edit rollback, exact Undo/Redo, Unicode save/reopen, coordinate reports and orbit publications covered. Earth/Luna all-five geometry and Sun/custom-barycenter execution covered; broader body/epoch regimes pending. |
| `src/gui/view/EditorPanel.hpp` | Active save/sync/run, empty-input protection and shared SavePanel actions audited. Qt Mission menu adds Save/build and Save/build/run; ScriptEditingTests covers chooser Cancel, successful save before Build/Run, Unicode paths, independently checked outputs, failed-save/source/identity protection, invalid-script recovery and empty-script protection. Existing FileTests/WorkflowTests cover encoding, Undo/Redo, syntax, Find/Replace, running/close and pending-panel protection. Multiple inactive documents and broader shared editor/menu behavior remain pending. |
| `src/gui/app/CompareFilesDialog.hpp` | Active wx modes, absolute tolerance, skip blanks, baseline/candidate prefixes, up to three directories, file limit and result export audited. Qt File > Compare files workspace and report Compare action provide these controls plus two-file comparison. ComparisonTests covers UTF-8/BOM/CRLF, tolerance boundaries, UTC columns, maxima, trailing rows, invalid input, three-directory matching/.truth fallback/exact limits, editable widths, picker Cancel/row removal, full export/input protection, Stop/close and real-engine report/save/reopen invariance. Native Wayland inspected. Non-UTF-8 files, arbitrary initial header/data ambiguity, mid-read replacement and extreme directory/record regimes remain unqualified. |
| `src/gui/app/ScriptPanel.hpp` | Legacy plain editor save/sync/run, line-number navigation and failed-save identity behavior audited. Qt script editor provides line numbers, syntax coloring and bounded Edit > Go to line with Cancel; Save/build and Save/build/run are qualified by ScriptEditingTests. Single-document workflows are covered; multiple inactive documents, line-navigation edge cases and broader shared SavePanel behavior remain pending. |
| `src/gui/solarsys/CelestialBodyOrientationPanel.hpp` | Pending audit |
| `src/gui/solarsys/UniversePanel.hpp` | Pending audit |
| `src/gui/solarsys/CelesBodySelectDialog.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyPanel.hpp` | Pending audit |
| `src/gui/solarsys/CelestialBodyVisualizationPanel.hpp` | Pending audit |
| `src/gui/subscriber/GroundTrackPlotPanel.hpp` | Active body/object, sampling/update/retention/redraw, visibility, solver and texture controls audited against current GroundTrack runtime and legacy GL behavior. Grouped Qt setup, typed selections, per-body maps, decoded-image validation and engine texture-path resolution implemented. Cancel/pending Apply, compact scrolling, Undo/Redo/save/reopen, rendered custom-map pixels, station-only plots, Mars frame/report agreement and one-point retention covered. Broader solver, body/station and runtime asset-loss combinations remain to qualify. |
| `src/gui/subscriber/XyPlotSetupPanel.hpp` | Active wx ShowPlot/ShowGrid/SolverIterations, single X and ordered Y selection audited. Focused Qt setup and numeric property/frame/array browsers implemented. Cancel, pending Apply/reopen, invalid-reference rollback, exact Undo/Redo/save/reopen, grid/visibility and curve/report agreement covered. Full solver-iteration modes and broader burn/hardware parameter execution remain to qualify. |
| `src/gui/solarsys/CelestialBodyPropertiesPanel.hpp` | Pending audit |
| `src/gui/solarsys/BarycenterPanel.hpp` | Active body add/remove/clear and colors audited. Qt membership checklist, retained order, nonempty/unique/celestial-body validation, pending/Cancel/rollback, exact Undo/Redo/save/reopen, mass-weighted positions and dependent frame/libration execution covered. Built-in membership is protected while colors remain editable and persist without creating a new definition. Broader membership/epoch regimes pending. |
| `src/gui/solarsys/CelestialBodyOrbitPanel.hpp` | Pending audit |
| `src/gui/app/CompareTextDialog.hpp` | Source audited: this compiled legacy class has no caller in the current wx GUI. GmatMainFrame::CompareFiles uses CompareFilesDialog for text/numeric comparison, mapped to the qualified Qt comparison workspace above. No separate exposed workflow was found. |
| `src/gui/subscriber/EphemerisFilePanel.hpp` | Active wx output, sampling, interval and dependent-format controls audited. Grouped Qt editor, typed spacecraft/frame selection, editable sampling/endpoints, paired epoch conversion, format-specific byte order/units/events and filename chooser implemented. OEM (custom extension), STK meters, Code-500 both byte orders and SPK exports/readback covered by pending/Cancel, invalid-edit rollback, exact Undo/Redo, Unicode script save/reopen, independent report/state checks, Output access, directory preservation, coverage and missing-file recovery. CK quaternion, covariance/acceleration, broader frames/bodies/event boundaries and disk-write cases remain unqualified. |
| `src/gui/burn/FiniteBurnSetupPanel.hpp` | Active wx individual/bulk thruster add/remove operations audited; Qt typed checklist and ordered selection serve the workflow. Cancel/pending Apply, paired-engine execution, analytic fuel/coast, report equivalence, Undo/Redo, Unicode save/reopen, wrong/missing/duplicate references, unattached-thruster recovery and clear-all covered. Empty active burns produce the same explicit engine diagnosis as scripts; GUI reselection recovers. Broader electric/shared-power combinations pending. |
| `src/gui/burn/ImpulsiveBurnSetupPanel.hpp` | Active wx fields audited. Grouped delta-V/frame/optional mass-depletion editor, single typed fuel tank, Isp/gravity dependency and corrective validation implemented. Inertial and all four Local axes, EarthFixed and zero delta-V covered by pending/Cancel, Undo/Redo, Unicode save/reopen, script-reference state, analytic fuel and VNB/LVLH transforms, backward restoration, invalid edit rollback, unattached-tank recovery and mass-off tank clear. Broader bodies, attitudes, epochs and fuel limits pending. |
| `src/gui/app/FileUpdateDialog.hpp` | Pending audit |
| `src/gui/app/TextEphemFileDialog.hpp` | Pending audit |
| `src/gui/subscriber/TsPlotOptionsDialog.hpp` | PlotWidget Style dialog: per-curve visibility, lines/markers, widths, marker sizes/shapes, line styles, colors and error bars; plot grid/legend. Cancel, existing-point styling, curve isolation and rendered differences tested. Remaining axes/range options pending audit. |
| `src/gui/subscriber/OrbitViewPanel.hpp` | Active object/draw, camera, frame/up-axis/scale, drawing/star, solver and data controls audited. Grouped Qt setup, ordered/paired visibility Apply, pending/Cancel, validation/rollback, exact Undo/Redo/save/reopen, report invariance, object/vector camera histories and UseInitialView rerun/close/reopen behavior covered. Drawing-only edits retain imported primary-camera metadata. Broader solver-loop display and camera/frame combinations remain pending. |
| `src/gui/app/RunScriptFolderDialog.hpp` | Pending audit |
| `src/gui/subscriber/OpenGlOptionDialog.hpp` | Pending audit |
| `src/gui/subscriber/SubscriberSetupPanel.hpp` | Generic writable subscriber fields, boolean choices, load/save and validation audited. Qt ResourceEditor exposes engine-typed subscriber properties and specialized report/plot/file controls; selected execution, round trips and recovery are covered by plot, dynamic-data, ephemeris and report suites. Remaining subscriber types and generic field combinations remain pending. |
| `src/gui/subscriber/DynamicDataDisplaySetupPanel.hpp` | Active grid resize/retained cells, cell editing/clearing and condition colors audited; grouped Qt setup with pending Apply/Cancel, Undo/Redo, Unicode save/reopen, live reports, adjustable widths and immediate close/reopen covered. Extreme dimensions and unnamed-cell styling remain pending. |
| `src/gui/subscriber/ReportFileSetupPanel.hpp` | Report parameter lists accept numeric array elements; readable delimiter selector, precision/width rejection, exact save/reopen and explicit/automatic report values tested. Append across repeat runs, fixed-width headers, left/right alignment and zero fill tested. Shared ordered parameter selector added; owned attitude and attached tank/thruster browsing tested; broader hardware/plugin types and solver-iteration combinations pending. |
| `src/gui/subscriber/DynamicDataSettingsDialog.hpp` | Active parameter selection, text/background colors and warning/critical bounds audited. Real/string/array-element references, duplicate/whole-array/nonnumeric/reversed-bound rejection, Cancel, alarm/custom colors and calculation-preserving round trips covered. Broader parameter contexts and unnamed-cell styling remain pending. |
| `src/gui/app/WelcomePanel.hpp` | Pending audit |
| `src/gui/app/AboutDialog.hpp` | Pending audit |
| `src/gui/app/InteractiveMatlabDialog.hpp` | Pending audit |
| `src/gui/app/SetPathDialog.hpp` | Pending audit |

## Selected runtime plugin inventory

Every row requires real-engine evidence, not just registration.

| Plugin | Configuration / execution / reports / recovery evidence |
| --- | --- |
| `../plugins/libDataInterface` | DataInterfaceTests: GUI input-file selection and format, typed Set target/source and all/seven field subsets, independent epoch/state/Cr and propagated reports, exact Undo/Redo/Unicode save/reopen, Output access, missing/malformed/invalid-epoch/missing-field/unknown-field recovery, Task-9 input and converted shortened shipped OF example covered. Broader bodies/frames, multiple records, repeated imports within one mission and filesystem permission failures remain pending. |
| `../plugins/libEphemPropagator` | Mars Express SPK configured through Qt kernel lists, converted viewer, exact round trips, report/view agreement and missing-clock recovery tested. EphemerisTests adds generated OEM, STK, Code-500 both byte orders and SPK readback, GUI-selected first spacecraft input files and propagator steps, Unicode script round trips, independent circular-orbit states, FromSpacecraft start clamping, after-coverage rejection and missing-file restore/reopen. Broader frames/bodies, segment gaps, backward/boundary stepping and multiple-kernel coverage cases remain pending. |
| `../plugins/libEKF` | KalmanTests: one-hour, noise-free GPS version of the shipped filter/smoother example with SNC process noise and Gauss-Markov drag. Typed run/reference/solve-for controls, owned model settings, warm-start input/output browsing and paired epoch conversion, both continuation boundaries, exact state/covariance CSV equivalence with independent script configuration, report access, pending/Cancel/Undo/Redo/Unicode mission reopen, invalid-edit rollback and missing/malformed/late-seed recovery covered. Native Wayland panels/execution passed. Creation/removal, full-day/noisy or real data, other measurement/model regimes, covariance editing, residual graphics, prediction, warm-start smoothing and broader malformed/disk cases remain pending. |
| `../plugins/libGmatEstimation` | EstimationTests: Qt tracking path/type table, typed simulator/estimator and station/solve-for lists, observation output selection, typed run commands, noise-free shortened shipped range-skin simulation/batch fit, independent state/observation equivalence, exact Undo/Redo/Unicode script save/reopen, report access, invalid-edit rollback and missing-observation recovery covered. Paired simulator/filter epochs, exact numeric observation boundaries and GUI-configured batch accept/reject frequency thinning and record rejection match independent state and residual edit-flag reports. KalmanTests also covers GPS simulation and concrete RunSmoother serialization, including labels/comments and command edits. Broader measurements, noisy/real data, level-one and other filter regimes, estimator epochs, multiple propagator mappings, pass biases and covariance settings remain pending. |
| `../plugins/libEventLocator` | CompatibilityTests: edited eclipse lists, exact save/Save As/reopen, invalid-type build recovery, eclipse intervals and Output report access. StationTests: GUI-edited station Cartesian/elevation/mask settings, save/reopen, automatic contact intervals and missing-mask recovery covered. EventLocatorTests: grouped configuration, paired epochs, bounded contacts, Transmit/Receive corrections, ISOYD max-elevation and azimuth/elevation/range reports, eclipse intervals, shipped Mercury intrusion and failed-output-directory restore/reopen covered. FixedGrid execution, region/spacecraft-observer contacts, broader hardware/FOV, remaining formats/coverage boundaries and disk-write failures remain pending. |
| `../plugins/libExternalForceModel_py314` | ExternalForceTests: existing force-model module selection from configured Python search paths, Cancel and pending function/exclusion Apply, shortened shipped no-API example with independent internal two-body state agreement, exact Undo/Redo/Unicode save/reopen, missing module/function run failure and recovery, invalid-setting rollback, independently script-configured combined forces and unrelated report edits covered. Owned force serialization now retains module/function/exclusion settings so GUI reconstruction does not drop the contributor. New contributor creation/removal, full-day/API-dependent examples, packages/custom search-path persistence, modified-module caching, multiple-spacecraft/variational and malformed-callback cases remain pending. |
| `../plugins/libExtraPropagators` | BulirschStoer: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Remaining cases pending. |
| `../plugins/libFormation` | CompatibilityTests: Add editing/reordering, non-spacecraft rejection, exact save/Save As/reopen and failed-build recovery, both members propagate 60 seconds. Remaining settings/output coverage pending. |
| `../plugins/libGmatFunction` | CompatibilityTests: edited cross-product arguments, exact save/Save As/reopen, invalid-type build recovery, expected numerical cross product. Broader function and report audit pending. |
| `../plugins/libMsise00` | AtmosphereTests: GUI selection/configuration, constant-flux density response, CSSI observed/predicted and selected Schatten prediction, source-preserving Undo/Redo/save/reopen, 600-second density/trajectory reports and missing-weather-file recovery covered. Broader operating regimes, file contents/coverage boundaries and remaining Schatten modes pending. |
| `../plugins/libNewParameters` | AtmosphereTests: AtmosDensity output from GUI-configured atmosphere models and SPAD drag, density/trajectory agreement with independently script-configured missions and save/reopen covered. Density unit metadata corrected to kg/km^3 without changing values. Other parameters and contexts pending qualification. |
| `../plugins/libPolyhedronGravity` | PolyhedronTests: existing contributor body/input-shape selection, density units, chooser Cancel and pending Apply, independent closed-cube far-field mass check and script state agreement, paired body/path configuration, exact Undo/Redo/Unicode save/reopen, SurfaceHeight parameter browser and report access, invalid density/body/missing/malformed shape rollback and recovery, CRLF/tabs/no final newline/decorative labels, relative paths and unrelated resource editing covered. Duplicate force serialization fixed; checked loader preserves valid record ordering and rejects malformed connectivity/geometry. New-contributor creation/removal, real asteroid meshes, custom bodies, multiple bodies/spacecraft, variational/precision propagation, geometric self-intersections and broader SurfaceHeight numerical semantics remain pending. |
| `../plugins/libProductionPropagators` | PrinceDormand853: step edit, exact save/Save As/reopen, invalid-build recovery, report creation and analytic circular-orbit endpoint. Remaining cases pending. |
| `../plugins/libPythonInterface_py314` | Shipped Python example: exact save/Save As/reopen and failed-build recovery, independently computed cross-product result and report. Remaining call types and runtime-error recovery pending. |
| `../plugins/libSaveCommand` | CompatibilityTests: edited object list, exact save/Save As/reopen, runtime spacecraft/variable export and reload, repeat-run replacement, loop snapshots, bad-path and disk-write recovery. Remaining resource types and multi-snapshot reimport pending. |
| `../plugins/libScriptTools` | ScriptEditingTests: the sole registered CommandEcho command has typed On/Off editing, pending Apply, label/comment preservation, exact Undo/Redo/Unicode save/reopen, bounded execution tracing, independently checked 2/5 reports, invalid-edit rollback and initially disabled/enabled RunComplete state restoration. Its generator now retains the terminating semicolon so the GUI can locate/edit it. One stopped While/If case, post-echo file-write failure, initially on/off settings, unexecuted/repeated cleanup and configuration-preserving clones are covered; cleanup/copy defects corrected. Broader completed nested/loop and argument-validation cases remain pending. |
| `../plugins/libStation` | StationTests: GUI location/elevation/ID/colors/mask configuration, physical position and source-preserving Undo/Redo/save/reopen; script-reference contact intervals for baseline, elevation 25 degrees and bundled mask, mask clear and missing-file restore/reopen recovery covered. Hardware, measurement/media/error-model settings and broader bodies remain pending. |
| `../plugins/libThrustFile` | ThrustFileTests: history creation and input selection, typed segment/tank/solve-for lists and clear/restore, pending/Cancel angle and sigma vector resizing, Begin/EndFileThrust selectors, independent state reports, analytic scaled fuel depletion and post-End coast, exact Undo/Redo/Unicode Save/Save As/reopen, wrong-reference rollback, missing/malformed-file recovery and Output access covered. All four data formats with None/Linear interpolation, relative-file Build/Apply/reopen and the full-day bundled example covered. Multiple spacecraft/segments, cubic interpolation, time-varying angles, estimator solve-fors and file-boundary regimes remain pending. |
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

## Named propagation publication metadata

Orbit and ground-track publication now send the publishing command label with
sample data. Qt retains it per plotted point. The engine's default `Unnamed`
summary falls back to the script command name; explicit summary names retain
precedence. Solver-current buffering records labels at collection time and
replays them through the base, OrbitView and GroundTrackPlot implementations.
This metadata does not change propagated values or introduce an OpenFrames
runtime dependency. Existing wx canvases ignore the additional plot action.

CompatibilityTests saves/reopens and executes two named propagation commands,
checks both labels in the orbit samples and checks the total 180-second duration
(60 seconds followed by 120 seconds). It repeats the label check inside a
DifferentialCorrector with SolverIterations=Current and verifies the solved
variable. This is a prerequisite for segment cameras, not completed segment
camera support: anonymous command identity, segment boundaries, selection and
conversion remain open.

Validation: user's GmatQt rebuilt; all 11 Qt tests passed in 44.99 seconds.
Native rendering/window checks used isolated Xvfb/software OpenGL and do not
qualify the user's Intel/Wayland desktop. Evidence:
`Qt6ParityValidation/check-provider-metadata.txt`.


## Focused spacecraft ballistics and mass editor

Audited all controls in wx BallisticsMassPanel: DryMass, Cd, Cr, DragArea,
SRPArea, both SPAD files, both scale factors and both interpolation methods.
The spacecraft property panel now offers **Ballistics and mass…**, with the wx
Spherical and SPAD files groups, numeric units, existing-input file choosers and
engine-provided Bilinear/Bicubic choices. Nonnegative mass/area/scale fields and
finite coefficients are validated together before OK. Cancel leaves the pending
property table unchanged; OK updates that table, and Apply remains the single
mission-edit operation. Other spacecraft properties stay pending alongside it.

WorkflowTests edits all eleven controls, cancels a dialog and file chooser,
recovers from a negative mass, selects the supplied SphericalModel.spo file,
checks pending/configured-state isolation, applies, performs exact-source
Undo/Redo, saves/reopens, and executes a 60-second SPAD SRP mission. Its report
contains the edited mass, coefficients and areas; the reopened SPAD paths,
Bicubic settings and scales are also checked. SPAD drag dynamics and the full
matrix of interpolation/scale settings remain open qualification cases.

Validation: rebuilt application/bin/GmatQt; all 11 Qt suites passed in 39.07
seconds. Evidence: `Qt6ParityValidation/check-ballistics.txt`.

## Wayland default-viewer interaction check

Ran the existing GmatQtWindowTests harness directly on the current Wayland
session with QT_QPA_PLATFORM=wayland and temporary QSettings. No forced software
OpenGL variable was set. This is a separate process with isolated output and
settings; it does not close or change another running GMAT instance. It uses the
same frontend library and startup configuration as the rebuilt GmatQt.

The default mission completed, the ground view minimized, the OrbitView opened
from Output, the ground view restored and minimized again. The test captured
29,380 blue ocean pixels and 953 red trajectory pixels and reported a responsive
event loop after 3.016 seconds. The captured scene was visually inspected and
shows textured Earth, trajectory, starfield and constellation lines. This proves
that sequence on the current desktop in this run, not extended-session or all
viewer lifecycle qualification. Evidence: `Qt6ParityValidation/wayland-windows.txt`
and `Qt6ParityValidation/wayland-orbit.png`.


## Spacecraft hardware attachment and removal

Audited wx TankPanel and ThrusterPanel: individual add/remove and add-all/remove-all
operations, configured type filtering, selected-list order and replacement Apply.
Qt's ordered attachment checklists now offer **Select all** and **Clear selection**.
The shared controls also serve other typed resource-list pickers. Audited the wx
PowerSystemPanel's single readonly selector and empty choice; Qt now offers a
direct typed **PowerSystem** dropdown with **No power system**, retaining pending
Apply semantics.

Accepting complete tank/thruster removal exposed a serialization bug: explicit
`Tanks = {};`/`Thrusters = {};` was interpreted as a hardware name `{}`. Empty
spacecraft hardware lists now omit their assignments, matching fresh-object
engine serialization. ReportFile empty lists use the same omission rule even
when no prior assignment exists. Explicit empty assignments are not introduced
for spacecraft Tanks, Thrusters, AddHardware or AddPlates.

WorkflowTests checks Select all/Clear selection and Cancel on the ordered tank
picker. CompatibilityTests replaces a spacecraft's decoy thruster through its
GUI, selects all tanks, changes their order and applies both lists together.
It checks typed candidates, Cancel, pending/configured-state separation,
exact-source Undo/Redo and invalid tank/thruster rollback. The existing reopened
two-tank finite burn now uses those GUI-selected attachments; its two report rows
verify the 3:2 fuel split, analytic total consumption and coast shutdown.

A separate fixture attaches a nuclear power system through the new dropdown,
checks both nuclear/solar candidates and the None choice, rejects a spacecraft
as a power reference and saves/reopens. A report verifies generated power 10,
bus demand 2 and available thrust power 7.2. After removing that dependent report
command, GUI edits detach power and clear all tanks/thrusters together. Save/reopen
and execution confirm all three attachment fields are empty. Electric tank,
thruster and power-consuming propulsion combinations remain open qualification.

Validation: rebuilt the user's GmatQt; all 11 Qt suites passed in 38.79 seconds.
Native viewer tests used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-hardware-attachments.txt`.


## Spacecraft visual-model editor and fractional-DPI viewport

Audited wx VisualModelPanel's filename/browse, three rotation and translation
slider/text pairs, logarithmic scale slider/text, Recenter Model, Autoscale Model,
new-file pose reset, Show Earth and the shared orbit/target color controls.
Qt now offers **Visual model…** from the spacecraft properties. Its splitter
pairs grouped controls with the existing Qt orbit renderer; controls scroll on
smaller displays. Wheel/drag/Fit retain Qt navigation. Choosing a new model
resets its offsets/rotations and scale, matching wx; Recenter sets offsets to
zero and Autoscale restores scale one. Orbit and target colors stay pending.

The dialog normalizes numeric edits through setters on a cloned spacecraft,
including offsets limited to [-3.5,3.5], wrapped rotations and scale limited to
[0.001,1000]. Numeric fields show the engine's normalized values on editing
completion. Nonfinite input and unreadable model geometry leave the last valid
preview and report the error. Model paths resolve through the engine's model
path rules and use the same reader as the native orbit viewer. OK updates only
the pending property rows; Apply remains the mission edit. Offscreen/minimal
platforms retain controls/validation but have no native mesh preview.

The Earth size reference uses the standard equatorial radius and a per-object
wireframe flag; this leaves the spacecraft mesh filled/textured. The shared
painter also respects body wireframe settings. WorkflowTests covers Cancel,
sliders, recenter/autoscale, new-file reset, numeric normalization, malformed
model recovery, file/color chooser acceptance and cancellation, pending/model
separation, Apply, exact-source Undo/Redo and save/reopen. It verifies all orbit
state elements are unchanged and reopened propagation still lasts 600 seconds.
Native tests render a textured OBJ in the dialog, change its rotation and verify
changed pixels, then show the wireframe Earth reference. Broader 3DS assets and
material combinations remain open qualification cases.

Direct Wayland execution exposed stale OSG viewport dimensions during widget
resize at fractional display scaling. Initialization now uses physical pixels;
each paint refreshes OSG's context size and viewport from the current widget
size/ratio. The native harness also waits for initial window exposure/configure
before subsequent resize requests, since an outstanding Wayland initial
configure can overwrite an immediate client resize. Centered Fit assertions and
framebuffer-size checks remain in place. Added QtGui.NativeOrbitFractionalDPI
at QT_SCALE_FACTOR=1.5 alongside normal and 2x native checks.

Validation: rebuilt the user's GmatQt; all 12 Qt suites passed in 47.61 seconds.
Native suite rendering uses Xvfb/software OpenGL. The preview capture mode also
passed directly on the current Wayland desktop without forced software rendering,
including the preceding centered-Fit/resize checks, textured preview, rotation,
Earth reference and cleanup. The 2250x1620 dialog capture was visually inspected.
Evidence: `Qt6ParityValidation/check-visual-model.txt`,
`Qt6ParityValidation/visual-model-wayland.txt` and
`Qt6ParityValidation/visual-model-wayland.png`.

After the viewport correction, GmatQtWindowTests was also rerun directly on
Wayland with temporary settings. The default mission, ground-view minimize,
Output OrbitView opening, ground restoration and repeated minimize passed in
2.891 seconds. Its capture contained 21,679 ocean and 685 trajectory pixels.
This is evidence for that sequence on the current desktop, not all viewer
lifecycle or extended-session qualification. Evidence:
`Qt6ParityValidation/visual-default-wayland.txt` and
`Qt6ParityValidation/visual-default-wayland.png`.


## Focused attitude model, orientation and rate editing

Audited wx AttitudePanel's model-dependent field display, coordinate-system
restrictions, orientation and rate representation choices, Euler sequences,
precessing-spinner and nadir-pointing settings, and AEM file chooser. Qt now has
an **Attitude…** dialog with grouped controls, explicit angle/rate units, typed
coordinate/body choices and an existing-file AEM chooser. Initial-state controls
are hidden when the model does not allow them; locked reference frames are not
editable. SPICE models direct users to the spacecraft kernel controls.

The dialog uses spacecraft clones, retaining each model's pending settings while
switching models within the dialog. Representation changes first validate and
submit current pending values, then display engine-converted values. Rejected
numbers, zero quaternions, invalid cosine matrices and failed frame changes leave
inputs available for correction and restore the previous selector. Required
orientation/rate fields must be readable before committing a conversion; a
failed getter cannot silently leave an empty state editor.

Attitude state/rate components are submitted together through GMAT's existing
vector/matrix setters. This also corrects ordinary resource Apply: initialized
attitude clones reject scalar quaternion setters, and ordering must establish
model/representation/sequence before dependent values. No engine mathematics
were changed. OK retains the selected model's complete settings until spacecraft
Apply, including other pending spacecraft changes. The generic table displays
available accepted values and marks original-model fields that are no longer
used; subsequent attitude edits go through the dialog.

WorkflowTests covers dialog Cancel, Euler/quaternion/MRP/DCM round trips, both
rate representations, invalid values and recovery, model-specific controls and
pending-data retention, locked and invalid frames, AEM chooser Cancel, reopening
pending settings, paired DryMass/attitude Apply, exact-source Undo/Redo and
save/reopen. A zero-rate Spinner mission propagates for 600 seconds and reports
quaternion samples that match the reference computed directly from the input
Euler angles through GMAT's conversion utility.
The mission-command tail remains unchanged.

Rebuilt the actual `application/bin/GmatQt` executable; all 12 suites passed in
60.54 seconds. Native suite checks used isolated Xvfb/software OpenGL. Evidence:
`Qt6ParityValidation/check-attitude.txt`. A separate Wayland process with temporary
settings and report files passed the attitude workflow and captured the final
dialog, including unit labels; the capture was inspected for control/button fit.
Evidence: `Qt6ParityValidation/attitude-wayland.txt` and `attitude-wayland.png`.
The direct Wayland command used WorkflowTests' `--attitude-capture` mode, which
runs through this attitude workflow and exits before later workflow scenarios.
This is not a full desktop or long-session qualification claim.

Nonzero-rate dynamics, other Euler sequences/singularities, specialized
PrecessingSpinner/NadirPointing/CCSDS-AEM execution, changing reference-frame
dependencies and broader attitude file/error cases remain qualification work.
The existing SPICE execution coverage does not qualify every operation in this
new dialog. The larger workflow, plugin and viewer acceptance gates remain open.


## Spacecraft orbit frame, representation and anomaly workflow

Audited wx OrbitPanel's frame binding, pending-state conversion, internal
Cartesian cache, epoch ordering, representation restrictions and anomaly labels.
Qt now offers a direct coordinate-system selector. A frame-name string change
alone left the old coordinate-system pointer attached; preview and Apply now
bind the chosen configured frame through the engine's public reference API.
Frame changes refresh available representations and fall back to Cartesian
when a body-fixed or celestial-body-origin requirement is no longer met.

Display changes retain the physical internal Cartesian state between selections,
as wx does. Only edited numbers are interpreted again; all six elements are
submitted together through GMAT's StateConversionUtil and CoordinateConverter.
Conversions use the pending epoch, including precision time when available.
Invalid numbers, dates, missing frames and spacecraft-dependent circular frames
restore the previous selector and leave correction inputs intact. Apply retains
that cached state after setting the epoch: the engine epoch setter can otherwise
reinterpret an already-converted state in the original frame. Preview changes
remain separate from the configured model and mission source.

Orbit values are read as complete converted state vectors. The engine's scalar
MA/EA getters are unsuitable for these displayed values, and its default labels
can still say TA after selecting another anomaly. Qt supplies the selected
sixth-element label and exposes the grouped editing path even when that element
is marked read-only for scalar assignment. Anomaly choices follow eccentricity:
TA/MA/EA for elliptic orbits, TA/MA/HA for hyperbolic orbits. Frame/representation
changes fall back to TA when the previous anomaly becomes unavailable. The
anomaly selector is disabled for representations that do not use it.

The engine's Planetodetic forward/inverse conversions are approximate; merely
re-parsing unchanged display values produced a 1.4 mm X drift in the fixture.
Its standard script writer also omits MA/EA/HA sixth elements through a read-only
filter. Qt resource reconstruction therefore writes Cartesian inputs in the
selected frame for Planetodetic and non-TA anomaly cases, retaining the selected
DisplayStateType and explicit AnomalyType. This applies to other spacecraft
when an unrelated resource is edited or deleted as well. No numerical engine
source or conversion formulas were changed. The saved script's state input
labels can be Cartesian while the GUI continues to display the selected type.

WorkflowTests covers all fourteen representation display round trips, including
Brouwer mean and hyperbolic incoming/outgoing asymptote displays. Frame cases
include Earth BodyFixed, a Moon origin, a barycenter origin, an unavailable frame
and a circular spacecraft reference. Cases include invalid numbers/dates,
pending epoch plus Cartesian edits, configured-model isolation, paired Apply,
unchanged mission-command source, exact Undo/Redo, save/reopen, unrelated Apply
and resource deletion, and actual 600-second report execution. Planetodetic,
mean/eccentric anomaly and hyperbolic anomaly round trips retain their display
settings, correct table labels/values and propagated states. The Orbit tab uses
one frame selector; the deprecated StateType alias and unused action column are
hidden. Column resize behavior remains interactive.

The numerical fixture specifies its initial epoch explicitly, uses 1e-13
integrator accuracy and requests a 1e-10 elapsed-time stop tolerance. It verifies
initial precision epochs within 1 ns. Actual stopping epochs can still differ
by fractions of a microsecond in the existing engine; comparisons bound that
offset below 1 microsecond and align only the endpoint offset using reference
velocity/acceleration. Position and velocity residual bounds remain 1e-6 km and
1e-9 km/s. Frame previews and display round trips use tighter 1e-7 component
bounds. This does not claim bit-identical trajectories or qualification of the
engine default constructor's rounded precision epoch.

Broader element editing/Apply across every representation, coupled elliptic to
hyperbolic changes, singular/parabolic cases, other specialized axes and epoch
formats remain qualification cases. In particular, display round-trip coverage
alone does not qualify all Brouwer mean input conversions. Orbit Designer and
Orbit Summary remain separate unaudited workflows. The overall workflow,
viewer and plugin acceptance gates remain open.


Rebuilt the user's actual `application/bin/GmatQt` executable; all 12 Qt suites
passed in 48.63 seconds. Native viewer/window/plot checks used isolated
Xvfb/software OpenGL and include normal, 150% and 200% native orbit scaling.
Evidence: `Qt6ParityValidation/check-orbit-frames.txt`. A separate process with
isolated settings ran the orbit workflow directly on the current Wayland
desktop, including the preceding epoch/representation tests, all frame/anomaly
cases, round trips and engine report execution. Its `--orbit-capture` mode exits
after this workflow and does not claim full desktop or long-session validation.
The final 2250x1620 capture was inspected for readable columns, selector fit,
unit labels and Apply/Close controls. Evidence:
`Qt6ParityValidation/orbit-frames-wayland.txt` and `orbit-frames-wayland.png`.


## Grouped atmosphere and drag configuration

Audited wx DragInputsDialog's nine controls: F107, F107A, MagneticIndex,
historic/predicted sources, CSSI/Schatten filenames, Schatten error and timing.
The force-model resource editor now offers **Atmosphere and drag…**, including
primary-body, atmosphere and Spherical/SPAD shape selection. Weather sources
and Schatten settings use typed choices; CSSI, Schatten and Exponential input
files have existing-file pickers. The grouped controls retain pending inputs
across model/source changes, reject invalid numbers and files, and keep edits
separate from the configured mission until force-model Apply. A scrolling form
keeps acceptance buttons available at 600 by 440 logical pixels.

Apply adds, replaces or removes the engine-owned DragForce and its atmosphere
model, consuming the dependent properties as one group. It also handles
generic drag property edits. Merely setting the old atmosphere-name property
did not replace the owned atmosphere object. Pending filenames and numeric
settings are applied before attaching a replacement atmosphere. The current
Earth factory qualifier still hides Exponential for an old R2013a limitation;
Qt offers it because the current engine registers it and ships an Earth
Exponential example. Selecting None removes drag through the same atomic
script-edit/rebuild operation.

The engine checks both weather filenames and formats during initialization,
even when sources are ConstantFluxAndGeoMag. Qt now explains this requirement,
keeps the weather pickers available, and validates both retained files before
acceptance. CSSI prediction additionally requires its daily/monthly prediction
sections. A prediction checker accepts either format, so Qt also rejects CSSI
files selected as Schatten and Schatten files selected as CSSI. Directories and
unreadable/missing files are rejected before entering the core format checker.
These checks verify headers/section markers, not all numerical file contents
or temporal coverage.

AtmosphereTests creates independent script-configured reference missions and
compares them with missions configured through ResourceEditor and the dialog.
Constant-source Earth MSISE90, JacchiaRoberts and NRLMSISE00 cases compare
initial/final density and state for 600 seconds of propagation. Changing F107,
F107A and Kp increases the NRLMSISE00 density in the test. CSSI observed data
is exercised at 1 January 2024, and CSSI and Schatten predictions at 1 January
2027; the Schatten case selects PlusTwoSigma/LateCycle. Exponential input-file
configuration and switching from/to NRLMSISE00 are exercised. Comparisons allow
1e-8 relative density differences, 1e-6 km position and 1e-9 km/s velocity
differences. These are calculation-preserving GUI comparisons, not independent
validation of the atmosphere equations.

The SPAD cases configure the spacecraft through **Ballistics and mass…** and
select SPADFile through the force-model dialog. Bilinear and Bicubic
interpolation with scale 1.5 use a temporary copy of SphericalModel.spo.
Reports match independently script-configured cases, and the resulting
trajectory differs from Spherical drag. Missing CSSI and SPAD files cause
runtime failure; restoring the copied files and reopening the unchanged saved
script recovers successful execution. Validation, chooser Cancel/acceptance,
pending-model reopening, a paired ErrorControl edit, exact-source Undo/Redo,
mission-command/comment preservation, save/reopen and complete drag removal
are also exercised.

The NewParameters AtmosDensity implementation already returns kg/km^3 by
multiplying the drag model's kg/m^3 density by 1e9. Its parameter metadata
incorrectly said Kg/m^3; the unit label is now kg/km^3. Numerical output and
dynamics are unchanged. This qualifies AtmosDensity in these scenarios, not
the complete NewParameters plugin. Other Schatten modes, malformed numeric
weather data, coverage boundaries, additional SPAD assets/scales, non-Earth
atmospheres and other force-model operations remain qualification work.

Rebuilt the actual `application/bin/GmatQt`; all 13 Qt suites passed in
79.53 seconds, including the new Atmosphere suite and native orbit/window/plot
checks. The NewParameters plugin was also rebuilt for the unit metadata fix.
A separate process with isolated settings exercised the complete atmosphere
and SPAD workflow directly on the current Wayland desktop. The captured dialog
was visually inspected for readable fields, file controls and visible buttons;
compact-window scrolling is checked in the test. Evidence:
`Qt6ParityValidation/check-atmosphere.txt`,
`Qt6ParityValidation/atmosphere-wayland.txt` and
`Qt6ParityValidation/atmosphere-wayland.png`. The broader workflow, viewer and
plugin qualification gates remain open.


## Legacy Orbit Designer and Summary source audit

The retained Orbit Designer source offers SunSync, RepeatSunSync,
RepeatGroundTrack, Geostationary, Molniya and Frozen design tools. However,
`src/gui/spacecraft/OrbitPanel.cpp` explicitly hides its launcher with
`orbitDesignerButton->Show(false)` and the comment “Remove At-risk Orbit
Designer feature, GMT-3383.” `OrbitSummaryDialog` is read-only text created
by `OrbitDesignerDialog::OnSummary`; it is not a separate action displaying
an existing spacecraft state. These files therefore do not establish a
missing exposed workflow in the current base wx GUI. No Qt design prototype
or new orbital mathematics was added. Active orbit editing and its remaining
representation/frame qualification cases retain their existing scope.

## Ground-station location, appearance and contact qualification

Audited the active wx GroundStationPanel ID, minimum elevation, central body,
state type, horizon reference, three dependent location fields/units and
orbit/target colors. Its station hardware controls are commented out. The Qt
resource editor now has a **Ground station…** action grouping these controls
and an optional horizon-mask input picker with Clear. Coordinate selectors
convert the pending numeric location through the engine, with the selected
body's radius and flattening. Changing the body retains the current numeric
location, matching wx. Cartesian coordinates disable the horizon selector;
spherical fields show latitude/longitude in degrees and altitude in km.
Rejected conversions restore selectors and retain the invalid inputs.
Content scrolls in compact windows while OK/Cancel stay available.

Dialog OK retains pending edits in ResourceEditor; only parent Apply updates
the configured resource and mission script. Apply sets body and representation
before location components. Previously Qt's generic alphabetical property
order could interpret edited location components in the old representation.
Engine latitude/elevation bounds, finite inputs, colors and mask existence are
validated together on a clone. Named colors initialize the picker correctly.
The engine's BodyFixedPoint parameter-label array was missing a comma between
LOCATION_LABEL_3 and LOCATION_UNITS_1, shifting subsequent label lookups; this
metadata defect is corrected. GroundStation's mask setter now accepts an
empty filename so an existing mask can be removed.

StationTests exercises picker acceptance/Cancel, grouped validation, pending
reopening, exact-source Undo/Redo, preserved mission comments, save/reopen,
Earth Cartesian/spherical and Sphere/Ellipsoid conversions, and Mars preview
geometry using its own radius/flattening. Coordinate round trips retain the
physical Earth position within 1e-8 km. A one-day spacecraft/contact scenario
compares GUI-configured station results with independently script-configured
references: default elevation (five events), elevation 25 degrees, and the
bundled Ex_Contact_Location_Station_Mask.txt. Contact endpoints match the
report's millisecond precision; durations allow 1e-5 seconds because coordinate
roundoff can alter the event root's last microsecond. Elevated and masked
cases must differ from baseline. A missing mask fails execution; restoring
it and reopening the saved script recovers matching contacts. GUI Clear
removes the configured mask and restores baseline contacts after save/reopen.
These comparisons qualify calculation-preserving UI behavior, not independent
validation of the contact algorithms.

Station hardware, antenna/custom FOV, media corrections, error models,
non-Earth contact execution, light-time/aberration modes and full EventLocator
configuration remain unqualified. This checkpoint does not close the broader
workflow, viewer or plugin gates.

Rebuilt the actual `application/bin/GmatQt` launcher target and Station plugin.
All 14 Qt suites passed in 53.17 seconds, including the new Stations suite and
native viewer/window/plot checks. A separate isolated-settings process also
passed the entire station workflow on the current Wayland desktop. The final
dialog capture was visually inspected for field readability, grouping, units,
file/color controls and visible action buttons. Evidence:
`Qt6ParityValidation/check-stations.txt`,
`Qt6ParityValidation/stations-wayland.txt` and
`Qt6ParityValidation/stations-wayland.png`.


## Grouped event-locator controls and report failure propagation

Audited the wx EventLocatorPanel common target/body selections, report path and
run mode, write-report toggle, entire-interval toggle, paired epoch-format and
endpoint conversion, search step, light-time/aberration dependencies, and
Contact/Eclipse/Intrusion-specific observer/direction, shadow-type, sensor,
central-body, phase and report-coordinate/grid-file controls. Qt now provides
an **Event locator…** grouped editor with these operations. It also exposes
current ContactLocator report format/time format/precision/alignment and
interval-step settings. Typed target choices include spacecraft and, for
ContactLocator, registered planetographic regions. Observer choices include
ground stations and spacecraft supported by the current engine, excluding the
selected target itself. Changing the contact target refreshes that exclusion. Intrusion
sensor choices include imagers, and changing the target refreshes the
intruding-body list to exclude the target, matching wx.

Entire-interval selection disables the epoch controls. Changing input format
converts both pending endpoints before updating either field; conversion
failure restores the format and retains both original inputs. Opening the
dialog with only a pending format change displays the engine-converted dates
in that format. Apply sets the input format before either endpoint. Turning
off light-time clears/disables stellar aberration and disables the contact
direction selector. FixedGrid enables the grid-frame file and validates a
readable regular file; SensorFrame retains the unused path. Azimuth/elevation
report formats enable their interval step and require a positive value.

Dialog OK validates a clone and retains pending settings; parent Apply updates
the resource and mission source together. List replacement clears/adds typed
engine lists on the clone, instead of passing individual entries through the
generic scalar setter. Wrong target/observer types and reversed explicit
intervals fail without changing the script. Input/output file chooser modes,
Cancel, reopening pending values, compact scrolling and visible action buttons
are tested. A pending edit does not alter the configured step or interval mode.

EventLocatorTests compares Qt-configured missions with independently
script-configured references. A one-day station-contact scenario uses a
19:00-to-midnight explicit interval (two contacts), 30-second search steps,
Transmit and Receive light-time with stellar aberration, and selected ISOYD
SiteViewMaxElevationReport and AzimuthElevationRangeReport output with
precision 8 and a 60-second report interval. The one-day eclipse case compares
Umbra/Penumbra event rows, counts and summaries. The shipped
Ex_IntrusionLocator_Mercury_Sun_Transit example is configured through Qt with
its imager, Mercury intruder and a 120-second step; its report matches the
script reference. Tests also reject phase values outside [0,1] and a missing
FixedGrid file, and cover target exclusions, source-preserving Undo/Redo,
mission comments, exact save/reopen and format-only Apply. Contact endpoints
match the millisecond report precision; numeric report values allow absolute
1e-5 differences (seconds, degrees or km according to the column), with all
text tokens matching. These checks establish calculation-preserving GUI
behavior, not independent scientific validation of locator algorithms.

The failed-output-directory case uncovered two engine reporting defects:
ContactLocator discarded ReportEventDataLegacy's failure result, and the base
EventLocator ignored a false ReportEventData result. The legacy result is now
returned and the base locator throws a named report failure, so Qt reports a
failed run instead of successful completion without the requested file.
Creating the directory and reopening the unchanged saved script recovers
matching contacts. Successful reporting and numerical algorithms are unchanged.

FixedGrid execution with a valid kernel, region/spacecraft-observer contacts,
additional report formats, append combinations, coverage boundaries and
write failures after successfully opening the report remain unqualified.
The broader workflow/viewer/plugin gates remain open.

Rebuilt the actual `application/bin/GmatQt` launcher target and EventLocator
plugin. All 15 Qt suites passed in 56.42 seconds, including EventLocators and
native viewer/window/plot checks. A separate isolated-settings Wayland process
passed the entire contact/eclipse/intrusion workflow. The captured contact
dialog was visually inspected for readable grouped controls, units, scroll
access and visible OK/Cancel buttons. Evidence:
`Qt6ParityValidation/check-events.txt`,
`Qt6ParityValidation/events-wayland.txt` and
`Qt6ParityValidation/events-wayland.png`.


## XY plot setup and plottable parameter selection

Audited the active controls and save path in wx `XyPlotSetupPanel.cpp`.
The Qt resource editor now has **XY plot setup…** with Show plot, Show grid,
engine solver-iteration choices, one X parameter and ordered Y parameters.
The Y list supports selection, remove, drag reordering and Up/Down. The generic
X/Y Select buttons also use the same specialized parameter browser.

The shared parameter browser now distinguishes plottable parameters from
reportable parameters. It offers numeric variables and object properties with
frame/body/hardware dependencies, plus indexed array elements; strings and bare
arrays cannot be selected as plotted values. Array indices must be positive and
within configured dimensions. Duplicate Y selections are prevented. The
existing report, function-argument and command modes retain their filters.

Qualification exposed a Qt serialization gap: XY Y lists rejected array-element
syntax even though the engine supports it. Y-list replacement now accepts array
elements and validates their plottability. Clearing a disabled Y list removes its
assignment rather than emitting an empty parameter reference. Enabling a plot
without X/Y selections is rejected with corrective feedback; wx instead warns
and deactivates that plot. Qt keeps the requested visibility pending while the
user corrects the selections. Turning a plot off retains its existing X
parameter; clearing an existing X is not offered as a supported operation.

`PlotSetupTests` selects `A(1,1)` for X and, in order, a variable, `A(1,2)` and
`Sat.EarthMJ2000Eq.X` for Y through the GUI. It exercises filtering, array bounds,
duplicate prevention, property/frame browsing, picker and dialog Cancel, pending
Apply/reopen, list movement, invalid-input recovery and exact-source Undo/Redo.
After a Unicode-path save/reopen, every X sample is 2, the variable/array curves
remain 3/7, and the spacecraft curve endpoint agrees with an independently
script-configured report within 1e-8 km. The reference elapsed-time check allows
one microsecond for epoch precision. Disabling and clearing the plot survives
save/reopen and suppresses its viewer; re-enabling an empty plot is rejected.

All 16 Qt suites pass, including the new `QtGui.PlotSetup` suite, and the actual
`application/bin/GmatQt` is rebuilt. The full new workflow also passes directly
on Wayland, with the dialog capture visually inspected. Evidence:
`Qt6ParityValidation/check-xy-setup.txt`, `xy-setup-wayland.txt` and
`xy-setup-wayland.png`. Ground-track setup, remaining orbit redraw/camera modes,
full solver-iteration combinations and broader parameter contexts remain open.


## Ground-track setup, texture defaults and redraw limits

Audited wx `GroundTrackPlotPanel.cpp`, its active main-frame route, and the actual
subscriber factory. In this branch the `GroundTrackPlot` script alias creates
`GroundTrack`, whose native callbacks publish planetodetic spacecraft coordinates
and separate fixed station markers. The legacy concrete `GroundTrackPlot` remains
in the source. Qt's new **Ground-track setup…** serves both types, with Drawing,
Data and Other options groups: typed bodies, spacecraft/station checklists,
bulk selection/clear, collection/update frequencies, retention/redraw counts,
Show plot, solver iteration modes and a texture picker/Default action.
The color controls behind `__USE_COLOR_FROM_SUBSCRIBER__` are not enabled by the
current wx source/build; object colors remain editable through their resources.

Changing the body chooses its default map and retains custom choices per body
when returning. GroundTrack marks an empty/default TextureMap readonly to omit
it from serialization; this is not a restriction on the active wx texture
control. Qt now exposes and permits that field while retaining the engine's
serialization behavior. Map validation resolves the engine's texture path and
actually decodes the image, rejecting missing or truncated images before Apply.
The receiver also resolves bare texture filenames through the engine's texture
path, so a body-default filename selected by the GUI renders successfully.

Object-list reconstruction now avoids legacy `DrawObject` assignments for
current GroundTrack, where that property does not exist, and for the legacy
concrete subscriber, where it is readonly. Station selection and deletion reach
the current runtime's separate marker list without leaving stale spacecraft
curves or station markers. Show plot with no selected objects is rejected with
corrective feedback; station-only plots remain supported.

Qt previously discarded the GL `NumPointsToRedraw` argument and ignored the
current GroundTrack field. It now honors the requested recent segments during
runs and replay, including the preceding endpoint needed to connect each segment.
This follows legacy wx `ViewCanvas::ComputeActualIndex` semantics. Current
GroundTrack stores the same setting but its wx native window does not consume
it; Qt gives that existing GUI setting an effect. Retained history and stable
camera bounds are unchanged. `SetGlEndOfRun` and GroundTrack's `RunComplete`
restore the full retained trajectory, and clearing for a rerun resets that state.
`MaxPlotPoints = 1` is now respected by the receiver and model, matching the
engine's positive-integer range.

`PlotSetupTests` qualifies typed selection and map defaults, file-picker
accept/Cancel, a PNG with a recognizable header but no image data, pending
Apply/reopen, compact scrolling/action buttons, exact Undo/Redo and Unicode-path
save/reopen. A GUI-configured Mars plot renders the selected custom image,
verified from actual canvas pixels. Its longitude agrees with an independently
configured body-fixed report within 1e-9 degrees; planetodetic latitude agrees
with the engine's separate state converter within 1e-5 degrees, allowing the
existing ground-track iteration tolerance. The tests verify collection frequency,
retained sample count, one-point retention, update/redraw settings, a real
OrbitView GL redraw callback, default-map recovery, deselected station removal,
station-only execution and disabled-view suppression.

`PlotTests` captures both native OpenGL and fallback orbit drawing, plus 2D
ground tracks. It verifies recent-segment images differ from complete retained
tracks, completion/zero redraw restore full tracks, replay honors the limit,
history remains intact and clearing resets completion state. All 16 Qt suites
pass and the actual `application/bin/GmatQt` is rebuilt. The final setup and
execution workflow also passes directly on Wayland. Compact scrolling and buttons were tested, and dialog/map
captures were visually inspected. Evidence: `Qt6ParityValidation/check-ground-setup.txt`,
`ground-setup-wayland.txt`, `ground-setup-wayland.png` and `ground-view-wayland.png`.
Broader solver-loop display modes, body/station combinations, runtime asset loss
and remaining orbit camera/option workflows stay open; this does not close the
full viewer or replacement qualification gate.


## Grouped OrbitView setup and camera lifecycle

Audited active wx OrbitViewPanel controls and the native initialization path.
Qt now provides **Orbit-view setup…** with ordered space-point Add/Remove/Clear,
Up/Down and drag movement, per-object Draw flags, three object-or-vector camera
selectors with retained numeric fields while switching modes, view/up coordinate
systems, six up axes, positive view scale, drawing/star controls, solver iteration
choices and collection/update/retention/redraw counts. Content scrolls while
OK/Cancel remain outside the scroll area. Perspective and FOV behind the disabled
`__ENABLE_GL_PERSPECTIVE__`/FOV macros are not active wx resource requirements;
Qt's existing live Camera dialog and saved projection comments continue serving
those options. Overlap and subscriber color overrides are likewise inactive in
this base build.

OK validates a clone and returns pending resource values. Apply reconstructs Add
and DrawObject together, preserving flags by name when only Add changes. Visibility
metadata uses the engine's named show-object map rather than including implicit
camera/Sun entries from its runtime boolean array. Empty lists on a disabled plot
are omitted from serialization; explicit empty object/boolean assignments fail
the base interpreter. Shown empty plots, duplicate/wrong-type references, invalid
vector syntax/nonfinite values, wrong frames and nonpositive scales are rejected
without changing the mission. A zero vector ViewDirection is a valid target at the
coordinate-system origin, and remains supported.

The audit found a missing return in OrbitView's full-string ViewPointReference
setter. It fell through into ViewPointRefType, replacing the intended Vector/Object
classification with the supplied string. The return is corrected; indexed vector
interpretation and propagation equations are unchanged. The new workflow checks
complete-vector Apply, its serialized type, and resulting camera history.

Qt previously ignored the UseInitialView callback and reset each new canvas.
It now captures session camera state in the plot model, retaining it after viewer
closure, and restores it on rebuild/rerun when UseInitialView is Off. The retained
state includes zoom, rotation, pan, Fit, projection/FOV and selected camera name;
On restores the scripted camera. New/Open clears this session state. The model
still obtains new tracking history from the engine, and replay begins at Latest
for a new run. Unchanged vector formatting in the grouped editor does not count
as an explicit camera edit, so drawing-only changes preserve imported primary
camera metadata.

The show-object callback now controls body/model visibility independently of
trajectory lines, matching wx Draw Object behavior. Fully undrawn implicit objects
are excluded from Fit bounds and legends; otherwise the implicit Sun could expand
Fit to astronomical distances. A hidden body radius no longer enlarges its
trajectory bounds. Native and fallback captures exercise this distinction.

`OrbitSetupTests` covers object add/remove/duplicate prevention/reorder/clear,
paired visibility, object/vector mode switching, all three camera roles, typed
frames, scale and up axis, drawing/stars/sampling callbacks, compact scrolling,
Cancel/pending reopen, corrective validation, exact source Undo/Redo and Unicode
save/reopen. Reports of spacecraft Earth-fixed XYZ are identical before and after
the GUI configuration. Constant vectors yield the expected eye/target/up history;
object targets and positions track published spacecraft/Luna points. Multiple
reruns and viewer close/reopen retain the adjusted zoom, rotation, pan, Fit and
projection with UseInitialView Off, and On resets them. A drawing-only grouped
edit preserves an imported body/LookAt camera and executes it successfully.

Additional solver-loop modes, named-camera retention during camera-list changes,
more view/up-frame combinations, geometric degeneracies, scene/asset loss and
broader OF conversion qualification remain open. This checkpoint does not close
the full replacement, viewer or plugin gates.


Rebuilt the actual `application/bin/GmatQt` launcher target. All 17 Qt suites
passed in 60.73 seconds, including OrbitSetup, native/fallback plots, native window
lifecycle, conversion workflows and normal/fractional/200% renderer checks. A
separate isolated-settings process passed the entire new workflow on the current
Wayland desktop. Dialog and rendered-orbit captures were visually inspected: the
object/camera controls and action buttons are readable, lower drawing/data options
are accessible by scrolling, and the orbit capture shows the textured Earth,
trajectory, wireframe and configured guides/star field. Evidence:
`Qt6ParityValidation/check-orbit-setup.txt`, `orbit-setup-wayland.txt`,
`orbit-setup-wayland.png` and `orbit-view-wayland.png`.


## Thruster direction, electric models and tank recovery

Audited the active wx ThrusterConfigPanel controls and the thruster branch of
BurnThrusterPanel. Qt now offers a grouped Thruster setup dialog with Local or
configured coordinate systems, dependent axes/origin, direction components,
duty cycle, scale and gravitational acceleration. Electric controls include all
three thrust models, minimum/maximum usable power, efficiency, Isp and constant
thrust. Model changes retain inactive values and enable applicable controls;
electric polynomial coefficients remain in the existing coefficient dialog.
The compact dialog scrolls while its action buttons remain accessible.

Conditionally readonly Origin/Axes are available when switching a foreign frame
to Local; Apply sets the coordinate system before dependent fields. The original
generic application order incorrectly attempted readonly Axes first. Coupled
power limits and zero direction receive corrective feedback before Apply.
Conditionally readonly MixRatio is exposed for first-tank selection, with paired
Tank/MixRatio serialization. Tank pickers and Apply enforce ChemicalTank versus
ElectricTank. Clear-all omits the default empty Tank/MixRatio assignments because
the interpreter cannot read an explicit empty ratio vector. An active burn still
requires a tank; the GUI can retain a tankless hardware configuration and recover
after restoring its tanks. These are GUI configuration/serialization changes;
propulsion mathematics is unchanged.

ThrusterTests configures a chemical thruster and all three electric models through
Qt, plus a below-minimum-power case. The cases exercise direction/model dependency
controls, Cancel, pending reopen, first-tank addition, paired reordered mixture
ratios, coefficient editing, compact layout, Apply, exact Undo/Redo and Unicode
save/reopen. Electric cases deliberately fail without a power system, then attach
a nuclear system through the spacecraft GUI and execute successfully. Ten-second
burn reports match an independently script-configured mission within 1e-8 in
fuel, inertial position, thrust, Isp and mass flow. With power clipped from 10 kW
to 4 kW, analytic fuel losses are 0.0025 kg chemical, 0.0001 kg constant-thrust,
0.0000256 kg fixed-efficiency, 0.00003 kg polynomial and zero below minimum power.
The 3:1 ordered mixture consumes 75% from FuelB and 25% from FuelA. Fuel remains
unchanged during coast and with DecrementMass off. Clearing all tanks survives
save/reopen/build, fails an active burn, and restoring tanks recovers the original
report results. Invalid frame/body/direction/duty/scale/gravity/tank/power edits
leave source unchanged and permit another successful run.

Rebuilt application/bin/GmatQt. All 18 Qt suites passed in 64.27 seconds. The full
new workflow also passed in an isolated-settings process on native Wayland;
its captured dialog was visually inspected for labels, model dependencies and
accessible action buttons. Evidence: Qt6ParityValidation/check-thrusters.txt,
thruster-wayland.txt and thruster-wayland.png.

The impulsive-burn branch, other local/reference frames, shared electric power,
solar/eclipsed propulsion and broader polynomial/tank operating cases remain
open, along with the larger workflow, viewer and plugin qualification gates.


## Impulsive and finite burn resource workflows

Audited the active wx ImpulsiveBurnSetupPanel, its BurnThrusterPanel branch, and
FiniteBurnSetupPanel. Qt lacked the impulsive Tank field entirely because it
was excluded from supported object-array metadata. Foreign-frame Origin/Axes
were also hidden rather than made available when selecting Local. The new grouped
Impulsive burn setup dialog exposes delta-V elements in km/s, Local/named frames,
dependent axes/origin, optional mass depletion, one fuel tank, Isp and gravity.
Mass toggling retains values and disables inactive Isp/gravity. Zero delta-V is
valid. The dialog scrolls in smaller windows with fixed action buttons and keeps
changes pending until resource Apply.

Burn and thruster frame application now share the ordering rule: CoordinateSystem
before Local Origin/Axes. Impulsive tank replacement clears and rebuilds the
cloned list before mass-depletion validation, allowing the first tank and
DecrementMass to be applied together. Empty impulsive tank lists serialize by
omitting the default assignment. Multiple tanks with mass depletion receive the
engine's supported single-tank restriction as corrective GUI feedback. Existing
multi-tank input can remain represented while mass depletion is off. No burn or
propagation mathematics was changed.

FiniteBurnSetupPanel's individual and bulk selection operations are served by the
existing typed Qt checklist, including drag ordering. Apply now rejects wrong-type
or missing thruster names directly; empty lists serialize by omission. Clearing a
finite burn is a valid configuration operation and survives save/reopen/build.
Executing it produces the base engine's explicit diagnosis that the FiniteBurn
identifies no Thrusters. BurnTests confirms the same failure from an independently
script-configured empty burn and successful GUI reselection/recovery; the GUI does
not substitute a silent coast for the engine's rejected active burn.

BurnTests covers seven impulsive cases: inertial, Local MJ2000Eq/VNB/LVLH/
SpacecraftBody, named EarthFixed and zero delta-V. It exercises frame/depletion
control dependencies, Cancel, pending reopen, first-tank paired Apply, compact
scrolling, Isp/no-tank validation, exact source Undo/Redo, Unicode save/reopen and
label/comment preservation. Nine reported state/fuel fields match independent
script-configured missions within 1e-8. Fuel depletion matches the rocket equation
within 1e-9 for a 1200 kg initial spacecraft, Isp 400 s, g0 10 m/s² and the chosen
delta-V magnitude. Inertial components and VNB/LVLH basis transforms receive
independent numerical checks. BackProp restores state and fuel within 1e-9.
Invalid values/references roll back; a selected tank absent from the spacecraft
fails execution, and reselection restores the original report. Turning mass
depletion off and clearing the tank survives round trips and burns without fuel
loss. An imported two-tank mass-off configuration also retains its ordered tank
list through a grouped vector edit, Apply, Unicode save/reopen and execution.

The finite-burn workflow exercises typed individual/bulk selection, clear-all,
ordering, Cancel/pending Apply, exact Undo/Redo and labeled mission preservation.
A reordered pair of GUI-selected chemical engines produces the same position/fuel
reports as the script reference. The ten-second constant-rate burn consumes
0.002 kg from FuelA and 0.003 kg from FuelB, and coast fuel remains unchanged.
Wrong/missing/duplicate selection rolls back; unattached-thruster and empty-burn
failures recover through corrected selection and save/reopen.

Rebuilt the actual application/bin/GmatQt target. All 19 Qt suites passed in
70.19 seconds, including the new Burns suite and existing Thrusters/mission/viewer
regressions. The entire new workflow also passed on native Wayland in an
isolated-settings process. Its dialog capture was visually inspected for readable
labels, selectors, units, group layout and action buttons. Evidence:
Qt6ParityValidation/check-burns.txt, burn-wayland.txt and burn-wayland.png.

Broader burn bodies/attitudes/epochs, fuel limits and finite shared electric-power
combinations remain unqualified. The larger workflow, viewer and plugin gates
remain open; 52 wx workflow inventory entries are still marked Pending audit.


## Ephemeris output setup and generated-file readback

Audited the active wx EphemerisFilePanel controls: spacecraft, output frame, write
enable, format/filename, interpolator/order, editable sampling step, Code-500 byte
order, STK distance/event settings, epoch format and editable interval endpoints.
The commented-out wx StateType control is not an active workflow. Qt's grouped
Ephemeris output dialog supplies these controls with scrollable groups and fixed
OK/Cancel actions. It retains format-specific pending sampling/frame settings,
filters compatible frames, normalizes extensions when changing formats and
converts both interval dates atomically while preserving spacecraft-epoch
sentinels. Failed conversion restores the format and both dates. Cancel does not
modify the resource; OK remains pending until Apply.

Generic property controls now permit typed sampling/endpoints, convert paired
dates and update format dependencies. STK settings remain available when switching
from another format despite their conditional engine readonly metadata. Apply
sets format/epoch format before their dependents, rebinds the cloned spacecraft
and output-frame references, then uses engine validation. A Cartesian CK selection
receives a corrective message; quaternion CK output is not qualified here.

Spacecraft EphemerisName now offers an input-file chooser even when initially
empty. The engine's readonly flag in this state controls serialization visibility,
rather than preventing first assignment. The current OEM/STK/Code500 propagators
read this spacecraft field; their retired EphemFile setters perform no assignment.
Tests configure first input selection through the GUI rather than relying on the
retired field. SPK uses the existing spacecraft kernel list and matching NAIF ID.

Output now lists ephemeris files. OEM and STK text open in the paged report viewer;
binary formats show format/path/size with Copy path and Open folder actions. The
listed path retains custom extensions for non-SPK formats, matching engine
writing rather than the wx new-filename extension suggestion.

Qualification exposed two file-handling defects. An unresolved absolute output
path could yield an empty resolved filename without the setter rejecting it.
The setter now rejects that case. On Linux, the existing stream-based file check
also accepted directories; startup remove() could remove an empty directory and
replace it with ephemeris output. Startup now diagnoses a directory target before
removal. Qt rejects directory selection before Apply, and the runtime check
protects a target changed to a directory after Apply. These are file-handling
changes; propagation, interpolation and time-conversion mathematics are unchanged.

EphemerisTests executes OEM with a custom extension, STK in meters without event
boundaries, Code-500 little/big endian and SPK. GUI exports match independently
script-configured propagation reports within 1e-9 per reported component. The
600-second reference propagation agrees with a circular point-mass Earth orbit
within 1e-6 km position and 1e-9 km/s velocity. Generated files are read through
their actual plugins after GUI step edits, file selection and Unicode script
save/reopen; the state at 300 seconds is checked independently within 2e-4 km
position and 2e-7 km/s velocity. Tests cover group/generic dependencies, chooser
Cancel, pending reopen, failed date conversion, compact scrolling, exact
Undo/Redo, labeled mission/comment preservation, invalid reference/step/order/
interval rejection, Output access, directory preservation and recovery.

FromSpacecraft intentionally clamps a start epoch before coverage to the first
ephemeris epoch. Tests preserve that engine behavior and verify the same resulting
state, reject starts beyond coverage, then recover through the saved script.
Missing generated input files likewise fail and recover after restoration.

CK quaternion, covariance/acceleration output, broader frames/bodies, event
boundaries, segment gaps, backward propagation, exact boundary stepping, multiple
kernels and disk-full/permission cases remain unqualified. The larger workflow,
viewer and plugin gates remain open; 51 wx inventory entries remain Pending audit.

The full new workflow passed on native Wayland with isolated settings. The
compact-layout test waits for window exposure and resize acknowledgement before
checking scrolling and fixed buttons, so initial compositor sizing cannot
overwrite the tested resize. The native dialog capture was inspected: groups,
labels, enabled/disabled fields, long dates, filename and action buttons remain
readable. Observed readback maximum component errors were about 1.71e-6 km
position and 5.20e-8 km/s velocity across these fixtures; SPK was smaller. Native
evidence: Qt6ParityValidation/ephemeris-wayland.txt and ephemeris-wayland.png.

Rebuilt the actual application/bin/GmatQt target. All 20 Qt suites passed in
71.39 seconds with the final test implementation, including native viewer/window
checks, mission/plugin/file regressions and the new Ephemeris suite. Evidence:
Qt6ParityValidation/check-ephemeris.txt. Full replacement acceptance remains open.


## Dynamic-data setup, selective updates and retained viewer data

Audited the active wx DynamicDataDisplaySetupPanel and DynamicDataSettingsDialog
controls. Qt now has grouped Dynamic data setup with row/column resizing,
retained cells and default new cells, parameter selection, text/background
colors, warning/critical bounds, condition colors, double-click editing and
Delete/Clear selected. Both dialogs keep changes pending until resource Apply;
Cancel leaves the preceding state intact. Duplicate parameter references are
rejected because the engine otherwise silently drops them and shifts the layout.
Whole arrays require an indexed element; real and string parameters are accepted.
Numeric bounds must be finite and ordered. Black text retains the wx convention
of automatic condition colors; nonblack text overrides those colors.

UpdateDynamicData command settings provide a typed display selector and a
checklist of that display's actual parameters. An empty selection updates all
cells; an explicit selection retains the engine's cached values for other cells.
Edits preserve command labels/comments and validate the complete mission.

Testing exposed a shared MDI lifecycle defect: close removes a child from the
workspace before its deferred destruction, so immediate reopen could reuse a
still-live but detached window. Reopen now creates a fresh child from retained
plot/table data. Dynamic callbacks also clear stale padding when later updates
contain shorter rows, retain user-adjusted widths on live updates, and update
cached data while the window is closed.

DynamicDataTests configures a two-row grid through the GUI and compares numeric
reports with an independently script-configured mission. It covers real/string/
array values, blank cells, selective/all updates, inclusive normal boundaries,
warning/critical/custom colors, parameter browsing, resizing/clearing, pending
reopen, Cancel, invalid-input rollback, exact Undo/Redo, Unicode save/reopen,
command Apply and saved execution, immediate close/reopen, ragged and closed-table
callbacks, adjustable widths and clearing the entire table. A missing reference
fails full-candidate validation and recovers without changing the source script.
No propagation or numerical algorithms changed.

The focused suite and native Wayland workflow passed with isolated preferences.
The captured setup dialog was inspected for readable fields, grid columns and
accessible actions; compact resizing retains its buttons. Evidence:
Qt6ParityValidation/dynamic-data-wayland.txt and dynamic-data-wayland.png.

Extreme grid dimensions, styling unnamed cells and broader parameter/mission
contexts remain qualification cases. There are now 49 wx inventory entries marked
Pending audit; the broader workflow, camera/OF and plugin acceptance gates stay
open.

The actual application/bin/GmatQt executable was rebuilt. All 21 Qt suites passed
in 102.19 seconds, including native orbit/window checks, plotting/HiDPI, mission
and plugin/file regressions and the new DynamicData suite. Evidence:
Qt6ParityValidation/check-dynamic-data.txt. Full replacement acceptance remains
open.


## Calculated-point resource controls and viewer execution

Audited active wx BarycenterPanel and LibrationPointPanel controls, including
GmatColorPanel as instantiated by these panels (its segment override checkbox is
hidden here). Barycenter membership was absent from Qt's supported list fields.
It is now editable through the existing ordered membership checklist, with typed
celestial-body choices. Add/remove/clear, drag ordering, pending changes and
Cancel use the shared resource editor. Apply replaces the complete BodyNames
list and rejects empty, duplicate, missing and non-body entries. Although the
engine's diagnostic text mentions barycenter members, its actual Barycenter
SetRefObject rejects non-celestial bodies; Qt follows that executable behavior.

Libration points now offer all five L1–L5 choices and primary/secondary selectors
containing celestial bodies and custom barycenters. Spacecraft, other libration
points and SolarSystemBarycenter are excluded, matching the engine's supported
references. Apply validates the final pair so swapping bodies can be atomic,
while identical bodies are rejected. Both resources retain orbit/target color
entry and color pickers, with edits pending until Apply.

Testing exposed a built-in appearance serialization gap: a default built-in
barycenter has no generated script block to replace. Applying a color therefore
returned the specialized-editor error. Qt now writes/replaces only the validated
appearance assignments and reinterprets the whole candidate. It never emits a
Create or changes membership. Built-in editors explain their fixed definition
and expose only orbit/target colors. First color changes, updates to existing
appearance, preserving the other color, exact Undo/Redo, save/reopen and numerical
report invariance are tested.

CalculatedPointTests configures Earth/Luna membership and a Sun/custom-barycenter
libration point through actual GUI controls, then saves/reopens a Unicode script
and runs its dependent coordinate systems and OrbitView. Nine coordinate report
components match an independently script-configured mission exactly as written.
Mass-weighted body positions agree with published barycenter positions within
1e-7 km. GUI-applied Earth/Luna L1–L5 selections are individually saved/reopened
and executed; collinear position/order and equilibrium residuals and triangular
geometry provide independent checks on the selected definition reaching the
viewer. Two-body reorder and single-body membership, empty GUI selection recovery,
wrong references/types, duplicate membership and built-in protection are covered.
No propagation or calculated-point algorithms changed.

The native Wayland workflow passed with isolated preferences. The inspected
resource-editor capture shows readable properties, selection/color buttons,
L1–L5 dropdown and Apply/Close actions. The orbit capture and pixel check confirm
the configured L4 marker appears in its chosen color; nearby Earth/Sat/Center
labels can overlap at this scene scale, so this is not a label-placement parity
claim. Evidence: Qt6ParityValidation/calculated-points-wayland.txt,
calculated-points-wayland.setup.png and calculated-points-wayland.view.png.

Broader bodies, epochs, mass ratios and ephemeris-error boundaries remain
qualification cases. There are now 47 wx inventory entries marked Pending audit.
The full workflow, viewer/OF and plugin acceptance gates remain open.

Rebuilt the actual application/bin/GmatQt target. All 22 Qt suites passed in
84.12 seconds with the final implementation, including native orbit/window
checks, mission/plugin/file regressions, plotting/HiDPI and CalculatedPoints.
Evidence: Qt6ParityValidation/check-calculated-points.txt. Full replacement
acceptance remains open.


## File-interface configuration, import selection and execution

FileInterface already used the generic resource editor's format dropdown and
input-file chooser. Apply now checks for an existing, readable regular input
file and rejects blank names, directories and missing files before changing the
script. The Set command now has typed spacecraft/data-interface reference
selectors and a field checklist with an Import all fields option. A mission
command template inserts Set using current resources when available. Labels,
comments and unrepresented command syntax retain the shared editor behavior.

The field list comes from an initialized clone, leaving the configured interface
and target unchanged while browsing. TVHF_ASCII exposes Epoch, CartesianState
and Cr as assignable fields. Its additional SupportedFields entries are internal
vector components and coordinate/body metadata: selecting individual components
does not assign them, while explicitly selecting frame/body metadata throws.
They are therefore excluded from the picker. Previously entered unknown names
remain visible and checked so Cancel preserves them; OK requires a nonempty
supported subset or All. Returning to All removes the Data clause.

DataInterfaceTests exercises the real input chooser's Cancel and selection,
format dropdown, pending resource edits, Apply validation, exact Undo/Redo and
Unicode save/reopen. It compares all seven nonempty field subsets plus default
All against independently initialized missions, including initial epoch,
Cartesian state and Cr and the state after 600 seconds of propagation. The
reference epoch and state come from the bundled TVHF fixture's literal values;
propagation/coordinate/time-conversion algorithms are unchanged. It checks typed
target/source selection, existing subset and explicit All handling, unknown-field
preservation/rejection, report output and the read-only Output report viewer.

Missing-file, malformed-header, invalid-month, missing-epoch and unknown-field
failures recover after restoring the input or valid script. File repair is tested
with an immediate rerun as well as reopen. Task-9 input also reproduces the
expected report. The shipped Ex_FileInterface example is exercised after Qt OF
conversion, adding reports and shortening its two-day propagation to 600 seconds;
its converted viewer exists and a Unicode save/reopen repeats its report. This
does not qualify the unmodified two-day example or broader OF camera cases.

The native Wayland test passed using isolated preferences. The inspected field
picker capture has readable controls and accessible Cancel/OK actions. Evidence:
Qt6ParityValidation/data-interface-wayland.txt and
Qt6ParityValidation/data-interface-wayland.fields.png.

Broader bodies/frames, multiple records, repeated imports within one mission and
filesystem permission failures remain qualification cases. The 108-entry wx
inventory still has 47 entries marked Pending audit; qualifying Set alone does
not close the generic wx command panel audit. Of the 20 selected runtime plugins,
6 still have no qualification evidence and 14 have partial evidence. Full
workflow, viewer/OF and plugin acceptance gates remain open.

Rebuilt the actual application/bin/GmatQt target. All 23 Qt suites passed in
101.59 seconds, including native viewer/window checks, mission/plugin/file
regressions, plotting/HiDPI and the new DataInterface suite. Evidence:
Qt6ParityValidation/check-data-interface.txt. Full replacement acceptance remains
open.


## Tracking configuration and range-skin simulation/batch estimation

Audited estimation plugin parameter/setter/serializer behavior and the shipped
Ex_Estimate_RangeSkin mission. Compound AddTrackingConfig arrays and measurement,
station and solve-for lists were absent from Qt's supported properties. Added a
dedicated tracking table with add/remove/reorder controls, ordered spacecraft/
station/attached-hardware signal paths, repeated participants, multiple type
selection, pending edits and Cancel. Apply replaces all definitions and validates
the entire candidate script; invalid participants, hardware, types, duplicate
rows/types and malformed settings are rejected. Column widths use the shared
content sizing and remain user adjustable.

Simulator AddData, estimator Measurements, spacecraft/ErrorModel SolveFors and
station ErrorModels/AddHardware now use supported list editing. Simulators and
estimators expose their default propagator without replacing their additional
propagator mappings. FileName/RampTable ordered file pickers support new
simulation outputs and existing ramp inputs respectively. ErrorModel Type has
measurement choices; NoiseSigma uses the measurement's bias unit metadata and
noise/bias units update when the pending type changes. RunSimulator/RunEstimator
have typed selectors and insertion templates. Algorithms and numerical values
are not changed by these controls.

The audit found missing Resources/creation categories for tracking sets, error
models and interfaces, and added those along with calculated points, celestial
bodies, data filters and FOV navigation. Tests open tracking controls through
the actual Resources tree and create/delete unused tracking sets, error models
and file interfaces through the normal resource transactions.

Two script reconstruction gaps surfaced. Empty ground-station hardware/error
lists must be omitted because the engine rejects their explicit {} assignments.
Imager serializes an unset optional FOV as UndefinedFieldOfView, making antennas
fail unrelated resource/mission edits when that nonexistent object is referenced.
Qt reconstruction now omits this diagnostic placeholder only when the actual
reference name is empty. Explicit FOVs remain, including a real FOV named
UndefinedFieldOfView; another unset antenna does not acquire that object.

EstimationTests uses the shipped range-skin example with explicit bounded test
adjustments: simulation from 10 Jun 2012 00:00 to 08:00 UTC, 60-second sampling,
noise off, OLSEAdditiveConstant=1, temporary files and supplementary state reports.
The additive filter margin avoids rejecting the nearly noise-free fit; 60-second
sampling supplies enough observations to solve six state components. The full
two-day noisy example is not claimed qualified by this bounded test. The
selected fixture converges in four iterations.
Estimated initial positions recover the literal input truth within 1e-5 km and
velocities within 1e-8 km/s. GUI configuration reproduces the independently
script-configured mission's fourteen state/epoch report values and serialized
observations. The shared propagation, signal/time conversion and estimation
algorithms are unchanged.

Changing the default propagator previously caused the engine serializer to
omit the old default's explicit spacecraft mapping. Qt now retains the original
compound mapping assignments while applying the new scalar default. The test
selects an alternate default through both simulator/estimator pickers, checks
that their original spacecraft mappings survive, saves/reopens and reproduces
the reference observations and fit. Editing the mappings themselves and broader
multiple-propagator execution remain pending.

GUI coverage includes path/type picking, repeated participants, row reorder,
add/remove, adjustable widths, pending reopen/Cancel, new output selection
without file creation, station error-model clear/restore, solve-for and tracking
lists, type-dependent units, exact Undo/Redo, Unicode script save/reopen, run
command labels/comments and Output report access. Invalid references/types,
negative noise, incompatible Bias/PassBiases and malformed tracking edits roll
back. Estimation-only missing-observation failure recovers after file restoration
without the simulator recreating it. Existing engine validation rejects Unicode
estimator report paths; this is tested as a source-preserving rejection rather
than claimed support. Unicode script paths and observation paths with spaces are
covered. An unset antenna displays an empty FOV value, and selecting a real FOV
named UndefinedFieldOfView through its picker retains that newly applied
reference rather than removing it as an old diagnostic placeholder. Replacing
an existing FOV serializes the new reference even if the cloned hardware still
holds its old object pointer; clearing removes the assignment. Wrong-type FOV
selection rolls back without changing source.

The native Wayland workflow passed using isolated preferences. The inspected
tracking capture shows readable signal/type columns and accessible row/path/type
and Cancel/OK actions. Evidence: Qt6ParityValidation/estimation-wayland.txt and
Qt6ParityValidation/estimation-wayland.tracking.png.

Remaining cases include other measurement/data formats, noisy and real inputs,
DSN ramp/media/relativistic corrections, filters, paired epoch conversion,
multiple propagator mappings, pass biases, covariance/solve-for regimes and
broader RF hardware operation. EKF remains a separate pending plugin. The wx
inventory still has 47 of 108 entries marked Pending audit; this plugin work
does not close generic-panel audits. Of 20 selected runtime plugins, 5 have no
qualification evidence and 15 have partial evidence. Full replacement acceptance
remains open.

Rebuilt the actual application/bin/GmatQt target. All 24 Qt suites passed in
106.28 seconds, including the estimation suite, native viewer/window checks,
plotting/HiDPI, mission/plugin/file regressions and launcher validation. Evidence:
Qt6ParityValidation/check-estimation.txt. This checkpoint leaves the broader
replacement acceptance gates open.

## Paired simulation/filter epochs and batch data editing

Simulator and data-filter EpochFormat/InitialEpoch/FinalEpoch now have a shared
Time interval dialog. Format changes convert both endpoints atomically, in the
dialog and the property table. Cancel retains the earlier pending properties;
failed conversion retains the earlier format and both edited date strings.
Apply uses a clone, sets the format before either endpoint and rejects invalid
or reversed intervals. A format-only resource transaction converts both existing
endpoints automatically, instead of interpreting old text in the new format.

Numeric conversion initially used the converter's Real-based display string.
That rounding moved an endpoint beyond the simulator's nanosecond comparison
tolerance and changed its final sample by 60 seconds. Qt now serializes numeric
epochs using GmatTime::ToString. The bounded fixture's numeric interval survives
Unicode save/reopen and produces exactly the original observations and the same
fourteen state/epoch report values within 1e-8. Conversion back to UTCGregorian
also reproduces those outputs. All engine-advertised representations are cycled
through the dialog. GregorianDate's existing parser accepts milliseconds; the
UI retains that supported representation and explains its precision. This does
not qualify arbitrary fractional-calendar, UT1 or leap-second boundary cases.
The time conversion and simulation algorithms remain unchanged.

Estimator and TrackingFileSet DataFilters now provide typed checklists.
AcceptFilter/RejectFilter expose observed-object, tracker and measurement-type
lists, ordered input files, record numbers/ranges and available thinning modes.
File selection can restore All or an accept filter's From_AddTrackingConfig.
Positive records and ascending ranges are validated before candidate script
interpretation; wrong filter/reference/types, unknown measurements, zero
thinning frequency and invalid/reversed ranges roll back without source changes.

EstimationTests extends the bounded noise-free range-skin fixture with an
independently script-configured accept filter for the observation file,
EstSat and Range_Skin, every-second-record frequency thinning, and a reject
filter for records 1-3. GUI configuration changes the lists, frequency and
record range, converts filter epochs through the property table and selects
both estimator filters. Exact Undo/Redo and Unicode save/reopen retain the
settings. The resulting state/epoch values and measurement residual rows,
including USER edit flags, match the independent reference. Run metadata is
excluded from residual comparison. Clearing the estimator filter list restores
the original unfiltered state/epoch report; the simulator observations remain
unchanged throughout. File sentinel controls and Cancel are exercised as well.

Native Wayland execution passed with isolated settings; the inspected interval
capture shows readable complete numeric epochs and accessible Cancel/OK.
Evidence: Qt6ParityValidation/estimation-intervals-wayland.txt and
Qt6ParityValidation/estimation-intervals-wayland.interval.png. The actual
application/bin/GmatQt was rebuilt and all 24 Qt suites passed in 106.79 seconds.
Evidence: Qt6ParityValidation/check-estimation-intervals.txt.

Level-one filtering execution, time-based thinning, filtered tracker/hardware
and GPS identities, overlapping/multiple filters and interval boundaries remain
to qualify, along with estimator epoch conversion and the earlier broader
estimation cases. This expands partial evidence for GmatEstimation; 47 of 108 wx
inventory entries still await audit, and 5 of 20 runtime plugins still have no
qualification evidence. Full replacement acceptance remains open.

## Script-event comment/body workflow

Audited wx ScriptEventPanel's separate comments/body controls, fixed outer
BeginScript/EndScript labels, resizable sash layout and pending Save/validation.
Qt CommandEditor now offers Script event controls for complete event blocks.
The dialog separates plain comments from a numbered, highlighted script body
with a vertical splitter. Outer labels and inline comments remain fixed;
nested script events, branches and quoted marker text stay in the body. Both
the engine's semicolon-free canonical BeginScript and script-file wrappers are
recognized. Incomplete/non-event text retains the ordinary command source editor.

Opening without changes preserves the exact command string. Comment-only
updates retain the entire body and boundary text. OK updates only pending
command text in one Undo step; Cancel preserves it. Apply uses the existing
whole-mission transaction, so invalid commands restore the earlier model/source
and can be corrected without reopening the panel. Empty bodies are supported.
No mission execution or numerical algorithm was changed.

MissionTests executes a named outer event containing an If/Else branch, a named
nested event, a string containing EndScript/% and a comment containing BeginScript.
Its literal reference result is 10. An invalid Propagate edit fails without
changing that result or source. A corrected GUI body edit and two comment lines
retain both event names and all inline/nested comments and produce 12. Exact
mission Undo restores 10; Redo and Unicode save/reopen restore 12. Clearing the
body removes its commands and produces the expected pre-event result 5.

Native Wayland mission execution and dialog capture passed with isolated settings.
The inspected image shows readable comments, numbered/highlighted nested script,
fixed labelled boundaries, a splitter and accessible Cancel/OK. Evidence:
Qt6ParityValidation/script-event-wayland.txt and
Qt6ParityValidation/script-event-wayland.png. Rebuilt the actual
application/bin/GmatQt target; all 24 Qt suites passed in 106.34 seconds.
Evidence: Qt6ParityValidation/check-script-event.txt.

The ScriptEventPanel row now has executable audit evidence; 46 of 108 wx entries
remain marked Pending audit. Common editor/menu workflows remain under their
separate audits, and broader viewer, plugin and file gates stay open. Full
replacement acceptance is not established by this checkpoint.

## File/report comparison workflow

Audited wx CompareFilesDialog and GmatMainFrame::CompareFiles: three comparison
modes, absolute tolerance, text blank-line selection, baseline/candidate prefixes,
up to three directories, file count and result export. CompareReportPanel is
read-only, unwrapped output with Close; the active default wx comparison path
instead opens ViewTextDialog. CompareTextDialog remains compiled but has no
caller in the current wx sources. Qt now provides File > Compare files and a
Compare action in report viewers, with two-file or directory comparison,
adjustable directory columns, scrollable setup and a resizable result area.

The Qt worker streams UTF-8 input and complete difference output to temporary
storage. Text handles BOM/CRLF and optional blank-line skipping. Numeric-line
comparison reuses GmatFileUtil::CompareLines; numeric columns reuse
GetRealColumns for the existing numeric/UTC representations and report maximum
absolute differences. Engine simulation, estimation and numerical algorithms
are unchanged. Unlike the wx file loop, Qt counts trailing rows, diagnoses
malformed data after the first record and differing column counts, and does not
hang on lines beyond the wx fixed buffer. Lines larger than 16 MiB, non-finite
numbers and non-UTF-8/binary text receive explicit errors. Initial column headers
are skipped; arbitrary initial header/data ambiguity remains unqualified.

ComparisonTests independently specifies text differences, blank-line behavior,
numeric tolerance boundaries, Gregorian/ISO UTC equivalence, column maxima,
extra rows, unequal columns, malformed/non-finite data and missing files.
Three-directory GUI execution checks prefix matching, .truth fallback, backup/log
exclusion, an exact one-file limit, error counts, manually adjusted widths and
picker row identity after removal. Real picker Cancel and Save controls are
exercised using Qt dialogs; native desktop-portal chooser integration remains
unqualified. Invalid settings and Stop retain previous completed results.
Closing an active comparison cancels its worker without accessing deleted UI.

A result larger than 16 MiB remains inspectable through ReportViewer paging;
event processing continues during comparison, and export matches the entire
stored result. Unicode export, explicit replacement, missing-directory failure
and protection of inputs including a symbolic-link alias are covered. A real
engine report produces literal values 2 and 5; opening its Compare action
prefills the baseline and compares equal to the reference. Mission source and
dirty state stay unchanged, and Unicode save/reopen/run reproduces the report.

The actual application/bin/GmatQt executable was rebuilt. All 25 Qt suites
passed in 103.93 seconds; evidence: Qt6ParityValidation/check-comparison.txt.
Native Wayland execution passed and the inspected capture shows readable table
columns, mode/tolerance controls, scrollable setup, paged output and accessible
Compare/Stop/Save/Close controls. Evidence: Qt6ParityValidation/comparison-wayland.txt
and Qt6ParityValidation/comparison-wayland.png. The log includes Qt Wayland
text-input focus warnings during picker handling; this run passed.

Mid-read file replacement, non-UTF-8 formats, broader numeric/header/record
regimes, extreme directory counts and common editor/menu behavior remain
unqualified. The wx inventory has 43 of 108 entries still marked Pending audit.
The selected plugin evidence remains 5 without qualification and 15 partial.
Full replacement acceptance remains open.

## Thrust-file inputs, script actions and repeated viewer lifecycle

ThrustFilePlugin's history and segment fields and Begin/EndFileThrust commands
were audited against their engine implementations and the wx generic resource
editor. Qt now provides history creation, an existing-file chooser, typed
segment/tank/solve-for lists, clear/restore operations and resizable angle/sigma
coefficient grids. Coefficient dimensions remain pending until Apply; Cancel
keeps the prior values. File-thrust command controls select the history and an
ordered spacecraft list while retaining labels and comments.

ThrustFileTests compares GUI-configured missions with independently configured
scripts and verifies a five-kg baseline burn, a 1.875-kg scaled burn and constant
fuel after EndFileThrust. It covers all four acceleration/thrust data formats
with None/Linear interpolation, list clearing, exact Undo/Redo, Unicode
Save/Save As/reopen, input-picker Cancel, invalid-edit rollback, missing and
malformed input recovery and generated report access. The full-day bundled
example depletes 180 kg to a final total mass of 1170 kg and reproduces its
report after save/reopen. Multiple spacecraft/segments, cubic interpolation,
time-varying angles, estimator solve-fors and coverage boundaries remain open.

This execution exposed a GUI file-context bug: stream interpretation did not
set the engine's script directory. Build, Apply and restoration now set it to
the current document's folder; an untitled/default mission uses the startup
folder. Thrust-file Apply uses the engine's matching resolver. Process CWD is
unchanged, preserving startup asset lookup. Relative inputs beside their
script survive Build/Apply/save/reopen. Moving a document with raw Save As does
not rebase its relative asset references; the bundled test explicitly selects
an absolute input before moving the script. General asset rebasing and broader
relative-path consumers remain open.

The wx EditorPanel/ScriptPanel and spacecraft wrapper source were audited.
Qt adds Mission > Save and build script (Ctrl+Shift+F7), Save, build and run
mission (Ctrl+Shift+F5), and Edit > Go to line (Ctrl+L). Saving must succeed
before either combined action builds or runs; empty scripts are rejected.
ScriptEditingTests covers Cancel, exact saved content, Unicode paths, bounded
line navigation without modification, literal 2/5 execution outputs,
save-failure source/identity protection, invalid-script and empty-input recovery.
Multiple inactive documents and broader shared editor/menu operations remain
pending. The spacecraft wrapper maps to the existing focused Qt dialogs and
shared pending resource editor; broad mixed-page transactions remain pending.

ScriptToolsPlugin registers only CommandEcho. Qt adds its On/Off control and
creation template. Its generated script now includes the missing terminating
semicolon, allowing GUI mission lookup/editing without changing execution.
Tests verify pending Apply, retained names/comments, exact Undo/Redo and
save/reopen, trace boundaries, calculation-preserving output, invalid-edit
rollback and restoration of initially disabled/enabled echo settings.
Nested branches and stopped/failed execution remain pending.

WindowTests now repeats the default mission viewer lifecycle three times:
title-bar ground minimize/restore, Output double-click opening, small and large
resize, orbit maximize, immediate close/reopen of both views, and rerun with
ground minimized and orbit closed. Every checkpoint requires retained spacecraft
samples plus rendered blue texture/map and red trajectory pixels; the native
top-level surface and mission source must remain unchanged. X11's default test
also requires top-level main-window minimize/restore.

Native Wayland passed all three viewer cycles in 19.88 seconds with no forced
software-OpenGL setting. The inspected capture shows textured Earth and the
trajectory. Top-level minimize/restore is explicitly unqualified on this
desktop: both a plain Qt window and an isolated Qt MDI/OpenGL-anchor window
report WindowMinimized immediately and WindowNoState one second later.
The strict Wayland test fails that assertion; subsequent programmatic
restoration also fails synthetic Output activation. The successful viewer run
uses the explicit --skip-main-minimize flag and logs that limitation on every
cycle. This is not evidence for main-window compositor minimize/restore.
Evidence: Qt6ParityValidation/viewer-lifecycle-wayland.txt/.png,
qt-minimize-probe.cpp/.txt and qt-minimize-mdi-probe.txt. The probe source can be
compiled with pkg-config's Qt6Widgets/Qt6OpenGLWidgets flags; run without
arguments for a plain window, or with `mdi anchor` for the MDI case.

Native Wayland ThrustFileTests and ScriptEditingTests passed with isolated
preferences and temporary input/output files. Inspected captures show readable
thrust fields, coefficient buttons and script/navigation layout. Evidence:
Qt6ParityValidation/thrust-wayland.txt/.png and script-editing-wayland.txt/.png.
Native script-dialog focus handling emitted Qt Wayland text-input warnings.

The actual application/bin/GmatQt executable was rebuilt; all 27 Qt suites
passed in 122.38 seconds, including strict X11 main-window minimize/restore.
Evidence: Qt6ParityValidation/check-thrust-script-lifecycle.txt. The exact user
launcher was also run from /tmp with its default startup and default mission,
isolated settings, native Wayland and --run/--screenshot. It exited successfully;
the inspected capture shows the textured ground map and propagated track.
Evidence: Qt6ParityValidation/checkpoint-launch-wayland.txt/.png. This CLI
capture proves launch/default execution; native interactive lifecycle evidence
comes from WindowTests using the same frontend and startup.

The wx inventory now has 40 of 108 entries marked Pending audit. Selected
plugin evidence has three without qualification and 17 partial. Full workflow,
viewer/OF and plugin acceptance remains open.

## Existing external Python force-model editing and serialization

The plugin's three configuration fields, its Python import wrapper, owned-force
serialization and the wx propagation panel were audited. No dedicated wx
external-force controls were found. Although ScriptFileName is declared as a
filename, the runtime imports it as a Python module name. Qt now supplies a
module selector from configured Python search directories instead of a file
chooser that would store an unusable absolute filename. Dotted/custom module
names can still be typed. A tooltip explains the module-name and code-cache
contract. The function and ExcludeOtherForces setting are editable alongside
the module on an existing external contributor, with pending Apply and Cancel.

The plugin marked all fields read-only and omitted them during normal owned
serialization. Reconstructing a mission for any GUI resource edit could
therefore lose the external force. The owned serializer now writes the module
declaration before the dependent function and exclusion settings, using the
interpreter's supported syntax. Qt explicitly exposes only the three supported
configuration fields; inherited runtime properties remain protected. No force
evaluation or propagation algorithm was changed.

ExternalForceTests uses the shipped no-API Python model and example, shortened
to 600 seconds. Its six external state components agree with the independent
internal two-body propagation within 1e-5 km / 1e-8 km/s. GUI selection, chooser
Cancel, pending edits and Apply reproduce the independently configured script
report. Exact Undo/Redo, Unicode save/reopen, missing module/function runtime
failure and recovery, invalid-setting rollback and both exclusion settings are
covered. The combined-force result differs from the external-only result and
matches its separate script reference. An unrelated report precision edit also
retains that combined-force result and serialized external configuration.

Native Wayland execution passed with isolated settings and temporary reports;
the inspected capture shows readable module/function fields, selection and
boolean controls. Evidence: Qt6ParityValidation/external-force-wayland.txt/.png. The rebuilt
user executable and all 28 Qt suites passed (118.73 seconds); full log:
Qt6ParityValidation/check-external-force.txt.
New-contributor creation/removal, full-day/API-dependent examples,
package/custom-path persistence, modified-module caching, multiple spacecraft,
variational propagation and malformed callback results remain open. Property-comment
fidelity is also open: the interpreter can attach dotted external-field comments
to the resource declaration, overwriting earlier comments. The serializer retains
attribute comments when present on the owned model, but this does not repair
that import behavior. This is initial qualification of the existing-contributor workflow, not full plugin
qualification. The selected plugin inventory now has two without qualification
and 18 partial; 40 of 108 wx workflows remain Pending audit.

## Shared wx panel and dialog contracts

The five shared GmatBaseSetupPanel, GmatDialog, GmatPanel, GmatSavePanel and
SubscriberSetupPanel sources were reviewed alongside Qt ResourceEditor,
CommandEditor and MainWindow. These are shared contracts, not five independent
mission features. The inventory now records the existing typed controls, pending
edits, cloned validation, script reconstruction, save-before-build/run and
rollback evidence together with specific remaining differences. In particular,
context Help, object-script previews, command summaries, staying open after
Apply, INI layout metadata, complete dialog navigation and multiple inactive
script documents are not qualified.

This is a source audit, not additional execution qualification. Individual
resource and command rows retain their own unverified combinations. There are
now 35 of 108 entries marked Pending audit; audited partial rows still carry
open requirements. The selected plugin inventory remains two without initial
qualification and 18 partial.

## Existing polyhedron gravity controls and checked shape inputs

The three plugin fields, owned serialization, body-shape reader and SurfaceHeight
parameter dependency were audited. Qt now provides a celestial-body selector and
an existing-file shape picker, shows density in kg/m^3, and explains that mesh
coordinates use kilometres. Changes remain pending until Apply. Density must be
finite and positive; body selection must refer to a celestial body. Paired body
and shape edits retain the original field ownership while the canonical body
prefix changes.

The plugin reported itself as a user force although PolyhedralBodies already
declares its contributor. Engine serialization therefore emitted both
PolyhedralBodies and UserDefined, causing any GUI reconstruction to fail with a
duplicate-force error. Its classification now emits only the supported
PolyhedralBodies declaration. All uses of IsUserForce in this engine are
serialization/list construction; gravity and propagation evaluation are unchanged.

The old file loader ignored numeric-conversion failures and did not validate
vertex references before indexing them. The loader now parses into temporary
storage and commits a mesh only after counts, finite coordinates, connectivity,
nondegenerate triangles, paired/oppositely directed edges and positive enclosed
volume pass. This rejects open or inconsistently oriented meshes and globally inward
face winding before gravity evaluation. Relative inputs use the engine's script-directory
search context. Valid vertex/face ordering and the legacy decorative numeric
record labels remain intact. Tabs, CRLF and a missing final newline are accepted.
Self-intersecting surfaces are not detected or qualified.

PolyhedronTests uses a synthetic closed cube centered on Earth with 20-km edges
and density 2000 kg/m^3. This is a controlled input fixture, not an Earth physical
model or asteroid qualification. Its 60-second velocity agrees with an
independent far-field mass approximation within 0.2 percent. GUI selection and
density Apply match the independently configured script's state and legacy
SurfaceHeight report. The parameter browser selects Sat.FM.SurfaceHeight with
the correct force-model dependency; Output opens the generated report. Exact
Undo/Redo and Unicode save/reopen preserve execution. A Mars body/path change
and Earth restoration qualify paired configuration, not Mars propagation.

Ten malformed meshes cover invalid counts, truncated data, out-of-range and
repeated indices, open surfaces, nonfinite/collinear vertices, reversed face
orientation and trailing data. GUI Apply rejects each without changing source;
raw-script failure followed by load/build/run of the valid mission recovers the
reference report. Missing input and invalid density/body recover similarly.
Relative-path save/reopen and an unrelated report precision edit retain the
contributor and calculation. Native Wayland execution passed with isolated
settings; the inspected capture shows readable fields, density units, body
selection and shape browsing. Evidence: Qt6ParityValidation/polyhedron-wayland.txt/.png.
After correcting the separately discovered CommandEcho lifecycle failure, the
rebuilt user executable passed all 29 Qt suites in 126.18 seconds. Combined
validation log: Qt6ParityValidation/check-polyhedron-echo.txt.

New-contributor creation/removal, real asteroid meshes/custom bodies, multiple
bodies/spacecraft, variational and precision propagation, disk-access errors,
geometric self-intersection and broader SurfaceHeight numerical semantics remain
open. The selected plugin inventory now has one without initial qualification
(EKF) and 19 partial. The wx source-audit inventory remains 35 of 108 Pending
audit; this is not full replacement qualification.

## CommandEcho restoration across rebuild, failure and stop

The first 29-suite polyhedron checkpoint exposed a repeatable ScriptEditing
failure when echo was enabled before a subsequent Run. Run reconstructs the
mission, and cleanup of the previous configured commands unconditionally restored
a stale captured setting; newly constructed commands also had an uninitialized
capture field. This could overwrite the current user setting before the next
mission began. The failure is preserved in
Qt6ParityValidation/check-echo-before-fix.txt.

CommandEcho now initializes its runtime state and restores only a setting that
it changed during execution, once per run. Cleanup of an unexecuted command and
repeated cleanup do not overwrite later user settings. Base command cleanup is
retained. Clones and assignment preserve command configuration/name and start
with no pending runtime restoration; the previous Clone returned a default Off
command and the assignment operator's self-assignment condition was inverted.

ScriptEditingTests covers the originally failing repeated Run with echo initially
on, unchanged 2/5 report values, clone script/name fidelity, cleanup before
execution and double cleanup after a later setting change. A Save command's
missing-output-directory error occurs after CommandEcho executes; the run fails
and preserves the initial setting. A stopped While/If mission with a nested Off
command restores the initial setting. Both cases run with echo initially off and
on, then the saved valid script reopens/builds/runs successfully.

Native Wayland execution passed with isolated settings and temporary output:
Qt6ParityValidation/echo-lifecycle-wayland.txt/.png. All 29 Qt suites passed in
126.18 seconds after rebuilding the user's GmatQt executable; combined log:
Qt6ParityValidation/check-polyhedron-echo.txt. More extensive completed nested
branches/loop combinations and malformed argument syntax remain unqualified.
This closes the observed lifecycle defect without claiming full plugin or
replacement qualification.

## GPS Kalman filter, smoother and warm-start qualification

KalmanTests runs a one-hour, noise-free version of the shipped
Ex_FilterSmoother_GpsPosVec.script, retaining its GPS receiver/antenna/error
model, eighth-order Earth gravity, drag/SRP, SNC process noise and estimated
Gauss-Markov Cd coefficient. Seven generated observations feed the EKF and
Fraser-Potter smoother. The test reports the epoch, six Cartesian components and
Cd after each stage. Independently configured script results agree with the GUI
configuration within 1e-8 per reported column; observations and the complete
warm-start state/covariance CSV match exactly. This compares configuration
routes through the existing engine, not an independent filter implementation.

Resources now exposes Process Noise Models and Estimated Parameters. Their
owned models provide writable settings, typed coordinate-system and supported
solve-for controls, acceleration-noise vector cell editing and half-life units.
Spacecraft solve-for lists include named EstimatedParameter resources and its
noise-model picker lists configured ProcessNoiseModels. The smoother's Filter
picker explicitly selects sequential estimators because its dynamic engine
reference-type metadata does not populate the generic picker. RunSmoother has
a typed form/template; RunEstimator choices exclude smoothers. Pending edits,
picker Cancel, exact Undo/Redo, Unicode mission save/reopen and filter/smoother
Output report viewers are exercised. Production choices exclude the testing-only
LinearTime process-noise model; that model is not qualified here.

Warm-start input uses an existing-file chooser and output uses a save chooser;
empty input means cold start and empty output disables CSV writing. The
format/epoch pair applies in format-first order. Format changes convert explicit
dates with GmatTime precision, preserve boundary sentinels and retain the old
format/date after a failed conversion. The suite cycles every supported time
representation and compares converted, format-only, saved/reopened and
independently configured warm starts. It also compares FirstMeasurement and
LastWarmStartRecord continuation results using observations after the selected
seed. A missing prior seed and a seed already at the end of the data fail with
the engine's existing diagnostic, then valid input recovers. Missing GPS/warm
files and a malformed CSV header recover similarly. The simulator is omitted
from input-failure cases so it cannot silently recreate a missing file.

The work exposed two configuration/serialization defects. SNC's vector setter
assigned an accepted vector and then fell through to an unsupported base setter;
it now returns the accepted vector. RunSmoother inherited a formatter hard-coded
to RunEstimator, corrupting named commands into text such as
`RunEstimato 'Smooth GPS'r FPS`. The formatter now uses the concrete command
type. The rejected command-edit evidence is preserved in
Qt6ParityValidation/kalman-command-before-fix.txt. Neither change alters the
filter, smoother, process-noise or propagation calculations.

Native Wayland execution passed with isolated settings and temporary reports.
Exposed-panel captures were inspected for the warm-start, SNC and Gauss-Markov
controls: Qt6ParityValidation/kalman-wayland.txt and
kalman-wayland.{ekf,warm,snc,fogm}.png. The rebuilt user executable passed all
30 Qt suites in 153.67 seconds; combined log:
Qt6ParityValidation/check-kalman.txt. This does not qualify the desktop portal
file chooser or Wayland top-level main-window minimize/restore.

All 20 selected plugins now have initial execution evidence, and all remain
partial qualifications. Creation/removal, full-day/noisy or real observations,
additional measurements and estimation models, covariance controls, residual
graphics, prediction, warm-start smoothing, malformed numeric/covariance CSVs,
relative warm-start files and disk-write failures remain open. Engine solver and
warm-start filenames still reject non-ASCII characters; this suite rejects an
unsupported Unicode output edit without changing source while qualifying Unicode
mission filenames. The wx inventory remains 35 of 108 Pending audit and the
broader GUI/camera/OF acceptance gates remain unfinished.


## Applied-script previews and command/mission summaries

The shared wx inspection workflows were audited in GmatPanel::OnScript and
OnSummary, ShowScriptDialog::Create, ShowSummaryDialog's coordinate-system and
Save As handlers, and MissionTree's command/mission summary actions. wx supplies
summary display names while populating its tree; Qt now assigns temporary names
while reading summaries and restores all original names afterward. BeginScript
uses its matching EndScript's captured state with the script-event name, then
restores the EndScript name. Summary construction and coordinate conversion use
the existing engine; no propagation or numerical algorithms changed.

Resource and command editors have Show script buttons displaying the applied
configuration in read-only, monospaced, unwrapped text. The resource preview is
captured when its panel opens, so an engine rebuild cannot leave a dangling
object pointer. Pending resource/command text is retained and excluded from the
preview. Command editors also offer Summary; Mission tree context menus and
Run > Mission summary provide command/entire-mission access. A local Find dialog
supports searching and Copy; read-only viewers no longer show ineffective
replacement controls.

Summary dialogs provide coordinate-system selection and a Physics commands only
checkbox for entire missions. Spacecraft-origin and spacecraft-dependent axes
are excluded, including configured axes whose reference pointers have not yet
been initialized. Changing frame or filtering refreshes the engine text; an
error restores the previous selection and text. A model-generation guard
prevents an open summary or command panel from resolving an old index after a
rebuild. Run is required, rebuilding/loading invalidates results, and source
edits reject stale summaries. Failed/stopped runs can expose the states that
executed; unexecuted commands retain the engine's explicit no-data explanation.

InspectionTests reports the first and second coast states separately and compares
both six-component command summaries against those reports in EarthMJ2000Eq,
EarthFixed, Moon-centered MJ2000Eq and Earth-Moon barycenter MJ2000Eq frames.
BeginScript's end state matches the first coast, its name is restored, a skipped
If branch produces no fabricated state, and all/physics mission views differ
while retaining the executed propagations. This compares two output routes
through the same engine, not independent propagation mathematics. Actual MDI
resource/command previews preserve pending edits and source, read-only typing is
rejected, local Find/Copy works, and inspection adds no script undo entry.

Save as exports UTF-8 atomically. The Qt widget chooser's accepted and canceled
paths, Unicode output, unwritable-directory retry, and original/current mission
filename protection are covered; symlinks to the mission are rejected as well.
Tests use isolated settings, temporary reports/exports and a temporary engine
output directory. A generic ClearPlot command without a specialized form retains
editable command text: a missing-reference edit rolls back, correction to
MarkPoint applies, and Unicode save/reopen preserves the independent state
report. Runtime negative-mass rejection and a stopped While loop retain prior
command summaries, then a corrected run reproduces the full report.

Native Wayland passed; the saved summary layout was visually inspected:
Qt6ParityValidation/inspection-wayland.png and inspection-wayland.txt.
All 31 registered Qt suites passed across the headless/native runs. The
initial check-qt invocation passed all 26 headless suites; five X11 checks could
not connect to their temporary display inside the sandbox. All five passed
with display access in 37.75 seconds. Both results are retained in
Qt6ParityValidation/check-inspections.txt and check-inspections-native.txt.
This does not qualify the desktop portal chooser, top-level Wayland
minimize/restore, singleton previews, font zoom, broader command/plugin summary
fields or solver-loop summary semantics. Resource Variable/String setup still
needs its own wx audit; the generic property table hides their engine-read-only
expression metadata. Context Help and keeping applied panels open also remain
unfinished. The wx inventory now has 32 of 108 Pending audit; audited rows can
still be partial, and the broad replacement goal remains active.


## Variable/String initialization and direct Array cell controls

Audited the active wx ParameterSetupPanel, ParameterCreateDialog,
ArraySetupPanel and ArraySetupDialog and their GmatMainFrame routing. The active
Variable/String editor shows a disabled name and edits numeric Value or String
Expression. Production creation uses numeric literals and literal strings;
conditional experimental expression/object-selection paths are not enabled.
Arrays accept 1–1000 rows and columns and have a direct row/column Value/Update
workflow as well as the numeric grid.

Qt Variable and String resources now open focused Initial value panels with
applied-script previews. The generic table previously hid the read-only numeric
metadata; setting String Value changed the runtime string without updating its
serialized initializer. The focused Apply path updates the effective source
initializer, interprets the candidate, and checks the numeric value or both
String Expression and Value before committing the source. It preserves grouped
Create declarations, earlier initializers, inline comments and subsequent
mission assignments. Omitted initializers are inserted after the declaration;
optional semicolons and implicit mission boundaries are supported. If the
engine's first mission statement cannot be located safely, Apply reports an
error instead of rewriting executable assignments. No numerical algorithms or
shared engine parser code changed.

GMAT preserves inner apostrophes literally and does not decode doubled quotes.
Its existing percent/comment parser cannot represent every combination of quotes
and percent signs; some inputs build but truncate. Interpreted-value checks
reject those candidates and restore the prior model/source. When a representable
new literal contains percent and the initializer has a trailing comment, the
comment is preserved on its own preceding line to avoid it becoming string
data. Control characters and multiline literals are rejected. This is an
explicit engine-format limitation, not general string-format qualification.

New resource includes typed initial values for Variable/String, preserves each
pending value when changing type, validates duplicate/reserved names and opens
the created resource. Array creation and resizing now match the wx 1–1000 range
on both axes. Numeric grids add synchronized Row/Column selectors, scrolling to
the selected cell, a Value field and Set cell/Enter action. Values remain local
until grid OK and panel Apply; finite-value errors do not change the cell.
Column widths remain adjustable.

Parameters tests exercise actual MDI panels and New resource dialogs, numeric
and Unicode String Apply, pending previews, invalid correction, source/model
rollback, exact Undo/Redo, grouped and omitted initializers, scientific notation,
optional semicolons and implicit mission sequences. GUI-configured and separately
scripted missions produce identical numeric/text reports (including later
mission assignments), and Unicode mission save/reopen preserves both. This
compares configuration routes through the same engine, not independent numerical
implementations. Empty strings, spaces, apostrophes, doubled apostrophes,
semicolons, percent, quotes/backslashes and statement-looking literal data are
covered, together with rejection of known truncating combinations.

Creation Cancel, invalid-value correction without closing, retained values across
type changes, atomic creation Undo/Redo, Unicode String creation and reserved or
duplicate names are covered. Actual New resource creates a 1000×1000 Array; its
last cell is navigated directly, invalid Set is rejected, grid Cancel leaves the
model intact, and accepted pending data applies and survives save/reopen.
Deleting the added resources reproduces the original reports. Settings and
reports are isolated in temporary directories.

All 32 registered Qt suites passed in 172.91 seconds with display access,
including native X11 normal/HiDPI/fractional rendering, viewer lifecycles and
launch. The user executable application/bin/GmatQt was rebuilt. Combined log:
Qt6ParityValidation/check-parameters.txt.

Native Wayland passed and the Variable, String, creation and last-cell layouts
were visually inspected: Qt6ParityValidation/parameters-wayland.txt and
parameters-wayland.{variable,string,create,array}.png. The maximum-grid evidence
covers this host; it is not a scalability guarantee for every system. Desktop
portal chooser behavior, shared Help, keeping panels open after Apply, broader
keyboard/focus operations, and all imported source/parser combinations remain
unqualified. The wx inventory now has 30 of 108 entries Pending audit; audited
rows and all 20 selected plugins still contain unfinished qualifications. The
broader replacement goal remains active.
