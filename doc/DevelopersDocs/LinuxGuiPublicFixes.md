# Public-feature fixes after the Linux GUI sweep

Implemented five focused production/sample patches, tested independently and together, and backed up the production branches to the user's fork. No PR, issue or external comment has been submitted. Existing Linux GUI fixes remain in the integration checkout; `Old` and the R2026a reference source were not edited.

## Reviewable patches

Each branch starts at NASA baseline `9363e129be366520c6edb0b4079204ed60666007` and contains one production/sample commit. Tests and audit records remain on `linux-gui-integration`.

| Finding | Branch | Integration commit | Standalone commit |
|---|---|---|---|
| LGUI-017 | `pr/python-diagnostics` | `b904b645f09c` | `91953c519cf1` |
| LGUI-020 | `pr/python-return-conversion` | `c001a19dab33` | `3ffc37638fa0` |
| LGUI-020 | `pr/linux-python-extensions` | `07b3989fe6ed` | `927e7fb46bac` |
| LGUI-018 | `pr/orbitview-plane-alias` | `2791d7c1e752` | `001400757fed` |
| LGUI-021 | `pr/tle-examples` | `584c63e2538f` | `2b4b9f27c372` |

Local descriptions are in `LinuxGuiPRs/`: python-diagnostics, python-return-conversion, linux-python-extensions, orbitview-plane-alias and tle-examples. They explain reproductions, results and dependencies without requiring adoption of the full supplemental harness. Refresh the baseline and run platform checks before eventual submission.

## Changes and focused evidence

**Python diagnostics:** GMAT now includes the exception type and UTF-8 value in its error instead of a blank “Python Exception:”. Fetched and temporary references are released, and the error translated to a GMAT exception is consumed. Failed string conversion retains the original exception type with fallback text. Public signatures are unchanged. The implementation follows Python's [exception ownership and error-state rules](https://docs.python.org/3/c-api/exceptions.html).

The pre-fix diagnostic regression recorded nine failures; the fixed case passes. Missing modules/functions, ValueError with Unicode and percent characters, failed `__str__`, and a missing exception value are covered. The real GUI test alternates errors and successful commands in one session, checks the actual dialog and preserves the full log. No manual Python error clearing is performed by that GUI test.

**Python conversion:** the vector path previously queried a scalar with `PyList_Size`, leaving a pending SystemError. Two consecutive conversions reproduce the error-state failure. The replacement checks types before list access, accepts mixed integer/float vectors and rectangular matrices, preserves scalar/string/multiple-output behavior, and rejects empty, empty-row, ragged, nonnumeric and overflowing values. The owned return reference is released on failure. All eleven focused conversion modes pass; the GUI test also covers repeated multiple outputs and successful commands after malformed arrays.

**Linux NumPy loading:** PythonInterface locates its linked interpreter library with `dladdr`, promotes that already-loaded library using `RTLD_NOLOAD | RTLD_GLOBAL | RTLD_NOW`, and retains one handle for interpreter lifetime. Loader failures are reported as GMAT interface errors. The change and dynamic-loader link dependency are Linux-guarded; the general plugin loader, startup settings and Python paths are unchanged. This uses Linux's documented [promotion mechanism](https://man7.org/linux/man-pages/man3/dlopen.3.html).

The independent GUI probe imports NumPy and returns a scalar twice with only the loader patch applied. It neither preloads Python nor links the GUI executable directly to Python. Separately, the unchanged `application/samples/Ex_IOD.script` completes both calculations with the integrated loader and conversion fixes. Reading both full-precision velocity vectors from the GUI message pane and comparing with the same Python functions gives a maximum absolute difference of **8.881784197001252e-16 km/s**:

| Algorithm | GUI velocity (km/s) |
|---|---|
| Herrick-Gibbs | [-0.3592000605844987, 1.073557248829957, 7.362324021132054] |
| Gibbs | [0.5518403238018362, 1.384871044853208, 7.294561069351383] |

`LD_PRELOAD` was removed for these integration runs. The original script was not rewritten to report results or bypass either calculation.

**OrbitView compatibility:** six production lines map the deprecated `CelestialPlane` getter/setter to the qualified OpenFrames `ECLIPTIC_PLANE` parameter. On and Off match native OrbitView. Reading both aliases, saving and reloading preserve the canonical value. The unchanged `Tut_HohmannTransfer.script` and `Tut_Mars_B_Plane_Targeting.script` complete at 1x, 2x and 3x with conversion enabled.

Active-tab and capture checks passed at two sizes for both tutorials at all three scales. Representative Hohmann/Mars captures were inspected at each scale. A 200 ms capture showed an incomplete Mars redraw at 3x under concurrent load; a targeted 1.5-second settling probe showed correct full-area rendering at both sizes. Both attempts are retained. This establishes successful settled rendering, not a guarantee of immediate redraw or a fix for the external OpenFrames resize warning. Native physical-viewport assertions remain separate from OpenFrames visual sampling.

**TLE samples:** all eleven scripts now use `SPICESGP4`, as does the sample generator. The absolute Windows input path is replaced with its adjacent fixture, and samples point to the bundled sibling TLE directory. All five fixture-backed tests complete offline, independently and in integration; `PropLightsail2.script` also completes. All eleven scripts pass interpretation. The remaining five fail at their documented missing catalogs: no orbital data, satellite names, epochs or propagation settings were substituted. Exact input requirements are in `plugins/TLEPropagatorPlugin/doc/source/Scripting.rst`. The isolated runner now copies `TLE` and `test/TLE` siblings while retaining relative paths.

