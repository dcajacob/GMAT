# Help tutorial 5 — Optimal Lunar Flyby using Multiple Shooting

**Passed for bounded private actual-input qualification.** All five Help stages were independently authored
in the actual Qt GUI script editor, executed and inspected. The final owned scenario was saved,
reopened/built and frozen before reference access. Post-freeze comparison preserved an original sample
failure, then verified a narrow starting-guess repair with one actual updated-sample run. This chapter
explicitly teaches code authoring; this pass does not establish that all equations and branches can be
constructed using separate resource forms.

## Independent construction and scope

The instructions are `doc/help/src/Tut_OptimalLunarFlyby.xml`, SHA256
`4e33d79b320d7e0a9a8db3b32115c0173981a7eb14aa0e07361683de368fcf07`. Actual Welcome → Tutorials → lunar-flyby
Help → blank New Script preceded entry. All launch records have `script:null`. Resource/mission blocks and
later edits were typed through actual GUI keyboard input, then saved/built/run; continuation sessions opened
only owned milestones until the recorded freeze. Previous corpus exposure is distinct; no Step 1–5 reference
content seeded this construction.

The chapter requires VF13ad and presupposes Tutorials 1–2, 4 and Fundamentals training. The selected
optional VF13ad runtime performed these solves; no MATLAB was used. Its availability on other installations
is not established. Resources include MoonMJ2000Eq, five spacecraft, two force models/propagators,
Local/Earth/VNB MOI, NLPOpt, EarthView, four XY error plots and debugData. Retained settings include Earth
primary gravity 8/8, Luna/Earth/Sun point masses for the Moon model, PrinceDormand78 accuracy 1e-11, VF13ad
Tolerance 1e-4, FeasibilityTolerance 0.1, MaximumIterations 200 and forward numerical differences. Full
settings, guesses, command order, eighteen equalities and twenty-two Vary controls are retained in
source/action records and the post-freeze comparison JSON.

## Milestones and required observations

| Stage | Actual GUI action and outcome | Bounded interpretation |
|---|---|---|
| 1 — Verify configuration | Active Help Stop; build/run **0.647 s**, then fit/visibility controls and mouse rotation inspect initial segments | Expected intentional Stop; configuration milestone, not optimizer convergence |
| 2 — Smooth trajectory | Comment Stop and set EarthView.ShowPlot=false; twelve patch equalities, six mission equalities/objective still commented; **15.528 s, five nominal passes** | Patch continuity; actual copied Message Window values saved separately |
| 3 — Optimal trajectory | Enable six mission equalities and Minimize, including full continued inclination expression; **36.514 s, eleven passes** | Full published optimization milestone |
| 4 — New initial guess | Append printed Help alternative below originals; preserve both blocks and original MOI guess; **36.066 s, eleven passes** | Alternate-guess milestone; all eighteen equalities active |
| 5 — First exercise attempt | Add initial-control-state Luna.RadPer>=5000 and a radius report; final report **failed line search in 43 iterations**, last nominal pass 44 | Failed attempt retained; no successful GUI Stop/completion captured for this attempt |
| 5 — Own continuation | Manually append all twenty-two own nominal stage 4 controls; same model/settings/constraint; **11.291 s, four passes** | Converged, lunar periapsis **5000.202164763518 km≥5000 km**; actual own reopen/build precedes freeze |

Stage 2 nominal patch position RSS values are **0.000132651361256 km** and **0.000346054943275 km**;
velocity RSS values are **6.856446850784e-10 km/s** and **1.024101103586e-9 km/s**. Its inactive
final-radius goal remains unmet; this milestone is not a full mission solution. Stage 3 nominal MOI is
**−0.0919589453073 km/s**; stage 4 is **−0.0911640161905 km/s**. Their full nominal component residuals are
preserved in JSON/raw reports, rather than treating plot extrema or illustrative Help burns as exact
acceptance targets.

The successful own exercise nominal MOI is **−0.0909803139584 km/s**. All eighteen equality residuals are
retained; the largest absolute numerical component is **0.000231396785239 km**, patch 1 X. Velocity and
angle components have other units, so this is not a combined physical norm. Inequality variance
**−0.202164763518 km** corresponds to radius **5000.202164763518 km**, satisfying the unchanged lower bound.
XY plots include perturbations and use Help's unqualified difference formulas; active patch constraints
explicitly compare EarthMJ2000Eq components. Plotted extrema are not nominal equality residuals.

