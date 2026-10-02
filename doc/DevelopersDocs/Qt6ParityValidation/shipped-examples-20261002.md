# Shipped example qualification — 2026-10-02

Work is in `/home/dan/GIT/GMAT-Qt` on `codex/qt6-gui`. All executions used
isolated offscreen Qt children and the actual `application/bin/GmatQt` executable.
This is build/execution qualification, not scientific validation of every example
or native desktop/rendering qualification. Live desktop tests remain stopped
after the GNOME Shell crash; Windows/macOS and MATLAB remain deferred.

## Inventory and current outcomes

The complete inventory has 168 `.script` files: 164 standalone mission candidates
and four include fragments. Nine `.gmf` helpers are also hashed and their explicit
parent references are traced. Plugin developer tests, the CInterface MATLAB
configuration, Python/MATLAB API clients, notebooks and generators are outside
this Qt mission-script corpus.

| Standalone scope | Missions | Build passed | Build failed | Run completed | Unexpected run failures | Expected tutorial Stop | Timed out |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `application/samples` | 138 | 123 | 15 | 119 | 0 | 1 | 3 |
| Help tutorial downloads | 19 | 17 | 2 | 15 | 1 | 1 | 0 |
| API and TLE sample missions | 7 | 7 | 0 | 3 | 4 | 0 | 0 |
| Total | 164 | 147 | 17 | 137 | 5 | 2 | 3 |

Every standalone candidate has an actual build attempt; every successful build
has an execution attempt. Failed builds prevent execution. There are no recorded
crashes in these stages. All 311 current child-stage records say their main
source remained unchanged during that attempt. Repaired tutorial sources have
new hashes and retain earlier failures in history. The two expected Stop rows
retain raw exit 1/failed status plus an explicit tutorial-checkpoint assessment;
raw counts are 137 passes, seven failures and three timeouts. They are not counted
as completed missions or optimizer convergence.

The Yukon launch-window mission's existing 7200-second attempt reached its
bound after **11 of 20 windows** completed. The terminal stage is timeout/exit
-15, duration 7200.026 seconds, with unchanged source SHA256. Its complete raw
results are retained in `/tmp/gmat-shipped-examples-yukon` and the durable ignored
`build/example-qualification/20261002/yukon` copy. Completed windows range from
launch epoch 27348.0637387/cost 9.597325279040319 through
27368.0637387/cost 7.951719590661567. The next window was still optimizing.
This is incomplete, not a passed full mission or proof of a hang. Its prior
180-second attempt remains in history; no unchanged third attempt is scheduled.
Only this terminal result/history was merged into the main ledger after matching
source identity. The running process used the earlier recorded frontend/runner
snapshot; later plugin-snapshot additions and GUI rebuilds do not retroactively
qualify that provenance.

The newly enabled VF13ad MarsLaunchWindowAnalysis and MarsPatchConic runs each
reached their 300-second bound. The first completed one window (epoch 27348.0637387,
cost 9.597324034564597); the latter was still iterating. These are incomplete,
not evidence of a hang or an algorithm defect. No unchanged rerun is scheduled.

## Fixes established by the failing examples and focused checks

- GMAT working-directory paths now retain the required trailing separator in
  ordinary documents and folder runs. This fixes both bundled ephemeris
  comparisons while retaining nested includes, original-source asset lookup
  for saved copies, exact endpoint reports and pending source/Undo state.
- OpenFrames grouped declarations and camera list reset/scalar append semantics
  now preserve empty-entry/dedup behavior. `Ex_GEOTransfer` builds and runs.
- Automatic cameras can follow real undrawn SpacePoints and retain physical
  body bounds. `Ex_GivenEpochGoToTheMoon` builds and runs; drawing-flag membership
  remains strict, and replay uses retained positions/radii.
- Relative Position and Body-Fixed OpenFrames vectors now retain source/target,
  pose, color, label and Auto/Manual length in bounded histories. Native and
  fallback rendering share value-only arrow geometry; replay does not consult
  discarded engine objects. Quaternion propagation, Electric Propulsion and
  Extra Shadow Bodies now build and execute with required vector metadata.
  Thrust vectors, dynamic vector settings and unknown fields still reject with
  original source preserved. Native vector pixels remain unverified.
- `Ex_ExternalForceModel` now works with the built public Python API. The local
  preset enables the Python 3.14 API modules, the development package initializer
  is generated without installing GMAT, and the callback locates its API paths
  relative to its own file. Java/MATLAB stay off; force equations are unchanged.