## Final sweep comparison

| Lane | Scripts | Completed, before → after | Interpretation failures | Mission failures |
|---|---|---|---|---|
| Primary (1x) | 210 | 125 → 132 | 80 → 69 | 5 → 9 |
| Legacy conversion (2x) | 24 | 6 → 10 | 17 → 13 | 1 → 1 |

No previously completed script regressed in either lane. No crashes, unresolved timeouts, native viewport mismatches, wrong active tabs or failed screenshot commands occurred in the final sweeps. The primary lane uses native OrbitView but still loads public OpenFrames for scripts explicitly declaring it. The conversion comparison matches the baseline's 2x scale; an additional 1x conversion sweep produced the same outcomes.

The primary improvements are the IOD example and six TLE scripts. Five other TLE files advance from obsolete-type interpretation failures to explicit missing-catalog mission errors. In conversion mode the two targeted tutorials, Lightsail and `LEOStationKeepingTutorial.script`, which uses the same alias now complete. Missing proprietary components, MATLAB, disabled Python API, historical support fragments and unrelated historical syntax/data requirements remain outside this patch set.

The sweep ran at integration revision `06b8a0f`, which contains all five production changes. Subsequent changes are test timing and audit records only. The 210 original input hashes were rechecked against the tested copies; only the eleven intentionally repaired TLE scripts differ from the saved baseline. `LinuxGuiPublicFixResults.csv` records both lanes with prior/current outcomes and reasons. Full logs, metadata, focused evidence, screenshots and comparison JSON are in the local `outputs/GMAT-public-fixes/evidence` package. The original baseline was preserved.

## Existing GUI suite and timing correction

The existing suite reports **30 passing checks**, including native high-DPI viewport checks, editor failure recovery, model preview, plot lifecycle, animation, Stop/restart, normal close, native and OpenFrames integration.

Its first attempts timed out at the long-mission Stop check. An instrumented reproduction on the validation baseline without any of these five patches showed the timer firing while the publisher was IDLE (10000), although the frame already indicated a mission was starting. `Moderator::RunMission` later resets the interrupt state before entering execution. The supplemental test now waits until `Publisher::GetRunState()` is RUNNING before requesting Stop. This changes only the test; both the failed evidence and successful full rerun are retained. It does not establish that all earlier intermittent shutdown concerns are resolved.

## Independent validation and dependencies

A separate Release/Ninja validation checkout was built with each patch applied individually, then that patch was removed before building the next. No sibling production patch was present during its focused check. The unchanged validation baseline `f1983e2` contains five pre-existing Linux prerequisites: `pr/de-header`, `pr/wxwidgets-path`, `pr/linux-layout`, `pr/groundtrack-map` and `pr/native-viewport`. Thus these are independent *feature-patch* tests with explicit startup/GUI prerequisites, not claims that pristine NASA main starts successfully on this host.

| Patch | Independent runtime check | Feature prerequisites |
|---|---|---|
| Python diagnostics | Actual exception type/message and clear state, then successful wrapper calls | Configured PythonInterface and Python 3.14 |
| Python conversion | Eleven scalar/string/vector/matrix/invalid-result modes, twice per mode | PythonInterface; no NumPy required |
| Linux extension loading | Two real GUI calls importing NumPy and returning 6 | Python 3.14, NumPy 2.3.5; no preload |
| OrbitView alias | Converted On/Off, getter agreement, save/reload | External OpenFramesInterface and its dependencies |
| TLE samples | Five offline scripts, each completing through GUI and normal close | Public TLEPropagator and bundled fixtures |

The full IOD example requires both the Linux extension-loading and return-conversion patches; it is an integration test for those two changes. External OpenFrames uses the existing R2026a dependency installation. No proprietary plugin, MATLAB configuration or disabled Python API was enabled. Tests used Xvfb/X11, wxGTK 3.2.9, GTK 3.24.52 and Mesa 26.0.8 software rendering. Windows/macOS builds, native Wayland, physical-GPU checks for these new patches and other Python versions remain unvalidated.

## Repeating checks

From a completed Linux integration build, run the focused Python, GUI and alias entry points under `src/UnitTests/TestLinuxGui`, followed by `run_tests.py`. The new scripts accept the build directory as their positional argument. For the sweeps:

```sh
env -u LD_PRELOAD python3 src/UnitTests/TestLinuxGui/test_examples.py build/linux-gui --label new-native --jobs 2 --limit 600
env -u LD_PRELOAD python3 src/UnitTests/TestLinuxGui/test_examples.py build/linux-gui --label new-converted --mode converted --legacy-only --scale 2 --jobs 2 --limit 600
```

Use fresh labels: existing evidence is intentionally not overwritten. The independent-validation driver and exact extended-settling probe are included with the local evidence. All patch whitespace checks pass.

## Retained follow-ups

LGUI-019 and the external OpenFrames resize/tooltip diagnostics, missing Aura `GFOIL1.JPG`, GTK notebook warnings, the previously reported pixman rectangle warning and intermittent shutdown concerns remain separate. No warning suppression or renderer changes were added. Settled screenshot checks and clean sweep exits do not replace platform-wide GUI testing or scientific validation of every example.
