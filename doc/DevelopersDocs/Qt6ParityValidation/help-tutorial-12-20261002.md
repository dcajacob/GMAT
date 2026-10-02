# Help tutorial 12: simulate and estimate inter-spacecraft tracking

**Passed for the bounded private actual-input GUI workflow**, with a separately verified correction to the Help/sample tracking frame. The original independent construction remains an unchanged record of the old Help inputs. Those inputs executed successfully but did not describe the advertised geosynchronous orbit. The shared residual-plot failure, its affected retry, and the later scientific input repair are distinguished below.

The published chapter is `Tut_Simulate_and_Estimate_Inter_Spacecraft_Tracking.xml`, “Simulate and Estimate Inter-Spacecraft Tracking” (legacy Help index 16). Its prerequisite is familiarity with Basic Mission Design; no prerequisite mission file was loaded. The chapter explicitly teaches script-editor construction. This is actual keyboard/editor evidence, rather than resource-form construction of every object.

## Independent construction and preserved first failure

The application started with `script:null`. Actual Welcome → Tutorials opened the chapter, then New mission and Ctrl+A/Backspace produced an empty editor. Ten Help construction listings were entered using printable XTest keyboard text and Enter keys. No generated mission file was opened and no engine object/model setter populated the scenario. A separate namespace-aware XML check confirmed that each entered block was the literal original Help programlisting; the saved source is their exact concatenation with separator newlines.

The scenario defines four simulation/estimation spacecraft, RF hardware, Range/RangeRate TrackingFileSets, two noise models, Earth point-mass forces, RungeKutta56 propagation, Simulator sim and BatchEstimator bat. The ordered mission is BeginMissionSequence → RunSimulator sim → RunEstimator bat. Settings followed Help and AddNoise stayed On. The early absent Transponder1 reference is later replaced by HGA; the full literal sequence built and ran. No incomplete intermediate build is inferred.

Actual Save As produced the [unchanged independently authored source](help-tutorial-12-authored-20261002.script): **5,478 bytes**, SHA256 `43e4aa2ef3d96f37b98dc081b3a7c88abf754e1e86f29d88e9e347698556acaf`. Actual F5 completed in **12.392 s**, with estimation converged in **three iterations**, 1,613/1,618 observations used and WRMS 0.965313320914. Required residual windows nevertheless showed legends and empty default axes. Actual Range Export data saved a 97-byte file containing headings and **zero samples**. Successful engine execution did not qualify the required plotting workflow, so this initial checkpoint was Partial.

Actual Ctrl+O reopened the saved own source and Build succeeded. Simulator/estimator readback confirmed data sets, Prop, UTC June 10→11, step 60, Noise On, maximum ten iterations and ShowAllResiduals On; the mission tree retained the ordered commands. No Apply or source edit followed Save.

Initial multiline helper requests and an oversized action batch were rejected before text entry. Screenshots 005/006 have premature labels over a blank editor; 008/actions.jsonl show successful entry. Screenshot 014 captured a collapsed category, with actual simulator readback in 015. These operator corrections remain in raw evidence.

## One affected plot retry, reopen and final freeze

The shared [residual plot repair](owned-residual-plot-20261002.md) separated redraw suspension from data admission during OwnedPlot bulk replacement. Ordinary subscriber Toggle behavior stays separate; its focused QtGui.OwnedPlotBatch result was not repeated here.

After the coordinated application rebuild, one affected retry opened the **same unchanged own source** through Ctrl+O and ran F5. It completed in **4.425 s**, converged in three iterations, and visibly populated both required plots. Actual Export data yielded **806 Range points and 804 RangeRate points**. Independent read-only checks reconciled every point to an unedited observation in final residual iteration 2: no duplicates, extra points or unmatched epochs. Residual values equal printed Observed − Computed within half the report’s six-decimal unit, **0.5 × 10⁻⁶ km or km/s**. Each exported epoch exactly equals the parsed double of GMD TAI MJD + `0.0343817/86400`, the known A1 offset; maximum decimal-text conversion difference is 0.1704 µs.

The retry contains 1,618 observations at 809 paired epochs, 1,610 accepted and eight OLSE edits. With random noise On, edited counts/report bytes need not match the earlier realization. Schema, IDs, pairing, band 2, ten-second Doppler interval and sixty-second grid were checked. Fifteen interior gaps cover 619 missing minute epochs; their geometric cause is unproved.

