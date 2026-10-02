# Viewer data availability — 2026-10-02

An explicit presentation mask now prevents an unpublished spacecraft from being
plotted as a fabricated origin. The rebuilt application and the new focused
regression pass; two directly affected checks pass once and are reused. This
record preserves the original test-fixture failure separately from the viewer
defect. One affected private actual-input scene retry also passes below; its
image evidence is separate from the regression's retained-history checks.
Neither result qualifies the physical host desktop or GPU.

## Defect and narrow repair

The final [multi-thrust case](multi-thrust-live-20261002.md) exposed SatA at the
Earth origin after sequential SatB legs, while the independently reported SatA
state at 30 s was finite and nonzero. OrbitPlot kept all plotted spacecraft names
in each callback and supplied its legacy zero/previous-data placeholders for
spacecraft omitted from that publication's six state labels. Qt treated finite
zero as a valid state and appended it, moving the latest object pose and joining
false history. Zero is also a valid body or spacecraft-centered coordinate, so
rejecting all-zero positions would have broken legitimate views.

Committed repair **`08cdda3c561cc9e7eab8e67ed876b870e59e41b9`** adds the following
bounded contract:

- OrbitPlot derives an epoch and ordered spacecraft-name/presence mask from the
  publication's actual X/Y/Z/Vx/Vy/Vz labels. It sends the mask immediately before
  the corresponding real plot callback. The existing numerical arrays, legacy
  zero/previous-data policy and numerical commands remain unchanged.
- Qt consumes the mask once for a real callback and requires exact epoch and
  complete spacecraft-name agreement. Explicitly absent curves retain their
  previous real points and set `breakNext`; a resumed sample begins a new line.
  Camera and vector source/destination lookups also honor explicit absence.
- Current-iteration buffers retain one mask per sample. OrbitPlot and both
  production overrides, OrbitView and GroundTrackPlot, send the corresponding
  stored masks during replay and clear the new buffer/context afterward.
- Pre-decimation named-camera preparation receives the same publication mask.
  Its captured pose list may be a subset, with exact epoch matching. Metadata-only
  preparation reads the mask without consuming the later real callback's mask,
  preventing an absent-spacecraft fallback from being finalized as a camera.
- Malformed replacement metadata clears an older pending mask; stale or
  mismatched metadata is ignored. Data/object/solver and ground reset paths
  clear pending masks. Existing finite REAL_MAX/unrenderable filters and valid
  finite origin coordinates retain their existing meaning.
- The wx receiver rejects the new capability probe alongside its existing
  metadata probes. It receives its existing data callbacks/arrays unchanged.
  This one-line guard is source-reviewed only; the selected build has wx GUI
  disabled, so no wx build/runtime qualification is claimed.

Read-only review caught two required wiring gaps before build: derived Current
replay overrides and the pre-decimation named-camera preparation/finalize bypass.
Both are included above. No unrelated numerical algorithm or GUI notification
change was made. Existing core absent-data warnings remain; the repair changes
presentation metadata rather than suppressing those warnings.

## Build and focused check evidence

Full baseline, source snapshots, command receipts and stdout remain under
`build/example-qualification/20261002/viewer-availability-controls`.

| Recorded stage | Result and limit |
| --- | --- |
| Build 1, 23:29:09.570–23:29:48.491 UTC | Exit 0, about 39 s and 65 logged Ninja steps. Rebuilds GmatQt and the new/affected test targets. |
| First new `QtGui.OrbitDataAvailability` | **FAIL 0.63 s**, retained unchanged. Its test incorrectly used `QtPlotReceiver::changed` as a publication callback. That callback reports structural window events; `refresh()` does not emit it for data updates. The combined report/live-observation assertion therefore failed. |
| Directly affected `QtGui.InvalidPlotData` | **PASS 1.49 s**, once. Existing finite SPK unavailable-state omission, separated resumed paths and independent report/source checks remain covered. |
| Directly affected `QtGui.SolverPlots` | **PASS 4.07 s**, once. Existing DifferentialCorrector All/Current/None display-mode checks, reports and retained histories remain covered; other solver matrices were not selected. |
| Build 2, 23:38:58.622–23:39:02.295 UTC | Exit 0. Recompiles/links only the corrected new test; the routine startup copy-if-different also runs. App/core/util/startup byte identities are unchanged. |
| Corrected `QtGui.OrbitDataAvailability` | **PASS 0.31 s**, the only check selected in check 2. Stdout records exactly **three independent report rows**, **SatA 23 samples, providers ALeg/ALater**, and **SatB five samples, provider BLeg**. |

The fixture correction inspects the actual retained-history contract after the
same short sequential A/B/A mission: for every B publication frame, choose A's
latest real sample at/before that frame and compare it with the independently
typed post-A Report state. A fabricated same-frame zero fails that comparison.
The resumed A line must break, final A/B points must match their independently
reported states, and the separate spacecraft-centered view retains legitimate
origin samples. The source remains byte-exact.

