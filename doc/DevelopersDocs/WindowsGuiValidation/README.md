# GMAT Windows compatibility validation

**Windows validation completed.** All 59 baseline-completing examples still complete with the patches. The final 25-case focused suite passes all 667 assertions. No PRs, issues, or external comments have been submitted.

## Scope and fixed inputs

- Upstream: `9363e129be366520c6edb0b4079204ed60666007`.
- Initial patched integration: `54bdf63f829735b4dd443d6957ff8d5716d42162` (35 prepared GMAT patches).
- Additional production fix found by Windows testing: integration `916d796`, isolated branch `pr/gui-message-encoding` at `9130aae` (LGUI-042).
- Test portability changes: `3bf4e2a`; no production rendering changes were made to accommodate the VM.
- Both full native Windows builds succeeded. Both produced 45 compiler warnings, with no new warning types in the patched build. See `build-retry.log` and `build-warning-comparison.json`.

## Environment

Windows 11 Enterprise Evaluation 25H2, x64, build 26200; KVM VM with 8 cores and 12 GiB RAM. Visual Studio 2022 Build Tools 17.14.41, MSVC 19.44, Windows SDK 10.0.26100, and CMake 3.31.6. Dependencies: official wxWidgets 3.2.10 VC14x x64 binaries, CSPICE N0067, locally built Xerces 3.2.2, Python 3.12.10 and NumPy 2.5.3. MATLAB and the disabled GMAT API were excluded.

GUI tests ran in the logged-in Windows desktop, not SSH's noninteractive session. Display: 1280×800 at 100% scaling, Microsoft Basic Display Adapter; actual OpenGL renderer `GDI Generic / 1.1.0`.

## Completed focused evidence

- Both builds pass startup/close and native viewport sizing before and after resizing.
- Patched Python interface passes all 12 direct groups (112 assertions): diagnostics, Unicode/percent text, failed exception formatting, recovery, consecutive vectors, mixed numeric values, matrices, scalar/integer/string values, and rejected malformed arrays. Upstream passes four groups and fails eight, including access violations for empty/ragged arrays.
- Patched GUI passes editor recovery, GroundTrack ownership, lookup, station markers, editor/output handling, state mapping/sampling, resource deletion, and default Windows tree colors.
- Direct GroundTrack closing/recreation passes on both builds. Patched GroundTrack toolbar animation passes 33 assertions.
- All seven patched native animation cases pass, covering ring-buffer wrapping, trail settings, interruption, and repeat playback.
- Windows texture upload passes 284 assertions, comparing every GLU mip level with a tightly packed reference and checking restoration of caller pixel-store state. Upstream fails and crashes under the same deliberately nondefault pixel-store settings.
- All four patched wheel modes pass camera-state checks. Upstream retains astronaut movement but lacks centered/free wheel zoom and centered Shift-wheel field-of-view zoom.
- The unmodified IOD example completes both calculations; patched velocities differ from direct execution of the same Python functions/inputs by at most `8.881784197001252e-16` km/s.
- Corrected map regression passes 30 assertions, including missing/corrupt maps, recovery, and zero-size resize. Windows ClearType contributes a few blue subpixels to gray labels, so the test permits less than 1% blue coverage for an absent map; an actual map occupies far more. Captured pixels confirmed this was a test assumption, not retained map rendering.

## Paired 210-example sweep

| Outcome | Upstream | Patched |
| --- | ---: | ---: |
| Completed | 59 | 70 |
| Interpretation failed | 139 | 131 |
| Mission failed | 3 | 2 |
| Time limit reached | 9 | 7 |

There are **zero newly failing previously completing examples**. Nine additional completions follow repaired interpretation/mission failures; two additional completions were baseline time-limit stops and are not evidence of a performance improvement. All five bundled TLE tests complete. The remaining mission failures require unavailable MONTE integration or the disabled GMAT Python API. Missing plugins, external input data, and out-of-scope examples are recorded individually in [the comparison](example-comparison.csv) and raw logs.

Both builds pass actual native MDI canvas resizes from 832×429 to 500×300 to 700×400 (13 assertions each). Two multi-plot tutorial probes initially read stale shared-context viewport state. Explicitly repainting each canvas reduces the mismatches, but one mismatch per tutorial remains identically on upstream and patched builds. This is an unresolved baseline/test-observation limitation; it is not counted as a passing multi-plot viewport check. Both tutorials complete their missions. Single-canvas resize checks pass.

