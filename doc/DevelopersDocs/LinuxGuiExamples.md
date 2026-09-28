# GMAT Linux GUI example sweep — 2026-09-28

Every tracked `.script` file outside the unit-test directory was attempted through the real GUI: **210 unique scripts**. After rerunning the five slow missions and correcting six test-launch failures, **125 completed, 80 failed during script interpretation, and 5 failed during mission execution**. No application crash or unresolved timeout was observed in these final attempts. This is a bounded lifecycle audit, not a claim that every GUI interaction or scientific result is correct.

No production code was changed in this sweep. A reusable test runner and this evidence record were added to the integration work. No PRs, issues or external comments were submitted.

## Coverage and results

| Scope | Scripts | Completed | Build failed | Mission failed |
|---|---:|---:|---:|---:|
| Shipped `application/samples` | 140 | 109 | 29 | 2 |
| API, plugin, documentation and prototype examples/support fragments | 70 | 16 | 51 | 3 |
| Total | **210** | **125** | **80** | **5** |

The inventory deliberately includes support fragments and historical examples so they are not silently omitted. A successful result means that GMAT returned mission result 1 and closed normally; for fragments it does not establish that a meaningful standalone mission was defined. GMAT functions (`.gmf`) and Python/MATLAB/C++ driver programs are dependencies or separate execution environments, not standalone `.script` missions. `Old` was excluded entirely.

The primary lane preserves native OrbitView and GroundTrackPlot while loading installed OpenFrames support for examples that explicitly create OpenFrames objects. Thus “native” in the evidence directory does not mean that every script used a native renderer. A separate lane attempted all **24 scripts declaring legacy OrbitView**, with OVtoOFI conversion enabled at **2×**: 6 completed, 17 failed interpretation, and 1 failed mission execution. Two of those interpretation failures are confirmed conversion regressions relative to the primary lane.

[Per-script results and reasons](LinuxGuiExamples.csv) provide all 210 paths, input hashes, final outcomes and durations. The user-facing evidence bundle retains full logs, machine-readable results, reliable representative images and superseded initial outcomes. Generated output and screenshots are not tracked in Git; the directory names below refer to that local evidence bundle.

## Actionable follow-ups

| ID | Priority / owner | Evidence and impact | Next focused change or investigation |
|---|---|---|---|
| LGUI-017 | Medium / GMAT PythonInterface | `Ex_IOD.script`, `Ex_ExternalForceModel.script`, and the MONTE dynamics-sharing example show a dialog ending with “Python Exception:” but omit the underlying error. The log prints only the exception class. In `PythonInterface::PyErrorMsg`, the Python 3 branch never assigns the returned `msg` and does not include the exception value. | Add a small Python-failure regression; populate the message with type and value while preserving error state and reference ownership. Validate non-ASCII exception text and failed formatting. Keep this separate from Python runtime compatibility work. |
| LGUI-018 | Medium / GMAT OVtoOFI plugin | `doc/help/src/files/scripts/Tut_HohmannTransfer.script` and `Tut_Mars_B_Plane_Targeting.script` complete with native OrbitView but fail with conversion enabled. `CelestialPlane = Off` is rejected as an unsupported On/Off parameter on OpenFramesInterface. | The NASA checkout contains `plugins/OVtoOFI/src/base/subscriber/OVtoOFI.cpp`. Its deprecated CelestialPlane parameter falls through `SetOnOffParameter` to the base class. Add a compatibility regression and decide how to preserve or explicitly ignore this deprecated option, following existing conversion conventions. |
| LGUI-019 | Low / external OpenFrames–OFI boundary | 59 primary attempts emit the resize warning that the OpenGL context was not properly updated. The external OFI render pool installs its update-context callback only on macOS, while OpenFrames warns when that callback is absent. Representative plots render successfully. | Verify the Linux callback contract before changing behavior. The warning alone does not establish rendering corruption; do not suppress it without understanding the contract. Track outside GMAT core. |
| LGUI-020 | Medium / Linux Python embedding | `Ex_IOD.script` fails with ImportError although standalone Python 3.14 imports NumPy 2.3.5. An isolated C embedding probe with `RTLD_LOCAL` reproduces NumPy's undefined `PyObject_SelfIter` symbol. Preloading libpython advances the original script from line 47 to a later SystemError at line 71. | Investigate library symbol visibility and the later Python C API failure separately. Preloading is diagnostic evidence, not a validated fix. Avoid changing flags for every GMAT plugin without checking the consequences. |
| LGUI-021 | Low / example maintenance | All 11 TLE plugin sample/test scripts use the historical `Type = TLE`; the installed plugin registers `SPICESGP4`. They fail interpretation despite the plugin being loaded. | Update and test this example family in a separate maintenance patch, including its relative input files and optional OpenFrames requirements. |

Existing follow-ups were also reproduced: GTK notebook negative-dimension warnings in 47 primary attempts, the external OFI tooltip-before-widget warning in 21, and missing Aura `GFOIL1.JPG` in 59. These counts are scripts emitting a diagnostic, not independent defects. The reported pixman rectangle warning was not observed. The earlier intermittent OpenFrames workflow timeout remains open: this sweep's clean closes do not disprove it.

