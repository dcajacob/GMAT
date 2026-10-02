# Estimator-owned residual plots — 2026-10-02

Independent actual Help walkthroughs 10–12 exposed residual viewers with legends
and empty 0–1 axes even after completed estimation. Fit did not recover samples;
actual exports contained headers without numeric series. Original failed GUI
observations, reports and exports are retained in their walkthrough artifacts.

Estimator.cpp:2965–2969 sends ClearData → Deactivate → SetData → Rescale → Activate.
OwnedPlot.cpp explicitly requires Deactivate to receive/store data while delaying
redraw. The Qt receiver instead set PlotModel.active=false, dropping every point
in PlotModel::append. The wx receiver suspends redraw and refreshes at activation.

QtPlotReceiver now keeps a separate redraw-suspended flag. Both bulk and single
curve callbacks retain data; Activate clears that flag and forces refresh. Normal
subscriber ToggleOff/ToggleOn retain their independent admission and line-gap
behavior. No numerical engine, estimator algorithm or input was changed.

The new QtGui.OwnedPlotBatch regression invokes actual OwnedPlot/PlotInterface
callbacks. It checks exact two-curve numeric samples, asymmetric errors and line
connections, deferred/final shared replay refresh, populated export/rendering,
closed-viewer retained data/reopen/refill and independent subscriber Toggle.
A read-only peer review found no substantive blocker. The first execution failed
at a test-only dangling reference from const QMap::operator[]'s temporary curve;
copying that point value fixes the test. Both logs remain. Only the new test was
rebuilt/retried: **Passed, 0.08 s**. No successful old suites or corpus repeated.

GmatQt/application bin was rebuilt with the fix before new private GUI sessions.
Application SHA256 `b8074b0222cbd4d3a4c67cacf37262181aeb94d13b3ebd3ed2ea061b8ba8dad5`;
base `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`;
startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
Build/test/raw identity logs are under
`build/example-qualification/20261002/owned-plot-batch`.

One affected actual GPS filter retry completed in 1.038 s, visibly populated all
three residual series and exported 435 samples (145 per coordinate). Its CSV is
byte-identical to the original cold filter run, and every report line matches
except Run Date. Independent DSN/inter-spacecraft retries and remaining tutorial
steps are recorded separately; this focused check does not complete those chapters.

Offscreen widget regression and private X11/software-GL input do not establish
host GNOME/Wayland, portal or physical GPU acceptance. Windows/macOS/MATLAB remain
deferred and full Linux replacement qualification remains open.
