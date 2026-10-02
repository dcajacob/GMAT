# L2 tutorial initial energy bracket repair

Current disposition: the opposite-sign initial bracket is established, but the
repaired original mission still fails during an intermediate geometric event
wait. This tutorial is not an end-to-end numerical pass.

The tutorial's chapter (`doc/help/src/Tut_L2Transfer.xml`, Other Equations and
While Loop Section 2) requires opposite signs at the initial B·T bounds and
instructs changing the bounds when the original interval does not contain the
solution. The target remains `DesiredEnergy = -0.2076893509448376` km²/s².
`Sat.Energy` is the original Earth-centered orbital energy parameter
(`src/base/parameter/OrbitalParameters.cpp`, Energy constructor).

The preserved original run report at
`/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferToL2-tutorial.script/run/attempt-01/output/output/SampleMissions/Ex_FindL2TransferWithLunar.report`
gives:

| B·T (km) | EnergyError (km²/s²) | Sat.Energy (km²/s²) |
| --- | --- | --- |
| 10000 | +0.356311052242573 | +0.1486217012977354 |
| 11000 | +0.2860377299423941 | +0.07834837899755648 |
| 16000 | −0.1482905554907792 | −0.3559799064356168 |

The first two rows are the already completed original attempt; their same signs
correctly cause the retained `If Prod > 0; Stop;` guard to terminate the mission.
Only the new 16000 km endpoint was evaluated separately, offscreen, in 5.1 seconds.
The temporary script changes LB/UB to that one endpoint, limits the existing loop
to one iteration, and stops the mission after EndWhile; the original first
RAAN/AOP target, B-plane target, propagations, and EnergyError equations are
unchanged. Raw logs, report, executable/core inode hashes, and exact source diff
are retained under `/tmp/gmat-l2-bracket/endpoint-16000`; the consolidated result
is `/tmp/gmat-l2-bracket/endpoint-16000-result.json`.

Both source and installed tutorial script copies now change only the initial
UB from 11000 to 16000, plus an explanatory comment. LB stays 10000. The chapter's
Equation19 and accompanying description match. The bracket guard, DesiredEnergy,
force models, state, targets, propagation stops, arithmetic, tolerance, counter
limit, and second target remain unchanged. The endpoint signs establish the
initial bracket. The parent's repaired full-example run subsequently failed as
recorded below, so bisection completion and the final L2 target remain unqualified.
This is example-input repair, not numerical engine or scientific qualification.

Endpoint evaluation context:

- Original source SHA256: `0cd3aa16d8a40967f1bbc3646ca0b38148581ac2a4aa2c9d80398e773538fad5`
- Probe source SHA256: `361c5781e61a05bc97ccf7b2bf2ac10a8fbeac31fd154515af8dc7a2bedc5138`
- Qt executable SHA256: `45f4245e214b9f15c746fdd7ef2c381df9231c659e601a4536b4adfe259fb4c9`
- libGmatBase.so.R2026a: `febb403ef90c4ab4825c8849abe9199ab98c719491a128fe8cd138b3b30f8e02`
- libGmatUtil.so.R2026a: `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`
- Source HEAD at evaluation: `ca52d977e12a4271e92f845541e50da2a6c82005`
- Evaluation UTC: `2026-10-02T07:11:44.194020+00:00`

## Repaired original run and bounded diagnostic

The current corpus result is the original tutorial run at
`/tmp/gmat-shipped-examples/scripts/application__docs__help__files__scripts__LunarTransferToL2-tutorial.script/run/attempt-02`:
exit 1 after 79.261 seconds, with source unchanged and SHA256
`f1e211c7c58edc0a693ae9081cbc83f0829eb800953b339b7debdc8786ef5e26`.
The initial guard passed and the B-plane target for B·T = 15109.375 km converged
in nine iterations. The next energy row was never written. The actual exception
requests A1 epoch 95008.539427940 beyond the bundled DE405 file's coverage.
The final P2/L2 velocity target was never entered.

This source uses no Earth Apoapsis event in the energy loop. Its first
post-target propagation waits only for ESML2Centered.X = ±500000 km, with no
time fallback; the second waits for another 160 days or X = ±1000000 km.
Earlier completed energy rows are nonmonotonic:

| B·T (km) | EnergyError (km²/s²) |
| --- | --- |
| 13000 | +0.224317859595919 |
| 14500 | +0.3438504037608082 |
| 14875 | +0.1494559300984878 |
| 15062.5 | +0.2222107481408793 |
| 15156.25 | −0.1615527382757742 |

Only one further endpoint was evaluated, at the failing B·T = 15109.375 km.
The temporary script preserves the targeting, forces, X events, energy equation
and bracket guard, adds a diagnostic 160-day fallback to the first geometric-only
wait, writes pre/post phase reports, and stops after this one energy evaluation.
It completed in 4.544 seconds; this instrumented success is not a pass of
the shipped mission. Its measured states are:

| Phase | A1ModJulian | Earth radius (km) | L2-centered X (km) | Earth Energy (km²/s²) |
| --- | --- | --- | --- | --- |
| After P1 target | 28028.622259363525 | 384289.9650647635 | −1237775.1915570735 | −0.56995808267217907 |
| First leg, exactly 160 days later | 28188.622259363525 | 497442.66989185777 | −1066727.1232588771 | −0.43928016896777827 |
| Second bounded leg, another 160 days | 28348.622259363521 | 415333.01245697867 | −1107107.0526647798 | −0.45870541122065811 |

The intermediate bound Earth-return trajectory has not reached the intended
±500000 km L2 entry planes on this timescale. The unbounded geometric leg permits
the multi-decade propagation leading to ephemeris exhaustion. Initial opposite
signs are necessary but do not establish reachability or continuity of this
event-defined energy evaluation across the interval. A production time cap would
change where energy is evaluated and would not by itself establish the intended
L2 transfer or final target. Extending ephemeris coverage, removing events or the
guard, or declaring this instrumented run a tutorial pass is not justified.

No further endpoint search, force/algorithm change, or additional shipped-script
change was made. Retain the valid initial bounds and guard; classify this as an
unresolved shipped numerical transfer/event-domain failure, not a missing optional
dependency or public-data waiver. Transfer input/branch analysis remains necessary
before another original end-to-end run is warranted.

The diagnostic's full child/component inode provenance, phase report, exact source
diff and logs are at `/tmp/gmat-l2-bracket/endpoint-15109.375-bounded-result.json`
and `/tmp/gmat-l2-bracket/endpoint-15109.375-bounded/`. The temporary source diff is
`/tmp/gmat-l2-bracket/endpoint-15109.375-bounded-source.diff`. Original source hash
was retained unchanged through the diagnostic.
