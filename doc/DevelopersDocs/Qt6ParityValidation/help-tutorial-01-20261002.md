# Help tutorial 1: Simulating an Orbit — 2026-10-02

**Passed for bounded private actual-input construction.** The mission was built
from New through the visible Qt GUI using Help, executed to periapsis, inspected
through native viewers/command summaries/animation, saved and reopened, then
compared with the shipped reference after construction was frozen. This is the
first completed Help walkthrough, separate from the prior shipped-script corpus.
Full Linux replacement and host/hardware acceptance remain unfinished.

## Independence, runtime and artifacts

The construction session began with no script argument, selected New Mission and
used the application's Simulating an Orbit Help pages (`SimulatingAnOrbit.html`,
`ch05s02.html` through `ch05s05.html`). Actual XTest mouse/key events targeted a
fresh authenticated Xvfb/Openbox/software-GL display, private settings/runtime and
startup clone changing OUTPUT_PATH only. No engine/model setter or generated
reference mission was used. The first session reached its 600-second harness bound
at the parameter picker; the second resumed only its own saved GUI-authored
milestone. This is continuation of independent construction, not reference seeding.

At 17:26:07.859650 UTC the completed authored source and run/result/reopen evidence
were frozen before reference consultation. Earlier exposure during corpus work is
explicitly disclosed. The tracked
[authored script](help-tutorial-01-authored-20261002.script) is unchanged: 5672 bytes,
SHA256 `f0becc897f413aa8eb211813125e063ebd57e4e0d5fcf4136dd17142411dada4`.
Both preserved authored copies and the later source hash retain these bytes.

| Launch file | SHA256 |
| --- | --- |
| `GmatQt-R2026a` | `7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3` |
| `libGmatBase.so.R2026a` | `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` |
| Selected `gmat_startup_qt.txt` | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |

Ignored durable evidence is
[`build/example-qualification/20261002/help-tutorials/01-simulating-orbit`](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/).
The preservation index hashes the action records, milestones, runtime/source
snapshot, all screenshots, full summaries and first failed reference attempt.
`construction`, `authored-run-reopen` and `reference-compared` retain their separate
sessions/startup clones. The first two application sessions and successful
reference session ended with bounded owned-process cleanup (application -15),
not a graceful Quit qualification. Host GNOME was not targeted.

## Written steps and actual GUI results

| Help stage | Recorded construction/result |
| --- | --- |
| Spacecraft | Rename DefaultSC→Sat; select UTCGregorian and `22 Jul 2014 11:29:10.811`; set Keplerian SMA 83474.318, ECC 0.89652, INC 12.4606, RAAN 292.8362, AOP 218.9805, TA 180 through the editor. Screenshot012 shows pending input; subsequent saved source and completed summary establish its application. |
| Propagator/forces | Rename DefaultProp→LowEarthProp, including its associated ForceModel. Help places gravity/atmosphere/point-mass/SRP controls inside Propagator; Qt exposes the associated resource separately under Force Models. Actual controls select Earth JGM2 degree/order 10, JacchiaRoberts, Sun+Luna point masses and SRP On. The separate placement is a workflow adaptation, with the requested physical settings retained. |
| Plot | Set eye `[-60000 30000 20000]` and XYPlane Off through OrbitView controls. The default GroundTrack remains available alongside native OrbitView; reference OF resources are reserved for later comparison. |
| Mission | Use the actual stop parameter picker to select Sat, Earth and Periapsis; Apply produces `Propagate LowEarthProp(Sat) {Sat.Earth.Periapsis};` without adding a time target. |
| Run/analyze | Actual F5 completes the independently authored mission in 0.361 s. Native Earth/trajectory and ground history render. Camera orientation is manipulated; Command Summary exposes the final state. Subsequent actual EarthFixed selection changes the displayed summary frame in screenshot010. |
| Animation | Master Start/Play yields screenshot011: master, GroundTrack and OrbitView labels all show 17%. This is actual shared animation, not a model-driven fixture or every animation-mode qualification. |
| Save/reopen | The saved own mission is reopened through the GUI and its command editor still shows LowEarthProp/Sat/Sat.Earth.Periapsis (screenshot013). This establishes retained stop/configuration and source; it is not a claim of a second authored execution after reopening. |

