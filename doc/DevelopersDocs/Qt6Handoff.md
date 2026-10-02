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

Current priority, clarified on 2026-10-01: deliver a usable Qt GMAT GUI with broad
capability coverage. Prioritize ordinary resource, mission and output workflows
and failures that block using them; defer exhaustive visual and edge-case
refinements until that coverage is in place. Do not unnecessarily repeat
previous tests. The full Linux replacement scope remains, with native desktop
testing stopped after the recorded compositor crash.

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

## Initial handoff repository checkpoint

Historical checkpoint from handoff preparation; use Qt6Project.md and the latest
qualification appendices for the active new checkout/build:

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

## Work completed at the initial handoff

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

The following is the historical handoff checkpoint; later appendices supersede
its pending dispositions. Evidence directory: `doc/DevelopersDocs/Qt6ParityValidation/`.

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

The current qualification inventory has source/caller audits for all 108 wx
entries. Its mapped rows and latest appendices distinguish implemented cases,
inactive/deferred workflows and remaining limits. Continue missing active
operations and their bounded qualification; do not repeat the completed audits.
Folder runs, About/license, shared Help/Apply, debugger, welcome, parameter
selection and mission navigation have subsequent implementation/evidence.
Resource Clone and local Rename are also implemented; use their latest evidence
for coverage and external-source boundaries.

### 3. Shared desktop/editor behavior

Shared Help, retained resource/command Apply and independent active/inactive
script documents have subsequent implementation and bounded evidence. Qualify
remaining desktop input, keyboard/focus and portal chooser behavior without
repeating the covered source/transaction checks. Dirty/pending Apply/Discard/
Cancel, mixed-page edits, failed-edit recovery and document differences remain
part of the full contract. Report search/options and broader combinations keep
the explicit limits recorded in the current inventory and appendices.

### 4. Selected plugins and file recovery

Use the 20-row selected runtime plugin table in the qualification document.
Each plugin has bounded evidence, but many rows remain partial. Finish required
GUI settings, creation/removal and representative runtime/report/round-trip cases;
explicitly resolve fixture requirements and unsupported scope. Do not silently
turn every possible numerical regime into a new algorithm qualification task.
Check relative assets/includes, kernel paths, filesystem/write failures and
recovery. The SPICE diagnostic-file repair has subsequent independent-process
evidence; preserve its remaining race/storage limits and historical failures.

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

Run from `/home/dan/GIT/GMAT-Qt`. Its Qt-only build is configured and has completed
the first engine/frontend/plugin build. See `Qt6Project.md`; keep incremental
builds and select checks affected by current changes.

```bash
cmake --build build/linux-gui --target GmatQt --parallel 2
cmake --build build/linux-gui --target check-qt --parallel 2
```

Launch exactly as the user does:

```bash
cd /home/dan/GIT/GMAT-Qt/application/bin
./GmatQt
```

A focused CTest example, after rebuilding its executable:

