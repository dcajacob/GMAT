# Linux GUI change tracker

No upstream issues or pull requests have been opened. The branch names below reserve proposed PR scopes; they are not submitted PRs.

- Upstream: `nasa/GMAT`, base `9363e129be366520c6edb0b4079204ed60666007` (verified against `origin/main`).
- Fork: https://github.com/dcajacob/GMAT (`fork` remote); NASA remains `origin`.
- Working branch: `linux-gui-integration`, containing all fixes and the combined regression runner.
- `main` remains unchanged at the upstream base.
- Each `pr/*` branch starts directly at that base and contains only its feature plus the test support it needs. It does not inherit the other production fixes.
- Integration and standalone PR commits may have different IDs because their parent trees differ.
- Local IDs below are bookkeeping labels, not GitHub issue numbers.

## Proposed changes

| Local ID | Branch | Integration commit | PR branch commit | Test entry point | Status |
|---|---|---|---|---|---|
| LGUI-001 | [pr/de-header](https://github.com/dcajacob/GMAT/tree/pr/de-header) | `00f98cce80e9` | `00f98cce80e9` | `test_header.py` | Prepared; not submitted |
| LGUI-002 | [pr/wxwidgets-path](https://github.com/dcajacob/GMAT/tree/pr/wxwidgets-path) | `f3a66269d70a` | `42158c089d88` | `Fresh CMake configure` | Prepared; not submitted |
| LGUI-003 | [pr/groundtrack-map](https://github.com/dcajacob/GMAT/tree/pr/groundtrack-map) | `e236844b3297` | `8245a6bf727b` | `test_groundtrack.py` | Prepared; not submitted |
| LGUI-004 | [pr/linux-layout](https://github.com/dcajacob/GMAT/tree/pr/linux-layout) | `dba102c8cc2c` | `a74f93a0ebdc` | `test_layout.py` | Prepared; not submitted |
| LGUI-005 | [pr/linux-geometry](https://github.com/dcajacob/GMAT/tree/pr/linux-geometry) | `025e86c185b4` | `e8ceedebd24c` | `test_geometry.py` | Prepared; not submitted |
| LGUI-006 | [pr/linux-script-exit](https://github.com/dcajacob/GMAT/tree/pr/linux-script-exit) | `02fd4e4f5c91` | `d53b3342be72` | `test_exit.py` | Prepared; not submitted |
| LGUI-007 | [pr/linux-plugin-lifetime](https://github.com/dcajacob/GMAT/tree/pr/linux-plugin-lifetime) | `7321902c3fa4` | `bb4109147eb6` | `test_plugins.py` | Prepared; maintainer design review needed |

## Validation and dependencies

The split test suites passed on the existing Release integration build: 22 reported checks. Standalone syntax compilation passed for every modified C++ translation unit on its proposed PR branch, including the separately reconstructed layout and geometry headers. The production source bytes on the integration branch match the pre-organization version. Git patch whitespace checks pass.

Full application builds and runtime tests have **not** been repeated independently on all seven PR branch trees. On this host, upstream startup has the fortified DE-header failure, so the ephemeris fix should land before meaningful standalone GUI runtime validation. Strict GTK tests may also need the separate tree/layout fix. Do not describe integration test results as isolated-branch runtime results.

Native test runs use X11 virtual displays and software OpenGL. Physical displays and native Wayland remain additional release checks. OpenFrames checks use optional external dependencies and retain known TimeDilator tooltip warnings in their logs.

## Review and submission order

1. Submit the ephemeris and CMake corrections first when submission is authorized.
2. Refresh the map and layout branches against the resulting upstream base and rerun their focused tests.
3. Refresh geometry and script-exit branches; the shared GmatApp/GmatMainFrame edits may need small conflict resolutions.
4. Seek maintainer agreement on the Linux plugin-lifetime tradeoff before submitting that change.
5. Keep shared test-support files identical across branches. Once one copy lands, rebasing the other PRs removes that duplicate infrastructure from their diffs.

## Running checks

After a completed CMake/Ninja build, from the repository root:

```sh
python3 src/UnitTests/TestLinuxGui/run_tests.py build/linux-gui
python3 src/UnitTests/TestLinuxGui/test_groundtrack.py build/linux-gui
```

The first command is the integration runner. On a focused PR branch, run its `test_*.py` entry point from the table instead. Logs and isolated settings are under `build/linux-gui/linux-gui-tests/<suite>/`. The CMake correction was verified with a fresh configure, not a mirrored unit test.

## Keeping this record current

- Make further fixes on the corresponding PR branch and commit them with the relevant test. Use a separate worktree if keeping the working integration build available matters.
- Port the production change back to the integration branch; resolve overlapping window hunks without replacing the unrelated fix.
- Update both commit IDs, validation evidence, dependencies and status here when branch tips change.
- Fetch upstream before eventual submission. Refresh affected branches and rerun their own checks against their new base.
- Populate real issue/PR links only after the user authorizes submission. Store maintainer feedback with its corresponding change.
- Keep machine-specific startup files, preferences, dependencies and build outputs untracked. `Old` stays out of scope.

## Unresolved follow-ups

- The reported `pixman_region32_init_rect` warning did not reproduce under the debugger and is not claimed fixed.
- The bundled Aura model references missing `GFOIL1.JPG`.
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
