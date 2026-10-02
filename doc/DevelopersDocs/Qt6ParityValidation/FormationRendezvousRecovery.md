# Formation rendezvous tutorial reconstruction

Prepared 2026-10-02 in the Linux Qt qualification checkout. This is a repaired
**demonstration**, not a recovered original mission or a qualification of new
numerical algorithms.

## Failure and source evidence

The source and installed tutorial attachments were both an unrelated Earth/Luna
transfer using `Sat`/`InitSat` and TOI/LOI burns. Their original SHA-256 was
`e070a6e169cd03e092bc27a5876291ebb1ad681ee1798f53669662d5883d89a4`. The chapter instead describes `Sat1` through `Sat4`, Sat1-centered
LVLH and Sat2-centered VNB frames, and two three-control differential-corrector
sequences. Its spacecraft paragraphs referred to missing "following Script
Syntax": no initial-state listing for the four spacecraft exists in the XML.

Read-only comparison with installed R2020a, R2022a, R2025a and R2026a references
found the same incorrect lunar attachment. Their raw files have SHA-256
`244ceac6c9088dae215cb7c5facc0dc6edc9d2dc03bbc4781bb2856e392b5f77`
(the leading blank line and CRLF differ from the checked-in copy). They cannot
recover the missing formation numbers. The canonical references were not edited.

Authoritative local evidence:

- `doc/help/src/Tut_FormationRendezvous.xml`: spacecraft names and common epoch,
  point-mass bodies, burn/frame/plot names, coasts, target intervals, Vary bounds
  and Achieve goals.
- `doc/help/src/files/images/Tut_FormRendezvous_Sat1SpacecraftObject.jpeg`:
  Sat1 SMA 7599.999999999999 km, ECC 1.355882865295668e-16, INC 100.51 deg,
  RAAN 278.85 deg, AOP 0 deg, TA 90 deg.
- `Tut_FormRendezvous_RKV_LowEarthPropagatorObject.jpeg`: RungeKutta89,
  accuracy 1e-11, RSSStep, initial step 60 s, minimum 0.001 s, maximum 900 s,
  maximum 50 attempts.
- `Tut_FormRendezvous_Burn1ImpulsiveBurnObject.jpeg`: local Earth VNB burn,
  initial V 0.0001 km/s and other components zero.
- `Tut_FormRendezvous_DCDifferentialCorrectorObject.jpeg`: 25 iterations,
  Normal reporting to DifferentialCorrectorDC.data, forward differences.
- `Tut_FormRendezvous_Sat1LVLHCoordinateSystemObject.jpeg`: origin Sat1,
  primary Earth, secondary Sat1, Y=-N, Z=-R.

The actual shipped `application/samples/Ex_SafetyEllipse.script` demonstrates
small orbital-element differences and object-referenced frames, but uses only
two spacecraft in a different orbit. `Ex_MarsOrbit.script` defines a different
four-spacecraft Mars formation. Neither is the missing tutorial mission.

## Reconstruction choices and preserved semantics

All four spacecraft use EarthMJ2000Eq Keplerian states, common epoch
`01 Jan 2000 11:59:27.966` UTCGregorian and SMA 7600 km; all AOP values are zero.
Sat1's negligible numerical eccentricity is rounded to the circular value zero.
The other states are deliberately chosen, not inferred historical values:

| Spacecraft | ECC | INC (deg) | RAAN (deg) | TA (deg) | Origin of values |
| --- | ---: | ---: | ---: | ---: | --- |
| Sat1 | 0 | 100.51 | 278.85 | 90 | Recovered screenshot, circular rounding |
| Sat2 | 0.0005 | 100.515 | 278.855 | 90.05 | Chosen small eccentricity/plane/phase offsets |
| Sat3 | 0.0005 | 100.505 | 278.845 | 89.95 | Chosen opposite plane/phase offsets |
| Sat4 | 0 | 100.52 | 278.85 | 90.1 | Chosen plane/phase offsets |

The common SMA and small offsets provide a nearby formation and a nontrivial
Sat2 transfer. Sat3/Sat4 remain unmanoeuvred throughout. No additional force
models or proprietary dependencies are introduced. The force model uses Earth,
Sun, Luna and Jupiter point masses, no primary-body gravity harmonics, no drag
and no SRP, matching the chapter.

Both targets keep the documented 5498.65387429-second propagation interval.
Target1 varies three Earth-VNB burn components with initial values
(-0.0001, 0, 0), perturbation 0.001, bounds +/-0.3 and maximum step 0.05 km/s;
it targets Sat2.Sat1LVLH.X/Y/Z = 0 with per-component tolerance 0.0001 km.
Target2 uses the same guesses/perturbations, bounds +/-1 and maximum step
0.5 km/s; it targets Sat2 inertial VX/VY/VZ equal to Sat1's values with
per-component tolerance 0.0001 km/s. Both 0.2-day coasts are retained. The final
coast is outside Target2, after its endpoint constraints. The elapsed-time
stopping conditions measure each propagation command's duration, not absolute
mission elapsed time; this matches `StopCondition::Evaluate`'s cyclic-time path.
`DiscardAndContinue` preserves the legacy default solver option and the final
solved trajectory; the two Vary initial guesses remain explicit in the source.

The second target does **not** also constrain position. Position coincidence
at the first endpoint and velocity matching at the second endpoint do not
constitute simultaneous six-state docking or bounded post-transfer relative
motion. The residual report includes the unconstrained second-endpoint position
to make this distinction visible.

