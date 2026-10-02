# Mars B-plane targeting using GMAT functions — actual-input walkthrough

Tutorial 6 passes the bounded Linux private-X11 walkthrough: independently
configured resources, an independently typed Help function, Global sharing and
a named no-output function call, targeting inside and outside the function,
required summaries, three displays, shared animation and own save/reopen.
The shipped mission/function were first compared after the final independent
freeze at **2026-10-02 20:43:38.795064 UTC**. No new reference execution occurred.

The chapter is `doc/help/src/Tut_UsingGMATFunctions.xml`. Construction began from
New Mission using actual resource and mission controls; the function editor was
used for the literal listing that the Help explicitly teaches. Intermediate
mission checkpoints came only from these controls. No shipped mission/function
seeded construction, and no model API or generated mission replaced GUI input.

## Authored sources and observations

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| [Own mission](help-tutorial-06-authored-20261002.script) | 8161 | `75b78de1e33bfc99e16318ca29dcc9a9b367b6b3ba030d06d6abc8104936ee05` |
| [Own Help-entered function](help-tutorial-06-authored-function-20261002.gmf) | 1435 | `797454748b8a85d6fd8a3e911f449e72d814848c7961c7f86f41e40c4cc8fd74` |
| [MOI summary](help-tutorial-06-moi-summary-20261002.txt) | 3796 | `0b5fb65a40c6cce70a8904a14ed6738a33f46bcf2326dc4eb6948c2ce45909aa` |
| [Achieve RMAG summary](help-tutorial-06-rmag-summary-20261002.txt) | 3270 | `1e524720828c560be0a9b9440f443ff891e9d1f844a3d19b43d56b2836a9b363` |
| [Function-call summary](help-tutorial-06-call-summary-20261002.txt) | 3526 | `abfd3486a86387bfe46bd45aa65ade3af8404dcca761f7d04730e77429068cb0` |

The final mission retains the independently saved function's original `/tmp`
FunctionPath. The archived copies preserve exact bytes; replay after cleanup
requires explicitly selecting an available function path. The archived mission
has not been re-executed with a rewritten path.

Resources cover MainTank 1718 kg/1000 kg/m³/5000 pressure/2 m³, the Help MAVEN
UTC/Keplerian state and tank attachment, Earth TCM and Mars MOI Local-VNB burns
with mass depletion, and three associated named force models/propagators.
NearEarth uses offered RK89/JGM2 8×8 with Luna/Sun/SRP; DeepSpace uses PD78 and
the nine prescribed point masses/SRP; NearMars uses PD78/Mars50c 8×8/Sun/SRP.
SunEcliptic/MarsInertial, three OrbitViews, the default DifferentialCorrector
and an initially empty rf parameter list were committed through their editors.

The function is the exact Help listing plus a trailing newline: 12 Global
objects, three elapsed-day stops 3/12/280, three TCM Vary controls, Maneuver,
Mars periapsis propagation, BdotT/BdotR Achieve controls and the ordered report.
The main mission has Global, the named no-argument CallGmatFunction, the same
seven-field Report, Mars Capture Target/Vary/Maneuver/Apoapsis/Achieve/EndTarget,
and the final one-day propagation outside that branch. The function and main
reports intentionally produce two identical rows.

## Execution, summaries and displays

The repaired full mission completed in **4.748 s** on app `ef933221...`.
One justified post-rebuild run on app `e5337acd...` completed in **4.915 s** to
capture missing required summaries and viewer observations. Both retained runs
converged in **6 iterations inside the function and 11 for Mars Capture**.
No unchanged legacy matrix, corpus or reference mission was repeated.

The exact final rf and solver data are retained with the raw evidence. At the
function's final periapsis, UTC is 21 Sep 2014 22:35:28.416. TCM elements are
0.003662304915184419, 0.006313404963982417 and
3.026652791693243e-05 km/s. BdotT is −1.845860879257089e-6 km and BdotR is
−7000.000000696478 km: both satisfy the explicit **1e-5 km** goals. Inclination
is 90.00000001360537°; the Help supplies no additional numerical inclination
tolerance. The actual CallGmatFunction summary in MarsInertial shows this state.

The MOI summary in MarsInertial records elements
**[−1.6032580312354, 0, 0] km/s**, delta-V magnitude 1.6032580312354 km/s,
Isp 300 s and mass change **−1075.9520123944 kg** from MainTank. The Achieve RMAG
summary records **12000.017391244 km**, within the explicit 12000±0.1 km goal,
and TA **179.99999914623°**. Burn, mass and TA examples in the Help differ
slightly; no bitwise equality or invented tighter tolerance is asserted.
Other summary fields were not independently scientifically qualified.

