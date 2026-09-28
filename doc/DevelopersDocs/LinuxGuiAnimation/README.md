# Native animation replay repair (LGUI-035)

The native OrbitView recording stays intact during animation, including replay after Stop. This is a focused production patch in OrbitViewCanvas.cpp and ViewCanvas.cpp, with no public or plugin API changes. No PR, issue or external comment has been submitted. Old and R2026a sources remain unchanged.

## Reproduced defects and repair

A mission with 151 published points and a 31-point plot buffer reproduced changed recording indices after animation, different epochs on the second replay, and only one or two painted animation frames. The actual recorded epoch values were unchanged, but playback had incorrectly advanced the recording cursor. A partial-trail regression then showed displayed ranges extending outside the selected recording prefix.

Replay now selects chronological recorded points without advancing the write cursor, restores the original display bounds after playback or interruption, and requests each paint through normal wxWidgets Refresh/Update handling. Trail bounds are clamped to the selected data. The final refresh restores the complete recorded view. No direct wxPaintDC calls or new rendering architecture were introduced.

## Validation

The regression observes the real GUI canvas and its existing buffer state through protected member pointers, without production test hooks. It verifies unchanged counts, indices and epochs; the exact expected chronological sequence at paint events; identical second replay; partial-trail bounds; and Stop followed by successful replay. Cases cover wrapped and unwrapped buffers with full and short trails, exact capacity, a single-point buffer, and a trail longer than the recording.

All seven cases pass in the integration build and independently with the standalone patch applied to prerequisite baseline `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`. The independent checkout and binary were restored afterward. The existing GUI suite also passes after the change. Baseline failures, complete logs and the exact patch are included.

```sh
python3 src/UnitTests/TestLinuxGui/test_native_animation.py build/linux-gui
```

Coverage is Linux wxGTK/X11 with Xvfb and Mesa software rendering. Windows, macOS, native Wayland and hardware-specific rendering remain unvalidated. This patch fixes native playback; GroundTrack/OpenFrames main-toolbar support is a separate remaining assessment.

## Review and backup

- Integration production commit: `cd2eacad668e4a9f8e986c9049a7f98d00102f33`.
- Standalone branch: `pr/native-animation-replay`, commit `b07be71d2e1c382c59d9cd7186dba1d53c2ab0ce`.
- Standalone base: NASA `9363e129be366520c6edb0b4079204ed60666007`.
- Tests and audit records stay on `linux-gui-integration`. This does not depend on the new OpenFrames time-control patch.

The overall goal remains active. Remaining assessments include main animation toolbar behavior, unrelated-resource deletion, plugin-parent lookup, font fallback, texture filtering, missing Aura texture provenance, GroundTrack state-index validation, and the final complete-scope audit.