Sat1LVLH retains the screenshot's -N/-R orientation. Sat2VNB is completed with
the conventional X=V/Y=N axes and Earth/Sat2 primary/secondary; these axes are
an explicit reconstruction choice. The chapter's contradictory XY plot names
and Sat1VNB typo are corrected to their described plotted spacecraft/frames.
The relative OrbitView retains Earth reference/direction, scale 0.15 and X up;
its undocumented camera vector uses the native OrbitView default [0 0 30000]
km, with the view-up frame explicitly set to Sat1LVLH. All four spacecraft and
Earth are included for context. Display framing is not recovered from the old
figure and no claim of visual equivalence is made.

## Files and acceptance evidence

The source and installed `FormationRendezvousTutorial.script` copies are byte
identical, with replacement SHA-256 `98e589cfd2bc78c5a7bf154c7a1f131b4d1e18e89d73874b1992fba9cb22153b`.
The chapter download now points to `files/scripts/FormationRendezvousTutorial.script`
and includes the reconstruction notice and complete initial-state table.

`DataReport` executes the chapter's twenty six-component EarthMJ2000Eq Cartesian
Report commands into `FormationRendezvousTutorial.data`. Commands inside Target
also execute during solver trial passes. `RendezvousResiduals` additionally writes the
first position residual components/norm, second inertial velocity residual
components/norm, second endpoint's unconstrained position, and both final VNB
burn component sets to `FormationRendezvousResiduals.data`. Component tolerances,
not norm tolerances, are the actual solver requirements.

Static preparation confirms XML well-formedness, twenty numbered Report
commands, two target blocks, four propagations and matching source/runtime bytes.
No build or numerical mission was run by the reconstruction agent. The parent
qualification run must record actual build/run status, convergence of both
DC target blocks and the per-component residuals above. Historical screenshots
are illustrative only. A passing run qualifies this explicit repaired
demonstration; it cannot qualify the missing historical initial states.


## Recorded coordinated offscreen build/run, 2026-10-02

The parent ran this replacement once through the actual Qt executable with
isolated output/settings and `--convert-views`. Build passed (exit 0,
0.471 s) and mission execution passed (exit 0,
1.072 s). Source and referenced source hashes were retained
unchanged. The numerical evidence comes from the actual generated residual
report; this evidence update did not rerun the mission.

The log reports Target1 completed in **3 iterations** and Target2 in
**2 iterations**. Endpoint values below are copied without rounding from
`FormationRendezvousResiduals.data`:

| Target endpoint quantity | Component X | Component Y | Component Z | Per-component tolerance |
| --- | ---: | ---: | ---: | ---: |
| Target1 Sat2 position in Sat1LVLH (km) | -1.153207360647924e-05 | -1.07592500856551e-09 | -6.657351145399684e-06 | 0.0001 km |
| Target2 Sat2 minus Sat1 EarthMJ2000Eq velocity (km/s) | 1.006027180228131e-06 | 8.793346012447856e-06 | -4.44401234389602e-06 | 0.0001 km/s |
| Target2 unconstrained Sat2 position in Sat1LVLH (km) | -0.00880748534512703 | -0.002697776245731491 | 0.002447191900005059 | None |

First-endpoint position norm is `1.331574433128033e-05` km. Second-endpoint
velocity-difference norm is `9.903750324735098e-06` km/s. Every constrained
component satisfies its specified tolerance. The unconstrained position at the
velocity-match endpoint is nonzero, directly illustrating why these separate
endpoint goals must not be described as a simultaneous docking solution.

| Solved Earth-VNB burn (km/s) | V | N | B |
| --- | ---: | ---: | ---: |
| Target1 | -0.0004506796161015988 | -0.000499779195287861 | 0.0006605229348967335 |
| Target2 | 0.0004506095125711486 | 0.001022952706819816 | 0.00236690884579844 |

Both burns remain inside their documented component bounds. No claim is made
that the historical omitted formation had these burns, trajectories or
residuals. This proves execution and convergence of the explicitly repaired
example with the selected existing GMAT engine; it is not independent numerical
algorithm validation, a sustained rendezvous/docking analysis, or native viewer
qualification.

Evidence root: `/tmp/gmat-shipped-examples-formation/scripts/application__docs__help__files__scripts__FormationRendezvousTutorial.script`. The raw build/run `attempt-01/result.json`,
stdout/stderr, `output/GmatLog.txt`, solver text and generated reports remain
in that folder. Residual report SHA-256:
`fd9983485ff78943c1fc37d07675c6d7f61642b7d59b7787e1f5212a4effc6ec`.
Actual running Qt process binary SHA-256:
`45f4245e214b9f15c746fdd7ef2c381df9231c659e601a4536b4adfe259fb4c9`.
Recorded linked bin-directory core library snapshots:

- `libGmatBase.so.R2026a`: `febb403ef90c4ab4825c8849abe9199ab98c719491a128fe8cd138b3b30f8e02`
- `libGmatUtil.so.R2026a`: `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`

HEAD `ca52d977e12a4271e92f845541e50da2a6c82005` and source diff hash
`180958ef00ee1f22d5467a17f7a386f060361ec947c47cfa41ecd5eb18d68a4c` record the source context, not proof
that all installed binaries were built from that exact dirty tree. The linked
library snapshot records installed files named by ELF dependencies, not all
possible loader overrides, plugins or mission data. The run result contains
its additional plugin/startup provenance and those limits.


Report-file interpretation correction: the actual Cartesian output contains
**76 numeric rows and four header rows** because the twenty explicit Report
commands include repeated targeter trial/final passes. `Report::Execute` calls
`ReportFile::WriteData` directly; the `SolverIterations=None` filtering in
`ReportFile::Distribute` does not suppress those command writes. The qualified
script's line 184 comment suggesting trial suppression is inaccurate and is
recorded here as an erratum; qualified script bytes remain unchanged. Use the
separate residual report's single numeric row for endpoint acceptance. Neither
this correction nor the extra Cartesian trial rows require a numerical rerun.
