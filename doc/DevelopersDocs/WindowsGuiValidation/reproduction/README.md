# Reproducing this Windows lab run

These helpers are supplemental audit infrastructure. They are not part of a proposed production PR and do not replace the team's TestComplete infrastructure.

## Layout and prerequisites

The lab uses `C:\GMAT-Test\upstream` and `C:\GMAT-Test\patched` as independent source trees, `C:\GMAT-Test\deps` for dependencies, and `C:\Python312` for Python. Adapt the explicit lab paths for another machine. Source revisions and versions are listed in the parent report. Put these helpers directly under `C:\GMAT-Test`.

Install Visual Studio 2022's x64 C++ tools/SDK and CMake; wxWidgets headers, VC14x x64 developer libraries and release DLLs; CSPICE Windows libraries; Xerces headers/static library; Python 3.12 plus NumPy and Pillow. The dependency roots used here are in `build.ps1`. Xerces was built with its default Windows networking backend, not the Unix socket backend.

Run `build.ps1` to configure/build both native GMAT trees and copy the wxWidgets runtime DLLs. MATLAB, OpenFrames and the disabled API are excluded explicitly. Do not add broad plugin-loader changes or preload settings.

## GUI harnesses

The Linux regression sources live in `patched\src\UnitTests\TestLinuxGui`. Use their Windows-portable versions from integration commit `3bf4e2a` or later. Put the three `Windows*Regression.cpp` files there too. `MessageEncodingRegression.cpp` is also included for convenience.

From a Visual Studio x64 developer command prompt:

```
C:\Python312\python.exe C:\GMAT-Test\compile-harness.py C:\GMAT-Test\patched C:\GMAT-Test\patched\src\UnitTests\TestLinuxGui\ViewportRegression.cpp
C:\Python312\python.exe C:\GMAT-Test\prepare-runtime.py C:\GMAT-Test\patched
```

Repeat for upstream and the other harness sources referenced by `make-jobs.py`. The compiler helper reads the actual generated VS project and link log, then links the test entry point with the production GUI objects. It does not patch production headers or APIs. Optional trailing C++ source paths substitute just those compiled objects into upstream, producing a test executable ending in `_isolated`; this was used for independent patch checks. The initialized compiler environment is cached locally under the lab root and should not be published.

`make-jobs.py smoke` creates four startup/viewport cases; `make-jobs.py focused` creates the 49 focused cases. Run `run-gui-tests.py` **in a logged-in Windows desktop**, for example through an interactive scheduled task. SSH alone runs in session 0 and is not a graphics test. Keep the display awake. The job manifest records commands; each case records logs, assertion counts, exit status, and any timeout screenshot. Never run multiple GUI suites on the same desktop concurrently.

The map regression maximizes the Windows MDI child for pixel capture and allows less than 1% blue subpixels when checking an absent map, because ClearType labels are not perfectly gray. The camera-state wheel regression supplements the original color-based rendering metric, whose result is unreliable with the GDI Generic renderer in this VM.

## Python and examples

Compile `PythonRegression.cpp` through `compile-python-test.py`, then run `run-python-tests.py`. Run the GUI IOD case before `compare-iod.py`; it reads input assignments from the unmodified example rather than duplicating them.

`make-example-jobs.py` stages the 210-script inventory for both source trees. It copies mutable sample inputs, preserves sibling TLE fixtures, and uses junctions only for bundled shared runtime assets. The result is 420 separate GUI jobs. The actual run's final job manifest records its per-script time limits, applied equally to upstream/patched pairs. Time-limit stops must not be called passing missions.

Raw results are retained even when upstream crashes or cannot interpret a dependency-specific example. Compare successful baseline cases with patched results and investigate any regression before claiming compatibility.

## Final checks

`final-jobs.json` preserves the 50-case final batch (including the initial preview assertion investigation). Compile the named harnesses first; copy the manifest to `jobs.json` and run the desktop runner. `validated-patched-*` are the final relinked 25-case suite. Actual Windows MDI resizing requires the later `ViewportRegression.cpp` included here; resizing only the main frame does not resize an unmaximized MDI child. The sweep explicitly repaints the observed canvas before checking shared GL state, but retains the documented multi-plot mismatch limitation.
