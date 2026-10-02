# Named regular arcs and keyboard navigation — 2026-10-02

Three focused offscreen checks pass after rebuilding the actual application.
Named cameras now use regular trajectory identity and copied poses rather than
contiguous command-name runs. Resources, Mission and Output accept focused
Return/Enter through their existing guarded activation handlers. The bounded
private Wayland station case below adds actual compositor-delivered keyboard
evidence. Wider viewer, host-desktop and full replacement gates remain open.

## Runtime and preserved evidence

| File | SHA256 |
| --- | --- |
| `application/bin/GmatQt-R2026a` | `7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3` |
| `application/bin/libGmatBase.so.R2026a` | `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` |
| Selected `gmat_startup_qt.txt` | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |

The ignored durable evidence root is
[`build/example-qualification/20261002/segment-arc`](../../../build/example-qualification/20261002/segment-arc/).
Its `final-passed` directory retains exact authored/reference scripts and complete
reports for all three new arc cases, plus the checks and detailed camera output.
The combined check log contains the SegmentArc pass **and** initial TreeActivation/
SegmentCameras failures; it is not an all-green run. Final focused dispositions:

| Check | Result and raw evidence |
| --- | --- |
| `QtGui.SegmentArc` | 0.61 s; [combined log](../../../build/example-qualification/20261002/segment-arc/final-passed/gmat-arc-tree-camera-checks-20261002.txt) |
| `QtGui.TreeActivation` | 0.59 s; [focused retry](../../../build/example-qualification/20261002/segment-arc/final-passed/gmat-tree-publication-check-20261002.txt) |
| `QtGui.SegmentCameras` | 0.54 s; [focused retry](../../../build/example-qualification/20261002/segment-arc/final-passed/gmat-camera-boundary-fixture-check-20261002.txt), [detailed output](../../../build/example-qualification/20261002/segment-arc/final-passed/LastTest.log) |

No prior successful solver, shipped-example or full Qt matrix was repeated.

## Arc identity, pose and ownership

[OrbitPlot metadata](../../../src/base/subscriber/OrbitPlot.cpp) observes each raw
publisher sample before DataCollectFrequency, using the existing TakeGlAction
path. Extra position/attitude conversion is limited to requested segment-camera
spacecraft. Provider identity, effective trial/accepted state and actual flush
boundaries feed [regular arc records](../../../src/qtgui/SegmentArc.hpp) and
[receiver camera snapshots](../../../src/qtgui/QtPlotReceiver.cpp). Same-provider
continuation and connected same-direction A→B→A joining retain the initial name;
selection uses the first regular arc with that name. Duplicate/nonmonotonic
epochs do not extend an arc. Trials are separate from regular arc ownership.

These rules follow local R2026a OpenFrames `TrajectoryDealer.cpp` AddState
(lines 155–223 and 228–252) and `OFSpaceObject.cpp` TrajectorySegment (614–623),
under `/home/dan/Downloads/GMAT/GMAT-src_and_data-R2026a/GMAT-R2026a/depends/OpenFramesInterface/`.
Start/latest position and attitude are copied while engine objects are live;
finalized endpoints survive decimation, point trimming and viewer deletion.
A review caught accepted Current samples being displayed immediately and again
through replay in the new, unshipped implementation. Only trial samples now use
that replay path when metadata is enabled; the new check observes publications
and rejects duplicate accepted samples. The test callback's owned capture also
avoids retaining dead stack state. These are pre-release fixes, not demonstrated
regressions in a previously shipped binary.

[wx query handling](../../../src/gui/app/GuiPlotReceiver.cpp) returns false for
the new metadata capability queries. Empty-context legacy collection/replay and
its previous solver flag behavior remain unchanged. No receiver virtual ABI or
numerical integration/optimization algorithm changed.

[PlotModel](../../../src/qtgui/PlotModel.hpp) keeps only compact start/latest
records per disconnected regular arc, independent of MaxPlotPoints. Point and
camera histories remain bounded by MaxPlotPoints. Arc records have no separate
count cap: repeated disconnected loops can grow this metadata until the model/
data reset. This retention is necessary for first-named-arc selection after trim;
no unbounded-run memory qualification is claimed.

