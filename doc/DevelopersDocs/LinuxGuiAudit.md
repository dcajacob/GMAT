# Linux GUI audit — first cycle, 2026-09-28

## Outcome

Completed the first reliability batch: three reproduced defects corrected, separately committed and prepared as production-only branches. No PRs, issues or comments were submitted. The expanded integration runner reports 30 passing checks (previously 25), including two editor failure suites and three workflow configurations. Every checklist area below was exercised or explicitly left with a validation gap; this is not a claim that all GUI behavior is defect-free.

The actual Intel Iris Xe desktop also passed the native workflow and 3× viewport checks. Native Wayland failed GTK initialization before a GUI could be tested. All automated sessions use isolated preferences and temporary script copies. `Old` was not inspected or modified.

## Findings and disposition

| ID | Priority / owner | Reproduction and evidence | Disposition |
|---|---|---|---|
| LGUI-009 | High / GMAT editor | Edit a loaded script, then reload an unavailable file. Before: LoadFile returned success, discarded the buffer and undo history. Three initial assertions failed. | Fixed on `pr/editor-reload`: preserve text, undo, filename and panel modified state on failure. Actual Undo/Redo, disappeared-file reload and recovery now pass. |
| LGUI-010 | High / GMAT editor | Save As to a path that cannot be opened, then try save-and-build. Before: panel was marked clean and script identity changed despite the failed write. Four initial assertions failed. | Fixed on `pr/editor-save`: check write results before renaming or clearing state; cancel build on failure. Styled and plain-text editor failures and retry pass. |
| LGUI-011 | High / GMAT native GL | Run plots, close them, rebuild/stop a mission, then open spacecraft properties. Before: X11 BadDrawable terminated GMAT. Synchronous debugger stack reaches VisualModelCanvas constructor via Mesa XGetGeometry. | Fixed on `pr/model-preview-context`: remove constructor GL calls already performed in the context-bound paint handler. Full workflow passes on software rendering and Intel hardware. |
| LGUI-012 | Medium / GMAT filename conversion | UTF-8 filename `workflow café.script` exists and opens in the editor, but GmatMainFrame::BuildScript reports it missing. The same mission builds with an ASCII filename containing spaces. | Confirmed in the linked GUI harness. Investigate wxString-to-native-file-path conversion and verify stock GUI entry points before choosing the smallest portable fix. Retained as next-batch work. |
| LGUI-013 | Medium / GMAT/wxGTK boundary | Native plot creation emits GTK negative notebook gadget dimensions; debugger stack captured. | Reproduced; no proven connection to the user's pixman rectangle warning. Pixman itself remains unreproduced in these checks. Do not suppress the diagnostics. |
| LGUI-014 | High if reproducible / optional OpenFrames lifecycle | One expanded OpenFrames workflow timed out after earlier successes; attaching afterward was denied by host ptrace restrictions. Later complete suite passed. | Intermittent, unresolved. Next reproduction must capture live stacks under a debugger launched with the test; timeout logs/process cleanup are now retained. Not attributed to the plugin without stronger evidence. |
| LGUI-015 | Unconfirmed / GMAT preview rendering | VisualModelCanvas paint still passes logical dimensions to glViewport and uses a fixed projection aspect. | Code-review candidate; add a visible model-preview DPI/resize reproduction before editing. Separate from the constructor crash fix. |
| LGUI-016 | Compatibility gap / host toolkit | GDK_BACKEND=wayland fails with GTK initialization error. | Native Wayland lane blocked on this configuration; successful XWayland checks do not imply native Wayland support. |

Known lower-priority follow-ups remain: missing Aura `GFOIL1.JPG`, external OFI TimeDilator tooltip ordering, toolbar overflow usability and editor theme/focus behavior. Ownership and impact must be confirmed before changing external dependencies or visual policy.

## Workflow coverage

