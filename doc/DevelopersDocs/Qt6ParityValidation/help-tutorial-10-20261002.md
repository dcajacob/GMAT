# Help tutorial10 — DSN and GN orbit estimation — 2026-10-02

**Passed for the bounded private actual-input Linux Qt workflow.** The full Help chapter and Appendices A–D were followed from an empty script editor, using the independently authored Tutorial9 observations and ramp. Required DSN estimation, per-iteration residual/report inspection, style/export, mandatory GN simulation/estimation, and own save/reopen/Build completed before any Tutorial10 sample read. MATLAB-dependent analysis remains deferred.

## Construction and source separation

The full 1579-line Help XML (`Tut_Orbit_Estimation_using_DSN_Range_and_Doppler_Data.xml`, SHA256 `bc121c0afd49a6a6c97a4aa4cbf252854a59489d8a931b0926e625e5be5e3bcf`) was read first. Eleven literal resource blocks were typed into the actual New Script editor, with Save/Build after each segment. The chapter prescribes this script-editor workflow. Absolute owned input paths replace the Help filenames; no generated mission, model call, disk-authored mission, or shipped sample seeded construction.

The immutable own Tutorial9 inputs are:

| Input | Bytes | SHA256 |
| --- | ---: | --- |
| Realistic 21-day noisy observations | 181345 | `a66ac094dde6495afc1fe4c21226ff39be14ae4efaf0f54e9ba371666f857648` |
| Three-week ramp | 99 | `d91497ac999d53de9920e674a698be24adc732e1de88dd79740533bab0659bb9` |
| Frozen own simulation source | 5060 | `492402ba7eb2d9978a05b3e4b1d755ef4d7d12e90a686c6c9af280ccc480b661` |

Own Sun-centered MJ2000Eq epoch19Aug2015 state is the prescribed simulation truth perturbed by +5/+4/+4 km, with unchanged velocity and CartesianState solve-for. Hardware/station/tracking/noise/force/propagator settings and bounded BatchEstimator limits follow the Help. MaximumIterations10 and MaxConsecutiveDivergences3 were retained.

## DSN estimation and residual repair

The original actual estimator completed **35.728 s**, producing a complete report with 1348 observations and 1344 used, two residual iterations, and WRMS `1459.977978 → .963600`, predicted `.963530`. Its relative criterion `7.25677e-5 < .0001` converged. Four final OLSE rows are retained: CAN range record892; GDS TCP records3/303/427. The rounded final position error against own frozen truth is 3.676522 km versus 7.549834 km prior; rounded velocity error is 1.171068e-6 km/s. These are realization-specific diagnostics, not scientific acceptance across all regimes.

All six original Qt residual windows were blank0–1 after completion/Fit/Local, although the estimator report was populated. Actual ExportData contained zero points and SaveImage retained the empty graph. This exposed a shared Qt receiver defect: the core bulk protocol ClearData→Deactivate→SetData→Rescale→Activate requires accepting data while drawing is paused. The minimal shared fix and focused `QtGui.OwnedPlotBatch` result are recorded separately in [owned residual evidence](owned-residual-plot-20261002.md).

One changed-path DSN retry on appb807 completed **34.357 s**, with the saved numerical source and original observations unchanged. All six residuals rendered at iteration0 and iteration1. CAN range actual text export has **209 points**, matching the report's209 accepted rows and one omitted OLSE row. Maximum residual difference from rounded report is `5.000000005e-7`; exported epochs match own GMD TAI epochs plus the primary MathSpec A1 offset `.0343817 s` exactly in this export. Actual SaveImage produces a PNG, and grid/legend toggles preserve populated data. Own Ctrl+O reopen/Build succeeded.

## Mandatory GN appendix

