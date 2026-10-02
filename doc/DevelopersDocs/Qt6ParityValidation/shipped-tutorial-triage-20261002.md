# Help tutorial qualification and repairs

Snapshot: manifest inventory `2026-10-02T06:47:39.014581+00:00` in [manifest.json](/tmp/gmat-shipped-examples/manifest.json). Scope is all 19 scripts in `application/docs/help/files/scripts`, under the selected Linux Qt runtime. They contain complete standalone missions; none is an include fragment. Multiple-shooting files represent successive complete tutorial stages.

Final observed outcomes: **10 build passes (9 completed runs and 1 deliberate runtime Stop), plus 9 build failures**. The latter is not a completed mission or a pass. Original first attempts and subsequent attempts remain under the same evidence tree. Offscreen build/execution confirms executable workflow only; it does not establish native viewer qualification or scientific correctness of all reported trajectories.

| Tutorial | Latest observed outcome | Exact latest artifact directories |
|---|---|---|
| `Ex_AlgebraicOptimization.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Ex_AlgebraicOptimization.script/build/attempt-01) / not run |
| `Ex_LowEarthMinFuelTransfer.script` | Build failed: MATLAB/Fmincon + legacy script errors | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Ex_LowEarthMinFuelTransfer.script/build/attempt-01) / not run |
| `Ex_MinFuelLunarCapture.script` | Build failed: MATLAB/Fmincon + legacy script errors | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Ex_MinFuelLunarCapture.script/build/attempt-01) / not run |
| `ForceModelsTutorial.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__ForceModelsTutorial.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__ForceModelsTutorial.script/run/attempt-01) |
| `FormationRendezvousTutorial.script` | Build failed: wrong formation content + legacy script errors | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__FormationRendezvousTutorial.script/build/attempt-01) / not run |
| `LEOStationKeepingTutorial.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LEOStationKeepingTutorial.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LEOStationKeepingTutorial.script/run/attempt-01) |
| `LunarTransferToL2-tutorial.script` | Build passed; run deliberately stopped (unbracketed inputs) | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferToL2-tutorial.script/build/attempt-02) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferToL2-tutorial.script/run/attempt-01) |
| `LunarTransferTutorial.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferTutorial.script/build/attempt-02) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferTutorial.script/run/attempt-01) |
| `MarsBPlaneTutorial.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__MarsBPlaneTutorial.script/build/attempt-02) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__MarsBPlaneTutorial.script/run/attempt-01) |
| `ReportTutorial.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__ReportTutorial.script/build/attempt-03) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__ReportTutorial.script/run/attempt-01) |
| `Tut_HohmannTransfer.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_HohmannTransfer.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_HohmannTransfer.script/run/attempt-01) |
| `Tut_Mars_B_Plane_Targeting.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_Mars_B_Plane_Targeting.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_Mars_B_Plane_Targeting.script/run/attempt-01) |
| `Tut_MultipleShootingTutorial_Step1.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step1.script/build/attempt-01) / not run |
| `Tut_MultipleShootingTutorial_Step2.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step2.script/build/attempt-01) / not run |
| `Tut_MultipleShootingTutorial_Step3.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step3.script/build/attempt-01) / not run |
| `Tut_MultipleShootingTutorial_Step4.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step4.script/build/attempt-01) / not run |
| `Tut_MultipleShootingTutorial_Step5.script` | Build failed: VF13ad absent | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step5.script/build/attempt-01) / not run |
| `Tut_Target_Finite_Burn_to_Raise_Apogee.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_Target_Finite_Burn_to_Raise_Apogee.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_Target_Finite_Burn_to_Raise_Apogee.script/run/attempt-01) |
| `Tut_UsingGMATFunctions.script` | Build + run passed | [build](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_UsingGMATFunctions.script/build/attempt-01) / [run](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__Tut_UsingGMATFunctions.script/run/attempt-01) |

Each artifact directory contains `stderr.txt` and `output/GmatLog.txt` (and generated reports where applicable). These links point to the actual attempts, not planned retries.

## Confirmed repairs

Tracked script sources are in `/home/dan/GIT/GMAT-Qt/doc/help/src/files/scripts`; repaired bytes were copied into the corresponding `application/docs/help/files/scripts` files. LunarTransfer, L2, MarsBPlane and Report received only confirmed compatibility repairs: obsolete built-in coordinate declarations/settings were removed where present; unsupported custom `UpdateInterval`/`OverrideOriginInterval` and the L2 BodyInertial `Epoch` were removed; an explicit `BeginMissionSequence` was inserted before the first intended runtime command. Report also lost its unused configuration self-assignment and its exact heading text was quoted as `'GMAT Report File in Time X Y Z VX VY VZ format'` after the next build exposed that invalid math expression.

All spacecraft initial states, equations, command order, propagation and targeting settings were preserved. Existing comments and report filenames were preserved. The runner provisioned expected nested `output/` and `output/SampleMissions/` directories inside each isolated output root; scripts were not edited to redirect reports. The lunar download XML link now uses singular `LunarTransferTutorial.script`; the report tutorial XML programlisting now uses the matching quoted heading.

## Remaining build failures

Six failures require the actual optional **VF13ad** plugin: Ex_AlgebraicOptimization and MultipleShooting Steps 1–5. Their logs identify unknown VF13ad; subsequent nonexistent-solver messages cascade. No optimizer substitution was made.

Two scripts require the deferred **MATLAB/FminconOptimizer** dependency and also contain independent legacy errors. Ex_LowEarthMinFuelTransfer redeclares/modifies built-in frames and lacks the mission boundary before runtime assignments. Ex_MinFuelLunarCapture sets unsupported EarthSunRot Epoch/UpdateInterval and likewise lacks the boundary. Both need their existing report-path parents. These two scripts were left unchanged; adding a dependency alone would not repair the independent errors.

**FormationRendezvousTutorial is the wrong bundled mission.** Its original bytes are identical to the original LunarTransferTutorial (SHA256 `e070a6e169cd03e092bc27a5876291ebb1ad681ee1798f53669662d5883d89a4`). It contains the lunar Sat/InitSat mission, whereas the formation tutorial calls for Sat1–4 with Sat1LVLH/Sat2VNB and rendezvous targeting. Its actual source remains unchanged. A correct formation mission matching the tutorial is required; modernizing this lunar duplicate would not demonstrate formation capability.

## L2 deliberately stops on the supplied numerical bracket

The actual run has no remaining syntax exception. Its initial orientation target converged in 19 iterations, followed by three B-plane targets that each converged in 10. The existing c==3 guard then executed `Stop` because FLow and FHigh have the same sign. Source lines 456–459 in the repaired L2 script retain `Prod = FLow*FHigh; If Prod > 0; Stop;`. The help tutorial explicitly describes that safety guard and instructs changing the guesses if they do not bound a solution.

Actual [report artifact](/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferToL2-tutorial.script/run/attempt-01/output/output/SampleMissions/Ex_FindL2TransferWithLunar.report):

| BdotT input | EnergyError | Sat.Energy | DesiredEnergy |
|---|---|---|---|
| 10000 (LB) | +0.356311052242573 | +0.1486217012977354 | -0.2076893509448376 |
| 11000 (UB) | +0.2860377299423941 | +0.07834837899755648 | -0.2076893509448376 |

The retained inputs are `LB=10000`, `UB=11000`, `DesiredEnergy=-0.2076893509448376`. Completion requires scientifically reviewed bracketing inputs whose resulting energy errors have opposite signs (or a zero endpoint), while keeping the desired mission energy and other settings. No valid replacement bracket has been established; none was guessed or substituted. The log's “stopped by user” wording is the engine's rendering of the Stop command, not evidence of a human stopping this run. Do not repeatedly rerun these unchanged inputs expecting completion.

## Generated Help mirror check

The exact lunar link and report heading were sought in all 250 current generated `application/docs/help/html/*.html` files, including the possible corresponding chapter names. No generated page contains `LunarTransferTutorial` or `stringVar`, so neither source correction had an existing HTML literal to mirror. No HTML pages were created or rebuilt. This is an absent generated chapter, not a stale corrected literal. The corrected lunar download target was verified present in both `doc/help/src/files/scripts/LunarTransferTutorial.script` and `application/docs/help/files/scripts/LunarTransferTutorial.script`. The XML heading now matches the quoted text in both source and runtime ReportTutorial exactly.
