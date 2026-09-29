# Initial Qt 6 desktop acceptance audit

Audit date: 2026-09-28. Implementation revision: `b1e4c75` on `codex/qt6-gui`.
The request was a new Qt 6 branch with the existing PR fixes and a working GUI
recognizable as the wx layout, using the Old attempt only as reference. The
user subsequently clarified that Windows and Mac builds are not needed now.

## Evidence against the requested deliverables

| Requirement | Inspected evidence |
| --- | --- |
| New Qt 6 branch | Current branch is `codex/qt6-gui`; the branch adds `src/qtgui`, a Widgets frontend requiring Qt 6.4 or newer. Linux uses Qt 6.10.2. |
| Include the existing PR fixes | `git merge-base --is-ancestor 0ef9a8a HEAD` succeeds. The wx GUI, utility library and application data have no differences from that integration baseline. The only existing engine-source change is Moderator's null UI-interpreter detach guard. The Qt-specific equivalents are mapped in the parent document. |
| Independent design, familiar layout | The new QMainWindow uses Resources/Mission/Output tabs on the left, an MDI workspace, bottom Message Window, menus and toolbar. These correspond to the wx GmatMainFrame/GmatNotebook arrangement. The new frontend does not compile or link sources from Old; the original message-adapter reuse and changes are documented in the parent guide. |
| Working resource and mission workflows | Real-engine Workflow and Mission tests cover building/running, pause/resume/stop, resource creation/editing/deletion, validation and rollback, undo, pending/stale edits, nested command editing, and the propagation form. Unsupported specialized forms retain the script workflow. |
| Plotting and output | Plot tests compare trajectory, XY and geodetic samples with an engine report, check maps and retained histories, close/reopen/replay, and exercise default-mission visibility. They run at normal and doubled display scale. |
| Build integration and launch | `check-qt` builds the frontend, enabled plugins and tests. Generated Qt startup paths are used by default. Launcher tests cover unrelated working directories, paths with spaces, mission execution/screenshots and failure exits. |
| Runtime/layout evidence | Native Linux X11 launch from `/tmp`, without a startup override, ran the default mission and exited 0 after capture. The inspected screenshot shows the completed mission's ground-track plot in front. Resource and script screenshots show the same Qt workspace with the corresponding editors. |
| Shared-code regression safety | The existing wx/console exit regression still passes, including identical mission state reports and invalid-script exit status 1. |

## Results and visual evidence

- [Qt CTest result](ctest.txt): six of six passed after the final plot-focus fix.
- [Shared exit regression](shared-exit.txt): three checks passed.
- [Default mission, native X11](default-mission.png).
- [Spacecraft property sections](spacecraft-editor.png).
- [Script syntax highlighting and line numbers](script-editor.png).

The native launch used an isolated Xvfb display with `QT_QPA_PLATFORM=xcb`,
not the offscreen Qt backend. Editor screenshots use the offscreen backend;
they demonstrate layout, while the functional tests verify edits and engine
behavior. The spacecraft screenshot precedes the later script-highlighting
and plot-focus changes; its resource editor code is unchanged by those commits.

The audit found a default-launch defect that the fixture-only plot test had
missed: plot data/windows existed, but the script regained focus and covered
them. The fix gives shown plots focus, and the regression now verifies the
default mission after processing pending events. Native X11 capture confirmed
the corrected behavior.

## Limits of this delivery

This is the requested working, familiar initial Qt GUI, not a claim of complete
wx feature parity. The property editor still needs specialized forms and
compound-property controls. OrbitView currently draws trajectories and body
disks; spacecraft models, body textures, stars, reference planes and scripted
camera tracking are not implemented. Some advanced XY styling/solver behavior
also remains outstanding. Known wx-only OpenFrames plugins are rejected before
initialization; arbitrary third-party plugin compatibility is not guaranteed.

These are follow-on migration capabilities, not capabilities proven by this
audit. Linux runtime use is verified. Windows has earlier native validation,
but further Windows/Mac builds and standalone installers are outside this pass,
as requested. No claim of macOS runtime validation is made.
