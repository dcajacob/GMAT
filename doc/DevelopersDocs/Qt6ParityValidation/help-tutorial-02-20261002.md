# Help tutorial 2: Simple Orbit Transfer — 2026-10-02

**Passed for bounded private actual-input construction.** The mission was built
from New through the visible Qt GUI using the application's Help, targeted in
seven iterations, updated through Apply Corrections, rerun in one iteration,
saved and reopened. Only after that construction/result freeze was the shipped
reference opened and executed once; its entire solver report equals the original
authored seven-iteration report. This is one Help walkthrough, separate from the
shipped-script corpus and independent scientific or host/hardware qualification.

## Independence, runtime and retained evidence

`construction-1/session.json` records `script: null`. Actual input selected
Welcome → Tutorials → Simple Orbit Transfer (Help Chapter 6), then New Mission.
DefaultSC/DefaultProp were kept; TOI/GOI/DC1 and commands were authored through
controls. Continuation loaded only the own saved milestone, not Tutorial 1 or a
shipped reference. Previous corpus/reference exposure is disclosed;
`before-run-identity.json` and the freeze record state that no reference was
consulted during this construction.

Actual XTest input used fresh authenticated Xvfb/Openbox/software GL and private
settings/runtime. The 600-second sessions changed only startup OUTPUT_PATH;
no engine/model setter or generated reference mission seeded construction.
Xvfb/WM exit 0 and app -15 record bounded cleanup, not graceful Quit. Host GNOME
was not targeted.

