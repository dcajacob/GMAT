# Help tutorial 4: Mars B-Plane Targeting — 2026-10-02

**Passed for bounded private actual-input construction.** The mission was built
from New through resource and command forms using Help, then solved in stages.
The B-plane constraints and Mars capture radius meet their stated tolerances;
actual Apply Corrections makes both final Targets converge in one iteration.
Required command summaries, three views and own save/reopen were completed before
freezing the authored mission. A later source comparison and single actual GUI
reference run satisfy the same numerical goals with explicit differences.

## Independence and runtime

The first session records `script: null`; actual Welcome → Tutorials → Mars
B-Plane Targeting → New Mission opens the chapter and starts construction. All
continuations load only independently saved milestones. No shipped tutorial
script was read or used to seed the resources/commands before freeze. Help XML
and the Help-only checklist supply the inputs. Prior general corpus exposure is
separate from this independent construction.

Fresh authenticated Xvfb/Openbox/software GL sessions have private settings and
OUTPUT_PATH-only startup clones. Construction sessions are bounded to 600 seconds;
the post-freeze reference session is bounded to 180. Actual missions use app
`3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1`,
selected startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`
and helper `4232bdb0a050ab22b5f0513a15f92339501edc88719339e2dac1c476a8d3358f`.
Early construction used older apps before coordinated fixes. The controlled core
checkpoint is `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`;
X11 session metadata does not independently hash core on each launch.

Ignored durable evidence is
[`04-mars-b-plane`](../../../build/example-qualification/20261002/help-tutorials/04-mars-b-plane/):
eight owned sessions, all intermediate screenshots/actions and saved sources,
full Help/checklist, progress, exported summaries, reports, source comparison and
SHA256/size artifact index. Earlier unsuccessful actions remain retained.
Final authored and reference sessions gracefully exit through GUI Alt+F4;
`session.json` records Xvfb/WM/app exit 0/0/0. Other construction sessions used
bounded helper cleanup, with app −15. No host desktop input or process was used.

## Written steps and actual observations

| Help stage | Actual GUI evidence |
| --- | --- |
| Resources | Create MainTank (1718 kg, density1000, volume2, pressure5000), rename DefaultSC to MAVEN, enter UTC epoch 18 Nov2013 20:26:24.315 and the prescribed hyperbolic Earth Keplerian state, attach tank. Create TCM Earth/VNB and MOI Mars/VNB with mass decrement/MainTank. Create DefaultDC, SunEcliptic Sun/MJ2000Ec and MarsInertial Mars/BodyInertial. |
| Propagators/forces | NearEarth RK89, step600/min0/max600, accuracy1e−13, Earth/JGM2 8×8, Sun/Luna/SRP. DeepSpace PD78, 600/0/864000,1e−12, Sun central/no primary, nine prescribed point masses/SRP. NearMars PD78,600/0/86400,1e−12, Mars/Mars50c 8×8, Sun/SRP. Saved/reopened source retains explicit separate force-model links and the correctly cased official Mars file. |
| Views | EarthView native OrbitView has vector[0,0,30000], scale4; SolarSystemView plots MAVEN/Earth/Mars/Sun in SunEcliptic with Sun reference/direction and vector[0,0,5e8]; MarsView plots MAVEN/Earth/Mars in MarsInertial with Mars reference/direction and vector[22000,22000,0]. Default GroundTrack is deleted. Screenshot028 shows Earth departure, heliocentric transfer and the captured Mars orbit. |
| First Target | SaveAndContinue/DefaultDC; Prop3Days NearEarth → Prop12Days DeepSpace → three TCM Vary commands (initial1e−5, perturbation1e−5, bounds±10e300, MaxStep.002) → ApplyTCM → Prop280Days DeepSpace → MarsPeriapsis NearMars → AchieveBdotT0/BdotR−7000, both tolerance1e−5 km. |
| First solve/corrections | First F5: 6 iterations/4.227 s. BdotT3.445461516093928e−7 km and BdotR−7000.000005352436 km meet tolerance. Actual Target editor Apply Corrections updates all three Vary guesses; saved corrected stage reruns in1 iteration/.973 s with the same acceptable residuals. |
| First summaries | ApplyTCM export: ΔV vector[.0039376911841,.0060423210045,−.0006747163077] km/s, magnitude.0072436383868, mass change−6.3128745863502 kg. MarsPeriapsis summary in MarsInertial: RMAG3953.0476316801 km, altitude556.04763168008 km, INC89.99999997460°. Polar-arrival intent is observed; Help specifies no separate INC tolerance. |
| Mars capture | After first EndTarget, append Mars Capture/DefaultDC/SaveAndContinue: VaryMOI.Element1 initial−1, perturbation1e−5, bounds±10e300, MaxStep.1 → ApplyMOI → NearMars to MarsApoapsis → AchieveMars.RMAG12000, offered tolerance.1 km. After second EndTarget, NearMars propagates1 elapsed day. |
| Full initial capture | F5 completes in1.864 s with Target counts[1,11]. MOI.Element1=−1.603439847729028 km/s; RMAG12000.01965272428 km, residual.01965272428424214 <.1 km. The Mars viewer shows a captured ellipse. |
| Capture summaries | ApplyMOI export confirms Mars/VNB, ISP300, ΔV[−1.6034398477290,0,0] km/s and mass change−1076.0639629591 kg; remaining MainTank635.62316245451 kg. AchieveRMAG summary in MarsInertial gives TA180° and RMAG12000.019652724 km at that command, rather than the final one-day endpoint. |
| Final corrections/reopen | Actual Mars Capture Apply Corrections changes MOI initial−1 to−1.603439847729028. Saved final mission reruns with counts[1,1]/1.078 s and RMAG12000.01965271864 km. Actual Ctrl+O own reopen succeeds; screenshot031 shows the complete sequence and all four corrected Vary guesses. No additional unchanged rerun after reopening. |

The DC reports were preserved before each next run. The full flushed stage2 log
records initial [1,11] and final [1,1] counts; screenshot results and command exports
provide the stated observations. These are actual GMAT outputs checked against
independently known Help goals, not independent physical-science validation.

## Failures retained and repaired

Valid final ChemicalTank settings initially rejected Create with
`Hardware Exception Thrown: Fuel volume exceeds tank capacity`. A GUI workaround
created lower mass then edited to1718; the later batch-order repair has its own
focused regression evidence. The first save attempt was closed while its dialog
was still open, so no file was saved; that short resource setup was reconstructed
and each subsequent save was verified on disk before closing.

Actual construction also found missing Local burn Mars Origin choices,
propagator Type/FM changes lost on save/reopen, shared implicit new-propagator
models without editable declarations, and a case-sensitive Mars gravity default
`MARS50C.cof` versus official `Mars50c.cof`. Coordinated fixes were rebuilt and
verified narrowly; actual continuation confirms Mars selection, PD78/separate
model retention and successful NearMars Apply. See
[tutorial resource fixes](tutorial-resource-fixes-20261002.md) and
[Mars gravity resolution](mars-gravity-default-20261002.md).

A queued scroll changed field positions during view setup; the actual Earth
scale4 was corrected before save, without asserting a product defect. Pre-run
readback caught an unintended NearMars SRP Off selection; it was changed to On
before the first mission execution. That earlier saved source remains retained.
Some summary-dialog focus/input attempts failed; actual exported files were
verified before using them. Their exact actions remain recorded and no modal
lifecycle defect is inferred from those attempts.

## Freeze and reference comparison

The authored file was frozen at **2026-10-02 19:57:35.160009 UTC** before reference
access, after construction, required stages/results/summaries/views and own
reopen. The unchanged [authored source](help-tutorial-04-authored-20261002.script)
is8628 bytes, SHA256
`e889a806838921518b614b770539181732ed870106e3919b5683da37ca4db13b`.
`authored-before-reference-comparison.json` retains that timestamp and result
identities. Initial resources and pre-correction milestones remain distinct.

After freeze, the original shipped reference SHA256
`f334cf650aaa435f132de1d8e7823333378a0d40cd179e0bb0e3143af4ff3178`
was opened once in an actual private GUI. Normal Convert views translated its
OpenFrames resources; the translated buffer was saved only to an owned /tmp
file, SHA256`4c2d151c0e90af5443ec773c3bf3b2019b3db1608b1eb7a101de1e24cf8e7ef5`.
Its actual F5 completes once in2.677 s with counts[3,1]. Old reference TCM guesses
initially miss the B-plane goals, then converge to BdotT−1.52682241605e−6 km and
BdotR−7000.00000036 km, within1e−5. Reference MOI remains−1.603439847094663 km/s;
RMAG12000.01965120208 km is within.1. Reference ApplyMOI and AchieveRMAG command
summaries were exported. The original shipped source remains unchanged.

The post-freeze comparison finds the prescribed epoch/state, force membership,
integrators, frames, hardware associations, command order, bounds/perturbations,
goals and tolerances aligned. Initial elements differ in final decimal places;
reference guesses are already corrected, whereas Help starts1e−5/−1. The authored
script leaves some tank/burn/solver/force defaults implicit; omission does not
establish equality of all fields. Current NAIF identifiers/report filename and
point-mass ordering differ. Native OrbitViews differ from converted OpenFrames
geometry, up vectors, star counts and retained camera/object metadata. Numerical
outputs satisfy the goals; complete-report, every-state and pixel equality are
not claimed. Help's illustrative MOI−1.6034665169868 also differs from the modern
reference, so it is not an exact-equality acceptance oracle. The full source
comparison remains in ignored evidence.

## Selected unchanged screenshots

- [Help opened from Welcome](help-tutorial-04-help-20261002.png)
- [First B-plane solve](help-tutorial-04-first-run-20261002.png)
- [Corrected one-iteration first Target](help-tutorial-04-first-corrected-20261002.png)
- [Full corrected capture result](help-tutorial-04-capture-corrected-20261002.png)
- [Three views](help-tutorial-04-views-20261002.png)
- [Own final source reopened](help-tutorial-04-reopened-20261002.png)
- [Post-freeze reference result](help-tutorial-04-reference-20261002.png)

No old matrix/corpus or unaffected passing test was repeated. This bounded Help
walkthrough does not qualify host GNOME/Wayland, hardware rendering, crash repair,
every regime or other chapters. Full Linux replacement gates remain separate;
Windows/macOS/MATLAB remain deferred.
