# Linux GUI change tracker

No upstream issues or pull requests have been opened. The branch names below reserve proposed PR scopes; they are not submitted PRs.

- Upstream: `nasa/GMAT`, base `9363e129be366520c6edb0b4079204ed60666007` (verified against `origin/main`).
- Fork: https://github.com/dcajacob/GMAT (`fork` remote); NASA remains `origin`.
- Working branch: `linux-gui-integration`, containing all fixes and the combined regression runner.
- `main` remains unchanged at the upstream base.
- Each `pr/*` branch descends from that base and has a production-only net diff for its feature. Supplemental tests remain on the integration branch. It does not inherit the other production fixes.
- Integration and standalone PR commits may have different IDs because their parent trees differ.
- Local IDs below are bookkeeping labels, not GitHub issue numbers.

## Proposed changes

| Local ID | Branch | Integration commit | PR branch commit | Test entry point | Status |
|---|---|---|---|---|---|
| LGUI-001 | [pr/de-header](https://github.com/dcajacob/GMAT/tree/pr/de-header) | `00f98cce80e9` | `437c812a1684` | `test_header.py` | Prepared; not submitted |
| LGUI-002 | [pr/wxwidgets-path](https://github.com/dcajacob/GMAT/tree/pr/wxwidgets-path) | `f3a66269d70a` | `42158c089d88` | `Fresh CMake configure` | Prepared; not submitted |
| LGUI-003 | [pr/groundtrack-map](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-map) | `e236844b3297` | `d7acd5aebeb1` | `test_groundtrack.py` | Prepared; not submitted |
| LGUI-004 | [pr/linux-layout](https://github.com/dcajacob/GMAT/tree/pr/linux-layout) | `dba102c8cc2c` | `4acd19e622b4` | `test_layout.py` | Prepared; not submitted |
| LGUI-005 | [pr/linux-geometry](https://github.com/dcajacob/GMAT/tree/pr/linux-geometry) | `025e86c185b4` | `b3e0c889e640` | `test_geometry.py` | Prepared; not submitted |
| LGUI-006 | [pr/linux-script-exit](https://github.com/dcajacob/GMAT/tree/pr/linux-script-exit) | `02fd4e4f5c91` | `6e8f646ec3ba` | `test_exit.py` | Prepared; not submitted |
| LGUI-007 | [pr/linux-plugin-lifetime](https://github.com/dcajacob/GMAT/tree/pr/linux-plugin-lifetime) | `7321902c3fa4` | `596215497ff9` | `test_plugins.py` | Prepared; maintainer design review needed |
| LGUI-008 | [pr/native-viewport](https://github.com/dcajacob/GMAT/tree/pr/native-viewport) | `b3e61889e716` | `097c67fc4036` | `test_viewport.py` | Prepared; not submitted |
| LGUI-009 | [pr/editor-reload](https://github.com/dcajacob/GMAT/tree/pr/editor-reload) | `bfe0c353d4e4` | `fffdfe934f60` | `test_editor_io.py` | Independently tested with prerequisites; not submitted |
| LGUI-010 | [pr/editor-save](https://github.com/dcajacob/GMAT/tree/pr/editor-save) | `d8f14d423743` | `bd34b116e50c` | `test_editor_io.py` | Independently tested with prerequisites; not submitted |
| LGUI-011 | [pr/model-preview-context](https://github.com/dcajacob/GMAT/tree/pr/model-preview-context) | `a401cd960ec5` | `ba65591932e1` | `test_workflow.py` | Independently tested with prerequisites; not submitted |
| LGUI-017 | [pr/python-diagnostics](https://github.com/dcajacob/GMAT/tree/pr/python-diagnostics) | `b904b645f09c` | `91953c519cf1` | `test_python.py --mode diagnostics; test_python_gui.py` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-020 | [pr/python-return-conversion](https://github.com/dcajacob/GMAT/tree/pr/python-return-conversion) | `c001a19dab33` | `3ffc37638fa0` | `test_python.py; test_python_gui.py` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-020 | [pr/linux-python-extensions](https://github.com/dcajacob/GMAT/tree/pr/linux-python-extensions) | `07b3989fe6ed` | `927e7fb46bac` | `test_python_iod.py; independent NumPy GUI probe` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-018 | [pr/orbitview-plane-alias](https://github.com/dcajacob/GMAT/tree/pr/orbitview-plane-alias) | `2791d7c1e752` | `001400757fed` | `test_orbit_alias.py; two unchanged tutorials` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-021 | [pr/tle-examples](https://github.com/dcajacob/GMAT/tree/pr/tle-examples) | `584c63e2538f` | `2b4b9f27c372` | `test_examples.py --only TLEPropagatorPlugin` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-022 | [pr/satellite-separation-example](https://github.com/dcajacob/GMAT/tree/pr/satellite-separation-example) | `9c7c2429694f` | `12e10aa7f344` | `test_public_examples.py --mode satsep` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-023 | [pr/global-function-example](https://github.com/dcajacob/GMAT/tree/pr/global-function-example) | `4f4f077c7123` | `b1ca38c21703` | `test_public_examples.py --mode global` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-024 | [pr/force-model-tutorial](https://github.com/dcajacob/GMAT/tree/pr/force-model-tutorial) | `631051823e76` | `0350a31697c5` | `test_public_examples.py --mode force` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-025 | [pr/sensor-contact-example](https://github.com/dcajacob/GMAT/tree/pr/sensor-contact-example) | `18029f89f280` | `746cdb101bd2` | `test_public_examples.py --mode sensor` | Independently tested with prerequisites; backed up; not submitted |
| LGUI-026 | [pr/groundtrack-runtime](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-runtime) | `b40f40bc3f3b` | `230bd15efacf` | `test_gui_followup.py` | Independently tested with prerequisites; not submitted |
| LGUI-029 | [pr/groundtrack-sampling](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-sampling) | `569a91d231f2` | `548bec34370a` | `test_gui_followup.py` | Independently tested with prerequisites; not submitted |
| LGUI-030 | [pr/groundtrack-window-lookup](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-window-lookup) | `4c7feb215d9d` | `4489b59bf1a2` | `test_gui_followup.py` | Independently tested with prerequisites; not submitted |
| LGUI-028 | [pr/native-wheel-zoom](https://github.com/dcajacob/GMAT/tree/pr/native-wheel-zoom) | `19dd83be5c76` | `ea64c9fb581a` | `test_gui_followup.py` | Independently tested with prerequisites; not submitted |
| LGUI-027 | [pr/native-plot-text](https://github.com/dcajacob/GMAT/tree/pr/native-plot-text) | `979c0d644ba1` | `57f4031b0a3f` | `test_gui_followup.py` | Independently tested with prerequisites; not submitted |
| LGUI-031 | [pr/groundtrack-station-markers](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-station-markers) | `daf193ed67d2` | `934e3b852cdf` | `test_gui_followup_more.py` | Independently tested with prerequisites; not submitted |
| LGUI-032 | [pr/groundtrack-editor-output](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-editor-output) | `a0a5a7078f30` | `33098ec0bb68` | `test_gui_followup_more.py` | Independently tested with prerequisites; not submitted |
| LGUI-033 | [pr/tree-theme-colors](https://github.com/dcajacob/GMAT/tree/pr/tree-theme-colors) | `4c742c8e1081` | `246cdd40f3a3` | `test_gui_followup_more.py` | Independently tested with prerequisites; not submitted |
| LGUI-034 | External OFI patch, archived on integration | `751a352b39ab` in isolated OFI | See external audit | `test_openframes_time.py` | Fixed in local build; external patch not submitted |
| LGUI-035 | [pr/native-animation-replay](https://github.com/dcajacob/GMAT/tree/pr/native-animation-replay) | `cd2eacad668e` | `b07be71d2e1c` | `test_native_animation.py` | Independently tested with prerequisites; not submitted |
| LGUI-036 | [pr/resource-delete-menu](https://github.com/dcajacob/GMAT/tree/pr/resource-delete-menu) | `ba5bc315deb8` | `57e22b63e052` | `test_resource_delete.py` | Independently tested with prerequisites; not submitted |
| LGUI-037 | [pr/groundtrack-state-mapping](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-state-mapping) | `2ea51e533646` | `a8d0e9a74126` | `test_groundtrack_state.py` | Independently tested with prerequisites; not submitted |
| LGUI-038 | [pr/groundtrack-direct-close](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-direct-close) | `f327cf93c381` | `9bcf36765304` | `test_groundtrack_close.py` | Independently tested with prerequisites; not submitted |
| LGUI-039 | [pr/groundtrack-animation](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-animation) | `34140f6dd515` | `afe90a6789b4` | `test_groundtrack_animation.py` | Independently tested with prerequisites; not submitted |
| LGUI-040 | [pr/aura-reflection-map](https://github.com/dcajacob/GMAT/tree/pr/aura-reflection-map) | `4210a0757126` | `f6894739d367` | `test_aura_model.py` | Independently tested with prerequisites; not submitted |

## Validation and dependencies

The split test suites passed on the existing Release integration build: 30 reported checks. Standalone syntax compilation passed for every modified C++ translation unit on its proposed PR branch, including the separately reconstructed layout and geometry headers. The native viewport correction additionally passes nine GL viewport assertions across 1×, 2× and 3× scaling. Git patch whitespace checks pass.

Full application builds and runtime tests have **not** been repeated independently on the eight earlier PR branch trees. On this host, upstream startup has the fortified DE-header failure, so the ephemeris fix should land before meaningful standalone GUI runtime validation. Strict GTK tests may also need the separate tree/layout fix. Do not describe integration test results as isolated-branch runtime results.

Native test runs use X11 virtual displays and software OpenGL. The new audit also passed physical Intel GPU/XWayland checks; native Wayland remains blocked at toolkit initialization. OpenFrames checks use optional external dependencies and retain known TimeDilator tooltip warnings in their logs.

## Review and submission order

1. Submit the ephemeris and CMake corrections first when submission is authorized.
2. Refresh the map and layout branches against the resulting upstream base and rerun their focused tests.
3. Refresh geometry and script-exit branches; the shared GmatApp/GmatMainFrame edits may need small conflict resolutions.
4. Seek maintainer agreement on the Linux plugin-lifetime tradeoff before submitting that change.
5. Keep supplemental test machinery on the integration branch; offer focused fixtures in the maintainers' preferred format. The small native viewport patch can be reviewed early alongside the startup/build corrections.

## Running checks

After a completed CMake/Ninja build, from the repository root:

```sh
python3 src/UnitTests/TestLinuxGui/run_tests.py build/linux-gui
python3 src/UnitTests/TestLinuxGui/test_groundtrack.py build/linux-gui
```

The first command is the integration runner. Run these test entry points on the integration branch; they are deliberately absent from the production-only proposed PR diffs. Logs and isolated settings are under `build/linux-gui/linux-gui-tests/<suite>/`. The CMake correction was verified with a fresh configure, not a mirrored unit test.

## Keeping this record current

- Make further fixes on the corresponding PR branch and commit them; maintain the corresponding supplemental test on the integration branch. Use a separate worktree if keeping the working integration build available matters.
- Port the production change back to the integration branch; resolve overlapping window hunks without replacing the unrelated fix.
- Update both commit IDs, validation evidence, dependencies and status here when branch tips change.
- Fetch upstream before eventual submission. Refresh affected branches and rerun their own checks against their new base.
- Populate real issue/PR links only after the user authorizes submission. Store maintainer feedback with its corresponding change.
- Keep machine-specific startup files, preferences, dependencies and build outputs untracked. `Old` stays out of scope.

## Unresolved follow-ups

- The reported `pixman_region32_init_rect` warning did not reproduce under the debugger and is not claimed fixed.
- The bundled Aura reflection reference was repaired in LGUI-040; earlier logs retain the original diagnostic.
- External OFI TimeDilator can warn about setting a tooltip before creating its widget.
- Native OrbitView default-mission window creation can produce GTK notebook gadget warnings.
- Toolbar overflow and dark-theme editor styling remain separate future work.

## Local PR description drafts

- [Fix fixed-width DE ephemeris header copies](LinuxGuiPRs/de-header.md)
- [Preserve PATH when wxWidgets_ROOT_DIR is supplied](LinuxGuiPRs/wxwidgets-path.md)
- [Resolve ground-track textures and handle failed redraws safely](LinuxGuiPRs/groundtrack-map.md)
- [Keep Linux navigation and message panes within the window](LinuxGuiPRs/linux-layout.md)
- [Restore Linux window geometry from personalization settings](LinuxGuiPRs/linux-geometry.md)
- [Close plots and report failures during Linux automatic exit](LinuxGuiPRs/linux-script-exit.md)
- [Keep Linux plugin libraries mapped through process shutdown](LinuxGuiPRs/linux-plugin-lifetime.md)

- [Native viewport correction](LinuxGuiPRs/native-viewport.md)

See [contribution research and preparation recommendations](LinuxGuiContributing.md).

## First systematic audit batch

[Audit evidence, coverage gaps and next findings](LinuxGuiAudit.md). The three new branches were built and tested independently with the five prerequisites listed in that record; all three also pass combined validation. Hardware XWayland checks now pass on Intel Iris Xe at 3×; native Wayland fails GTK initialization.

## Example coverage and follow-ups

[Example sweep](LinuxGuiExamples.md) and [all 210 outcomes](LinuxGuiExamples.csv): 125 completed, 80 interpretation failures, 5 mission failures. New findings LGUI-017 through LGUI-021 are recorded for follow-up; no production fix or new PR branch was created during the sweep. `test_examples.py` adds opt-in, isolated GUI execution with logs, physical viewport checks and independent screenshots. No PRs, issues or external comments were submitted.

## Public-feature fix batch

[Five patches, independent tests, final sweep comparison and limitations](LinuxGuiPublicFixes.md). The original sweep above is retained as the baseline. Production-only branches are backed up to the fork; no PR, issue or external comment has been submitted.

## Public documentation-example repairs

[Four independent sample patches, numerical checks and 10-script regressions](LinuxGuiExampleMaintenance.md). The earlier sweep records remain intact.

## External OpenFrames time-control repair

[Patch, baseline failures, scale regressions and build prerequisites](LinuxGuiExternal/OpenFramesTime/README.md). This supersedes the pending-repair status in the second follow-up snapshot.

## Native animation replay

[Wrapped-buffer failures, isolated patch validation and regression results](LinuxGuiAnimation/README.md).

## Resource deletion with unrelated editors

[Menu reproduction, dependency protection and independent validation](LinuxGuiResourceDelete/README.md).

## GroundTrack state mapping

[Reproductions, missing-point handling, independent validation and integration notes](LinuxGuiGroundTrackState/README.md).

## GroundTrack close and animation

[Direct-close crash and independent fix](LinuxGuiGroundTrackClose/README.md).

[Toolbar playback, scale/lifecycle tests and patch dependencies](LinuxGuiGroundTrackAnimation/README.md).

## Bundled Aura reflection reference

[Provenance, exact asset edit and identical native/OSG loaded-scene checks](LinuxGuiAuraModel/README.md).
