# GroundTrack state mapping (LGUI-037)

GroundTrack now matches all six state components to their spacecraft labels instead of assuming six contiguous values beginning at X. Missing or truncated spacecraft states retain their curve slot but contribute no point. NaN is the internal absent-position marker; the GUI filters it before adding curve data. No public API or startup option was added.

## Reproduction and validation

The baseline assigned the second spacecraft's data to the first when the first was absent, produced different coordinates for reordered state components, and emitted stale/incomplete positions from short publications. Its map receiver also accepted non-finite coordinates. The baseline log records these failures.

Focused checks now pass for complete states, a missing first spacecraft, fresh second-spacecraft data, reordered components, a truncated publication, a complete first state with a truncated second state, a missing component, epoch-only and null publications, recovery on the next complete publication, and stable curve identity through missing data. Published components may use Vx/Vy/Vz or VX/VY/VZ velocity labels. Non-finite state components and epochs are omitted safely.

The test observes the real subscriber's delivery boundary and wraps the real map curve insertion function. It verifies exact longitude/latitude agreement with complete-publication results and confirms that absent points never reach the curves. A separate real GUI mission propagating two spacecraft independently also completes successfully.

```sh
python3 src/UnitTests/TestLinuxGui/test_groundtrack_state.py build/linux-gui
```

The exact standalone patch also passes on prerequisite baseline `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`, which includes the existing Linux startup/layout/map/viewport/plugin-lifetime prerequisites. The independent checkout and binary are restored afterward. The current build also passes the first follow-up suite (ownership, sampling, lookup, text and wheel) and the existing GUI suite. Evidence is Linux wxGTK/X11, Xvfb and Mesa software rendering; Windows, macOS, native Wayland and hardware-specific paths remain unvalidated.

## Review and integration

- Integration production commit: `2ea51e533646e7dab343df103c242b93a461cdf2`.
- Standalone branch: `pr/groundtrack-state-mapping`, commit `a8d0e9a74126a630bd582df7f5b39df84f42ea31`.
- Standalone base: NASA `9363e129be366520c6edb0b4079204ed60666007`.
- Scope: GroundTrack.cpp/.hpp and GroundTrackArea.cpp. Tests/audit records stay on integration.

The standalone patch excludes the separate ownership, sampling and map-loading changes. When combining it with the ownership patch, retain the ownership-safe assignment and copy the new stateIndices member instead of xIndex. When combining with sampling, retain its countdown before state conversion. Include-context conflicts must retain the map-loading includes as well as cmath. The independent build used that include-only adaptation to its existing map prerequisite. No broader refactor or warning suppression is included.

No PR, issue or external comment was submitted. Old and the R2026a reference sources remain unchanged. The overall goal remains active: toolbar behavior, plugin hierarchy/font/texture assessments, missing Aura texture provenance and the final complete-scope evidence audit are still outstanding.
