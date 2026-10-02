# Help Tutorial 9 — Simulate DSN Range and Doppler Data

**Passed for bounded private actual-input Help construction**, both required simulation stages, output identity/metadata inspection, own save/reopen/freeze and post-freeze source/settings comparison. The independently generated full observations are retained for Tutorial 10. This is the Help's prescribed script-editor workflow; no form-based navigation construction, host Wayland/hardware qualification or complete scientific numerical equivalence is claimed.

The full `Tut_Simulate_DSN_Range_and_Doppler_Data.xml` chapter, including ramp rows, tables and equations, was read first. Welcome → Tutorials → Find “Simulate DSN” → chapter was followed through actual Qt input. No Tutorial 9 sample, sample ramp or Tutorial 10 reference was read before the own freeze at **2026-10-02T20:21:58.590173 UTC**. An empty **New script window** was created through File; each Help block was typed line by line and saved/built before the next block. The helper rejects multiline text, so the initial rejected input and empty-script validation are retained as harness/operator corrections; subsequent printable lines plus Enter worked. Numeric settings and model objectives were not altered.

## Independent construction and required runs

The short mission builds SunMJ2000Eq, Sat's specified Sun-centered Cartesian state/epoch/ID, HGA/transponder and station electronics, CAN, DSNrange/DSNdoppler, DSNsimData, the Sun force model/Prop and Simulator. Every staged Save and build script action succeeds; Ctrl+Shift+F5 completes the required 12-minute, 600-second-interval, noise-Off simulation in **0.081 seconds**. Own Ctrl+O reopens and builds the saved short source successfully before the session closes.

The fresh coordinated app reopens that own checkpoint. Save As creates a separate realistic mission. GDS/MAD, their error models and all six range/TCP tracking configurations build in stages. A separate inactive editor document receives the Help's three ramp rows and is saved through the actual chooser as `.rmp`; it is never built or run as a mission. Returning to the active own mission preserves the document distinction. The owned absolute ramp path is the only path substitution. The Help uses “3 weeks.rmp” in code and “Realistic GMD.rmp” in prose; one consistent **3 weeks.rmp** name is used. The output uses the Help's final **3 weeks.gmd** name under the cloned owned OUTPUT_PATH.

Later pre-mission assignments select 09 Sep 2015 00:00:00.000, noise On and 3600-second spacing; earlier short settings remain in the source. Save/Build succeeds. Actual F5 runs the full 21-day scenario once and completes in **22.963 seconds**. No repeated noisy solve was needed. Save As creates the final own authored source; actual Ctrl+O and confirmed Open reopen/build it successfully, screenshot024. Intermediate chooser navigation/completion attempts remain recorded and are not counted as successful reopens. The source, ramp and observations are then frozen before reference access.

| Observation check | Result |
| --- | --- |
| Short observations | Four: two DSN_SeqRange and two DSN_TCP |
| Short epochs (TAIMJD) | 27253.5004166666666666666665 and 27253.5073611111111111111107 |
| Short identities/metadata | CAN 22222; Sat 11111; band 2; 7.2e9 Hz; modulo 33554432; Doppler interval 10 s |
| Realistic observations | 1348 total: 674 range and 674 Doppler |
| Per type, station counts | CAN 210, GDS 233, MAD 231 |
| Distinct observed epochs | 505, spanning 27253.5004166666666666666665 through 27274.5004166666666666666665 |
| Pairing and values | Exactly one range/TCP pair at each retained station/epoch; every value finite |
| Range ramp metadata | All 674 rows agree with the three Help linear ramp records; maximum decimal difference 6e-18 Hz; checked within 0.0001 Hz |
| Range folding | Every generated range is at least zero and below modulo 33554432 |

Station visibility filters explain the counts; the unfiltered maximum is not asserted. The short numeric values differ slightly from historic Help illustrations (about 0.021 RU range, 0.00006 Hz Doppler). Neither those illustrations nor noisy observations are exact equality oracles.

## Freeze and durable evidence