[SegmentArcTests](../../../src/qtgui/tests/SegmentArcTests.cpp) adds exactly three
cases, each with byte-identical complete reports from an independent camera-free
mission and unchanged authored source:

1. The same A command runs twice with a real state reset. Distinct arc IDs retain
   the same provider membership; the camera freezes the first arc's endpoint.
2. Connected same-direction A/B/A/B execution joins into the original A arc,
   unions both providers and follows its independently reported latest endpoint.
3. DifferentialCorrector Current trials do not acquire the first regular camera.
   The accepted 37-second endpoint survives DataCollectFrequency=7, MaxPlotPoints=3,
   removal of every collected A point, trial cleanup and actual plot deletion/
   Output reopen. Position/epoch match the independent report; copied DCM matches
   an independent endpoint engine attitude query. Camera/point limits and exact
   Unicode Save remain checked.

## Fixture corrections and retained failures

The [first compile failure](../../../build/example-qualification/20261002/segment-arc/first-compile-failed/build.txt)
was a const StringArray passed to FindIndexOfElement's mutable-reference API;
using the existing mutable label array corrected it. The original source snapshot
is retained. TreeActivation's initial zero-delay timer ran during Build or after
its short mission, so it did not prove a running guard. Its
[preserved failure](../../../build/example-qualification/20261002/segment-arc/tree-timer-fixture-failed/check.txt)
is replaced by a synchronous real-publication observation, without lengthening
the mission.

The old SegmentCameras fixture assumed two command-summary names implied two
regular arcs. Its 60-second stop landed exactly on a 10-second step.
[Propagate::TakeFinalStep](../../../src/base/command/Propagate.cpp) places both the
final publication and FlushBuffers(false) inside `secsToStep != 0.0`; no flush
occurred at that stop. Diagnostics showed one FirstArc, with one provider member,
continuing to the 180-second endpoint. This is absence of a boundary, not A/B
joining. OF likewise opens a new regular arc only after finalization.

[SegmentCameraTests](../../../src/qtgui/tests/SegmentCameraTests.cpp) now uses
61+119 seconds, preserving the total 180-second duration while establishing a
real flush boundary. It requires two finalized provider-distinct named arcs and
checks each endpoint/A1 epoch/transposed DCM against the independent camera-free
17-column report. Original camera tolerances, body/inertial/LookAt/automatic
views, pixels, replay, close/reopen, source/Undo and failed-edit recovery remain.
[Original failures and diagnostic source](../../../build/example-qualification/20261002/segment-arc/first-regression-failed/)
are preserved; no production change was needed for this fixture correction.

## Keyboard scope and actual private Wayland evidence

[NavigationTree](../../../src/qtgui/MainWindow.cpp) consumes plain Return/Enter
(including keypad Enter) and dispatches once through the existing double-click
route. Mouse signals/direct callers remain available; held-key repeats do not
open extra forms. [TreeActivationTests](../../../src/qtgui/tests/TreeActivationTests.cpp)
uses focused key events for one resource/command/report form and an actual retained
plot reopen, preserves pending station fields/source/report bytes, and verifies
category no-op plus real-running and unbuilt-source guards.

The later owned private Wayland session `isolated-wayland-p8bnkx9c` launched the
application/core hashes above. Its raw actions record a Resources station click
then Return at 229.784 s, followed by screenshot013 at 229.939 s showing the
GroundStation1 settings editor (minimum elevation 21.5). The unchanged capture is
[station Return](navigation-station-return-20261002.png); full launch/action records
remain under
[`wayland/portal-first-attempt/isolated-wayland-p8bnkx9c`](../../../build/example-qualification/20261002/wayland/portal-first-attempt/isolated-wayland-p8bnkx9c/).
This confirms actual station keyboard activation only. Output Enter reopening is
proved offscreen here, not by that native session. Its separate portal protocol
failures and excluded Open/Save cases retain their own dispositions.

Host GNOME/Wayland/hardware and wider lifecycle/solver regimes are not qualified
by these cases; the earlier host crash is not diagnosed or proved fixed. Tutorial1
construction is now in progress under separate evidence, with no tutorial pass
claimed here. Full Linux replacement remains unfinished; Windows/macOS/MATLAB
remain deferred. This documentation step ran no further GUI or test.