Actual Ctrl+O reopened the unchanged source after plots/exports; Build succeeded. Own source/results/actions/runtime identities were frozen at **2026-10-02T21:01:07.402213Z**, before first reference access. Construction/readback were not repeated. A noninteractive-stdin launch ended during initialization without input, source load or F5. One redundant export dialog was cancelled; both corrections are retained.

## Post-freeze comparison and the original input limit

Only after final freeze was `application/samples/Tut_Inter_Spacecraft_Tracking.script` read. All **126 operational statements** match the frozen own mission, normalizing only insignificant whitespace outside quotes and numeric `10` versus `10.` for the Doppler interval. Original sample/Help bytes were preserved before repair. No original-reference GUI rerun or reference-seeded construction occurred.

Both original Help and sample assign the tracking spacecraft to EarthMJ2000Eq with:

- Position: `[-36517.051189, -21083.129334, 0]` km.
- Velocity: `[-0.005964, 0.010330, 0.267968]` km/s.

The initial radius is 42,166.258668 km but the inertial speed is only 0.268233345 km/s. Independent two-body arithmetic with the report’s Earth μ = 398600.4415 km³/s² gives perigee radius **161.081 km**, inside Earth. The original simulation’s Range reaches 14,120,317.346 km and RangeRate 215.180 km/s. Literal Help fidelity and estimator convergence therefore do not establish a physically credible geosynchronous scenario.

The EarthFixed interpretation is a **source-backed inference about the intended frame**, not a recovered historical original or a proven explanation of how the mistake arose. The Help describes a geosynchronous tracking spacecraft. GMAT’s EarthFixed declaration uses Earth origin and BodyFixedAxes (`src/base/executive/Moderator.cpp:8555–8575`); Earth defaults to FK5/IAU1980 (`src/base/solarsys/Planet.cpp:111`). The conversion includes `vI = Rdot rF + R vF` (`src/base/coordsystem/AxisSystem.cpp:1548–1557`), with the rotation/derivative construction in `BodyFixedAxes.cpp:729–769`, Earth rotation/LOD in `AxisSystem.cpp:2884`, and negative-angle polar-motion convention in `AxisSystem.cpp:2940–2946`.

Independent orbital invariants included the June 10, 2010 EOP row from `application/data/planetary_coeff/eopc04_08.62-now:17707`: xp −0.003783 arcsec, yp 0.466031 arcsec and LOD 0.0002437 s. Applying the retained state as EarthFixed yields SMA **42166.24166491408 km**, eccentricity **4.0331728446556 × 10⁻⁷**, perigee **42166.22465854 km**, and period **86170.4420523 s**. Inclination to the spin equator is approximately **5.000007°**. The period exceeds the corresponding sidereal rotation by about 6.343 s, so the description is **near-geosynchronous**, rather than exact geosynchronous or geostationary.

## Narrow repaired input and one changed-reference verification

The approved sample/Help repair changes only **SimTrackSat.CoordinateSystem and EstTrackSat.CoordinateSystem to EarthFixed**, plus frame/near-geosynchronous clarification. State numbers, observed-spacecraft frames, epochs, forces, hardware, noise, solver/tolerances and mission logic are preserved. The frozen own mission stays untouched.

One actual changed-reference GUI session opened the corrected sample through Ctrl+O. Both tracker panels read back EarthFixed. Browsing the simulation tracker’s EarthMJ2000Eq/Keplerian preview showed SMA **42166.24166491408 km**, eccentricity **4.0331728437629 × 10⁻⁷** and MJ2000-equator inclination **5.057065200895819°**. The SMA/eccentricity independently reproduce the frame-conversion invariants above; MJ2000 inclination is distinct from spin-equator inclination. Preview changes were explicitly **Discarded**, with no Apply or source Save.

The **single corrected F5** completed in **3.406 s**, converging in **two iterations**. Both residual plots contained visible data. The complete report reaches END OF REPORT, uses **1,646/1,652 observations**, removes six OLSE records, and reports WRMS **0.990936275466**, predicted WRMS **0.990784265763**, with the relative criterion met. All 36 Cartesian covariance entries are finite with positive diagonal; this observation does not establish positive definiteness or statistical consistency.

The corrected GMD has **826 paired epochs** on the sixty-second grid, spanning June 10 00:14 through June 11 00:00. Fifteen interior gaps cover 601 missing minute epochs, plus fourteen initially absent epochs. Range is **70,190.486206–86,607.742539 km** and RangeRate **−15.043764799–15.037172008 km/s**. The original extreme growth is absent. This is bounded output plausibility and initial-orbit evidence, rather than comprehensive scientific validation.