The ignored complete evidence tree is `build/example-qualification/20261002/help-tutorials/09-dsn-simulation`. It contains both original private session trees, all actions/screenshots/logs/startup clones, intermediate saved stages, short and realistic observations, full Help XML and extracts, input/observation checks and post-freeze comparison. `artifact-index.json` hashes every retained raw file. The selected PNGs are unchanged originals.

| Frozen asset | Bytes | SHA256 |
| --- | ---: | --- |
| [Short authored source](help-tutorial-09-short-authored-20261002.script) | 3342 | `8edf46a743f36ce55b4108727bd89e121408e9780994d33968ddd42d809b8312` |
| [Realistic authored source](help-tutorial-09-authored-20261002.script) | 5060 | `492402ba7eb2d9978a05b3e4b1d755ef4d7d12e90a686c6c9af280ccc480b661` |
| [Own ramp](help-tutorial-09-authored-ramp-20261002.rmp) | 99 | `d91497ac999d53de9920e674a698be24adc732e1de88dd79740533bab0659bb9` |
| [Own observations for Tutorial 10](help-tutorial-09-authored-observations-20261002.gmd) | 181345 | `a66ac094dde6495afc1fe4c21226ff39be14ae4efaf0f54e9ba371666f857648` |
| Short original GMD | 577 | `ff6452fea99e7265e43115dfcd8a04954a7f51c878190b60732e7bef7fdf62e2` |

The captured authored source retains its original owned `/tmp/...` ramp path. Replaying after those original paths disappear requires pointing RampTable at a retained copy; that path adjustment is not part of the frozen artifact or a new execution claim.

Selected views: [Help chapter](help-tutorial-09-help-20261002.png), [short result](help-tutorial-09-short-result-20261002.png), [short reopen](help-tutorial-09-short-reopened-20261002.png), [ramp editor](help-tutorial-09-ramp-20261002.png), [full settings](help-tutorial-09-realistic-settings-20261002.png), [realistic result](help-tutorial-09-realistic-result-20261002.png), [final own reopen](help-tutorial-09-reopened-20261002.png).

## Post-freeze reference comparison and limits

Only after freeze were `Tut_Simulate_DSN_Range_and_Doppler_Data.script`, `Tut_Simulate_DSN_Range_and_Doppler_Data_3_weeks.script` and their sample ramp read. Explicit effective numerical assignments and ordered tracking configurations match. The short comparison covers 58 properties and the realistic comparison 83; the differences are hardware attachment order (`{SatTransponder,HGA}` in Help versus `{HGA,SatTransponder}` in samples) and the owned ramp path. Both reference the same named hardware; ramp numeric tokens match exactly despite whitespace. Default omissions, stochastic distribution and scientific fidelity are not inferred equal from that parser.

The original sample source hashes match the existing corpus ledger, which already records passing 0.976/27.393-second runs on earlier app `4522c731...`. That earlier offscreen evidence is preserved, not relabeled as a current GUI run. No unnecessary noisy reference rerun or old matrix/corpus repetition occurred; no Tutorial 10 source has yet been read. Original source/ramp hashes remain `15ef7594...`, `e0d3de45...` and `8ed74d02...`.

Each private session is bounded to 600 seconds, isolated X11 with software GL and its own settings/output clone. Short app SHA256 is `3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1`; realistic app is `ef933221b09577c14df825d718e08d31abe9a8ec5319e32c615b4359d0f39af5`, after the coordinated unrelated GmatFunction Outputs fix. The controlled base remains `cf147e23...`; startup remains `5f80be1f...`; helper remains `4232bdb0...`. OUTPUT_PATH is the only startup-clone change. Both apps are closed through actual Alt+F4; session metadata confirms Xvfb/WM/app exits **0/0/0**. The helper's process-ended diagnostic follows normal app exit. Full Linux host, Wayland/portal, hardware drivers, crash qualification and remaining tutorials remain separate; Windows/macOS/MATLAB remain deferred.
