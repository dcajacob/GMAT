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

The Window menu lists open workspace views, marks the active view, and restores
minimized windows when selected. Tile, Cascade, Next and Previous remain
available. Reopening a resource focuses its existing panel and preserves pending
edits; a clean panel from an older model is replaced with current values.

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
application/bin/GmatQt
```

The build generates `gmat_startup_qt.txt` beside the executable, including the
enabled native plugin targets with their actual filenames. Building `GmatQt`
also builds those plugins. The wx startup file is untouched. The Qt startup
uses relative paths and is installed in `bin`; Debug build paths account for
the separate debug directory. Use `--startup /absolute/path/to/file` to override
this configuration explicitly. The wx OpenFrames and OVtoOFI adapters are excluded.
The currently available OpenFrames plugin uses wx and must not be used as
a Qt widget provider. No Qt OpenFrames support is claimed yet.

The `--screenshot /absolute/path.png` option captures the initialized window
and exits with failure if initialization, optional script interpretation,
or image writing failed. For headless layout inspection set
`QT_QPA_PLATFORM=offscreen`.
Add `--run` to execute the loaded mission before capturing it.

Linux installation has been staged with CMake, relocated to a path containing
spaces, and exercised from an unrelated directory without a startup override.
The installed copy ran the plot
fixture, loaded packaged maps, and resolved GmatBase/GmatUtil from the installed
`bin`. Qt itself is currently a system runtime dependency on Linux; this is not
a self-contained Qt distribution. Windows/macOS deployment remains unverified.
The repeatable launcher check is:

```
python3 src/qtgui/tests/LaunchTests.py /path/to/installed/bin/GmatQt \
  src/qtgui/tests/propagate.script
```

It checks default startup, paths containing spaces, execution and capture from
an unrelated directory, and failure exit codes for missing or invalid inputs.

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
prevent Build/Run from silently using older values. Plot object lists (`Add`),
XY Y-parameter lists (`YVariables`), and report parameter lists (`Add`) accept
comma-separated resource/parameter names. Apply replaces the complete list;
orbit visibility follows object names when reordering, with new objects shown.
Empty report lists can be populated and cleared. References use canonical
resource names (for example, `Luna`). Other arrays, indexed expressions, and
compound properties still require the script editor.

Use **Edit > New resource** or the Resources context menu to add a resource.
The dialog lists the engine's viewable spacecraft, hardware, burn, propagator,
force-model, coordinate-system, solver and subscriber types, plus Variable and
String. Creation validates a complete candidate mission and is one undoable
script edit. Duplicate names, invalid identifiers, stale script snapshots and
pending panel changes are rejected. The resource appears in the tree and its
property panel opens after successful creation. Some resource types require
further configuration before they can execute; array dimensions and specialized
forms remain future work. Force models now have their own Resources category.

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

The workflow suite also creates a spacecraft through the New resource dialog,
checks name/type validation and undo, inserts a propagation command using the
created object, and verifies that it advances by the requested 60 seconds.

The Edit menu routes text actions to the focused editor, including resource
table cells and command panels. It retains the originating editor while a menu
is open, and does not fall back to the script when a navigation tree has focus.
Workflow and Mission tests exercise Paste/Undo/Redo in those panels and verify
that the mission script stays unchanged until Apply.
They also check repeated resource opening and refreshing stale clean panels.
The plotting suite checks Window-menu activation, minimized-window restoration,
closed-window removal, and safe handling of an entry whose window was closed.

This remains an implementation checkpoint, not a completed replacement.
Still required: specialized resource and command forms, compound properties,
plugin compatibility handling, advanced graphics and plot
style parity, wider functional coverage, and platform build/package validation.
Only Linux has been built and exercised so far.

## Mission sequence editing

The Mission tree reflects the engine's nested command graph, including branch
ends and Else blocks. Double-click opens a command statement editor. The
context menu supports insertion before/after, append, and deletion of complete
commands or branches. Structural end commands cannot be removed independently.
Script events are edited as complete blocks. Templates help insert common
commands; specialized command forms remain future work.

Changes use an immutable snapshot of the complete mission with source ranges
matched within each branch. This distinguishes identical statements in different
branches. Applying validates the complete candidate before updating the script
as one undoable edit. Invalid candidates restore the previous engine model;
stale panels and unapplied changes receive the same protection as resource panels.

Build `GmatQtMissionTests` with `GMAT_QT_BUILD_TESTS=ON`, then run:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtMissionTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/mission.script \
  /tmp/gmat-qt-mission.png
```

This real-engine test verifies nested branches, repeated-command identity,
replacement/insertion/deletion/append through numeric execution results,
invalid-edit rollback, retry, stale panels, undo, and Mission-tree-to-Apply
interaction. It has passed on Linux with Qt 6.10.2.

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

The plot suite also exercises subscriber-list edits through the resource panel,
verifies that replacing XY parameters publishes the selected Z values, checks
added orbit-body data and reordered visibility, and rejects malformed,
duplicate, unknown, or noncanonical object names. Report list population and
clearing are checked against the rebuilt engine model.
