# GroundTrack toolbar playback (LGUI-039)

The main animation toolbar was enabled for GroundTrack but searched only native OpenGL children. Play therefore did nothing and could remain toggled. GroundTrack now replays recorded publications through a wx timer, with functional Play, Stop, Faster and Slower controls. It preserves all stored curve data and restores the full track on completion or interruption.

The recorded frame contains each curve's point count, so spacecraft missing from a publication keep their identity and replay timing. Publication order is retained even when epochs move backward. Drawing uses normal paint events. No PluginWidget virtual methods, plugin binary interface, startup settings or synchronous sleep loop were added.

## Regressions

`GroundTrackAnimationRegression.cpp` verifies missing spacecraft, unequal curve lengths, backward epochs, clearing history, actual toolbar dispatch, asynchronous painting, rate changes, interruption, replay twice, natural completion, global Stop, closing during playback, recreation, and empty-history Play. After switching to an OrbitView/OpenFrames tab, Stop and speed commands still target the playing GroundTrack. That cross-tab test exposed and drove a separate routing correction before final validation.

Run `python3 src/UnitTests/TestLinuxGui/test_groundtrack_animation.py build/linux-gui`. Current and independent builds pass native 1x/2x/3x and OpenFrames coexistence tests. All seven native replay cases also pass. The existing GUI suite is recorded separately in `gmat-gt-animation-final-suite.log`; it covers layout, geometry, map loading, viewport, editor I/O, workflows, shutdown, plugin lifetime and OpenFrames integration.

## Review and prerequisites

- Integration: `34140f6dd515d49bacf21b2a280bdaf3a88283ea`.
- Standalone: `pr/groundtrack-animation` at `afe90a6789b4ffb13100a749fa5e87ff4e6a4b9d`.
- Standalone base: NASA `9363e129be366520c6edb0b4079204ed60666007`.
- Scope: GmatMainFrame.cpp and GroundTrackArea/Window .cpp/.hpp. Tests and audit material remain on integration.

Independent validation used `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906` (Linux startup/layout/map/viewport/plugin-lifetime prerequisites), plus `pr/groundtrack-state-mapping` at `a8d0e9a74126a630bd582df7f5b39df84f42ea31` and `pr/groundtrack-direct-close` at `9bcf367653045528c3d8c1a4b2bc0db82b941786`. The state patch is needed for missing/non-finite positions; the close patch fixes the independently reproduced close crash. Both are separate review units.

When combining with map safety, retain its empty-curve guard and safe final-point label placement while replacing the visible point count with `GetDisplayedPointCount(i)`. The standalone patch includes the equivalent prefix-aware loop/label adaptation for upstream's older loop. The independent application needed only this drawing-context resolution and the state patch's include-context resolution; `applied.patch` records the exact validated combination. The validation checkout and binary were restored afterward.

Linux wxGTK/X11, Xvfb and Mesa software rendering were tested. Windows/macOS, native Wayland and hardware-specific paths remain unvalidated. This patch does not implement OpenFrames main-toolbar dispatch or redesign the plugin interface; those remain under assessment. It does not change GroundTrack's existing NumPointsToRedraw behavior. External warnings remain visible.

No PR, issue or external comment was submitted. Old and Downloads reference sources remain unchanged. The overall goal remains active pending the remaining OpenFrames, font, texture and asset assessments and final requirement-by-requirement audit.
