# Native GUI follow-up: first verified patch batch

Final status: see the [completed evidence audit](LinuxGuiFinalAudit/README.md). The dated batch snapshot below is retained as history.


28 September 2026. The durable goal remains active. This report covers five completed patches; it does not claim the entire Old-derived review is resolved. Qt and floating-window work remain excluded. Old and the Downloads reference sources were read only. No PRs, issues or comments have been submitted.

## Changes in the current build

- **GroundTrack runtime ownership:** copies allocate independent buffers and coordinate systems, repeated initialization releases previous allocations, repeated spacecraft/station binding replaces references, station-only plots initialize safely, all spacecraft origins are checked, and ShowPlot=false creates no window.
- **Readable native plot labels:** Linux rasterizes fonts at a fixed logical size scaled to the display, with per-context glyph caches. Four test glyphs previously occupied 32 physical pixels regardless of scale (32/16/10.67 logical pixels at 1x/2x/3x). They now occupy 40/40/41.33 logical pixels. Actual canvas resize is asserted. Other platforms keep the existing font path.
- **Normal wheel zoom:** centered and free-flight modes use reversible proportional distance changes with a fixed view center. Astronaut translation and Shift-wheel field-of-view behavior remain operational. On the default scene the measured colored-Earth region changes from 33,207 to 40,133 pixels, then returns to 33,207.
- **GroundTrack sampling/settings:** collection frequency now decimates data while retaining the first point, resets on initialization, and uses a bounded countdown. Nonpositive collection/update frequencies and negative redraw counts raise errors. Zero redraw count can be restored. Hidden plots send no data. Body rebinding avoids duplicates and reports accepted references correctly.
- **Mixed-window lookup:** GroundTrack creation, rename, options, actions and data delivery check actual window type and ignore closing windows. The reproduction previously reused an XY window under an old-name collision and crashed; it now creates/reuses the correct GroundTrack window.

## Focused production branches

| ID | Branch | Integration commit | Standalone commit |
|---|---|---|---|
| LGUI-026 | `pr/groundtrack-runtime` | `b40f40bc3f3b` | `230bd15efacf` |
| LGUI-029 | `pr/groundtrack-sampling` | `569a91d231f2` | `548bec34370a` |
| LGUI-030 | `pr/groundtrack-window-lookup` | `4c7feb215d9d` | `4489b59bf1a2` |
| LGUI-028 | `pr/native-wheel-zoom` | `19dd83be5c76` | `ea64c9fb581a` |
| LGUI-027 | `pr/native-plot-text` | `979c0d644ba1` | `57f4031b0a3f` |

Each branch has a production-only diff against NASA base `9363e129be366520c6edb0b4079204ed60666007`. Test infrastructure remains on integration. Branch names reserve review scopes; they are not open PRs. `branches.json` has full hashes, prerequisite base and evidence paths. Backup verification is recorded separately in `backup.json` when completed.

## Validation and prerequisites

All five proposed patches were built and exercised **one at a time** in the isolated validation checkout. The baseline is `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`: upstream plus the previously documented DE-header, wxWidgets path, Linux layout, GroundTrack map, native viewport and Linux plugin-lifetime prerequisites. The checkout is restored between patches. These are independent-with-prerequisites results, not claims that unpatched upstream runs successfully on this host.

The text branch keeps the physical viewport fix separate. Its single overlapping viewport hunk was merged with that prerequisite for validation; the font call still takes the canvas content scale. The standalone sampling branch initializes its counter in the upstream copy constructor as well as assignment because the separate ownership branch consolidates copying through assignment.

Focused tests cover ownership, copying, hidden windows, repeated references, actual data delivery/decimation, invalid settings, mixed-window rename/routing, text at 1x/2x/3x and actual resize, wheel controls at 1x/2x/3x, free flight, astronaut and Shift mode. Historical fixtures additionally cover station-only and mixed-station plots, hidden plots, invalid central bodies/frequencies and a mismatched third spacecraft origin. Expected script failures are explicitly asserted.

The established GUI suite passes: header checks, layout/geometry, GroundTrack maps, native viewports, editor save/reload recovery, native/light/dark and OpenFrames workflows, console/GUI exit, normal plugin shutdown, converted GroundTrack and Hohmann convergence. Its run preceded the final one-line accepted-body-reference return correction; the focused reference test was rerun afterward. One final broad regression pass is still required after the remaining GUI work.

Tests use Linux wxGTK 3.2, isolated X11 displays and software OpenGL. Windows/macOS and real monitor-scale transitions remain unvalidated. GTK notebook and external OpenFrames tooltip warnings are retained. A GTK screenshot can retain stale GL content during interaction, so wheel assertions read the rendered back buffer rather than infer zoom from a cached screen capture.

Run the focused suite from the current repository root:

```sh
python3 src/UnitTests/TestLinuxGui/test_gui_followup.py build/linux-gui
```

## Remaining goal scope

This is the first-batch snapshot. See LinuxGuiFollowup2.md for the subsequent editor, station and theme fixes, live-paint evidence and the OpenFrames reproduction.

| Finding | Current disposition |
|---|---|
| GroundTrack editor and Output tree integration | Source defects identified; implementation and editor/save regression pending |
| Ground-station marker type/missing-object guards | Unsafe lookup identified; isolated reproduction and guard pending |
| GroundTrack live updates | Long-run paint timing still needs measurement and a demonstrated repair |
| Dark-theme tree colors | Non-savable mode still forces WHEAT; paired system colors and contrast tests pending |
| OpenFrames time-control sizing/tooltips | Reproduction and isolated external dependency patch pending; reference source must stay unchanged |
| Animation replay and toolbar support | Wrapped-buffer replay/data preservation and toolbar dispatch need evaluation and fixes where demonstrated |
| Delete unrelated resources with other editors open | Dependency/open-editor behavior needs verification before changing menu availability |
| Plugin-parent lookup | Stable hierarchy needs testing; no speculative hierarchy rewrite |
| OpenFrames font fallback | Missing-Microsoft-font scenario needs testing |
| Texture filtering | Demonstrated visual defect needed before changing filtering or mipmaps |
| Missing GFOIL1.JPG | Asset origin and model search path need verification before packaging a fix |
| GroundTrack published-state indexing | Inspection additionally found unchecked six-element reads and label-index alignment risks; reproduce before a focused correction |
| Final completion audit | Pending until every item is resolved or supported by a tested no-fix disposition |
