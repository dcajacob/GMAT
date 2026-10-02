# Qt 6 Linux GUI replacement handoff

Prepared 2026-09-30 for a fresh Codex chat. This is a navigation and continuation
brief, not proof of qualification. Recheck the repository before making changes.

## Start here

1. Work in `/home/dan/GIT/GMAT-Qt` on `codex/qt6-gui`, the separate Qt
   checkout requested on 2026-10-01. See `Qt6Project.md` for launch/build details.
   The original `/home/dan/GIT/GMAT/GMAT` checkout and historical evidence are retained.
2. Read this file, `Qt6Gui.md`, and the acceptance gates and current inventory
   in `Qt6ReplacementQualification.md`. Read that document's latest appendices
   before treating an older checkpoint's pending items as current.
3. Prioritize viewer/window reliability on the user's Linux desktop, then
   finish the active wx workflows and the remaining qualification gates.
4. Make concrete fixes, validate them, record their evidence, and rebuild the
   actual application. Do not stop at another inventory or proposed plan.

## User intent and boundaries

Deliver a fully qualified Qt 6 replacement for the selected Linux GMAT GUI
runtime. Keep the familiar wx arrangement: Resources/Mission/Output navigation,
MDI workspace, messages, menus and toolbar. Improve from that recognizable base.

- Windows and macOS builds/deployment are deferred. MATLAB is outside the
  selected runtime; the current build has `PLUGIN_MATLABINTERFACE=OFF`.
- The user likes the current Qt viewer direction and dislikes the OF viewer.
  Preserve the Qt interaction model while matching required capabilities.
  Native rendering uses OSG inside Qt; this is distinct from embedding OF.
- Render textures and starfields and retain existing implemented viewer work.
- On opening an OF example that needs translation, automatically offer conversion
  instead of only reporting a build failure. Preserve compatible settings and
  explicitly handle unsupported settings. A warning alone is not parity.
- Tables should have sensible initial column widths and remain user adjustable.
- Preserve script comments, labels, expressions, unknown settings and implicit
  defaults through edits, deletion, Undo/Redo, Save/Save As and reopening.
- Keep the existing numerical engine. Check that GUI edits preserve calculations;
  do not expand this task into requalifying or rewriting numerical algorithms.
- Use independent judgment. Earlier experiments in `Old` are reference material,
  not a design constraint. Keep changes focused on the official checkout.
- Work autonomously within this scope; do not repeatedly ask permission for
  ordinary reversible fixes, builds and verification.

The user's most serious reported failures were: default example runs but Orbit
View does not open; ground-view minimization hangs; lingering stalls; Orbit View
shows only a vector triad. Several subsequent fixes and tests passed, but the
remaining native desktop acceptance gates must still be closed. Do not infer
that these exact bugs still reproduce, or that every desktop case is now fixed.

## Current repository and executable

Verified when preparing this handoff:

- Branch: `codex/qt6-gui`.
- HEAD: `df97b3b` — Guard stale Qt event reports and activate Output viewers.
- Working tree was clean before adding this handoff.
- User launches `/home/dan/GIT/GMAT/GMAT/application/bin/GmatQt`.
  It resolves to `application/bin/GmatQt-R2026a` in this checkout.
- Build directory: `build/linux-gui`, Ninja, Release, both wx and Qt GUIs enabled.
- Qt startup: `application/bin/gmat_startup_qt.txt`. Use its selected native
  plugin inventory; wx OF widget providers are not Qt plugins.

Recent commits worth inspecting:

| Commit | Change |
| --- | --- |
| `df97b3b` | Guard stale event reports; explicitly activate Output windows |
| `72d6a7b` | Delete resources without regenerating unrelated script source |
| `3b7be21` | Preserve resource source and implicit defaults during edits |
| `172dc55` | Match wx body selection; preserve implicit configuration |
| `1034956` | Celestial-body pages and kernel/appearance edit qualification |

## Most recent completed work

Resource edits now patch changed source assignments while retaining unrelated
configuration and mission text. Canonical regeneration had made implicit power
defaults explicit and changed results slightly; source retention addresses that
case. Grouped Array declarations support dimension changes. Owned ODEModel
configuration retains dependent ordering and legacy aliases. FOV placeholder
cleanup respects a real object named `UndefinedFieldOfView`.