Actual EarthView, SolarSystemView and MarsView retain departure, transfer and
capture paths. Shared Start shows all three at 0%; seeking shows all at 47.6%;
Play/Pause advances all to 88.6%. MarsView was closed using its own title,
then reopened by actual Output-tree selection/Enter, retaining its curve,
objects and 88.6% position. A new window size slightly changes framing.
SolverIterations **Current** remains the offered setting because the Help
omits an explicit All selection; full light-blue trial history is unqualified.

## Failure, repair and readback evidence

The named CallGmatFunction editor exposed a real defect: clearing Outputs from
the output-assignment template after entering a quoted caption retained `=`,
producing a malformed call and an unmapped tree item. Original sources04/05
and screenshots010/015 are retained under the main-commands session. An
authorized temporary unnamed call supported the first stage only. Root fixed
CommandForm parsing, rebuilt, and QtGui.GmatCallSyntax passed in 0.94 s.
Actual template/name/clear-Outputs/Apply on the repaired app retains the quoted
caption and proper CallGmatFunction mapping. Final source08 has no workaround.

Own mission Ctrl+O/Open/Build succeeds. The function resource's Edit function
file readback shows all 34 Help lines; mission-tree readback retains the Global,
named call/report and capture/final propagation placement. Mission/function
hashes remain unchanged through both freezes and post-freeze comparison.

Preserved harness mistakes include a non-TTY immediate EOF, a views phase
ending while SaveAs remained open, invalid HOME keysym, premature input appended
to a checkpoint filename, startup-settling input and an unsupported wheel count.
The independently reconstructed/reopened checkpoints and settled screenshots
identify the corrected outcomes. Rapid summary SaveAs events temporarily
targeted a closing file picker; saved files and subsequent screenshots verify
the results. These are disclosed separately from the product defect.

All nine private sessions have `closed:true`, Xvfb/WM exit0 and application
exit−15 from controlled helper SIGTERM cleanup. This establishes owned-process
cleanup, **not graceful application exit or crash-proof behavior**. Buffered
GmatLog tails are incomplete in these sessions; solver data, completed-run
screenshots and exported summaries establish final observations.

## Post-freeze comparison and limits

The source comparison at 20:47:49.920188 UTC uses the already frozen sources.
Own and shipped function statements match after whitespace, optional semicolon,
comments and continuation normalization, including labels, order and numbers.
Main force controls, hardware, Global object set, call/report fields, targets
and final propagation match. Preserved differences include tiny represented
state digits, point-mass order, offered NAIF/default fields, owned function and
resolved gravity paths, solver/report filenames and OrbitView versus shipped
OpenFrames display configuration. Omitted/default fields are not inferred equal.

Compatible earlier corpus execution is reused: source SHA
`4d7ec5f61e91c07aa3014b461a48e931a43eb0c8b6d935acc82ac41c2fdc3f2f`,
function SHA `d3206e3f94c6254a516be6b605d59c0422c0904083c6ba734bcb02f9af313081`,
Passed **5.588 s** on earlier app `4522c731...`, with its offscreen limits.
Both recorded runs satisfy the explicit B-plane/RMAG goals, with the same epoch
and 6+11 iterations; maximum TCM component difference is 2.3263e-11 km/s and
final RMAG difference 2.1141e-7 km. No cause or scientific equality threshold
is inferred from these small differences. This is not a current reference run.

Construction spans app `3ba673f1...`, repaired call app `ef933221...` and final
observation app `e5337acd...`; controlled base `cf147e23...` and original Qt
startup `5f80be1f...` are retained. Every launch uses owned authenticated
Xvfb/Openbox/software GL/settings/runtime and an OUTPUT_PATH-only startup clone.
The complete indexed raw tree has **269 files, 163 PNGs and 43,504,496 bytes**
under `build/example-qualification/20261002/help-tutorials/06-functions-evidence`.
It preserves all checkpoints/actions/logs/failures, both freezes and comparison
metadata; selected PNGs are unchanged originals. The older status file's Pending
paragraph is retained as history, superseded by the final required-observations
freeze and subsequent comparison.

This passes the bounded Help function/Global/targeting/editor/viewer workflow.
Independent scientific validation, host GNOME/Wayland/portal/hardware and broad
crash regimes remain separate. Windows/macOS/MATLAB remain deferred.

## Selected original screenshots

- [Malformed named call](help-tutorial-06-call-defect-20261002.png) and
  [repaired named call](help-tutorial-06-call-repaired-20261002.png).
- [Completed required-observation run](help-tutorial-06-run-20261002.png).
- [MOI mass depletion](help-tutorial-06-moi-summary-20261002.png) and
  [Mars RMAG/TA](help-tutorial-06-rmag-summary-20261002.png).
- [Shared 47.6% seek](help-tutorial-06-shared-seek-20261002.png) and
  [MarsView reopened at 88.6%](help-tutorial-06-viewer-reopened-20261002.png).
- [Own mission reopened](help-tutorial-06-reopened-20261002.png) and
  [own function editor readback](help-tutorial-06-function-reopened-20261002.png).
