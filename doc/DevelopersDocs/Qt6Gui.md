# Qt 6 desktop GUI

The `codex/qt6-gui` branch starts at integration commit `0ef9a8a`, retaining
the existing PR fixes and their regression infrastructure. The Qt frontend
links the same GmatBase and GmatUtil libraries. wx-specific behavior fixes
must also be implemented and tested in their Qt equivalents; ancestry alone
does not establish that equivalence.

## Design

Use Qt 6 Widgets with the recognizable GMAT desktop arrangement: Resources,
Mission, and Output navigation tabs on the left, an MDI workspace in the
center, messages below, menus and toolbar above. Retain platform palette and
font defaults. Start with familiar workflows before introducing new ones.

The old Qt experiment was inspected as reference. Only its message adapter
was reused initially, with changes to preserve UTF-8/local encoding fallback
and avoid indefinitely accumulating already delivered messages. The new
window and build integration do not depend on the old Qt 5 fallback or
placeholder plotting implementation.

## Build and first launch

Qt 6.4 or newer with Widgets development files is required. In an existing
configured GMAT build:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON
cmake --build build/linux-gui --target GmatQt --parallel 6
application/bin/GmatQt --startup /absolute/path/to/host-startup.txt
```

Use a host-specific startup file referencing the plugins actually built.
The currently available OpenFrames plugin uses wx and must not be used as
a Qt widget provider. No Qt OpenFrames support is claimed yet.

The `--screenshot /absolute/path.png` option captures the initialized window
and exits with failure if initialization, optional script interpretation,
or image writing failed. For headless layout inspection set
`QT_QPA_PLATFORM=offscreen`.
Add `--run` to execute the loaded mission before capturing it.

## Current evidence and remaining work

The initial shell compiles with Qt 6.10.2 on Linux, initializes the engine,
loads the default mission and renders the familiar layout. File loading
checks reads before replacing the editor; saving uses QSaveFile and only
changes document identity and modified state after successful commit.

Mission execution now services Qt events through GMAT's existing interruption
checkpoints. Engine and UI access remain on one thread. Run, Pause, Resume,
and Stop update the toolbar; active execution prevents script/model edits
and nested runs. Closing an active mission requests Stop and retains the
window until execution has unwound. `Moderator::SetUiInterpreter(nullptr)`
now safely detaches the Qt adapter during shutdown.

Double-clicking a resource opens an editable property panel. Scalar numbers,
booleans, strings, enumerations and references are supported. Applying edits
changes a clone, validates it, serializes it into the complete mission, and
interprets that candidate. A rejected candidate restores the previous model
without changing editor contents. Successful changes update the script as
one undoable edit. Stale panels reject changes, and unapplied panel edits
prevent Build/Run from silently using older values. Array/list and compound
properties still require the script editor.

## Functional validation

Enable and run the real-engine workflow executable with a host startup file:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_QT_BUILD_TESTS=ON
cmake --build build/linux-gui --target GmatQt GmatQtWorkflowTests --parallel 6
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtWorkflowTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/propagate.script \
  /tmp/gmat-qt-resource-editor.png
```

The screenshot argument is optional. The test has passed on Linux with Qt
6.10.2 and verifies actual 600-second propagation, repeat execution,
pause/resume/stop, exclusion of nested execution and editing, close-during-run,
invalid-script recovery, resource validation/rollback, stale panel rejection,
script undo, and resource-tree-to-Apply-button interaction. It does not prove
plotting, platform parity, or all property types. The separate wx/console
exit regression checks shared-engine behavior.

This remains an implementation checkpoint, not a completed replacement.
Still required: specialized resource forms and compound properties, mission
command editing, plugin compatibility handling, advanced graphics and plot
style parity, wider functional coverage, and platform build/package validation.
Only Linux has been built and exercised so far.

## Plotting and output

The Qt receiver now records and renders native OrbitView, GroundTrack,
GroundTrackPlot, XYPlot and DynamicDataDisplay publications. Plot windows use
the central MDI workspace and can be reopened from Output without losing
their recorded samples. The receiver retains numeric data independently of
window lifetime; a rebuild starts a fresh recording. Body and coordinate
system pointers are used only while receiving engine publications, never
during drawing or replay.

OrbitView is currently an orthographic trajectory view with body disks,
rotation, pan, reversible wheel zoom, Fit, image export, and replay. It uses
the view-coordinate positions already converted by OrbitPlot; non-spacecraft
bodies are converted from the internal frame separately. This is not yet the
full wx 3D renderer: body textures, spacecraft models, star fields, reference
planes, and scripted camera tracking remain outstanding. Unsupported camera
and drawing options are reported in Message Window. Advanced XY marker/style
and solver-iteration behavior also needs parity work.

GroundTrack uses the engine's geodetic longitude/latitude, resolved body map,
collection/update frequencies, configured point limit and line width. Missing
satellites preserve their curve slots and break the line. Dateline crossings
split at the map boundary. GroundTrackPlot's older Cartesian callback path
is implemented but still needs its own full engine regression. Ground station
markers reject objects that are not body-fixed points. Replay does not mutate
the retained histories. QPainter handles physical display scaling.

Dynamic tables preserve displayed values and cell colors when closed and
reopened. Output also lists report files and opens a read-only preview of up
to 16 MiB; larger files retain their complete contents on disk.

Build `GmatQtPlotTests` with `GMAT_QT_BUILD_TESTS=ON`, then run:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtPlotTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/plots.script \
  /tmp/gmat-qt-plots.png
```

This real-engine test compares final orbit XYZ, XY samples, and geodetic
longitude/latitude with a 16-digit GMAT report. It also verifies map loading,
sampling and retention settings, dateline interpolation, missing samples,
invalid station handling, close-during-run/reopen/rerun, reversible zoom,
replay retention, dynamic-table values/colors, and Output report opening.
The test has passed at display scale 1 and 2. The existing Qt workflow suite
continues to pass with the plot receiver installed. These checks do not
establish complete graphics or cross-platform parity.
