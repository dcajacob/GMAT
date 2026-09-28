# Four public documentation-example repairs

Four focused sample-only patches repair the remaining selected public examples. They are committed separately on `linux-gui-integration`, with independent branches based on NASA revision `9363e129be366520c6edb0b4079204ed60666007`. The supplemental regression and this audit stay on integration. No PR, issue or external comment has been submitted. Existing production fixes, `Old` and the Downloads R2026a source are unchanged.

| Finding | Branch | Integration commit | Standalone commit |
|---|---|---|---|
| LGUI-022 | `pr/satellite-separation-example` | `9c7c2429694f` | `12e10aa7f344` |
| LGUI-023 | `pr/global-function-example` | `4f4f077c7123` | `b1ca38c21703` |
| LGUI-024 | `pr/force-model-tutorial` | `631051823e76` | `0350a31697c5` |
| LGUI-025 | `pr/sensor-contact-example` | `18029f89f280` | `746cdb101bd2` |

## Behavior repaired and checked

**LGUI-022 — satellite separation:** `BasicFunction.script`, `SatSep.gmf` and its adjacent `dot.gmf` helper now declare the mission/function command boundaries required by the current interpreter. Function-local declarations precede command mode. SatSep's Y and Z components previously reused X; each now uses its corresponding coordinate. The full original example completes, including its maneuver and both propagation loops. Six independent known separations exercise pure X/Y/Z, positive and negative mixed components, and coincident positions. All components and norms match exactly in the reported double-precision results, including the 3-4-12 vector's 13 km norm. Global dx/dy/dz behavior and the nested function call remain intact.

**LGUI-023 — global function:** `RaiseApogee.gmf` uses the supported no-output function declaration and an explicit command boundary; `GlobalSample.script` also declares its mission boundary. The original four-call mission completes. A numerical probe calls the actual helper with 1, 2, 3 and 4, checking that the shared spacecraft gains 0.1, 0.2, 0.3 and 0.4 km/s respectively. The largest error is 7.22e-16 km/s. In that controlled perigee probe, apogee radius increases from 7601.861 km to 13871.406 km while perigee stays 7000 km. These values describe the probe, not a newly imposed orbit in the original sample.

**LGUI-024 — force-model tutorial:** the report uses a bare filename under configured OUTPUT_PATH, and the script declares BeginMissionSequence. After the path repair, execution exposed a second failure: fixed 30-second minimum steps violate the requested 1e-12 accuracy in the third-body stage. All six propagators now allow smaller steps while retaining that accuracy and the 30-second maximum. No force model, spacecraft state, propagation interval or error-stop policy was weakened. The seven report rows cover the initial state and six successive 0.1-day stages. A second run with a 15-second maximum agrees to 3.01e-8 km in position and 1.35e-11 km/s in velocity. Existing supported legacy Drag spelling is left unchanged; its warnings remain visible.

**LGUI-025 — station sensor contacts:** ground-station ContactLocator rejects ConicalFOV. The station now uses a supported CustomFOV sampled every 1 degree in azimuth at 10 degrees elevation; the spacecraft's conical sensor is unchanged. Great-circle polygon edges make this an approximation: the boundary is at most 0.0003731 degrees above the circular horizon. The script documents that limitation. A separate station with identical coordinates and MinimumElevationAngle=10, but no attached FOV, provides a circular reference on the same 30-day trajectory. Both find 30 contacts; the largest difference among all 60 start/stop endpoints is 0.128 seconds. Light-time direction, stellar aberration, lunar occultation, initial orbit and propagation settings are retained. Two GUI sizes render the mask successfully; screenshots were inspected.

## Focused and independent validation

The final integrated run completes all **10 selected scripts**: the four repaired documentation examples and six previously passing public function, force-model and station-mask examples. No selected script regressed; no native viewport mismatch, inactive-tab capture, screenshot failure or abnormal exit was reported. Exact names and previous/current status are in `LinuxGuiExampleMaintenanceResults.csv`. The earlier 210-script and 24-script sweep records are preserved; those full sweeps were not repeated for this sample-only batch.

Each patch was also applied alone to the existing independent validation checkout, tested numerically and through its complete original example, then removed before the next patch. The other three sample patches were absent. The first three use baseline `f1983e2`, which includes the previously documented Linux DE-header startup, wxWidgets path, layout, ground-track and native-viewport prerequisites. The sensor test additionally uses the already-prepared `pr/linux-plugin-lifetime` fix at validation baseline `584bf1e`. These are explicit prerequisites, not a claim that unpatched upstream starts and closes successfully on this host.

Feature prerequisites:

| Patch | Required public components |
|---|---|
| Satellite separation | GmatFunction plugin; keep all three adjacent files together |
| Global function | GmatFunction plugin; keep the script and RaiseApogee.gmf together |
| Force models | Standard GMAT force models, bundled data and a writable OUTPUT_PATH |
| Sensor contacts | Station, EventLocator, GmatEstimation (Antenna factory), SPICE and external OpenFramesInterface |

The initial independent sensor attempt exposed a missing test-environment dependency: GmatEstimation was disabled, so Antenna was unavailable. After enabling this public plugin, the mission completed but the process segfaulted during shutdown, reproducing the pre-existing OpenFrames/plugin-lifetime failure. With the existing, separately prepared Linux plugin-lifetime patch as a prerequisite, both the numerical probe and original example pass through normal close. All attempts are retained. This is not an additional production change or a claim to resolve every intermittent shutdown concern. No proprietary plugin is involved. The R2026a dependency installation supplies OpenFrames; its source is unchanged.

Validation used Linux/Xvfb/X11, wxGTK 3.2.9, GTK 3.24.52 and Mesa 26.0.8 software rendering at 1x. Windows/macOS, native Wayland and the physical GPU remain untested for these patches. Existing OpenFrames/GTK diagnostics, missing Aura texture and intermittent shutdown investigations remain separate; no renderer or warning-suppression changes were made.

## Reproduction and delivery

On the prior integration revision `ef1c6bc`, open and run each of the four documentation scripts: BasicFunction fails interpretation; GlobalSample fails in the helper declaration; ForceModelsTutorial fails report-path resolution; BasicSensors rejects the station's conical FOV. Applying each patch with its listed prerequisites makes the corresponding example complete.

From a completed Linux Ninja build, use a fresh evidence label:

```sh
python3 src/UnitTests/TestLinuxGui/test_public_examples.py build/linux-gui --label new-example-numerics
python3 src/UnitTests/TestLinuxGui/test_examples.py build/linux-gui --label new-example-sweep --jobs 2 --limit 600 --only '(BasicFunction|GlobalSample|ForceModelsTutorial|R2020a_BasicSensors|Ex_GMATFunction.*|Ex_ForceModel.*|Ex_Contact_Location_Station_Mask)\.script$'
```

The numerical runner also accepts `--mode satsep`, `global`, `force` or `sensor` for single-patch validation. It calls the real GUI, reads actual report values and retains generated probes and logs. Sample-only patch files, branch/commit metadata, local PR descriptions and full focused evidence are packaged under `outputs/GMAT-example-maintenance`. The four branches and integration are backed up to the fork; no submission has been made.