| Area | Exercised | Remaining gaps |
|---|---|---|
| Startup / shutdown | Original startup, geometry/settings and missing-map checks; repeated clean close and native/OF shutdown | Intermittent expanded OF timeout; pixman reproduction |
| Editing / saving | Open, edit, save, readback, Unicode editor I/O, failed save/reload, Undo/Redo, Save As cancellation, declined unsaved close, new-script action | Native file-picker navigation/overwrite confirmation and complete keyboard-only navigation; Unicode build path failure |
| Mission execution | Native/OF run and rerun, close/rebuild/recreate plots, timed Stop with return -4, recovery run; invalid-script and GUI/console comparison suites | Pause/resume and broader mission families |
| Plots | OrbitView/converted OF, ground track, XY curves and decodable image export, native animation, resize and existing 1×/2×/3× viewport checks | Visual correctness of exports, detailed mouse gestures, model-preview DPI, monitor transitions |
| Workspace / dialogs | Existing pane/bounds suite; spacecraft, propagator, orbit and XY settings open/close twice; invalid mass rejected without changing the configured object | Exhaustive dialog reachability, focus order and uncommon panels |
| Appearance | Light and dark GTK workflow runs, small-window screenshots, overflow arrow observed | Screenshots did not provide reliable editor visibility evidence; do not claim editor contrast/focus verified. Repeat with the editor explicitly active and painted. |

Modal-response automation exercises real GMAT handlers using wxModalDialogHook. It does not claim to test the desktop file chooser UI. Invalid numeric-input tests dispatch text-change notifications after marking the control dirty, then invoke the real Apply handler. Screenshot checks supplement state and file-content assertions.

## Independent validation

A fresh native Release build compiled 717 targets/steps with optional plugins disabled. Each of the three proposed patches was then applied, rebuilt, tested and removed independently. None relied on either of the other two new fixes.

Explicit prerequisite patches were `pr/de-header`, `pr/wxwidgets-path`, `pr/linux-layout`, `pr/groundtrack-map` and `pr/native-viewport`, all relative to NASA baseline `9363e129be366520c6edb0b4079204ed60666007`. Runtime assets were read from the existing application data directory. Editor branches ran their respective save/reload modes; the preview branch ran the full native workflow. This is independent validation with prerequisites, not a claim that unmodified upstream starts successfully on this host. Windows/macOS were not built.

Environment: wxWidgets 3.2.9, GTK 3.24.52, Mesa 26.0.8, Linux 7.0.0-1014-oem. Automated rendering uses Xvfb/X11/llvmpipe. The physical desktop uses XWayland with Mesa Intel Iris Xe (ADL GT2), OpenGL 4.6 compatibility profile and observed content scale 3.0. Native Wayland is a separate failed startup probe.

## Repeatable process

1. Run the integration suite from a completed build. It records source/build/environment metadata, then the focused suites; any assertion, nonzero exit or timeout fails the run. Timeouts retain logs and terminate the test process group, including its virtual display.
2. For each new candidate, record exact steps, expected/actual behavior, environment, evidence and ownership in this table. Reproduce first; use debugger stacks for crashes and pixel/state/file checks appropriate to the failure.
3. Select at most three confirmed reliability defects. Add a focused failing reproduction, make the smallest correction, and keep unrelated production changes out of its branch. Retain test machinery on the integration branch.
4. Run focused checks, the combined suite and applicable desktop checks. Validate each proposed branch independently with explicit prerequisites. Update this record and its PR draft with remaining gaps.
5. Back up prepared branches to the fork. Obtain authorization before submitting PRs, issues or external comments. No automation or unattended recurring schedule was created.

From the repository root:

```sh
python3 src/UnitTests/TestLinuxGui/run_tests.py build/linux-gui
python3 src/UnitTests/TestLinuxGui/test_editor_io.py build/linux-gui
python3 src/UnitTests/TestLinuxGui/test_workflow.py build/linux-gui
# Opt-in: briefly opens GMAT on the current desktop; no desktop screenshots.
python3 src/UnitTests/TestLinuxGui/test_desktop.py build/linux-gui
```

Results and isolated settings are under `build/linux-gui/linux-gui-tests/`; environment metadata is `environment.json`. Optional OpenFrames checks explicitly skip if its plugins are unavailable. Supplemental tests use the existing Linux/GNU/Ninja harness, with no production test hooks or new public APIs.

The reload correction relies on [wxWidgets' file-loading behavior](https://raw.githubusercontent.com/wxWidgets/wxWidgets/v3.2.9/src/stc/stc.cpp): the base styled-text control reads successfully before replacing its text. GMAT's premature ClearAll and unconditional success return were the destructive layer.