## Own reopen and freeze before reference access

Actual Ctrl+O reopened the saved own scenario and Build succeeded; screenshot readback retains the
spacecraft-copy assignments and the 5000-km exercise before propagation. The independently authored [final
source](help-tutorial-05-authored-20261002.script) is **18,701 bytes**, SHA256
`db65522abd06f9f4be8a52fb7c1b835db9e937751ab06887871595f9aa28f86c`. It was frozen at **2026-10-02
19:20:39.894341 UTC**, with outputs, before any newly authorized reference inspection. Its whole optimizer
report is **9850 bytes**, SHA256 `cc37f476f44d804abf2050adfe217c76be5703be7716eea298b944d368b7d5ec`. Frozen
report/log/debugData identities were checked; later reference output did not replace the frozen own
evidence.

The warm start uses exact printed **last Nominal Pass** stage 4 values, excluding the trailing MOI
perturbation. This additional starting point was justified by the failed exercise; it was not taken from a
reference, injected through an API, or obtained by changing solver/physics/constraint settings. Source
comparison confirmed the earlier guess blocks and complete constants/mission suffix remain intact when the
additional twenty-two controls are appended.

## Original reference failure and narrow repair

Only after freezing the own result, actual Ctrl+O opened original sample Step 5, Convert views changed the
UI buffer, and F5 ran it. Source conversion was not saved. The **162.538 s** run finished with failed solver
status: GUI **44 passes**, report **43 iterations**, line-search attempt limit, lunar radius **4860.44617331
km**, violating 5000 by **139.55382669 km**, and large equality residuals. “Mission completed” is not an
accepted optimization result. Original sample SHA256
`134f95e7c1e8a7fbd8aae47a338457b42676cf97806dc1c17dca07482939a409` was unchanged through that run and
archived before the authorized repair.

Its entire **90780-byte** report equals the owned first failed exercise's 90736-byte report after **only 44
printed inequality owner labels** change from `satFlyBy_Backward.Luna.RadPer` to
`satFlyBy_Forward.Luna.RadPer`. Both constraints are evaluated after backward=forward whole-spacecraft copy,
before either propagates, at the same epoch/state. Raw files are not byte-identical and were not edited.
This reproduces all printed controls, residuals, costs and failure messages of that local solve path; it
does not prove every numerical regime.

The reviewed repair appends the **twenty-two own nominal stage 4 controls** before Initial Guess Values'
EndScript in all three shipped Step 5 copies, preserving the earlier illustrative guesses. The original
files were copied with verified hashes to ignored postfreeze-reference-originals before mutation. All force,
propagator, Vary, propagation, eighteen equality, objective, tolerance and scaling settings remain
byte-exact in the source suffix. The lower bound remains 5000. The updated sample is **20773 bytes**, SHA256
`791ffd560ae1e9c88ad7278fcc5efcbf2736693e31a7ed1768835cb475bc0e56`. Runtime/source Help Step 5 copies are
byte-identical, SHA256 `508e7ec1c6374b065e60aa11e08319ca868e6d0dcc20296dcfb1f40cbb323521`. They are
synchronized inputs, **not separately executed cases**.

One actual updated-sample Welcome/Ctrl+O/Convert views/F5/readback run then converged in **10.977 s, four
nominal passes** (report: three iterations), with radius **5000.202164763518 km**, nominal MOI
**−0.0909803139584 km/s**, and the same eighteen equality residuals as the frozen own solve. The
**9854-byte** report, SHA256 `72531ef34b947aa49bb04ceb826a626aa7143d8bb4eef36a6ccd969b5238a588`, equals the
complete frozen own 9850-byte report after **only four printed backward→forward Luna.RadPer labels** are
normalized. No reference conversion Save occurred; updated sample source hash stayed unchanged. No Steps 1–4
rerun or numerical engine/tolerance rewrite was used to verify this input repair.

## Source-family and reporting limits