## Windows issue found and corrected

Python's UTF-8 exception text was correct in the plugin/log but decoded using the Windows ANSI code page in the GUI (`café` became `cafÃ©`). `GuiMessageReceiver` now decodes valid UTF-8 and falls back to the previous locale conversion for invalid UTF-8. No public signatures or startup options changed.

The corrected GUI passes 48 Python recovery assertions and 32 direct message/dialog assertions, including percent signs, empty text, Japanese text, and legacy ANSI input. The new message test fails eight assertions on upstream. The production fix also passes all 32 assertions when independently substituted into upstream GUI objects/DLLs. Linux message, Python GUI recovery, and GroundTrack regressions pass.

This is a separate, production-only proposed patch. The Python diagnostic patch's Windows end-to-end Unicode claim depends on this GUI decoding correction; the Python plugin patch alone cannot fix the old GUI decoder.

## Method and limitations

The main run tests the integrated patch set. It does **not** establish that all 35 original branches build independently. Focused tests link the production GUI objects and use the real application/plugin code; test-only access to internal state adds no public API. Windows test entry points use the console CRT with unbuffered output, avoiding the Linux harness's invalid zero-size line-buffering call under MSVC.

OpenFrames/OVtoOFI were not built: their Windows dependencies are absent from this VM. Explicit OpenFrames examples and the converted 24-script lane therefore remain unvalidated on Windows. This includes the OrbitView plane alias and external OFI time-buffer patch. Proprietary plugins and MATLAB are excluded.

The original color-based wheel pixel metric fails on both builds with GDI Generic; camera-state checks establish behavior, but this does not certify hardware-accelerated rendering. Captured native OrbitView images show abnormal red/dithered colors on both upstream and patched builds with this virtual renderer; visual fidelity is therefore not certified. Real GPU drivers, higher Windows DPI/mixed-DPI monitors, dark/high-contrast themes, and macOS remain gaps. Default-theme checks and 100% viewport checks must not be described as universal visual compatibility.

Public examples use isolated copies of input directories, including sibling TLE fixtures. Each script has the same mission limit on upstream and patched builds: the first 86 scripts retain 120 seconds; remaining pairs use 30 seconds. Time-limit stops are coverage limits, not passing missions. External-data and missing-plugin failures are retained in evidence.

## Submission guidance

Use per-patch coverage rather than a blanket “Windows tested” label. A suitable claim is: “Built with MSVC on Windows 11 and exercised the affected native GUI workflows against the upstream baseline; detailed results and remaining platform/graphics gaps are attached.” Add exact relevant test names and outcomes for each proposed PR. Refresh and independently test each branch before submission.

## Evidence and reproduction

- [Per-patch coverage](coverage-map.json), [independently substituted patch sources](isolated-branches.json), and [paired example results](example-comparison.json).
- [Raw logs, result records, and selected screenshots](windows-evidence.zip). Earlier failed test attempts are retained; use `validated-patched-*` for the final 25-case suite, `verified-*-ViewportRegression` for actual MDI resize checks, and the `final-preview-*` cases for the corrected preview checks (diagnostic cases explain the initial invalid observation).
- [Build and harness instructions](reproduction/README.md). The VM retains separate upstream and patched builds and desktop launchers.

Independent object-substitution checks pass for wheel zoom, animation replay, viewport sizing, direct GroundTrack close, resource deletion, default tree colors, texture rows, model-preview lifecycle, and the new message encoding fix. These compile the production file(s) from each branch and link against upstream objects/DLLs. They are not independent full CMake branch builds. Other branches have integrated-build coverage only or the explicit gaps in the coverage map.

## Model preview investigation

The first constructor-state probe failed identically on upstream, patched, and isolated builds. Instrumentation showed wxGLCanvas creation clears the current WGL context: it changed from a valid handle before construction to NULL afterward. Reading clear color without restoring the context was invalid. The corrected probe restores the orbit context before comparing its state. This is a test portability issue; repeated bundled-model loading, painting, closing, and returning to the orbit view already succeeded on all three builds. Windows does not reproduce the original Linux constructor-state defect.

The corrected model-preview check passes **13/13 assertions on each of upstream, integrated, and isolated patch builds**. No additional production fix was required.