Actual exports contain **824 Range points and 822 RangeRate points**. Read-only reconciliation matches every unedited final residual iteration 1 row, with the same report-rounding and exact double epoch checks used for the own retry. The corrected sample’s disk bytes remain unchanged after GUI readback/run; no reference Save occurred. The original frozen own source remains byte-exact. The verification checkpoint is **2026-10-02T21:18:24.780374Z**.

Afterward, demonstrative Help output was refreshed from that single run: first four GMD rows/corresponding dates/values, near-geosynchronous wording and tracker frame in the initial-condition report excerpt. Construction inputs stayed unchanged; printed noisy values are not exact acceptance thresholds. Original excerpts remain preserved.

The single `build-qt-help` build completed with exit 0 at **2026-10-02T21:31:25.812008Z**, installing the repaired offline Help. Read-only installed-content checks at **21:32:17.193642Z** confirm both EarthFixed inputs, the verified 00:14 measurement excerpt and the estimator-report tracking frame. Construction/output HTML chunks are 11,343/13,027 bytes; full installed Help is 4,256,562 bytes. Exact identities and build log are retained in the supplemental metadata. The first checker used nonexistent separate subsection filenames; correcting the lookup to the actual combined DocBook output required no repeated build or mission. This is installed-content consistency, not another tutorial run.

## Identity, retained evidence and limits

Original construction used app `e5337acddb900804aec9d327dd3e689b67995b2c30c1e35d9cc49421d5900f87`. The affected retry and corrected-reference verification used app `b8074b0222cbd4d3a4c67cacf37262181aeb94d13b3ebd3ed2ea061b8ba8dad5`. All three retain core `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` and original startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`. Per-launch util/helper identities and stat information are in the raw prelaunch records. Private startup copies change OUTPUT_PATH only.

The corrected sample is **6,780 bytes**, SHA256 `dcb5b40553f5f4c8ed4083bf13a08623b162f8c5f3fa0ec95c72ef2a69605300`; final Help is **36,981 bytes**, SHA256 `07777b2d99820b235e2d2150b16a89315cc072ba52cb10dc6d7c18003d8f729d`. Complete output/export identities are in machine records.

All three actual-input sessions closed through Alt+F4, with application/window-manager/Xvfb exits **0/0/0**. The helper CLI returned 1 because its loop treats an owned application exit, including zero, as fatal; raw behavior is retained. The initialization-only EOF attempt instead records app −15. No numerical Stop or timeout is inferred for the three completed runs.

A genuine transient `/tmp` capacity exhaustion was observed around 21:09: parent escalated df recorded 31G used, 100%, zero available, and a sandbox setup failed with “No space left on device.” A later escalated df showed 23G available. The source of that release is unknown; this agent performed no cleanup or process changes. The later free-space observation does not disprove the earlier exhaustion, and no truncated result is accepted here.

Indexed raw screenshots/actions/output/freezes/source snapshots/comparisons/failures/identities/proposal are retained under `build/example-qualification/20261002/help-tutorials/12-inter-spacecraft-tracking`. Historical temporary paths remain unchanged. No proprietary input was needed. The Help’s prose says two days, but authoring follows its June 10→11 listing.

This record qualifies the chapter’s bounded actual Qt editor/input, simulator/estimator, required plot/export and save/reopen workflow. It distinguishes the literal old-input limitation from a scientifically appropriate reviewed repair. It does not certify every estimation regime, the host GNOME/Wayland session, portal behavior, physical GPU drivers, the prior desktop crash fix, or complete Linux replacement acceptance. Windows, macOS and MATLAB remain deferred.

## Ten unchanged screenshots

- [Actual Help chapter](help-tutorial-12-help-20261002.png).
- [Independent blank editor](help-tutorial-12-blank-editor-20261002.png).
- [Successful Help keyboard entry](help-tutorial-12-keyboard-entry-20261002.png).
- [Preserved first blank-residual failure](help-tutorial-12-initial-blank-residuals-20261002.png).
- [Own saved source reopened and built](help-tutorial-12-own-reopen-20261002.png).
- [One affected own run with retained residuals](help-tutorial-12-own-populated-residuals-20261002.png).
- [Unchanged own source reopened after plots](help-tutorial-12-own-final-reopen-20261002.png).
- [Corrected tracker EarthFixed readback](help-tutorial-12-corrected-frame-20261002.png).
- [Corrected inertial Keplerian preview](help-tutorial-12-corrected-kepler-preview-20261002.png).
- [Single corrected sample run and residuals](help-tutorial-12-corrected-residuals-20261002.png).