The same focused executable exercises actual BufferOrbitData/Current replay
paths through OrbitView and GroundTrackPlot with independently supplied states,
then a subsequent buffer/replay to check mask cleanup. These synthetic replay
samples retain trial flags and do not claim solver convergence or accepted
trajectory behavior. Receiver assertions cover one-shot, stale/malformed and
data/object/solver/ground reset semantics. An instrumented absent camera reference
offers a known finite fallback and counts state queries; preparation/finalize
must perform zero absent-state queries, avoiding an uninitialized-getter false
positive. No old corpus, tutorial or successful resource-stage rerun was added.

## Source/runtime binding

The [commit binding](viewer-data-availability-commit-binding-20261002.json) matches
all nine changed source/test/CMake files byte-for-byte to commit `08cdda3c` and the
recorded build inputs. [Source 1](viewer-data-availability-source-1-20261002.json)
retains production plus the original 16,651-byte failed fixture (SHA
`4afb63a0474120f7313311d6e79d30d811213aed052787e2b6588ebbf4ff405d`).
[Source 2](viewer-data-availability-source-2-20261002.json) retains the corrected
17,179-byte fixture (SHA
`22c9496d5e8bdb8042d3ec276c3e097e56c25ea030484c9dace465aa4c7cbb71`);
production is unchanged between the two builds.

| Runtime file | Bytes | SHA-256 after Build 1 and unchanged after Build 2/check 2 |
| --- | ---: | --- |
| GmatQt-R2026a | 5,935,320 | `989d3499cfb85fe4d72703c9029823e1247e213fd26458b1c616d184d9530371` |
| libGmatBase.so.R2026a | 25,040,656 | `2082a341f5f123e81bad362972cd2b28a0a4753047ae51b5c09609e246be430c` |
| libGmatUtil.so.R2026a | 2,929,664 | `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc` |
| gmat_startup_qt.txt | 10,260 | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |

The [baseline](viewer-data-availability-baseline-20261002.json) retains the previous
compiled source `2f9810c0`, app `244dc41a…` / 5,926,536 bytes and core `cf147e…` /
25,036,288 bytes. Older native records keep those historical identities; the
new rebuild does not retroactively qualify them under the new runtime.

Useful unchanged receipts are [Build 1](viewer-data-availability-build-1-20261002.json),
[Build 2](viewer-data-availability-build-2-20261002.json),
[first checks](viewer-data-availability-checks-1-20261002.json) with
[complete first failure/affected passes](viewer-data-availability-checks-1-20261002.txt),
and [corrected check](viewer-data-availability-checks-2-20261002.json) with
[complete passing stdout](viewer-data-availability-checks-2-20261002.txt).
The full source duplicates/build logs stay in the owned ignored evidence tree;
they need not be duplicated as tracked documentation assets.

## One affected private actual-input scene retry

[The separate native retry record](multi-thrust-viewer-retry-20261002.md) retains
one effective F5 of the frozen independently authored 5,205-byte corrected thrust
mission, source SHA
`317e151e09253e9ef5fc30a57bfaf39b1765ec518170de291bfa1fd872a7417d`.
The owned raw tree is
`build/example-qualification/20261002/multi-thrust-viewer-retry/isolated-x11-uwu930qb`.
Launch provenance binds the same app `989d3499…`, core `2082a341…`, util and startup
recorded above, with a private X11/software GL session and cloned OUTPUT_PATH.

The effective run **Completed 0.335 s**. Actual completed **Latest** screenshot
005 shows both spacecraft at orbital endpoints, with Earth alone at its center;
actual **Output** screenshot 006 displays the report. This is private scene/image
observation, distinct from the regression's programmatic frame/point inspection.
The complete **2,502-byte** numerical report is byte-identical to the frozen
original, SHA-256
`c2a06348a5f9b7068255dae4dbc7d1c8396a008e35afe05591ddadbb45a989bd`.
The original fuel/epoch boundary checks also pass. Normal **File Exit** gives
application/WM/Xvfb **0/0/0**; helper CLI **1** is its already-exited-application
diagnostic, separate from application failure or deadline termination. Full
session actions, freeze/cleanup identities, report comparison and selected
unchanged images belong to the separate retry record and are not duplicated
here. The older defective scene and all earlier failures remain retained.

## Bounds

The focused regression provides no native pixel, wx-runtime or solver-convergence
claim; the separate affected retry provides one bounded private scene/image and
full-report comparison. These checks do not establish host GNOME/Wayland/physical
GPU, the desktop-crash cause, general numerical regimes or unlimited viewer
coverage. The new check covers collected publication absence and the named-camera
preparation contract. No additional old suite, corpus/tutorial or successful
resource-stage rerun was added. Windows/macOS/MATLAB and remaining full Linux
acceptance gates stay separate.
