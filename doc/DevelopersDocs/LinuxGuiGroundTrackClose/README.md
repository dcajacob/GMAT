# GroundTrack direct close (LGUI-038)

Closing a GroundTrack window could crash after its base close handler had already scheduled destruction. The derived handler then called `event.Skip()`, allowing another close handler to process the same event. The focused fix removes that second dispatch; normal base-handler veto and destruction behavior remain intact.

## Evidence and reproduction

`GroundTrackCloseRegression.cpp` runs a mission, calls the real plot window's `Close()`, waits for destruction, reruns the mission to recreate the plot, and closes it again. The unchanged prerequisite baseline segfaulted on the first close. The production patch passes both native and OpenFrames coexistence cases in the current and independent builds.

Run `python3 src/UnitTests/TestLinuxGui/test_groundtrack_close.py build/linux-gui`. Tests remain on integration. The independent checkout was restored and rebuilt afterward.

## Patch and prerequisites

- Integration: `f327cf93c381982979aaec1070869f9e592e0cc7`.
- Standalone: `pr/groundtrack-direct-close` at `9bcf367653045528c3d8c1a4b2bc0db82b941786`.
- Standalone base: NASA `9363e129be366520c6edb0b4079204ed60666007`.
- Independent validation base: `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`, containing existing Linux startup, layout, map, viewport and plugin-lifetime prerequisites.
- Scope: GroundTrackWindow.cpp only. Playback support is a separate patch.

Evidence covers Linux wxGTK/X11 under Xvfb with software rendering. Windows/macOS, native Wayland and hardware-specific behavior remain unvalidated. No PR, issue or external comment was submitted; Old and the Downloads reference sources were not changed.