## Why scripts failed

Of the 29 shipped-sample build failures, **28 require unavailable optional components**: MATLAB, SNOPT, VF13ad, optimal-control/EMTG-related factories or MarsGRAM2005. The remaining file is `SupportFiles/Ex_EphemerisCompareGeneric.script`, which requires objects supplied by a parent mission. No alternative solver, shortened mission or edited input was substituted to turn these into passes.

The two shipped mission failures are Python-related: `Ex_ExternalForceModel.script` requires the GMAT Python API, which this build disables, and `Ex_IOD.script` exposes the embedding problem described above. The other three mission failures are a historical GMAT-function output mismatch, an unsupported ground-station antenna/FOV combination, and an unavailable Python module in the MONTE dynamics-sharing example. Supplemental build failures include obsolete syntax/object types, missing externally generated ephemerides, hard-coded historical paths, missing output directories and unavailable plugins. Each has its observed error recorded in the CSV/log, rather than being counted as a GUI crash.

The five initial time-limited examples all completed with a longer allowance:

| Example | Final elapsed time, including GUI inspection/close |
|---|---:|
| Ex_LunarOrbitStationKeeping | 136.47 s |
| Ex_Yukon_MarsLaunchWindowAnalysis | 229.09 s |
| Navigation/Ex_Estimate_SPADDragScaleFactor | 149.84 s |
| Navigation/Ex_Estimate_TDRSAndUser | 144.33 s |
| Navigation/Ex_Estimate_TDRSUserTracking | 112.11 s |

Timing varies with concurrent load. Six apparent startup failures were the initial runner relinking a shared executable during another invocation. Each was retried with an independent executable: one completed and five reported historical script errors. They are excluded from application-crash claims. The reusable runner gives every invocation its own executable.

## Validation and limitations

The final primary attempts inspected 272 output windows at two sizes: 109 XY plots, 88 plugin windows, 47 solver-progress windows, 15 native OrbitViews, 12 native ground tracks and one dynamic display. All **30 native OrbitView viewport checks** matched the physical canvas size. No native viewport mismatch was recorded. This checks viewport dimensions, not scientific plot correctness, and does not assert equivalent checks for external OpenFrames canvases.

The initial 544 wxWidgets screen captures proved unreliable for switching tabs: some retained the first tab's pixels. Actual active-child checks passed, and independent ImageMagick captures showed the correctly selected tabs. The reusable runner now uses independent captures and records capture failures. Five representative scripts were repeated with the final runner to cover native orbit/ground-track, XY, solver, explicit OpenFrames, dynamic display, control flow and an include fragment; all completed with zero activation, viewport or capture errors. A separate converted run passed at 2×. A one-second limit test requested Stop and closed with `time_limit_stopped`; compatible evidence resumed without rerunning. Earlier screenshots are not treated as proof that every plot was visually inspected.

Reliable representative captures are in `example-sweep-evidence/visual-check/` and `capture-2x/`. Visual sampling showed full-area native plots and populated XY/OpenFrames displays. This sweep did not exhaustively inspect all pixels, exercise every mouse/keyboard action, verify numerical results, test Windows/macOS, or rerun all examples on the physical GPU or native Wayland.

Tested production revision: `0886716` on `linux-gui-integration`, with prepared Linux fixes already integrated. Environment: wxGTK 3.2.9, GTK 3.24.52, Mesa 26.0.8, Xvfb/X11 software rendering; scale 1 for the primary lane and scale 2 for conversion checks. Python 3.14/NumPy 2.3.5 were installed. All 210 input hashes were rechecked unchanged after execution. Each GUI session had isolated preferences/output and a read-only host/source filesystem via bubblewrap. No desktop preferences or source inputs were modified.

## Repeat the sweep

From a completed Linux Ninja build:

```sh
python3 src/UnitTests/TestLinuxGui/test_examples.py build/linux-gui --label examples --limit 600 --jobs 2
python3 src/UnitTests/TestLinuxGui/test_examples.py build/linux-gui --label converted-2x --mode converted --legacy-only --scale 2 --limit 600
```

Requirements: the built GUI, its configured dependencies, `ninja`, `bwrap`, `xvfb-run`, and ImageMagick's `import`. Installed optional plugins are registered; unavailable components remain visible as script failures. If multiple Python plugins are built, select one with `--python-version`. `--only` accepts a script-path regular expression. Use a new label for changed settings or code; `--resume` only reuses compatible input/configuration/harness hashes. The runner retains build and mission failures as findings; crashes, hard timeouts, capture/activation/viewport failures or harness errors make it return nonzero. Always read the per-script summary rather than treating exit status alone as a blanket pass.

Results default to `build/linux-gui/linux-gui-tests/example-sweep/<label>/`. This long-running sweep is opt-in and separate from the focused regression suite. Keep new fixes small, add a focused failing test, validate each proposed branch independently, and retain this broad sweep as supporting evidence. PR submission still requires the user's authorization.
