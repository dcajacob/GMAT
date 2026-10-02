# Mars gravity default filename resolution — 2026-10-02

Actual Tutorial 4 GUI creation of NearMars succeeded, but its force-model Apply
failed with `The file name "MARS50C.cof" does not exist`. The official local input
is `application/data/gravity/mars/Mars50c.cof`; the selected startup already maps
MARS50C_FILE correctly. The Qt gravity contributor helper treated the default
model alias as a literal filename, losing that case-sensitive startup resolution.

Newly added default primary gravity fields now resolve their model aliases with
Moderator::GetPotentialFileName and the existing interpreter's default-file
semantics. Retained user-selected gravity files and unrelated forces stay intact.
No startup, data file or numerical-engine algorithm changed.

GmatQt and the two relevant targets rebuilt successfully. QtGui.MarsGravity
passes in 0.21 s: independent interpreter-default Mars input, actual retained
editor invalid selection/recovery/Apply, canonical official Mars50c path and
4/4 degree/order, retained custom potential, unchanged unrelated source/comments/
mission, exact Undo/Redo and Unicode save/reopen. It performs no numerical mission.
The directly affected existing QtGui.GravityBodies passes in 0.97 s. Other already
passing suites/corpus and the preceding four resource checks were not repeated.

Actual Tutorial 4's failed Apply/settings/screenshots and owned saved resource/view
milestone remain preserved in `isolated-x11-i004njx9`; it resumes on this rebuilt
runtime for the failed settings and remaining targeting steps. Those steps are
not counted as completed by this focused check.

Rebuilt application/bin/GmatQt-R2026a (GmatQt symlink target) SHA256:
`3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1`.
Base remains `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`;
selected startup remains `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
Build/check logs, full stdout, input/source/runtime hashes and artifact index are
in ignored `build/example-qualification/20261002/mars-gravity-default`.

This is Qt default-file resolution and bounded transaction evidence. Full Linux
replacement, host GNOME/physical-driver and other tutorial gates remain open.
Windows/macOS/MATLAB remain deferred.