Deletion removes the selected declaration/assignments while retaining other
members of grouped declarations and comments. Referenced resources are protected.
Celestial bodies and SolarSystem remain protected in the GUI, matching wx
ownership restrictions. Unsupported custom-body deletion remains a text workflow.

Event output now consults the current model/run and running locator's
`FileWasWritten` state before displaying an existing file. Rebuilt, pending,
Disabled, WriteReport-off and unexecuted Manual configurations cannot present
stale results as current. Generated reports use ReportViewer; unwritten output
explains the required configuration/FindEvents action. Report, event placeholder
and binary-ephemeris windows now explicitly activate when opened.

Ten additional wx rows were audited. Several legacy solver dialogs are inactive
non-writing prototypes; MATLAB workflows are outside the selected runtime.
Active propagator selection maps to tested Qt controls. Those dispositions do
not qualify unrelated active workflows, About/license or batch-run reports.

## Evidence and its limits

Evidence directory: `doc/DevelopersDocs/Qt6ParityValidation/`.

- `check-event-output.txt`: all 36 Qt suites passed, 155.72 seconds.
- `check-event-output-report-disabled.txt`: final WriteReport-off event case and
  explanation passed the focused event suite, 3.84 seconds. The application was
  rebuilt with the final change. The complete suite predates that last small
  event addition; do not describe it as a subsequent full-suite rerun.
- `event-output-wayland.txt` and `event-output-wayland.png`,
  `.png.generated.png`, `.png.unwritten.png`: native Wayland configuration and
  generated/unwritten Output windows, captured and inspected.
- `check-source-deletion.txt`: all 36 suites passed after deletion changes.
  `round-trip-deletion-wayland.txt` records native deletion coverage.
- Existing native X11 lifecycle tests and multiple Wayland viewer/panel checks
  passed. Top-level main-window Wayland minimize/restore remains unqualified.
- Widget file choosers were exercised with native dialogs disabled. Desktop
  portal choosers have not been qualified.
- Negative SPICE runs emit `FILEOPENFAILED` / `IOSTAT 128` while writing
  `GMATSpiceKernelError.txt`. Correction/rerun succeeds; diagnostic-file handling
  remains unresolved/unqualified. Preserve the raw evidence of these messages.

Green suites establish their covered cases, not full GUI or scientific parity.

## Remaining work and suggested order

### 1. Native viewer/window acceptance

Use the user's executable and startup path. Exercise the default example,
Orbit View and Ground Track, switching/focusing windows, minimizing/restoring,
repeated close/reopen, run/stop/rerun, and pending edits. Finish top-level Wayland
minimize/restore qualification. Check rendered trajectories/bodies/textures,
camera histories/replay and remaining solver-loop display cases. Complete
required OF translation behavior while keeping the Qt viewer UX.

### 2. Finish source audits and active wx workflows

The current inventory has 10 of 108 source entries marked `Pending audit`:

- `src/gui/foundation/ParameterSelectDialog.hpp`
- `src/gui/debugger/InspectorPanel.hpp`
- `src/gui/mission/UndockedMissionPanel.hpp`
- `src/gui/mission/TreeViewOptionDialog.hpp`
- `src/gui/app/FileUpdateDialog.hpp`
- `src/gui/app/TextEphemFileDialog.hpp`
- `src/gui/app/RunScriptFolderDialog.hpp`
- `src/gui/subscriber/OpenGlOptionDialog.hpp`
- `src/gui/app/WelcomePanel.hpp`
- `src/gui/app/AboutDialog.hpp`

Inspect active callers and actual behavior, implement missing operations, and
provide execution/interaction evidence. Some may already map to Qt or be inactive;
the count is audit progress, not a completion percentage or ten known bugs.
Known delivery gaps include folder-run workflow/reporting and About/license.

### 3. Shared desktop/editor behavior

Context Help and keeping panels open after successful Apply remain pending.
Qualify dirty/pending Apply/Discard/Cancel, mixed-page transactions, keyboard and
focus behavior, portal choosers, and remaining multi-script/document differences.
Report paging/search/comparison exists; broader search/options still carry limits.