- Extra Shadow Bodies exposed a regression introduced on this Qt branch:
  string-array assignment unnecessarily called the parent's text getter for an
  encoded owned-force parameter id. The interpreter now requests that name
  only for its class-specific empty-list cases. SRP physics and unrelated
  ODEModel APIs are unchanged. The focused check preserves SPICE list clearing
  and implicit/default/nonempty/explicit-empty solar-shadow distinctions.
- Coordinate drafts now have usable default axes; nested Axes OK retains pending
  settings until outer Create/Apply. Cancel, mixed Origin/dependent settings,
  invalid correction and exact source/Undo/Unicode reopening pass. The first
  fixture wrongly expected newly inserted assignments to leave the entire
  baseline contiguous; its source comparison was corrected, not relaxed.
- Mission context menus can append inside If/For/While/Target/Optimize branches,
  and the command chooser offers typed Toggle creation. Focused insertion,
  validation, retained Apply and exact Undo/Redo checks pass.
- Four legacy tutorial sources have obsolete coordinate statements removed and
  explicit mission boundaries added. Their numeric initial conditions,
  propagation/targeting equations and command order are retained. The report
  heading is quoted without changing its text. Lunar Transfer, Mars B-plane and
  Report tutorials complete. The first L2 run reached its bracket guard; the
  measured replacement interval now exposes the separate event-domain failure
  described below. The lunar download and report-heading XML links are fixed.

- The available R2026a Ubuntu VF13ad binary is installed locally, with its
  distributor README/license retained. Its optional startup entry now survives
  CMake rebuilds. All 19 formerly blocked VF13ad example/tutorial variants build;
  15 finish, two reach the chapter-required Step 1 Stop, and two long missions
  reach their time bounds. No optimizer substitution was used. See
  `vf13ad-external-component-20261002.md` and `MultipleShootingStep1IntentionalStop.md`.
- FormationRendezvous now contains the documented four-spacecraft demonstration,
  with missing Sat2–4 states explicitly reconstructed and disclosed. Both targets
  converge and each constrained residual meets its documented tolerance. Separate
  position/velocity endpoints are not simultaneous docking. Original historical
  states/plot shapes remain unrecovered; see `FormationRendezvousRecovery.md`.
- Trailing empty OF terminators are parsed without changing original lines,
  comments or mission text. IntegratedFlyby static conversion now succeeds; its
  absent CSALT/SNOPT dependency still prevents numerical execution.
- Resources now offer Clone through one optional-name editor. Pending Cancel,
  correction, atomic commit/Undo/Redo and source retention cover station, scalar,
  string, array, frame and viewer copies, including managed array formulas and
  Qt camera metadata. Protected built-ins/bodies/PropSetup remain excluded as in wx.

Separate checks are in `qt-capability-checks-20261002.txt`: MissionInsertion,
ScriptAssets, UnplottedBodyCameras, OpenFramesSyntax, CoordinateCreation,
OpenFramesVectors and StringArrayDispatch. The syntax check was rerun after
vector support changed its previous expected rejection; unrelated old matrices
and already passing example stages were not repeated. The vector test retains
a byte-identical independently executed six-component state report. This is
not a claim that the full regression suite passed.

## Remaining failures and required work

The 17 current build failures require MarsGRAM (1), deferred MATLAB/Fmincon (4),
SNOPT (3) or CSALT/OptimalControl (9). SNOPT and MATLAB are proprietary; CSALT's
selected configuration also requires SNOPT. Missing legacy MarsGRAM plugin/data
remains an external input/component requirement, not a proprietary waiver. The
available free VF13ad binary removed all 19 of its build blockers; its actual
per-example execution and provenance remain recorded separately. New runner
stages also snapshot every configured Linux plugin file without claiming loader
or ABI correctness merely from its presence.

Lunar L2's original 10000/11000 B·T guesses had same-sign energy errors. A measured
16000 km endpoint has negative error, establishing a valid 10000/16000 bracket
without changing DesiredEnergy or removing the guard. The actual repaired run
then failed after 79.261 seconds at an intermediate 15109.375 km trial: its first
L2-entry propagation has only a geometric event; that trial misses the entry
box, continues for decades and exhausts DE405 coverage at epoch 95008.539427940.
One bounded diagnostic identifies a bound Earth-return trajectory rather than
an L2 entry. Changing the stop would change evaluated energy/branch semantics
and would not establish transfer success. Final targeting never starts. This
remains a numerical transfer/event-domain failure, not missing proprietary data,
and no further unchanged retry or exhaustive search was performed. See
`l2-tutorial-bracket-2026-10-02.md` for exact phase reports and limits.

The exact missing June 2020 catalog was recovered from the original Thinking
Systems R2020 distribution. Only its two newly unblocked runtime stages were
retried, using byte-identical staged sources and isolated historical input data:

| Example | Current outcome or remaining input |
| --- | --- |
| Falconsat7Jupe | Completed in 1.172 s at its unchanged fixed June 2020 epoch. |
| FalconSats | Both catalogs resolved, then the unchanged current-time mission failed in 0.622 s with `SPICE(BADMECCENTRICITY)`. It remains an unexpected failure. |
| Falconsat7Contacts | Current-time scenario for FalconSat-7, which CelesTrak records as decayed on 2021-07-02. Appropriate current-orbit elements cannot be supplied. |
| GSFCSats | Matching historical `active.txt` for 22 named satellites at 12 December 2019 remains outstanding. |
| Starlink | Matching historical `active.txt` for 117 named satellites at 12 December 2019 remains outstanding. |

FalconSats uses `SystemTime(now)` with historical elements; the failure is
consistent with stale-element/current-time extrapolation, while the diagnostic
does not identify which spacecraft failed. No current-tracking accuracy,
scientific success, expected-Stop or proprietary waiver is inferred. Public
historical inputs remain open; CelesTrak's name/email/CAPTCHA request route is
recorded, with all requested NORAD identifiers prepared. No request was submitted.
Do not rename the November/June catalogs as December `active.txt` substitutes.
This targeted check repeated no other TLE/build/corpus stage or unchanged run. See
[tle-dependency-inputs-20261002.md](tle-dependency-inputs-20261002.md) for exact
source/input hashes, original failures/history, runtime artifacts and limits.

The two SupportFiles fragments and five GMF helpers have inventoried parent
missions, including the repaired ephemeris comparison. This static relationship
does not prove every function branch executed. Four other GMF helpers and both
userinclude fragments remain without parent mission coverage; they are not
misclassified or run as standalone missions.

## Evidence and continuation

- `shipped-examples-20261002.manifest.json`: every source hash, dependency,
  current stage, history, exact child command, timing and provenance.
- `shipped-examples-20261002.results.tsv`: all 177 script/helper source rows,
  with raw statuses and separate expected-stop dispositions.
- `shipped-examples-20261002.failures.txt`: current diagnostic excerpts.
- `shipped-examples-20261002.failures.md`: source-matched current failed/timeout
  stages and their explicit qualification dispositions.
- `tle-dependency-inputs-20261002.md`: exact recovered catalogs and only two
  targeted runtime retries, plus remaining public/legacy input availability.
- `shipped-tutorial-triage-20261002.md`: all 19 tutorial outcomes and repair map.
- The full completed raw evidence is retained locally under
  `build/example-qualification/20261002/raw`, including reports/screenshots/logs.
  Original commands/ledger paths point to `/tmp/gmat-shipped-examples`; the
  preserved copy has the same relative layout. Formation, VF13 Help variants,
  the initial plugin probe and bounded L2 diagnostics are preserved in separate
  neighboring trees. These generated directories are ignored.

The runner `src/qtgui/tests/QualifyExamples.py` skips completed stages by default.
Use explicit `--only` and `--retry-failed` after a concrete repair. It provisions
literal contained ReportFile/EphemerisFile output subdirectories inside each
isolated output root without changing script filenames; it rejects parent,
absolute, drive, expression and escaping paths. New stages record this setup
and the two linked core library hashes in addition to frontend/API/input hashes.
Early records predate some provenance additions; their named binary snapshots
and HEAD/diff context are not proof of an exact build source or loader state.
Neither the runner nor green execution statuses establish scientific results,
native rendering, portals or the full replacement acceptance gates.

## Latest affected UI checks

The actual GmatQt frontend was rebuilt after Clone and parser changes. The new
Clone test passes 0.47 seconds; OpenFramesSyntax passes 0.14 and
OpenFramesVectors passes 0.46. Two initial Clone harness issues were corrected
(the TestSettings constructor and an invalid pair of same-line assignments),
with engine diagnostics now printed on a failed baseline build. The initial
compile/check logs and final focused logs are retained; no passing conversion
checks or old full suite were repeated after fixture-only edits. Independent
review also caught/fixed mission-boundary preference and managed Array-block
copying before the final Clone check. This is offscreen workflow/report evidence,
not native popup safety or a full replacement acceptance pass.

## Targeted TLE input recovery reconciliation — 2026-10-02

The source-matched manifest now includes the Jupe completion and FalconSats
runtime failure after exact input recovery. API/TLE completion rises from two
to three; corpus completion rises from 136 to 137. Raw failed executions fall
from eight to seven, including the same two expected tutorial Stops. Build
counts (147 passed / 17 failed), all three timeouts and helper/fragment coverage
remain unchanged. The 177-row table and current failure excerpts reflect this
checkpoint; earlier missing-file attempts remain in manifest history. No new
mission, build, GUI session or test ran while deriving these summaries.
