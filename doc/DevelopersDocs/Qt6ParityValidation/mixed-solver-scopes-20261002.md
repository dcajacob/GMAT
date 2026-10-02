# Mixed optimizer/targeter display scopes — 2026-10-02

## Failure and correction

A new bounded hierarchy puts an accepted DifferentialCorrector target inside an
unaccepted outer optimizer trial. The first Yukon run exposed trial leakage:
Current retained 96 Orbit / 15 Ground / 15 XY samples, while None retained 96 in
all three. At elapsed 30 s, None published Alpha=1 although the independently
known accepted outer optimum is Alpha=2. Raw first failure is retained.

Optimize changed the publisher run state directly in five internal and external
paths but did not update its own currentRunState. The nested plot-scope stack
therefore classified the outer optimizer as RUNNING. The correction updates
that member immediately before each existing publisher transition. It does not
propagate a different state into child commands or change numerical execution.

The same new driver covers Yukon and the installed free R2026a VF13ad component.
Current now retains 18 Orbit / 15 Ground / 15 XY samples; None retains 15 in each.
Ground/XY accepted paths agree; Current Orbit can retain its established solver
boundary duplicates. No old solver-mode matrix was repeated.

## Executable evidence

QtGui.MixedSolverScopes passes **0.99 s**; QtGui.MixedVF13adSolverScopes passes
**1.03 s**. Each checks:

- The independent quadratic Alpha=2 optimum, Beta=3 inner goal and final elapsed
  100 s, with complete ten-column iteration Reports (phase, Alpha/Beta/cost,
  elapsed, Cartesian XYZ and geodetic longitude/latitude).
- Every accepted None Alpha/Beta sample after the corresponding solve; Current
  versus None accepted Ground/XY paths; final Orbit/Ground/XY endpoints and camera
  target against the independent Report.
- Authored Unicode mission Save/reopen byte equality; exact reports across display
  modes; stopping at the first inner propagation debugger breakpoint, stopped
  classification, debugger/plot-scope cleanup, unlocked UI, then complete recovery
  with exact original source, report and retained display paths.

Yukon produces 35 Report rows; VF13ad produces 26. The full Yukon report before
and after the production correction is byte-identical, SHA256
`a983fce6054685bfea65c16cd3271e07c1cb66a9c1e39639025de756e5a5d8c6`.
Within each optimizer, Current and None reports are byte-identical. Final Yukon
Alpha=1.9999994999999999 / Beta=3 / cost=2.5000000006988898e-13; VF13ad Alpha=2 /
Beta=3 / cost=0. Fixed-step epochs produce elapsed 99.999999802093953 s, within the
fixture's stated 1e-5 s tolerance. Display path comparison uses 1e-8 degree/km and
1e-10 days; reports retain full 17-digit precision. NutationUpdateInterval=0 is
a fixture setting to align independent rotating-frame report and plot paths.

The first fixed attempt passed the numerical/path checks but failed the test's
Stop-file assumption: the breakpoint precedes the first Report command, so no
partial file exists after ReportFile begins a fresh run. The driver now explicitly
requires that absence; completed runs still require the full report. Both original
product failure and that fixture failure remain preserved.

Read-only peer review found no substantive production regression. It identified
an initially weak Stop proof; the final check now requires both commandIndex=9
and the exact inner propagation label before stopping. Both boundary files record
the actual expected command. It also established that VF13ad derives
InternalOptimizer: being an externally supplied plugin does not exercise
Optimize::RunExternalSolver. The driver explicitly asserts IsSolverInternal for
both fixtures. The three RunExternalSolver state assignments are source-reviewed,
not execution-qualified; external MATLAB/SNOPT paths remain deferred/unavailable.

## Artifacts, identity and limits

Raw attempts and final captures are under ignored
`build/example-qualification/20261002/mixed-solver/{first-failed,fixed-first-stop-fixture-failed,final-passed,boundary-verified}`.
Boundary-verified includes original authored sources, Current/None/recovered histories,
offscreen canvas captures, final CTest/LastTest logs and report-and-runtime-identity.json.
The C++ driver enters its own generated fixture through the script widget; this is
a focused engine/UI integration test, not an actual-input Help tutorial.

The actual application/bin/GmatQt and shared core were rebuilt. App SHA256
`c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3`;
core `68a79c56f22c22407ba93f462e73fad4c4050d0cc9f54b0203b79f0d471085c3`;
selected startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
The app hash alone does not identify a dynamically linked core change. VF13ad's
local binary/license remain ignored and are not committed.

This closes the reproduced mixed scope leak and bounded inner Stop/recovery in
two internal optimizer implementations. Wider solver hierarchies, backward camera
regimes, named-arc identity/eviction and full native desktop acceptance remain
open. All independent Help tutorial constructions remain Pending.