```bash
cmake --build build/linux-gui --target GmatQtEventLocatorTests --parallel 2
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

## Shipped examples and latest continuation — 2026-10-02

Every one of 164 standalone shipped mission/tutorial candidates has a real build
attempt, and every one of 147 successful builds has a run attempt. Current stages
are 136 completed runs, six unexpected runtime failures, two expected Step 1 Stop
checkpoints, three timed-out runs and 17 failed builds. Raw interrupted status
is retained. See Qt6ParityValidation/shipped-examples-20261002.md, manifest and
177-row table, plus the latest qualification appendix. Keep the full acceptance
scope; these are build/execution checks, not complete native/scientific parity.

All work and rebuilt runtime are in /home/dan/GIT/GMAT-Qt. Coordinate pending axes,
branch/Toggle insertion, relative assets, OF lists/undrawn cameras/retained vectors,
public Python API and SRP string-array fixes remain. New Clone covers unified
optional naming, pending/atomic source and managed Array/Qt metadata. Local Rename
now updates typed references, arrays and Qt metadata with explicit external-file
boundaries. Clone/OpenFramesSyntax/OpenFramesVectors passed after their frontend
changes; the final rebuilt Rename workflow passes its new focused check. No old full matrix was
repeated. Initial new-test harness errors are preserved/corrected.

The free R2026a VF13ad binary is installed locally in the ignored
application/plugins/thinksys/vf13ad directory. GMAT_QT_EXTERNAL_PLUGINS in the local
preset keeps its entry through startup regeneration. All 19 formerly blocked
variants build; 15 complete, two requested Step 1 Stop, two Mars time bounds. The
external binary/README is not committed or a built plugin target. Preserve its
license. Remaining failed builds: deferred MATLAB 4, SNOPT 3, CSALT 9 (SNOPT needed)
and legacy MarsGRAM 1. Do not automatically waive absent public plugins/data.

Formation now contains a clearly labeled reconstruction of the intended chapter,
with missing historic states disclosed; actual target residuals meet tolerance.
L2's measured wider bracket fixes its initial guard but the next full run exposes
an intermediate trial missing the geometric entry box and exhausting DE405.
One bounded diagnostic establishes the event-domain problem; do not remove the
guard, cap-and-claim-success or keep retrying without substantive transfer analysis.
Five TLE examples still require matching absent public catalogs. Their exact data
requirements and all helper/fragment coverage limits remain recorded.

The existing Yukon 7200-second attempt is now terminal timeout, with 11 of
20 windows completed. Its selected result/history was merged after matching
source identity; all raw outputs are preserved in the ignored durable yukon
tree. No unchanged restart is scheduled. This is incomplete, not a full pass.
The VF13ad/MarsLaunch and MarsPatchConic 300-second attempts were progressing,
not proved hung; preserve as incomplete. SPAD's earlier 209.91-second completed
retry must not repeat. QualifyExamples.py is resumable; select failed examples
only after concrete fixes. Completed raw trees are preserved in
build/example-qualification/20261002 (raw, formation, vf13-tutorials, vf13-probe,
l2-bracket, yukon). Live native desktop testing remains stopped until the GNOME Shell
crash/safety issue is resolved. Windows/macOS and MATLAB stay deferred.


Latest active workflow: Rename is implemented and its focused check passes
0.44 seconds after rebuilding GmatQt. See resource-rename-20261002.md and the
latest qualification appendix. Includes/external GMFs and unsupported tracking
paths stay explicit script-editor boundaries. Resource popups now use value
selection and are destroyed before modal dispatch; the offscreen lifetime check
is not proof of a compositor fix. Read-only Qt source/crash review found no
established current GMAT lifetime defect. Keep the native acceptance gate open.

## Next phase: interactive Help tutorials — 2026-10-02

After the current qualification work, follow the Help tutorials by constructing
each scenario through the Qt application, starting from a new mission or an
independently built prerequisite tutorial. Do not use shipped tutorial mission
scripts to construct it; compare those only after preserving the authored
mission and its results. See [Qt6TutorialWalkthrough.md](Qt6TutorialWalkthrough.md)
for the twelve published chapters, twelve additional source chapters, dependency
notes and evidence requirements. All walkthroughs are pending. Earlier example
runs and tutorial repairs do not satisfy this new GUI acceptance phase. Resolve
a safe interactive display route before testing; the native acceptance gate and
the existing Linux replacement goal remain open. Avoid repeating passing tests.


## Isolated actual-input windows, object guides and Undo — 2026-10-02

A safe private authenticated X11 route now operates the actual Qt application
with mouse/keyboard input, private settings/output and software GL. It avoids
the user's desktop. New mission execution, one main minimize/restore and viewer
close/reopen cycle, category-specific GroundStation creation/Cancel/automatic
name/Save, and long-fixture pause/stop were exercised. The host GNOME Shell PID
and start time remain unchanged. These are bounded X11 results; host GNOME/
Wayland, portals, hardware-driver and repeated lifecycle acceptance remain open.

The attempt exposed Ctrl+Z doing nothing with plot/tree focus. Undo/Redo now
fall back to the selected script when non-text controls have focus, preserve
local/inactive-document histories and block background modal actions. Owned
modal fields now follow parentWidget ownership across the dialog window boundary.
The final focused check passes 0.28 seconds. An actual Mission-tree Ctrl+Z retry
restores the entire saved authored source byte for byte, and its short mission
completes 0.563 seconds. The first failed shortcut assumption and the actual
modal-routing failures are preserved; no old full matrix was repeated.

Per-object DrawGrid and DrawXYPlane conversion, pending controls, native/fallback
geometry and retained replay now exist. QtGui.ObjectGuides passes 0.31 seconds;
actual native X11 controls show Earth's spherical grid and filled 15R radial
plane. The saved guide source retains global Grid Off / XYPlane On. Unsupported
velocity/other decorations and wider conversion/camera regimes remain open.

See Qt6ParityValidation/object-guides-20261002.md and
Qt6ParityValidation/isolated-x11-input-20261002.md for identity, selected captures
and limits. Full raw sessions/check failures are retained under
build/example-qualification/20261002/isolated-x11. The actual application/bin/GmatQt
is rebuilt, final SHA256 9bc8115f4200bc4c933b72de2383cfe7722d79c463d148a64c029238b460ea8d.
All Help tutorial walkthroughs remain pending; this route can support their
future independent construction. Full Linux replacement remains unfinished.


## Viewer preview/shared displays, velocity and command names — 2026-10-02

All development continues in /home/dan/GIT/GMAT-Qt on codex/qt6-gui. The original
checkout remains untouched. New actual-input evidence found that clean OrbitView
settings close destroyed a same-name preview and cleared the live viewer's
objects. ResourcePreview frontend ownership now suppresses just that clone's
synchronous ClearObjects; genuine engine cleanup remains active.
QtGui.PreviewLifetime passes 0.89 seconds. Its private X11 retry retains textured
Earth/path/legend/rotated camera at 47.6% through two Output delete/reopens,
three main minimize/restores and a viewer minimize/restore. Preserve the original
failed captures; these passes do not establish the prior GNOME crash's cause.

Three actual-input displays (Orbit/Ground/GUI-created XY) share scrubbing and
Start/Play/Pause;27% pause captures remain identical after 500 ms. Confirmed
per-Orbit Local override/global Latest works. Do not claim LocalXY from early
mislabeled global-slider actions or save from screenshots still showing SaveAs.
A new retained XY settings Window activation clips controls; its correction
needs the focused workspace check and actual-input retry. See the accompanying
viewer-preview-shared-20261002.md for full identities and bounded observations.

DrawVelocity now converts source-ordered/default/prefix/Add-reset flags and
provides pending per-object controls. Recorded view-frame velocities drive exact
OF-style position+1000 s × velocity line segments, native/fallback/replay/trim/Fit
and source/rename retention. QtGui.ObjectVelocity passes 0.85 seconds; affected
ObjectGuides passes 0.34 seconds with DrawUsePropLabel as its unsupported fixture.
The rotating-frame comparison uses NutationUpdateInterval=0 in the fixture to
align Report and cached-plot paths; no production numerical changes. Actual
input Apply/run and Output reopen show velocity at Latest. The attempted native
middle replay ended at Latest and remains unqualified in that session. Relative
Velocity/ThrustVector and wider scientific/solver regimes remain open.

Command panels now support pending optional Name and runtime-gated Write with
ordered parameters/full source fallback. A real named-assignment serialization
failure with GMAT keyword OFF required an 11-line GmatCommand::InsertCommandName
fix; no numerical execution changes. QtGui.MissionLabelsWrite passes 0.89 seconds,
including keyword OFF/ON/OFF/repeated serialization and report 9/2.5. Read-only
focused peer review found no additional substantive defects. All first failures,
diagnostics and successful retries remain preserved in the ignored durable
build/example-qualification/20261002 trees. No old successful full matrix or
shipped corpus was repeated.

All Help tutorial walkthroughs remain Pending. Qualification fixtures in these
sessions are independently GUI-authored but are not Help tutorial constructions.
After current gaps, use the Help actually available, build each scenario through
visible GUI actions from a new mission/prerequisite authored tutorial, save/run/
reopen and preserve results before consulting a shipped reference. Full Linux
replacement acceptance remains open; Windows/macOS/MATLAB remain deferred.


### Final workspace/native retry for this checkpoint

WorkspaceReachability passes 0.34 seconds. Initial placement inherited the active
script's maximized state and later restored an unclamped old geometry; normalizing
new configuration panels plus settled/direct and guarded final placement fixes
that case. Actual private X11 input confirms Window and Resources restore the
same displaced pending XY form with full controls. A fresh accepted master
Start/47.6% seek and actual Output Orbit reopen retain native velocity at that
position. Earlier failed/inconclusive actions keep their dispositions; see
Qt6ParityValidation/workspace-reachability-20261002.md.

Application/bin/GmatQt is rebuilt, final SHA256
c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3;
shared core 5eb86098fbc52890a95d3226c56a4edd8e090308a940bb097749e23237532ab9;
selected startup 5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a.
The authored fixture remains byte-identical. Host GNOME Shell remains PID399961,
start 2026-10-01 11:34:56 MDT. Full replacement and all Help walkthroughs remain open.

### Safe private Wayland route: preparation only

Read-only inspection found installed GNOME Shell/Mutter 50.1, Qt Wayland, Mesa
software EGL and Python Gio. A next isolated route may use an owned headless
compositor with --wayland --headless --no-x11 --virtual-monitor=1600x1000 and a
private --wayland-display under a fresh 0700 runtime/private HOME/config/cache and
its own D-Bus. Remove inherited DISPLAY/WAYLAND_DISPLAY/WAYLAND_SOCKET/XAUTHORITY/
DBUS session variables. Force Mesa/llvmpipe; bwrap may mask host sockets/dev/dri.
Namespace availability, compositor startup and Qt rendering have NOT been tested.
Do not use --display-server with headless. No compositor was launched here.

Actual input can use one persistent Gio connection to Mutter RemoteDesktop
CreateSession/Start/NotifyKeyboard and relative pointer/button/axis methods;
transient gdbus callers fail ownership checks. Private Shell --unsafe-mode allows
its screenshot service; relative input/screenshot need no PipeWire. Absolute
input requires ScreenCast/PipeWire. Keep these capabilities confined to the owned
private bus/runtime, with no physical devices or host desktop sockets.
Authoritative installed-version source:
[Mutter options](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/core/meta-context-main.c),
[headless backend](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/native/meta-backend-native.c),
[GPU-less renderer](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/native/meta-renderer-native.c),
[virtual input](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/meta-remote-desktop-session.c),
[private screenshot](https://raw.githubusercontent.com/GNOME/gnome-shell/50.1/js/ui/screenshot.js).
This is implementation preparation, not native Wayland acceptance or a proved
fix for the user's earlier crash. Segment-arc identity and mixed solver cleanup
remain substantive active gaps after the current checkpoint. Tutorial construction
must still follow the Help and reserve shipped references for later comparison.


### Mixed hierarchy follow-up

The new mixed Yukon/DC check found a real outer-state leak; the five existing
Optimize publisher transitions now synchronize its member display state. Both
Yukon and installed VF13ad pass the new analytic/report/accepted-history/inner
Stop/recovery checks (0.99/1.03 s). Numerical Yukon report bytes match the original
failed run. See mixed-solver-scopes-20261002.md; all original failures are retained.
App remains c15cb5c...; rebuilt dynamically linked core is now 68a79c56... .
Private Wayland preparation has progressed to a mapped software-rendered mission,
but accurate actual pointer input and window lifecycle are still being verified.
Do not claim full Wayland or tutorial acceptance. Named regular-arc identity and
trim-independent camera endpoints are being implemented next.

VF13ad is an optional binary plugin but derives InternalOptimizer. The final
new checks assert that path and the actual inner index9/command text;0.99/1.03 s
passes supersede the weaker0.99/0.97 s Stop proof. ExternalOptimizer execution is
still unqualified. See boundary-verified artifacts and the revised mixed record.