Post-freeze comparison covers all fifteen original Step 1–5 paths and retains hashes/line-anchored
differences in JSON. Runtime Help and source Help originals are byte-identical per stage. Original **sample
Step 3 differs physically**: Earth is included in PointMasses and Earth PrimaryBodies/8/8 assignments are
commented out; it is preserved, not treated as the authored Help gravity benchmark. Own stage 4's fixed
alternate matches sample Step 4; Help Step 4 instead uses different effective epoch/state guesses. Original
Help Step 5 also uses a different alternate, including MOI. Earlier overwritten guesses are distinguished
from effective inputs. Help's `sat*.Epoch.TAIModJulian` qualification differs from own/sample
`sat*.TAIModJulian`; runtime alias equivalence remains untested here.

Sample viewers use OpenFramesInterface/OpenFramesView and seven DrawObject flags; own/Help use legacy
OrbitView and five flags. Actual sample runs use explicit Convert views. Own retained legacy
OrbitColor/TargetColor warnings do not prove those colors were applied. Actual Moon force readback has empty
PrimaryBodies and Luna/Earth/Sun point masses; no silent physics correction was made. Own
`conLunarPeriapsis=8000` remains unused by its explicit **>=5000** exercise; original/repaired references
use the named constant set to **5000**.

The terminal report MOI retains **+0.0001 perturbation**: own/updated final nominal **−0.0909803139584**
versus terminal **−0.0908803139584** km/s. GUI nominal-pass counts exceed report iteration counts by one
throughout the converged stages. Use nominal passes/actual Message Window, not the trailing value, for
controls. Normalized whole-report equality establishes this report only: no claim of every propagated
sample, pixel, visual effect, global optimum or independent scientific accuracy. The bounded pass covers the
Help-taught editor workflow and named observations; remaining full replacement/tutorial gates stay open.

## Runtime and retained artifacts

The first three own sessions used app **7b4a0ae7…**; final own/original-reference session used
**7fb5bd6c…**; updated-reference session used **3ba673f1…**. Full hashes, selected startup **5f80be1f…**,
OUTPUT_PATH-only startup-clone identities, action traces and output hashes are in retained
session/comparison records. Final freeze records controlled base **cf147e23…**; the X11 launch harness does
not independently hash the loaded core per launch, and no later build identity is assigned backwards.

All sessions use owned Xvfb/Openbox, xcb/software GL, actual input and 1600×1200 screens. Earlier/final-own
sessions closed at their 600-second helper bound; that includes typing and multiple stages, not dedicated
numerical run timeouts. Updated reference closed after a processed helper quit request. All record
Xvfb/WM/application **0/0/−15**: owned teardown, not demonstrated voluntary GUI Quit or successful GUI Stop.
No host GNOME/Wayland/portal, physical-GPU, hardware-driver or desktop-crash acceptance follows.
Windows/macOS/MATLAB remain deferred.

Raw evidence is indexed under `build/example-qualification/20261002/help-tutorials/05-optimal-lunar-flyby`:
construction-resources-initial-guess, first-two-stages, stages3-5-bounded,
continuation-and-original-reference, repaired-reference-verification and postfreeze-reference-originals.
`postfreeze-comparison.json` holds full original and repaired source/result identities, eighteen-component
residuals and verified normalizations; the original failure, metadata duration annotation's original record,
and unchanged raw reports remain alongside it.

## Ten unchanged screenshots

| Evidence | Retained image |
|---|---|
| Actual Help chapter | [Help](help-tutorial-05-help-20261002.png) |
| Independently blank New Script | [Blank editor](help-tutorial-05-blank-20261002.png) |
| Expected Stop and rotated initial segments | [Initial segments](help-tutorial-05-initial-segments-20261002.png) |
| Patch-only convergence | [Continuity](help-tutorial-05-continuity-20261002.png) |
| Full optimization | [Full constraints](help-tutorial-05-optimization-20261002.png) |
| Fixed alternate guess solve | [New guess](help-tutorial-05-new-guess-20261002.png) |
| Own radius exercise completed | [Own exercise](help-tutorial-05-exercise-20261002.png) |
| Actual own final reopen/build | [Reopen](help-tutorial-05-reopened-20261002.png) |
| Original post-freeze solver failure | [Failed original reference](help-tutorial-05-original-reference-failed-20261002.png) |
| One verified updated-reference solve | [Repaired reference](help-tutorial-05-reference-20261002.png) |
