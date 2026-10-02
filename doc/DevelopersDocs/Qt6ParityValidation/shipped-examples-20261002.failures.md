# Current shipped-example failures and timeouts — 2026-10-02

Derived from the current manifest after matching all 177 inventoried source hashes and all 311 current child-stage source identities. This reports existing results only; no build, mission, GUI session or test was executed to generate it.

There are **17 failed builds, seven failed executions and three timeouts**. Two failed executions are the explicitly assessed tutorial Step 1 Stops. The other five runtime failures remain unexpected; FalconSats is one of those five. A failed build prevents execution. No timeout is promoted to completion.

| Source | Stage | Raw status | Seconds | Qualification disposition |
| --- | --- | --- | ---: | --- |
| `application/samples/Ex_MarsGRAM2005.script` | build | failed | 0.574 | failed build; no execution |
| `application/samples/Ex_Yukon_MarsLaunchWindowAnalysis.script` | run | timeout | 7200.026 | incomplete at configured bound |
| `application/samples/NeedMatlab/Ex_CallMatlabFunctions.script` | build | failed | 0.42 | failed build; no execution |
| `application/samples/NeedMatlab/Ex_MatlabEnv.script` | build | failed | 0.418 | failed build; no execution |
| `application/samples/NeedSNOPT/Ex_AlgebraicOptimization.script` | build | failed | 0.422 | failed build; no execution |
| `application/samples/NeedSNOPT/Ex_MinFuelLunarTransfer.script` | build | failed | 0.47 | failed build; no execution |
| `application/samples/NeedSNOPT/Ex_OptFiniteBurn.script` | build | failed | 0.418 | failed build; no execution |
| `application/samples/NeedVF13ad/Ex_MarsLaunchWindowAnalysis.script` | run | timeout | 300.036 | incomplete at configured bound |
| `application/samples/NeedVF13ad/Ex_MarsPatchConic.script` | run | timeout | 300.046 | incomplete at configured bound |
| `application/samples/NeedVF13ad/Tut_MultipleShootingTutorial_Step1.script` | run | failed | 0.821 | expected_tutorial_checkpoint |
| `application/samples/OptimalControl/Ex_CelestialBodyRendezvous_Mars.script` | build | failed | 0.47 | failed build; no execution |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_BusPowerType0.script` | build | failed | 0.419 | failed build; no execution |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_PowerSupplyCurve1.script` | build | failed | 0.42 | failed build; no execution |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_ThrustType1.script` | build | failed | 0.419 | failed build; no execution |
| `application/samples/OptimalControl/Ex_EarthToMarsSOI_C3Eq0_CSALTTutorial.script` | build | failed | 0.421 | failed build; no execution |
| `application/samples/OptimalControl/Ex_IntegratedFlyby_MarsFlyby.script` | build | failed | 0.469 | failed build; no execution |
| `application/samples/OptimalControl/Ex_OCPropTest_GEO_4x4.script` | build | failed | 0.419 | failed build; no execution |
| `application/samples/OptimalControl/Ex_PCLaunch_ConstrainedC3AndDLA_EarthLaunch_EarthOrigin.script` | build | failed | 0.419 | failed build; no execution |
| `application/samples/OptimalControl/Ex_Phase_Type_ImplicitRKOrder6.script` | build | failed | 0.419 | failed build; no execution |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/FalconSats.script` | run | failed | 0.622 | historical_input_current_time_domain_failure |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/Falconsat7Contacts.script` | run | failed | 0.519 | unexpected runtime failure |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/GSFCSats.script` | run | failed | 0.72 | unexpected runtime failure |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/Starlink.script` | run | failed | 1.27 | unexpected runtime failure |
| `application/docs/help/files/scripts/Ex_LowEarthMinFuelTransfer.script` | build | failed | 0.42 | failed build; no execution |
| `application/docs/help/files/scripts/Ex_MinFuelLunarCapture.script` | build | failed | 0.42 | failed build; no execution |
| `application/docs/help/files/scripts/LunarTransferToL2-tutorial.script` | run | failed | 79.261 | unexpected runtime failure |
| `application/docs/help/files/scripts/Tut_MultipleShootingTutorial_Step1.script` | run | failed | 0.72 | expected_tutorial_checkpoint |

FalconSats resolved the exact historical input files but its unchanged `SystemTime(now)` mission failed with `SPICE(BADMECCENTRICITY)`, mean eccentricity -5.3807189001060E-03 outside [-0.001,1.0). This is consistent with stale-element/current-time extrapolation; the diagnostic does not name the failing spacecraft. Its failure is not a scientific pass, expected Stop or proprietary exception. No unchanged retry followed.

The three other remaining TLE missing-file failures still retain their raw failed stages. The separate [TLE dependency record](tle-dependency-inputs-20261002.md) distinguishes the decayed current-time Contacts scenario from outstanding public December 2019 catalogs. Legacy MarsGRAM remains a missing optional integration/data case without a proprietary-only conclusion.

Exact commands, evidence paths, output hashes and prior attempts are retained in `shipped-examples-20261002.manifest.json`; current raw diagnostic excerpts are in `shipped-examples-20261002.failures.txt`. Both expected Stops remain raw failures and are not mission completion or optimizer convergence.