Through actual SaveAs and visible Edit→Replace, the owned Tutorial9 simulation and owned Tutorial10 estimator were adapted to the Help's GN directives: ratio240/221, transmitter2067.5 MHz, Range noise `.010 km`, RangeRate noise `.00001 km/s`, all six tracking configs changed, full ramp assignment and simulation modulo commented. Distinct filenames protect DSN observations and reports. The Help's multiline ramp continuation was also commented; leaving only the first line commented would be invalid syntax.

GN simulation completed **24.271 s**, yielding **1348 finite paired observations**, CAN/GDS/MAD counts210/233/231 per type, 674 station/epoch pairs. Source and generated observations were saved; own Ctrl+O reopen/Build succeeded. The full generated GN input is retained:

| Own artifact | Bytes | SHA256 |
| --- | ---: | --- |
| DSN authored estimator source | 5730 | `6c9d8aa849e42087403b4af6970465d746e6e946fde755350060a5bc4f5af382` |
| GN authored simulation source | 5029 | `8550a8a2b38de6a9d98e8b1940cdcaccdefacaec9a706f35d1ab63ac722dfa07` |
| GN observations | 140905 | `692970e405efea1f2c2985951be1ae7dc53511869428fd1dc6ccf3d8df421d13` |
| GN authored estimator source | 5646 | `2ec3c5abcad3fd1a73fdce18640f0edf3dc614b07374cc705457e3929be2029f` |

The first GN estimator GUI run completed54.573 s with six populated plots, but `/tmp` exhausted its31 GiB tmpfs during report writing. Its partial196769-B report ends at iteration0 record1123, lacks final summary/footer and has a duplicated line prefix. It is **not accepted as complete report evidence**. Exact partial report SHA`b527ebb87e303e30bbd433385515a9527ebef111860799ce8a3922353cd43c8e`, failed sandbox probes, screenshots and action history remain preserved. Storage recovered from0 to23 GiB free; the source of the large release was not established. No user process or unrelated file was removed by this agent.

One justified same-source/input GN retry then completed **53.322 s**. Complete729122-B report SHA`a3ae4c18f148c4c7de474e6969babd64c8d6e412fb60e46a613b6805de44bb91` has END OF REPORT, exactly1348 finite uniquely indexed rows in each of iterations0/1/2, and **1342 final used / six OLSE**. WRMS `431.749062 → .960039 → .959887` and relative criterion `2.19625e-7 < .0001` converged. Final accepted Range counts210/231/229 and RangeRate209/233/230 reconcile all six station/type summaries. All twelve reported Cartesian/Keplerian covariance/correlation matrices contain36 finite elements; independent positive-definiteness/science validation is not claimed.

The rounded GN final position truth error is **27.585935 km**, larger than the7.549834-km prior; velocity error is1.852404e-6 km/s. Final report position sigmas are8.888553/15.722599/16.397562 km. This noisily observed deep-space tutorial converges statistically; convergence does not establish improved truth accuracy. No numerical objective or limits were altered to force improvement.

Six GN residual plots are populated. Actual CAN RangeRate export has **209 points**, exactly the report's final accepted209/210 rows, with maximum residual difference `4.992151e-7` from six-decimal report rounding and epoch difference `3.637979e-12 day` (one double-precision ULP) from own GMD TAI plus known A1 offset. Picker timing accidentally gave this ASCII export a `.png` extension; the original is retained, with a byte-identical correctly named text copy SHA`20d29243cd13adb7cb0fb6ede94a13c3eec9def1e38946d0d99d478caea2a028`. **No GN PNG export is claimed.** DSN actual PNG export and unchanged GN full-window screenshots supply the image evidence.

## Reopen, freeze and comparison

All three authored missions were saved and actually reopened with Ctrl+O/Build. The own freeze is **21:17:24.535580 UTC**, before any Tutorial10 sample read. Original145 raw files/28336000 B are indexed in the freeze; the metadata freeze itself is retained additionally. Own Tutorial9 inputs and app/base/util/startup/Help identities are explicitly hashed. Raw actions, intermediate source/build states, original blank exports, storage-truncated report and all failed picker navigation remain preserved.