The serialized default `ForceModel.Drag = JacchiaRoberts` produces the engine's
supported deprecated-field warning. The mission completes; the warning is not a
runtime failure. It remains a visible serialization limitation.

## Post-freeze reference comparison

Only after the freeze, the shipped `application/samples/Tut_SimulatingAnOrbit.script`
was opened in a separate actual GUI session, using the application's normal OF
conversion, and executed once (0.174 s). The original shipped file was not changed.
Its saved command summary and the authored EarthMJ2000Eq summary were compared;
full values are retained in
[numerical-comparison.json](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/reference-compared/numerical-comparison.json).

Both show UTC `23 Jul 2014 20:48:34.393`, radius `8600.5686739410 km` and TA 360°
(periapsis). The maximum position component difference is
`5.5688005886622705e-6 km` (5.569 mm); position norm difference is 6.366 mm.
Maximum velocity component difference is `3.1780995612962215e-9 km/s`
(3.178 micrometres/s). No bitwise numerical equality is claimed.

A locally chosen 1 mm position/1 micrometre-per-second assertion failed. Help
prescribes no such comparison threshold; the actual difference and failed assertion
are retained, and no source/settings change or repeated runtime was used to force
equality. Initial state, force selections, integrator and Periapsis logic match.
[source-comparison.md](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/source-comparison.md)
records matching defaults plus differences in force ordering, supported drag
syntax, unused burn, metadata and display settings. Floating-point force ordering
is a plausible explanation for low-digit differences, not an established cause.
The pass is the Help's construction/required-result/save-reopen/reference-comparison
contract, not an independent scientific accuracy qualification.

## Preserved unsuccessful actions

The first summary Save attempted typing before the chooser received focus. Later
intended EarthFixed frame text entered the filename and saved the unchanged
EarthMJ2000Eq summary as `application/bin/EarthFixed`; its validated bytes were
moved unchanged into `authored-periapsis-summary.txt`. Screenshot009 does not prove
a frame change. Screenshot010 records the actual subsequent EarthFixed selection.
See the [first-save disposition](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/authored-run-reopen/summary-first-save-disposition.txt).

The first reference helper died in XGetWindowAttributes after a window disappeared
between enumeration and query (`BadWindow`, window 0x600012). No input or mission
was accepted in that attempt. Only the verified owned app/Openbox/Xvfb processes
were then terminated; host GNOME remained untouched. A narrow enumeration error
trap was added to the helper before the successful retry, preserving unexpected
protocol errors. The [failed-attempt disposition](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/reference-first-helper-failed/startup-badwindow-disposition.json)
remains a harness failure, not a GMAT mission pass or host crash fix.

The final helper additionally restricts tolerated BadWindow errors to enumeration
request codes and its exact request serial interval, so queued focus failures
remain failures. Nine mock contract checks passed, including handler restoration
and exact retained diagnostics. The helper SHA256 is
`4232bdb0a050ab22b5f0513a15f92339501edc88719339e2dac1c476a8d3358f`;
[x11-enumeration-contract](../../../build/example-qualification/20261002/help-tutorials/01-simulating-orbit/x11-enumeration-contract/)
retains the check script and results. These checks launch no GUI or mission.

Selected unchanged screenshots:

- [Pending authored Keplerian inputs](help-tutorial-01-orbit-settings-20261002.png)
- [Actual EarthFixed command summary](help-tutorial-01-earthfixed-summary-20261002.png)
- [Shared animation at 17%](help-tutorial-01-animation-20261002.png)
- [Own saved mission reopened with Periapsis stop](help-tutorial-01-reopened-20261002.png)
- [Post-freeze reference periapsis summary](help-tutorial-01-reference-summary-20261002.png)

Tutorial 1 is Passed for this bounded private input route; tutorials 2–12 and the
additional chapters retain their separate pending dispositions. Tutorial 2 is now
being constructed from New under separate evidence. No broader Qt matrix or
shipped corpus was repeated here. Host GNOME/Wayland/hardware/portal and full
replacement gates remain open; Windows/macOS/MATLAB remain deferred. This record
preparation launched no application, runtime or test and changed no product source.
