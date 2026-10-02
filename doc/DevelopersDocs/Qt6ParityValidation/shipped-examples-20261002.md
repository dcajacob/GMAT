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

| Standalone scope | Missions | Build passed | Build failed | Run completed | Run failed | Initial timeout |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `application/samples` | 138 | 110 | 28 | 109 | 0 | 1 |
| Help tutorial downloads | 19 | 10 | 9 | 9 | 1 | 0 |
| API and TLE sample missions | 7 | 7 | 0 | 2 | 5 | 0 |
| Total | 164 | 127 | 37 | 120 | 6 | 1 |

Every standalone candidate has an actual build attempt; every successful build
has an execution attempt. Failed builds prevent execution. There are no recorded
crashes in these stages. All 291 current child-stage records say their main
source remained unchanged during that attempt. Repaired tutorial sources have
new hashes and retain earlier failures in history.

The Yukon launch-window mission's initial 180-second attempt timed out while
its optimizer was progressing. A separate 7200-second attempt is running in
`/tmp/gmat-shipped-examples-yukon`; it has already completed its first window,
reporting launch epoch `27348.0637387` and cost `9.597325279040319`, and advanced
to the next optimization. It is **not counted as passed**. Resume the existing
process before starting another attempt. Merge only its terminal result into
the main ledger after confirming source identity.

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
  Report tutorials complete. Lunar L2 builds and reaches its guard described below.
  The lunar download link and matching report-heading XML instruction are fixed.

Separate checks are in `qt-capability-checks-20261002.txt`: MissionInsertion,
ScriptAssets, UnplottedBodyCameras, OpenFramesSyntax, CoordinateCreation,
OpenFramesVectors and StringArrayDispatch. The syntax check was rerun after
vector support changed its previous expected rejection; unrelated old matrices
and already passing example stages were not repeated. The vector test retains
a byte-identical independently executed six-component state report. This is
not a claim that the full regression suite passed.

## Remaining failures and required work

The 28 primary build failures require MarsGRAM (1), MATLAB/Fmincon (2), SNOPT (3),
VF13ad (13) or CSALT/OptimalControl (9). Eight tutorial failures additionally
require VF13ad (6) or MATLAB/Fmincon (2). MATLAB and SNOPT need proprietary
dependencies; MATLAB remains explicitly deferred. VF13ad and MarsGRAM are
missing optional installations/data, **not automatically accepted proprietary
exceptions**. CSALT is disabled and its selected build configuration requires
SNOPT. Original converter diagnostics also identified vector/compound-statement
limits in dependency-bearing examples; those full conversions/executions are
not established by the regular-example fixes. The manifest preserves exact
diagnostics, dependency provenance and initial failures.

`FormationRendezvousTutorial.script` is the wrong bundled content: it is a copy
of the old lunar transfer script, whereas the formation tutorial requires
Sat1–4 and its relative frames. Modernizing that duplicate cannot qualify a
formation rendezvous example. Restore the correct mission from the tutorial's
specified spacecraft, initial states, frames and sequence, then execute it.

Lunar L2's four target executions converge, then its explicit guard stops because
the unchanged BdotT guesses 10000 and 11000 produce energy errors
`+0.356311052242573` and `+0.2860377299423941`. The documented bisection therefore
has no initial sign-changing bracket. Completion requires revised numerical
bracketing inputs and verification against the tutorial's intended transfer;
do not remove the Stop or count an interrupted run as completed.

Five TLE examples require absent public catalogs, not another plugin:

| Example | Required catalog |
| --- | --- |
| FalconSat7Jupe, FalconSats | June 2020 `FALCONSAT-7` elements in `Active-2020-06-23.txt` |
| FalconSat7Contacts | `active.txt` elements appropriate to its actual current time |
| GSFCSats | `active.txt` for its 22 named satellites at 12 December 2019 |
| Starlink | Historical `active.txt` containing its 117 named satellites at 12 December 2019 |

Do not rename the bundled November 2019 catalog to satisfy these filenames.
The entries and epochs differ, and dynamic-time and historical examples require
different versions of `active.txt`. The plugin's `Scripting.rst` records exact
requirements; its GSFCSats epoch wording is corrected. Missing public input data
remains open, rather than being waived as proprietary.

The two SupportFiles fragments and five GMF helpers have inventoried parent
missions, including the repaired ephemeris comparison. This static relationship
does not prove every function branch executed. Four other GMF helpers and both
userinclude fragments remain without parent mission coverage; they are not
misclassified or run as standalone missions.

## Evidence and continuation

- `shipped-examples-20261002.manifest.json`: every source hash, dependency,
  current stage, history, exact child command, timing and provenance.
- `shipped-examples-20261002.results.tsv`: all 177 script/helper source rows.
- `shipped-examples-20261002.failures.txt`: current diagnostic excerpts.
- `shipped-tutorial-triage-20261002.md`: all 19 tutorial outcomes and repair map.
- The full completed raw evidence is retained locally under
  `build/example-qualification/20261002/raw`, including reports/screenshots/logs.
  Original commands/ledger paths point to `/tmp/gmat-shipped-examples`; the
  preserved copy has the same relative layout. This generated directory is
  ignored, rather than adding all raw screenshots/reports to Git.

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