### 4. Selected plugins and file recovery

Use the 20-row selected runtime plugin table in the qualification document.
Each plugin has bounded evidence, but many rows remain partial. Finish required
GUI settings, creation/removal and representative runtime/report/round-trip cases;
explicitly resolve fixture requirements and unsupported scope. Do not silently
turn every possible numerical regime into a new algorithm qualification task.
Check relative assets/includes, kernel paths, filesystem/write failures and
recovery; resolve the known SPICE diagnostic-file issue.

### 5. Final acceptance audit

Reconcile every acceptance gate with current implementation and evidence.
Compare representative wx/Qt workflows and independent script-configured reports.
Verify calculation-preserving save/reopen, Undo/Redo and failed-edit recovery.
Run the relevant regression suite after fixes and rebuild the actual user binary.
Full replacement is unfinished until the requirements have affirmative evidence.

## Useful implementation locations

- `src/qtgui/MainWindow.cpp`, `MainWindow.hpp`: navigation, Output, MDI lifecycle.
- `src/qtgui/MissionModel.cpp`: engine/build/run integration and transactional edits.
- `src/qtgui/UserParameter.cpp`: source-preserving edit/deletion helpers.
- `src/qtgui/ResourceEditor.cpp`, `EditablePanel.cpp`: common panel behavior.
- `src/qtgui/OrbitRenderer.cpp`, `OrbitCamera.hpp`, `PlotWidget.cpp`:
  native rendering, camera and viewer behavior.
- `src/qtgui/ScriptCompatibility.cpp`: OF example translation.
- `src/qtgui/tests/WindowTests.cpp`, `OrbitRendererTests.cpp`,
  `OrbitSetupTests.cpp`, `ResourceRoundTripTests.cpp`, `EventLocatorTests.cpp`:
  relevant regression entry points.
- `src/qtgui/CMakeLists.txt`: test targets and `check-qt` dependency list.
- `src/qtgui/tests/RunTest.py`: configured headless test wrapper.

## Build and run

Run from `/home/dan/GIT/GMAT/GMAT`. Use the existing configured build rather than
recreating dependency configuration without cause.

```bash
cmake --build build/linux-gui --target GmatQt --parallel 8
cmake --build build/linux-gui --target check-qt --parallel 8
```

Launch exactly as the user does:

```bash
cd /home/dan/GIT/GMAT/GMAT/application/bin
./GmatQt
```

A focused CTest example, after rebuilding its executable:

```bash
cmake --build build/linux-gui --target GmatQtEventLocatorTests --parallel 8
ctest --test-dir build/linux-gui -R '^QtGui.EventLocators$' --output-on-failure
```

Native tests need desktop/display access. In the previous sandbox, Xvfb/X11
checks failed without that access, then passed when authorized. Distinguish
environment access failures from application failures. Offscreen screenshots
cannot establish native OpenGL or desktop window lifecycle correctness.
Preserve `SYS_PATH` if reconfiguring; retain no-MATLAB settings.

## Durable goal state

The previous chat's stored goal objective is:

> Accomplish items 3, 4, and 5 of the Qt replacement qualification plan on Linux:
> audit wx resource editors and mission commands against Qt and implement missing
> GUI workflows (including specialized controls, array resizing and expression
> cells); close required plotting/camera and OpenFrames conversion behavior gaps
> while retaining Qt viewer UX; qualify supported plugins and file handling through
> configuration, execution, reports, save/reopen, error recovery and
> calculation-preserving script round trips. Maintain an evidence-backed checklist
> of scope and verify all requirements before completion; rebuild the user's
> GmatQt executable. Windows and macOS remain deferred.

The goal tracker returned `blocked` during handoff preparation. That state is
not completion and does not establish that repository work is blocked: concrete
GUI and qualification work remains possible. Goals live in conversation state;
this Markdown does not transfer or resume a goal. A new chat should create a
goal only when the user explicitly invokes it there. Do not manually edit goal
storage, mark the old goal complete, or narrow the original acceptance scope.
