# Help tutorial 3: Target Finite Burn to Raise Apogee — 2026-10-02

**Passed for bounded private actual-input construction.** Starting from New,
the mission was built through Qt controls using the application's Help. Its
finite-burn target converged in 13 iterations; four command summaries confirmed
fuel consumption, burn cutoff and the apogee goal. Actual own save/reopen and
trial-history viewing were completed before the construction freeze. One later
GUI reference run produced the same complete solver report. This is one Help
walkthrough, separate from corpus execution and scientific or host qualification.

## Independence, runtime and retained evidence

`construction-1/session.json` records `script: null`. Actual input selected
Welcome → Tutorials → Target Finite Burn to Raise Apogee (Help Chapter 7), then
New Mission. DefaultSC/DefaultProp were retained; hardware, FiniteBurn1, DC1,
BurnDuration and the mission sequence were authored through resource/command
controls. Continuations loaded only the own saved milestones. No shipped mission
was opened, copied or used to seed this construction. Prior corpus/reference
exposure is disclosed; the freeze record states that no reference was consulted
during the independent Help-driven construction.

Actual XTest input used fresh authenticated Xvfb/Openbox/software GL with private
settings/runtime. Each session was bounded to 600 seconds; startup clones changed
OUTPUT_PATH only. The recorded application SHA256 is
`7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3`;
the selected startup is
`5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
The controlled runtime checkpoint separately identifies core
`cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`
and previously recorded X11 helper `4232bdb0…`. These X11 session records capture
application/startup identity, not an independent per-launch core hash.

Ignored durable evidence is
[`03-finite-burn`](../../../build/example-qualification/20261002/help-tutorials/03-finite-burn/):
`construction-1`, `construction-2-and-first-run`, `reopen-viewer-and-reference`,
Help-only checklist, post-freeze source comparison and SHA256/size artifact index.
The index retains 114 files, including 69 unchanged PNGs, totaling 19,806,000
bytes, excluding the index itself. The 5693-byte first construction milestone
and 6194-byte first-run source remain distinct from the final 6190-byte mission.
All three sessions record Xvfb/WM exit 0 and app -15: bounded owned cleanup,
not graceful Quit. The separately checked host GNOME identity remained
PID 399961/start ticks 38989406; no host desktop was targeted.

## Written steps and actual GUI results

| Help stage | Recorded construction/result |
| --- | --- |
| Hardware and attachments | Create ChemicalTank1 and ChemicalThruster1. Set mass decrement On, C1=1000, tank ChemicalTank1 and mixture ratio 1. Keep the offered VNB thrust/ISP defaults. Attach both tank and thruster to DefaultSC and select ChemicalThruster1 in FiniteBurn1. |
| Target control | Create DifferentialCorrector DC1 and BurnDuration initially 0. Configure Target “Raise Apogee” with Solve, SaveAndContinue and progress window On. Vary BurnDuration with initial guess 200 s, perturbation 0.0001, upper bound 10000 s and MaxStep 100 s. The offered lower bound remains −10 s. |
| Ordered sequence | Prop To Perigee → Target/DC1 → Vary Burn Duration → BeginFiniteBurn “Turn Thruster On” → propagation stopping at DefaultSC.ElapsedSecs=BurnDuration → EndFiniteBurn “Turn Thruster Off” → Prop To Apogee → Achieve DefaultSC.Earth.RMAG=12000 km, tolerance 0.1 km → EndTarget. The elapsed-time stop retains the variable, not a literal duration. |
| First execution | Actual F5 completes in 0.749 s and 13 iterations. Solver and native Orbit/Ground viewers show convergence; BurnDuration=1213.193162777513 s and RMAG=12000.00001229124 km. |
| Required summaries | Actual command-summary inspection and Save export produce four text files: perigee, thruster on, thruster off and apogee. Mean anomaly is 0° at perigee, 25.131809686270° at cutoff and 180° at apogee. Initial fuel is 756 kg; cutoff fuel is 343.76990738327 kg, a decrease of 412.23009261673 kg. |
| Own reopen | Actual Ctrl+O reopens the own saved mission. Visible editors confirm C1=1000, ChemicalTank1 mixture=1, Vary initial 200/upper 10000/MaxStep 100 and the saved solver-history setting. The final source remains byte-exact after the final reopen. |
| Trial-history viewing | Change only OrbitView SolverIterations Current→All and run once in 0.736 s. The complete solver report remains byte-identical. Actual mouse rotation obtains the face-on raised-apogee/trial view. Stars, constellations and XY plane are disabled in the viewer for visibility; these viewer-only overrides do not alter the saved resource defaults. |

Help describes trial/perturbation paths but does not explicitly instruct changing
SolverIterations; the New mission's Current setting did not provide the requested
retained trial-history view. All was therefore an explicit display adjustment.
The second own execution qualified that changed display; it was not an unchanged
numerical retest or a Tutorial 2 Apply Corrections workflow. Comparing the two
saved authored sources confirms that SolverIterations Current→All is their only
semantic model change. The default XY plane and other source drawing settings
remain in the reopened resource despite the temporary viewer overrides.

An unintended pending coordinate-system wheel change was discarded before Apply.
The final source retains EarthMJ2000Eq; no altered-frame execution or engine defect
is inferred. The partial construction directory and first-run checkpoint retain
their original status; the later freeze/reference records establish completion.

The apogee radius residual is approximately `1.229124e-5 km`, within Help's
**0.1 km tolerance**. Help asks for a similar burn duration and gives
1213.19316329 s and fuel about 343.76990815648 kg, rather than prescribing exact
floating-point equality. The observed duration, fuel and mean anomalies satisfy
these bounded tutorial observations. They are GMAT results, not independent
physical accuracy assertions.

## Freeze and post-freeze reference comparison

Construction, required authored results, summaries, viewing and save/reopen were
frozen at **2026-10-02 18:26:43.612951 UTC** in
`reopen-viewer-and-reference/authored-before-reference-comparison.json`.
The final unchanged [authored script](help-tutorial-03-authored-20261002.script)
is 6190 bytes, SHA256
`8a4a6914110f27b306ad256c6c0e4dc7a7f943d2c0100d7810f1b2a3d34e84fa`.
The first-run source remains 6194 bytes, SHA256
`a645f88e7d7c379cd38c47d7b6ac5ae4238ccaca434b0023c4136423f55699b0`.

Reference access was recorded later, at 18:27:07.126526 UTC. Actual Ctrl+O opened
`application/samples/Tut_Target_Finite_Burn_to_Raise_Apogee.script`; normal
Convert views was accepted, then actual F5 completed once in 0.541 s. The view
translation remained in the UI buffer and was never saved to the original
reference. Its 10236-byte disk source remains SHA256
`6bab56e5fb2068331e8e2c497d74711551d01266989a922fe03b2613e3624fcb`.
The source comparison in the durable evidence was also performed after freeze.

All three complete DifferentialCorrectorDC1 reports—the first authored run,
authored All-history run and reference run—are **byte-identical, 5583 bytes**,
SHA256 `809ed5bba4fe90001b83612fc2e1790935e09f129a2a13f0689bcabb97853f14`.
The reference converges in 13 iterations with the same duration and radius goal.
The raw reports and `numerical-reference-comparison.json` preserve this result;
equality covers this entire solver report, not every propagated sample,
rendered pixel or independent scientific accuracy.

The post-freeze source comparison finds matching initial state, Earth/JGM2
4×4 forces, RK89 settings, explicit hardware associations, variable stop,
mission order, Target SaveAndContinue and radius goal/tolerance. Material
source differences and omissions remain explicit:

| Setting | Authored / reference | Limit |
| --- | --- | --- |
| Vary lower bound | −10 / 0 s | Help leaves the offered lower default unchanged. The solved duration lies above both bounds, but the search configurations differ. |
| Hardware and solver defaults | Tank/DC declarations omit most fields; reference serializes fuel, pressure, density, remaining thruster coefficients, throttle logic, scaling and DC options. | GUI attachment/coefficient checks, summaries and reports establish the stated behavior. Source omission does not establish equality of every remaining default. |
| Orbit/history configuration | Native OrbitView, All, 7000 stars/default geometry / converted OF view, Current, 40000 stars/explicit geometry. Both source camera eyes are `[30000,0,0]`, up Z. | The authored All adjustment supplies Help's trial-history observation. Rendering/source defaults differ; no pixel or every-sample parity is claimed. |
| Modern/unused fields | Extra spacecraft/force defaults and no unused DefaultIB / legacy values and an unused DefaultIB. | Neither sequence invokes DefaultIB. Unqualified default fields are not claimed equal. |

## Unchanged screenshots

Original PNG bytes and hashes are retained in the durable artifact index.

- [Actual Help chapter opened from Welcome](help-tutorial-03-help-20261002.png) — `construction-1/screenshots/003-finite-burn-help-chapter.png`
- [First authored finite-burn solve](help-tutorial-03-first-run-20261002.png) — `construction-2-and-first-run/screenshots/013-013-first-finite-burn-run.png`
- [Exported cutoff summary, mean anomaly and remaining fuel](help-tutorial-03-cutoff-summary-20261002.png) — `construction-2-and-first-run/screenshots/018-018-thruster-off-summary.png`
- [Face-on raised-apogee and retained trials](help-tutorial-03-trials-20261002.png) — `reopen-viewer-and-reference/screenshots/016-016-face-on-clean-trials-final.png`
- [Final own resource reopened with All retained](help-tutorial-03-reopened-20261002.png) — `reopen-viewer-and-reference/screenshots/018-018-reopened-all-history-settings.png`
- [Post-freeze reference convergence](help-tutorial-03-reference-20261002.png) — `reopen-viewer-and-reference/screenshots/021-021-reference-finite-burn-result.png`

Tutorial 3 is Passed for this bounded Help construction/execution/summary/viewing/
save/reopen/reference contract. No old matrix/corpus was repeated. Other tutorials
and full Linux replacement, host GNOME/Wayland/portal, physical-driver and crash
repair gates remain separate; Windows/macOS/MATLAB remain deferred.