Post-freeze comparison at21:18:46.565917 UTC finds86 shared explicit properties equal. Owned input paths/noise realization, hardware attachment order, Help's shared station electronics versus sample's separate names, and legacy MatlabFile versus modern DataFile/sample-only error reports differ. Omitted/default settings are not inferred equal. The current7736-B sample SHA`1b42dd76b4964717b6b5fa1b8e3d24e4dbc0192b754a71319092e5ea23856900` matches the earlier corpus source-unchanged Build`.418 s` and run`55.659 s` on app4522/offscreen. Those earlier limits are preserved; no needless noisy reference rerun was performed. Complete property comparison and original corpus provenance accompany the raw evidence.

## Session identity and limits

Three private X11/XTest sessions were individually bounded600 s. Initial DSN used appe5337acd...; patched DSN/GN used app`b8074b0222cbd4d3a4c67cacf37262181aeb94d13b3ebd3ed2ea061b8ba8dad5`. Controlled base`cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`, util`1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc` and original startup`5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` remain unchanged; each startup clone changes only OUTPUT_PATH. All three actual Alt+F4 exits and owned Xvfb/window-manager/application cleanup are0/0/0 before deadlines. The helper's “Owned application process exited(0)” terminal diagnostic is the expected closure, not a crash.

Native Qt file-picker asynchronous/autocomplete timing caused mistaken early labels, a misnamed ASCII export, and redundant picker attempts; raw action/screenshot evidence distinguishes actual acceptance. Ctrl+H did not open Replace because the Linux action has no shortcut; visible Edit→Replace succeeded. These navigation mistakes are not reported as product defects. No host desktop input was used.

MatlabFile is accepted with a deprecation warning, but no MATLAB artifact/analysis is qualified. Windows/macOS/MATLAB, host GNOME/Wayland/portal/hardware-driver/crash gates, exact noisy reference equality, pixel equality, and scientific validation beyond this finite scenario remain separate. No broad old matrix, corpus or unrelated mission was repeated.

## Curated artifacts

[Own DSN estimator](help-tutorial-10-dsn-authored-20261002.script), [own GN simulation](help-tutorial-10-gn-simulation-authored-20261002.script), [own GN estimator](help-tutorial-10-gn-estimation-authored-20261002.script) and [full own GN observations](help-tutorial-10-gn-authored-observations-20261002.gmd) are byte-identical to GUI saves/results. Actual exported data are [DSN CAN range](help-tutorial-10-dsn-can-range-residuals-20261002.txt) and [GN CAN RangeRate text copy](help-tutorial-10-gn-can-range-rate-residuals-20261002.txt).

The unchanged screenshots show [Help chapter](help-tutorial-10-help-20261002.png), [original blank residual](help-tutorial-10-dsn-original-blank-20261002.png), [repaired iteration0](help-tutorial-10-dsn-iteration-zero-20261002.png), [repaired completion](help-tutorial-10-dsn-completed-20261002.png), [style toggles](help-tutorial-10-dsn-style-20261002.png), [DSN reopen](help-tutorial-10-dsn-reopened-20261002.png), [GN simulation completion](help-tutorial-10-gn-simulation-completed-20261002.png)/[reopen](help-tutorial-10-gn-simulation-reopened-20261002.png) and [GN estimate completion](help-tutorial-10-gn-estimation-completed-20261002.png)/[reopen](help-tutorial-10-gn-estimation-reopened-20261002.png). [Actual DSN plot PNG export](help-tutorial-10-dsn-can-range-export-20261002.png) is separate from full-window screenshots.

The full indexed original/private-session tree and post-freeze comparison are preserved in ignored `build/example-qualification/20261002/help-tutorials/10-dsn-estimation`. `artifact-index.json` maps every retained raw file's exact byte count/hash. `authored-before-reference-comparison.json` identifies the145 original frozen artifacts and external own9/app/Help inputs; post-freeze comparison metadata is added separately without changing them.