The recorded application SHA256 is
`7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3`;
the selected startup is
`5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
The same controlled checkpoint identifies core
`cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`
and the previously recorded X11 helper `4232bdb0…`. These helper session records
capture application/startup identities, not a separate per-launch core hash.

Ignored durable evidence is
[`02-simple-orbit-transfer`](../../../build/example-qualification/20261002/help-tutorials/02-simple-orbit-transfer/):
`construction-1`, `construction-2-and-runs`, `reopen-and-reference` and the
SHA256/size `artifact-index.json`. It retains all 50 screenshots, actual action
records, resource/command milestones, both authored reports and the reference
report. The first construction directory is the earlier 5431-byte milestone;
the final corrected 5978-byte source is preserved separately in
`construction-2-and-runs/tutorial02-after-corrections.script`.

## Written steps and actual GUI results

| Help stage | Recorded construction/result |
| --- | --- |
| Resources and graphics | Keep default spacecraft/propagator; create DifferentialCorrector DC1 and ImpulsiveBurn TOI/GOI. Set OrbitView eye `[0,0,120000]`, up X and solver iterations Current through the controls. GroundTrack remains available. |
| Initial and Target sequence | Set the initial stop to DefaultSC.Earth.Periapsis. Build Target/DC1, Vary TOI, TOI maneuver, apoapsis propagation, RMAG Achieve, Vary GOI, GOI maneuver and ECC Achieve in the written order, followed by EndTarget and the final DefaultSC.ElapsedSecs=86400 stop. Labels are retained. |
| Goals and controls | Both initial Vary guesses are 1, perturbation 0.0001; TOI/GOI MaxStep are 0.5/0.2. RMAG goal is 42164.169 km with tolerance 0.1 km; ECC is 0.005 with tolerance 0.0001. The label “Achieve RMAG = 42165” is rounded and does not change the actual goal. |
| First run | Actual F5 completes in 0.760 s; DC1 converges in seven iterations. Native Orbit/Ground viewers and the solver window show the transfer and achieved goals. The complete 4991-byte solver report is retained. |
| Apply Corrections and rerun | The Target editor’s Apply Corrections button changes only the two Vary initial guesses: TOI.Element1=2.24026921075037 and GOI.Element1=1.440642858252199 km/s. Actual F5 then completes in 0.647 s and one iteration with the same achieved goals; its 926-byte report is retained separately. |
| Save/reopen | Actual Ctrl+O reopens the own saved mission; the TOI and GOI Vary editors show the corrected initial values. Saving after inspection retains the final source bytes exactly. No third authored execution after reopening is claimed. |

Two initial Achieve-entry attempts placed later input in the Name field. Both
commands were reopened and corrected through the visible GUI before F5;
screenshots 011–014 and the valid before-run source preserve that correction.
This is an entry correction, not a proved engine/numerical failure. No unrelated
UI repair or repeated old test matrix was used to produce the tutorial result.

Frozen achieved values are RMAG `42164.16907394781 km` and ECC
`0.005000000411268245`, residuals approximately `7.39478055e-5 km` and
`4.11268245e-10`. Acceptance uses the Help’s 0.1 km radius tolerance and 0.0001
ECC tolerance; both achieved values satisfy those limits. These are observed
GMAT results, not an independent physical accuracy assertion.

## Freeze and post-freeze reference comparison

At **2026-10-02 17:57:42.093446 UTC**, construction, both required authored runs,
solver corrections and actual Ctrl+O reopen were frozen in
`reopen-and-reference/authored-before-reference-comparison.json`. The final
corrected/reopened [authored script](help-tutorial-02-authored-20261002.script)
is unchanged: 5978 bytes, SHA256
`652c63e2559d6cda7e489f458c870ae8908fd8aa780adf8b518afd5cc4c4d20c`.
The before-corrections 5951-byte source is
`1f02bceedf6b84dd890f190c1730280f37a541e61eccd861a62128fd700d7e3d`.
Their diff changes only the two Vary initial guesses.

After the freeze, actual Ctrl+O opened
`application/samples/Tut_SimpleOrbitTransfer.script`; the normal Convert views
choice was accepted, then actual F5 executed it once in 1.881 s. No reference
source was saved after the UI-only view translation. The original shipped
source remains SHA256
`c12811942255832ce30a5e6224067760c59a86695969f71f692d281db835c485`.
The subsequent source comparison was also post-freeze.

The reference converges in seven iterations to the same two burn controls and
two goals. Its entire 4991-byte DifferentialCorrectorDC1 report is byte-identical
to the frozen first authored report, SHA256
`3edcbc50a784dcf587a7fb68c1d5b9a031e9de09b97beaddaa081f58a8a20d21`.
`reopen-and-reference/numerical-reference-comparison.json` and both raw report
copies preserve that result. Equality covers this complete solver report;
it does not establish equality of every propagated sample or independent
scientific accuracy.

The post-freeze source comparison finds matching initial state, Earth/JGM2
4×4 forces, RK89 settings, mission order, goals/tolerances and Vary
perturbations/MaxStep. Material differences remain explicit:

| Setting | Authored / reference | Limit |
| --- | --- | --- |
| Both Vary bounds | −10…10 / 0…3.14159 | Help construction leaves the offered defaults. Both solved controls lie inside both ranges, but the search setups are not identical. |
| Target ExitMode | DiscardAndContinue / SaveAndContinue | Different solved-control retention policy. The authored workflow separately uses the Help's Apply Corrections action; no silent source substitution was made. |
| Resource/display defaults | Authored burn/DC declarations use runtime defaults; reference spells them out. Qt OrbitView versus converted OF; eye 120000 versus reference 120001. | Source omission alone does not prove every omitted field equal. Required behavior/results have GUI/report evidence; visualization and unused modern defaults differ. |

## Unchanged screenshots

Original PNG bytes and hashes are retained in the durable artifact index.

- [Actual Help chapter opened from Welcome](help-tutorial-02-help-20261002.png) — `construction-1/screenshots/003-help-simple-orbit-transfer.png`
- [First authored target result and native transfer](help-tutorial-02-first-run-20261002.png) — `construction-2-and-runs/screenshots/016-first-target-run.png`
- [One-iteration rerun after Apply Corrections](help-tutorial-02-corrected-run-20261002.png) — `construction-2-and-runs/screenshots/019-corrected-one-iteration-rerun.png`
- [Own corrected TOI value reopened](help-tutorial-02-reopened-20261002.png) — `reopen-and-reference/screenshots/002-reopened-toi-correction.png`
- [Post-freeze reference result](help-tutorial-02-reference-20261002.png) — `reopen-and-reference/screenshots/005-reference-transfer-compared.png`

Tutorial 2 is Passed for this bounded Help construction/run/corrections/save/
reopen/reference-comparison contract. This record adds no every-sample corpus,
independent-science, host GNOME/Wayland/portal, physical-driver or crash-fix claim.
Other tutorial and full Linux replacement dispositions remain separate;
Windows/macOS and MATLAB remain deferred.
