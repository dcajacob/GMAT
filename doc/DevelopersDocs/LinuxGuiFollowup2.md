# Native GUI follow-up: second verified batch

28 September 2026. The goal remains active. No PRs, issues or external comments were submitted. This batch leaves Old and the Downloads R2026a source unchanged.

## Implemented and validated

- **Station-marker safety:** a nonexistent station previously caused a segmentation fault. Missing objects and objects of the wrong type now produce an informative message and return failure. A real ground station can still be added after both rejected requests.
- **GroundTrack editor/output integration:** the editor uses the common named-parameter interface instead of casting GroundTrack to the unrelated legacy GroundTrackPlot class. The Output tree recognizes the compatible subscriber type. Tests open the actual editor, edit/apply/save collection frequency, check visible/hidden tree entries, run the mission and activate the existing plot from the output entry. Editing already worked in the corrected baseline harness; the reproduced visible defect was the missing Output entry. The invalid concrete cast was confirmed by runtime type checks and source inspection.
- **Tree colors:** normal mode uses system foreground/background colors. Non-savable mode adds a restrained warm tint. Under Adwaita dark, non-savable contrast rises from 1.45:1 to 8.00:1; normal mode returns to the system background at 13.77:1. Light-mode non-savable contrast is 19.85:1. All three trees are checked through normal/non-savable/normal transitions.

| ID | Branch | Integration commit | Independent commit |
|---|---|---|---|
| LGUI-031 | `pr/groundtrack-station-markers` | `daf193ed67d2` | `934e3b852cdf` |
| LGUI-032 | `pr/groundtrack-editor-output` | `a0a5a7078f30` | `33098ec0bb68` |
| LGUI-033 | `pr/tree-theme-colors` | `4c742c8e1081` | `246cdd40f3a3` |

Each production-only branch is based on NASA `9363e129be366520c6edb0b4079204ed60666007`. Each was independently built and run on the same documented prerequisite baseline `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`, with the validation checkout restored between patches. Full hashes and logs are in `branches.json`. Fork verification is in `backup.json` when completed.

## Tested no-change disposition: live GroundTrack repaint

A long mission with one-second maximum integration steps and collection frequency 10 generated **132 paint events during 6,880 ms of actual RUNNING state**. The current publisher yields to wxWidgets, allowing scheduled Refresh calls to paint. This tested path does not require the old direct-repaint changes. The repeatable regression requires at least three in-run paints and successful mission completion. Existing map/resize regressions also pass. This evidence covers Linux X11/wxGTK on this build, not every possible platform or plugin callback.

## Regression results and limits

The existing GUI suite passes again, including native and OpenFrames workflows, maps, viewports, file operations, geometry and shutdown. Focused entry point:

```sh
python3 src/UnitTests/TestLinuxGui/test_gui_followup_more.py build/linux-gui
```

Windows/macOS and native Wayland remain unvalidated. The disabled legacy subscriber-color-picker macro is **not** a supported tested configuration: an explicit syntax probe fails in both unchanged prerequisite baseline and patched code because of pre-existing missing GetColor/SetColor methods and wxString conversions. This work does not enable that dormant feature; the normal build and normal editor path pass.

## OpenFrames failure reproduced; repair pending

The isolated TimeDilator regression constructs the widget in a deterministic nonzero-filled allocation, exposing its uninitialized cursor state. The original Init path calls SetCursorState, which calls GTK tooltip APIs before native Create. With GTK criticals fatal, this reproduces `gtk_widget_set_tooltip_text: assertion 'GTK_IS_WIDGET (widget)' failed` and aborts. This is a demonstrated external-plugin defect; it has not yet been repaired in the current build.

The separate `test_openframes_time.py` reproducer also contains zero/tiny/restored-size checks, but those checks have **not yet run past the construction failure**. Do not claim the pixman warning is fixed or reproduced by them. It is intentionally outside the passing core suites until the external patch is applied.

An isolated source clone of OpenFramesInterface at `82ea72584fc1c858bd3eb3899e5c3d3b31e32f3c` is available under the workspace work directory. The application still uses its original reference dependency; no reference source was edited and no plugin build setting was changed. The next step must use an isolated dependency build without letting automatic dependency setup rebuild into the reference installation.

## Remaining goal work

OpenFrames time-control repair and other demonstrated external issues; animation replay/toolbar behavior; unrelated-resource deletion; plugin-parent lookup; font fallback; texture-filtering assessment; GFOIL1.JPG provenance and packaging; the newly observed GroundTrack state-index validation risks; final complete-scope regression and evidence audit. No completion claim is made for these items.
