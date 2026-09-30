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

The script editor displays line numbers and colors GMAT commands/resource
types, numbers, quoted strings and percent comments. Keyword names are refreshed
from the loaded engine factories, including plugin types. Highlighting follows
the light/dark palette and does not change saved text or undo history. The
line-number margin grows with the document and follows scrolling/font changes.

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

Qt 6.4 or newer with Widgets and OpenGLWidgets development files, plus
OpenSceneGraph 3.6 development libraries, are required. The native orbit view
uses OSG rendering within Qt, without the OpenFrames viewer or its controls.
Texture decoding uses Qt image readers; OSG JPEG plugins are not required.
Spacecraft model files require the corresponding OSG reader plugins (3DS for
the bundled Aura model, OBJ for the synthetic native test). The renderer searches
the configured OSG library plugin directory and an `osgPlugins-<version>`
directory beside the executable, in addition to OSG's usual search paths.
In an existing configured GMAT build:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON
cmake --build build/linux-gui --target GmatQt --parallel 6
application/bin/GmatQt
```

For Windows, use an x64 MSVC build and the matching Qt MSVC development package.
Add `-DCMAKE_PREFIX_PATH=C:/path/to/Qt/6.x/msvc2022_64` to the GMAT configuration,
along with the existing CSPICE and Xerces dependency options. The Qt frontend
can be built with `GMAT_INCLUDE_GUI=OFF`; it does not require wxWidgets.
After configuring, build and deploy the Qt runtime beside the executable:

```
cmake --build build/windows-qt --target deploy-qt --config Release --parallel 6
application\bin\GmatQt.exe
```

The Windows-only `deploy-qt` target uses Qt's `windeployqt` tool to copy its
libraries and platform/image plugins. It does not install the MSVC
redistributable or provide GMAT's other third-party runtime dependencies.
Those must already be available. This target prepares the build directory;
it is not a standalone installer.

The build generates `gmat_startup_qt.txt` beside the executable, including the
enabled native plugin targets with their actual filenames. Building `GmatQt`
also builds those plugins. The wx startup file is untouched. The Qt startup
uses relative paths and is installed in `bin`; Debug build paths account for
the separate debug directory. Use `--startup /absolute/path/to/file` to override
this configuration explicitly. The wx OpenFrames and OVtoOFI adapters are excluded.
The currently available OpenFrames plugin uses wx and must not be used as
a Qt widget provider. No Qt OpenFrames support is claimed yet.
Custom startup files are checked before engine initialization: active
OpenFramesInterface or OVtoOFI plugin entries produce an error in the Message
Window identifying the startup line. Commented entries and unrelated native
plugins are unaffected. This detects these known wx providers; it is not a
general compatibility check for arbitrary third-party plugins. Failed startup
also prevents the command-line mission from being loaded or built.

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
a self-contained Qt distribution. Windows build-directory runtime deployment
has been tested; standalone Windows packaging and macOS deployment remain unverified.
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
Script files use UTF-8. Malformed or truncated UTF-8 is rejected before replacing
the current document, instead of silently substituting or dropping bytes.
GUI messages retain the UTF-8-first, locale-fallback behavior of the earlier fix,
including incomplete sequences at the end of a message.

Mission execution now services Qt events through GMAT's existing interruption
checkpoints. Engine and UI access remain on one thread. Run, Pause, Resume,
and Stop update the toolbar; active execution prevents script/model edits
and nested runs. Closing an active mission requests Stop and retains the
window until execution has unwound. `Moderator::SetUiInterpreter(nullptr)`
now safely detaches the Qt adapter during shutdown.

Command panels provide fields for Maneuver, finite burns, Vary, Achieve,
Minimize, NonlinearConstraint, Report, FindEvents and Target/Optimize headers.
These edit only matched source spans: command labels, comments, option order,
unrecognized options and nested branch bodies are preserved. Unsupported
syntax remains available in the command source editor. Apply still validates
the complete mission; the form itself does not replace engine validation.
Templates include solver branches, variables/goals, optimization constraints
and finite-burn pairs, using available configured resources where possible.

With `ShowProgressWindow = true`, solver branches create Qt MDI tables showing
variables, objectives, goals, desired values and residuals through GMAT's
listener interface. Convergence and failure are explicit. Closed windows do
not invalidate engine listener pointers; subsequent runs recreate them.
Mission stop/failure ends an otherwise unfinished progress indication.
This is current-value feedback, not yet iteration-history plotting.

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
resource names (for example, `Luna`). Spacecraft hardware lists, thruster tanks,
finite-burn thrusters, and force-model primary/point-mass body lists are also
editable. Tank reordering preserves mixture ratios by tank name; newly added
tanks receive a ratio of 1, editable after Apply. Other compound lists and
indexed expressions still require the script editor.

Arrays and writable numeric vector/matrix properties have an **Edit cells…**
grid. It preserves dimensions, rejects nonfinite or nonnumeric values, and
keeps changes local until Apply. Cancel leaves the property unchanged. Array
values serialize as indexed initial assignments and participate in the same
validation, rollback and undo flow as scalar properties. Array grids offer row and column controls, preserve retained cells and fill
new cells with zero. Apply validates the resized mission; Undo restores the
prior dimensions and values. Use **Expressions…** for formulas evaluated at mission start; see the expression
workflow below.

Spacecraft panels open on Orbit and group the existing editable fields under
the familiar Attitude, Ballistic/Mass, Hardware, Power System, SPICE and Visualization
tabs when those fields are available. All Properties retains access to every
supported field. Orbit uses the engine's current state-element labels. The
filter applies within the selected section; switching sections preserves
pending values and Apply validates changes across all sections together.
Thrusters, tanks, solvers and force models also group their available fields
into sections (for example Fuel, Direction, Convergence and Bodies).
These are grouped property controls, not yet the full specialized wx forms.

Use **Edit > New resource** or the Resources context menu to add a resource.
The dialog lists the engine's viewable spacecraft, hardware, burn, propagator,
force-model, coordinate-system, solver and subscriber types, plus Variable and
String and Array. Arrays have row/column controls (1–100 each in this dialog;
larger declarations remain available through the script). Creation validates a complete candidate mission and is one undoable
script edit. Duplicate names, invalid identifiers, stale script snapshots and
pending panel changes are rejected. The resource appears in the tree and its
property panel opens after successful creation. Some resource types require
further configuration before they can execute; specialized force-model and
attitude forms remain future work. Force models have their own Resources category.

The Resources context menu can delete unused resources. GMAT's dependency
checks protect resources referenced by other resources or mission commands;
built-in resources and generated parameters are protected separately. Pending
changes in the target's own panel must be resolved first. Unrelated open panels
do not block deletion. The serialized result is interpreted before committing
one undoable script change; a failure restores the prior engine model.
Unrelated panels retain their existing stale-snapshot protection after a change.

## Functional validation

The complete Qt suite is registered with CTest when `GMAT_QT_BUILD_TESTS=ON`.
From an already configured GMAT build, run:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_QT_BUILD_TESTS=ON
cmake --build build/linux-gui --target check-qt --config Release --parallel 6
```

`check-qt` builds the frontend, enabled native plugins and test executables, then
runs six checks: files, workflow, mission editing, plots, high-DPI plots and the
launcher. To rerun without rebuilding, use
`ctest --test-dir build/linux-gui -C Release -L qt-gui --output-on-failure`.
CTest locks serialize access to shared engine outputs even under parallel CTest.
Each GUI test uses temporary INI settings rather than normal platform settings;
the launcher uses the new `--settings-dir` option for the same isolation.
A small Python launcher supplies runtime DLL search directories on Windows.
The six-test suite passes on Linux and native Windows with Qt 6.10.2.
The Windows run uses MSVC 2022 with the wx GUI disabled. Its initial build
exposed Windows min/max macro collisions; the Qt frontend and its consumers
now define NOMINMAX. See [Windows evidence](Qt6WindowsValidation/README.md).
macOS has not been built or tested.

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
It also exercises deletion through the context menu and confirmation, rejects
resource/command dependencies, protects pending target edits, permits unrelated
open panels, restores deletion with Undo, checks independent variable declarations,
and runs the mission after deleting unused resources.

The Edit menu routes text actions to the focused editor, including resource
table cells and command panels. It retains the originating editor while a menu
is open, and does not fall back to the script when a navigation tree has focus.
Workflow and Mission tests exercise Paste/Undo/Redo in those panels and verify
that the mission script stays unchanged until Apply.
They also check repeated resource opening and refreshing stale clean panels.
The plotting suite checks Window-menu activation, minimized-window restoration,
closed-window removal, and safe handling of an entry whose window was closed.

The requested initial Linux Qt desktop is implemented and verified; see the
[acceptance audit](Qt6LinuxValidation/README.md). It is not a complete wx feature
replacement. Replacing wx entirely would additionally require specialized
resource and command forms, compound-property controls, broader plugin
compatibility, advanced graphics/plot style parity, wider functional coverage,
and platform packaging. Those limitations remain described below.
Linux and Windows have been built and exercised. Windows also passed a native
desktop launch with the deployed Qt runtime and no Qt directory on PATH.
macOS and a standalone Windows installer remain unverified.

### PR-fix audit

The integration baseline `0ef9a8a` is an ancestor of the Qt branch. Existing
non-Qt fixes remain present; Qt equivalents have separate runtime checks:

| Earlier fix | Qt evidence |
| --- | --- |
| Failed reload retains edits and undo (`bfe0c35`) | `GmatQtFileTests`: missing/malformed files leave text, dirty state and undo/redo intact |
| Failed save retains identity (`d8f14d4`) | `GmatQtFileTests`: failed new destination leaves title/dirty state/original bytes intact; later save succeeds |
| UTF-8 messages with locale fallback (`916d796`) | `GmatQtFileTests`: valid Unicode and incomplete trailing sequence |
| Reversible zoom and retained replay (`19dd83b`, `cd2eaca`, `34140f6`) | `GmatQtPlotTests`: inverse zoom and replay without history mutation |
| GroundTrack sampling, sparse data, station validation | `GmatQtPlotTests`: numerical report comparison, collection/retention settings, absent satellites, invalid station |
| Close/reopen and mixed plot safety | `GmatQtPlotTests`: mixed subscribers, close during execution, reopen and repeat run |

Build `GmatQtFileTests` with the other Qt tests and run it without a startup file:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtFileTests
```

These tests exercise the real editor and atomic save implementation in temporary
directories. They do not initialize a mission engine or alter user scripts.
The existing wx/console exit regression remains a separate shared-engine check.

## Mission sequence editing

The Mission tree reflects the engine's nested command graph, including branch
ends and Else blocks. Double-click opens a command statement editor. The
context menu supports insertion before/after, append, and deletion of complete
commands or branches. Structural end commands cannot be removed independently.
Script events are edited as complete blocks. Templates help insert common
commands. A Propagation form supports one spacecraft, one propagator, and a
single parameter/value stop condition, including elapsed seconds or days. Resource
selectors use the current mission; changing the spacecraft also changes an elapsed-
time stop condition. The form and script text stay synchronized, preserving labels,
comments and an existing BackProp modifier. Multiple stops are edited through
Stopping conditions; propagator assignments use Propagators and spacecraft. Periapsis and apoapsis are supported by both stop editors.

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

The workflow suite additionally opens a real Propagate command, verifies form
population and spacecraft selection, preserves an advanced statement, rejects
invalid duration input without changing the mission, and executes a form-selected
0.01-day propagation (864 seconds). Its optional screenshot captures this panel.

## Plotting and output

The Qt receiver now records and renders native OrbitView, GroundTrack,
GroundTrackPlot, XYPlot and DynamicDataDisplay publications. Plot windows use
the central MDI workspace and can be reopened from Output without losing
their recorded samples. The receiver retains numeric data independently of
window lifetime; a rebuild starts a fresh recording. Body and coordinate
system pointers are used only while receiving engine publications, never
during drawing or replay.

New and reopened plots receive focus after being shown, so activating the main
window does not unexpectedly raise the script over them. The default-mission
regression checks both retained samples and visible/frontmost plots after event
processing. A native Linux X11 launch from an unrelated working directory also
ran the default mission and captured its ground-track window in front.

OrbitView uses a native OpenGL surface with textured spherical bodies, shaded
lighting and depth-tested trajectories and spacecraft markers. It retains the
existing orthographic rotation, pan, reversible wheel zoom, Fit, image export,
and replay controls. Texture paths come from the configured celestial bodies;
missing images fall back to the body color. Body-fixed orientations are captured
at each recorded epoch and replayed without consulting live engine objects.
XY and ground-track chart headings shorten to fit narrow windows. Legends wrap
into rows and reserve space above the chart; excess entries use a count instead
of drawing over the data. Hover over the chart for complete title, axis labels
and visible curve names. Wider windows restore full labels automatically.

Orbit and ground-track playback has Start, Play/Pause, Latest and speed controls
(0.25×–4×). Pause/resume retains the selected position; dragging the timeline
pauses playback. Latest follows incoming data, and Play at the end restarts the
retained history. The timeline occupies a separate row so it remains accessible
in narrow tiled windows. Speeds describe a nominal three-second sweep of the
retained history, not elapsed simulation seconds.

Configured spacecraft models retain their texture materials, display scale,
offset and rotation. Like wx, normalized spacecraft display size is exaggerated
(1000 km at ModelScale=1), so it is visible on orbital scales. Fit includes that
size. Missing models fall back to a marker and report a diagnostic. Spacecraft
attitude is recorded with its inertial-to-body convention correctly inverted.

The Sunlight button selects illumination from the recorded Sun ephemeris;
turning it off uses light from the camera for inspection. Sunlight direction
and spacecraft/body orientation follow the replay frame. This is visual diffuse
lighting, including ambient fill, not an eclipse or radiometric simulation.
The orientation regression compares a body-fixed surface point with an independent
coordinate conversion at the first and last recorded epochs.

OrbitView star fields honor `EnableStars` and `StarCount`. The startup file's
`STAR_FILE` catalog supplies equatorial directions and magnitudes. Valid entries
are sorted by brightness; the requested count selects the brightest entries,
capped by the available catalog. Missing/empty catalogs and rejected malformed
rows are reported without preventing a mission from running.

Stars use a fixed 50-degree vertical celestial field of view, independent of the
orthographic scene's pan and zoom. Camera rotation and the recorded inertial-to-
plot-frame rotation determine their directions, including Earth-fixed replay.
They draw behind bodies and spacecraft, without writing depth. Five magnitude
bands control display brightness and point size; this is not a photometric
simulation. The native tests verify count/disable behavior, magnitude, rear-sky
rejection, occlusion, pan/zoom invariance, camera rotation and replay; the real
mission tests independently check Earth-fixed frame conversion.

The offscreen/minimal Qt platforms retain the earlier CPU body-disk renderer;
they do not exercise the native renderer. When Xvfb is available on Linux,
`check-qt` also runs native OpenGL tests at scale 1 and 2 using software Mesa.
Those checks cover texture colors, foreground/background occlusion, body
rotation, portrait resize, model materials, Sun/camera lighting, replay selection,
missing-texture/model fallback, text overlays and repeated viewer lifetime.
`QtGui.NativePlots` additionally runs the real-engine plot workflow with the
native renderer, including close during run, reopening and repeat missions.

The view uses positions already converted by OrbitPlot; non-spacecraft bodies
are converted from the internal frame separately. Scripted cameras support
object or vector references, viewpoints and directions, viewpoint scale, and
signed up axes in the configured view-up coordinate system. Camera states are
captured numerically with the mission frames; replay and reopened plots need no
live engine pointers. Drag/pan/zoom remain offsets from the tracked camera.
**Fit** frames the mission; **Script view** restores the scripted framing.
Native and fallback rendering share the camera basis. Orthographic is the
default, with perspective and vertical field of view available through the
Qt camera controls and imported OpenFrames camera settings. Degenerate eye/target settings
are reported; a parallel up vector gets a stable fallback roll.

Native rendering also honors constellation outlines from `CONSTELLATION_FILE`,
XY/ecliptic reference grids, body wireframe and Sun-direction lines. Catalog
directions follow the recorded plot frame and remain fixed under pan/zoom;
rear-hemisphere segments are clipped. Celestial-body `3DModelFile` assets retain
their physical dimensions and configured scale/offset/rotation, with a sphere
fallback when loading fails. Constellation borders/names and eclipse shadows
remain outside this milestone; constellation outlines are implemented.

XY plots support the ten standard marker shapes, indexed marker changes and
point highlights, distinct dotted/long-dash/short-dash/dash-dot/transparent line
styles, and the error-bar visibility setting. Current-iteration clearing retains
its break anchor for subsequent iterations and bounds old anchors with history
trimming. Tests exercise repeated clears, indexed changes and distinct rendered
styles at normal and high display scales.

GroundTrack uses the engine's geodetic longitude/latitude, resolved body map,
collection/update frequencies, configured point limit and line width. Missing
satellites preserve their curve slots and break the line. Dateline crossings
split at the map boundary. The engine's SubscriberFactory maps the script type
`GroundTrackPlot` to the newer `GroundTrack`; the real-engine plot fixture now
verifies that alias against geodetic report coordinates, map loading, and its
retention limit. The older Cartesian callback is checked separately with a
known spherical projection; the current script factory cannot exercise it.
Ground station
markers reject objects that are not body-fixed points. Replay does not mutate
the retained histories. QPainter handles physical display scaling.
`ShowFootPrints = All` draws the legacy five-degree reference circles, with
spherical geometry and dateline splitting even near a pole. These are not
computed sensor coverage or visibility-horizon footprints.

Dynamic tables preserve displayed values and cell colors when closed and
reopened. Output also lists report files and opens a read-only paged viewer.
Large reports can be inspected without loading the entire file.

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


### Resource, function and plugin workflows

Force-model editors expose owned gravity, atmosphere and radiation-pressure
properties under their canonical names, such as `GravityField.Earth.Degree`.
Spacecraft attitude controls follow the selected attitude model and display
representation; Apply refreshes the fields available in that representation.
Matrix/vector values use a cell editor. Hardware, force-body and supported
event-locator lists validate through reconstruction of the whole mission.

Function calls have argument/output controls beside their script source.
Event locators and functions appear in the resource tree and creation menus.
Solver and event-locator reports are accessible from Output. The Help menu's
**Available engine types…** lists the types registered by this runtime; this
separates actual plugin availability from dedicated Qt form coverage.

Opening an OFI-based example automatically offers **Convert views** before
building it. Build and Run offer the same choice for scripts pasted or edited
in the editor. Accepting validates the conversion and continues; **Keep
original** cancels the build without changing the script. The prompt explains
visual differences, with conversion notes under Details. Unsupported OpenFrames
features get a specific manual-conversion explanation instead of an unknown-type
build failure.

You can also use **Edit > Convert OpenFrames views for Qt**, or explicitly
approve conversion at launch with:

```sh
./application/bin/GmatQt application/samples/Ex_HohmannTransfer.script --convert-views
```

Conversion is explicit, validated, undoable and unsaved. Calculations remain
unchanged; viewer differences are documented in comments and the message
window. It imports common plot flags and named camera views into Qt OrbitView,
including perspective projection and vertical FOV. A camera selector switches
views during playback. Body-relative cameras follow object orientation;
two-frame look-at orientation is retained in both rotation modes. Trajectory views
remain pending. Unsupported
OpenFrames object kinds or dynamic viewer assignments require manual editing.
Keep the original file if it will also be used with the OFI application.

The Linux compatibility tests execute a GMAT function after editing its
arguments, verify the Yukon algebraic sample against its analytic optimum,
and locate eclipse intervals with editable event settings and an Output report.
See [parity acceptance evidence](Qt6ParityValidation/README.md) for the complete
scope and retained limits.


Table columns start with font-aware, content-based widths bounded for long
labels and values. Resource values receive spare initial space; empty numeric
editor button columns are hidden. All visible columns remain manually
resizable: drag a header border, or double-click it to fit the contents. Solver
and dynamic-data updates preserve adjusted widths for the open table. Widths
are not persisted after closing the table or restarting the application.


### Native window lifetime and desktop input

A hidden OpenGL composition anchor is created before showing the main window
on native platforms. Without it, opening the first OrbitView during Run made
Qt destroy and recreate the already-visible native surface. This is documented
[Qt behavior when adding the first QOpenGLWidget dynamically](https://doc.qt.io/qt-6/qopenglwidget.html).
Avoiding that replacement protects native input/focus continuity, especially
when Run was triggered by an input event on Wayland. The anchor lives until
the main window is destroyed; offscreen/minimal platforms retain the CPU path.

`GmatQtWindowTests` checks native-surface stability and exercises title-bar
minimize, Output activation, restore and repeat minimize in the real Qt event
loop. `QtGui.NativeWindows` runs it under Xvfb. To check the actual desktop
graphics path and saved settings without platform/rendering overrides:

```sh
build/linux-gui/src/qtgui/GmatQtWindowTests application/bin/gmat_startup_qt.txt --desktop-settings
```

The test reads saved layout settings but does not save changes. A sleeping
main thread in `QCoreApplication::exec()` alone does not establish a deadlock;
inspect input delivery/native window lifetime as well as rendering.


### Intel graphics stalls and missing OrbitView content

OrbitView uses a single-sample framebuffer. The previous four-sample MSAA
request reproduced Intel Iris Xe (ADL GT2) GPU hangs and context resets on the
GNOME Wayland desktop. The mission had 123 trajectory samples, but the captured
view was black and opening it stalled for several seconds. Disabling MSAA
rendered the textured Earth and orbit on the same hardware without a reset;
hardware acceleration remains enabled. Edges may be less smooth without MSAA.

The native window test now checks Earth-texture and orbit pixels after opening
OrbitView, in addition to window activation and minimize/restore behavior.
An optional fourth argument saves that captured image:

```sh
build/linux-gui/src/qtgui/GmatQtWindowTests application/bin/gmat_startup_qt.txt --desktop-settings /tmp/orbit.png
```

See [desktop GPU validation](Qt6ParityValidation/README.md#desktop-gpu-regression)
for the reproduction and validation evidence. Software-rendered Xvfb tests
alone did not expose this hardware failure.


### Replacement qualification in progress

The [workflow/viewer/plugin checklist](Qt6ReplacementQualification.md) tracks
remaining wx replacement work. Command settings now include For loop bounds
and step, If/While conditions, assignment destinations/expressions, Toggle
subscribers/state, and Global/Clear object lists. Editing branch headers leaves
nested commands unchanged; Apply validates the complete mission.


Resource editors offer **Select…** for references with known engine types and
**Browse…** for filenames. Multi-resource selection preserves existing order;
drag rows to reorder them. Cancel keeps the old selection, and accepted choices
remain pending until Apply. Filename browsing permits new output paths without
creating files. Formation membership is editable and restricted to existing
spacecraft. Text entry remains available for expressions and plugin-defined
references whose choices cannot be enumerated.


XY plots have a **Style…** toolbar action with per-curve visibility, line/marker
controls, width, size, shape, color and error bars, plus plot grid and legend
switches. Choices remain pending while switching between curves. OK applies
them to the displayed plot, including existing samples; Cancel leaves it
unchanged. These are display settings for the current plot, not saved script
properties, and rebuilding the mission resets them.


Orbit plots offer **Camera…**, with **Orthographic / Perspective** and a vertical field-of-view
control (1–150 degrees, before wheel zoom). Orthographic remains the default.
Perspective uses depth-dependent sizing and clips objects behind the camera;
native stars retain translation invariance and follow perspective zoom. These
interactive controls currently affect the displayed plot only. Conversion imports
the selected OpenFrames view’s perspective/FOV into a `% GMAT-Qt-Camera` JSON
comment. Qt validates and restores it when running a saved script; the base
engine treats it as a comment. Resource/mission edits retain these settings.
Use **Keep projection** to write the displayed projection/FOV into the script
as one undoable edit, then save the script normally. It preserves mission
calculations and keeps the current viewer open. Unbuilt script edits and pending
resource-panel changes must be resolved first. Orbit angles, pan and zoom are
not included in this action.


### Array expression cells

Array resource panels offer **Expressions…**, separate from numeric initial
values. Nonempty cells produce ordinary GMAT assignments immediately after
BeginMissionSequence, evaluated in row order before existing commands. Blank
cells have no generated assignment and keep the numeric initial value. Formula
text is retained in a marked script block and shown when reopening the grid.
Apply validates the candidate mission without executing it; Save keeps the
formulas in the script, and Undo restores the previous block. Unknown references
or extra statements are rejected without changing the previous mission.

Apply numeric/dimension edits separately from expression edits. Existing
assignments elsewhere in the mission are left untouched and can still override
these initializations later; edit those assignments through the Mission tree.
This grid does not reinterpret arbitrary existing assignment code as a formula
block. Resource edits preserve the existing mission section, including comments
and formula commands, rather than replacing it with engine-formatted output.


Propagator resource panels expose writable settings from the selected integrator
or ephemeris propagator, including InitialStepSize. Apply preserves the separate
force-model section for numerical propagators. TLE, BulirschStoer and
PrinceDormand853 configuration, save/reopen, execution and recovery scenarios
are covered by the compatibility suite; the qualification checklist records
remaining plugin and specialized-editor gaps.


OpenFrames conversion preserves arbitrary camera up vectors in the optional `up`
array of the `% GMAT-Qt-Camera` comment. Qt applies that vector in the configured
ViewUpCoordinateSystem, preserving camera roll; the standard ViewUpAxis remains
a nearest-axis fallback for the base viewer. Keep projection retains this vector.
An explicit ViewUpAxis edit in the resource panel removes the override, and Undo
restores it. When editing raw script, remove the `up` member to use ViewUpAxis.
Zero, malformed or nonfinite imported vectors are rejected with an explanation.
Trajectory-relative orientation remains open.


### Named camera views

Converted plots with multiple OpenFrames views have a camera selector beside
projection. Each camera retains its own projection/FOV and recorded tracking
history. Switching restores that view's scripted pose, resetting interactive
rotation, pan and zoom, and follows the same replay time without rerunning the
mission. Closing and reopening the plot retains the selected camera; a fresh
mission run starts with the first view. Keep projection updates only the selected
camera, and normal Save preserves all named definitions.

Additional views preserve stored Current/Default eye, center and up vectors.
Object references track positions, including center offsets. With InertialFrame
Off, body-relative views also follow object orientation; InertialFrame On retains
plot-frame axes. Named whole-trajectory views support stored and automatic
framing; segment views remain pending. Body views without a stored location use the rendered body/model bounds. Duplicate/unknown view names, invalid poses
and missing reference objects are rejected before execution. The base viewer
uses the first camera and ignores the additional Qt metadata.


Body-relative camera settings are retained in Qt metadata for the primary and
additional views. Eye, center offset and up rotate from body axes into the plot
coordinate system at each recorded sample. This applies to both celestial bodies
and spacecraft, with their different attitude-matrix conventions. Playback uses
these recorded poses without consulting live mission objects. Stored Current
location takes precedence over Default location; omitted stored components use
OF defaults, and malformed vectors are rejected. An explicit ViewDirection edit
clears an imported primary center offset. A body-relative primary view requires
an object reference and a vector eye offset; incompatible edits are rejected and
the previous mission is restored.


Two-frame LookAtFrame conversion rotates the entire stored eye/center/up pose
so its local +Y direction follows the reference-to-target vector. ShortestAngle
On uses the direct rotation; Off uses azimuth then elevation. The reference frame
can follow body attitude or retain plot-frame axes. This preserves the OF roll
behavior and differs from simply moving the center to the target object.
Coincident origins use the unaligned reference frame. Keep projection and
save/reopen preserve alignment modes; explicitly editing ViewDirection restores
standard OrbitView direction semantics, and Undo restores the imported alignment.

`tests/OpenFramesCameraReference.cpp` is an optional developer qualification probe
against an installed OpenFrames library, not a dependency of GmatQt. It compares
the shared alignment helper with the actual FollowingTrackball transforms in
absolute/body-relative modes, both rotation modes and singular directions.


### Save command

When the Save plugin is loaded, the mission editor offers a Save command template
and an Objects field. This command exports the current values of named resources
into the configured output directory; it does not save the mission script.
The shipped plugin writes one file named from the selected objects (for example,
`SavedSat_SavedNumber.data`). Repeated execution of the same Save command within
a loop appends snapshots; starting a new mission run replaces its previous file.
Exported resource definitions can be opened through File > Open.

The plugin now reports output-open, write and close failures, including the file
path, instead of reporting a successful mission with missing data. Rebuilding and
running after correcting the output path recovers normally. Copying commands and
reinitializing streams are safe, and export preserves each object's comment flags.


### Coordinate-system axes

Coordinate-system resource panels now offer **Axes…**. Choose an axis type and
edit its dependent properties together, then Apply to validate the complete
coordinate system. Changing the type resets pending axis edits; Cancel leaves
the mission unchanged. Apply pending edits in the outer resource panel first.
Primary/secondary objects use reference pickers, ObjectReferenced directions use
R/V/N selectors, and epoch fields are labeled A1ModJulian. Type-only changes also
apply correctly; previously the engine's generic Axes setter silently did nothing.

The main resource table exposes the selected axis model's writable properties.
ObjectReferenced settings require distinct primary/secondary objects and exactly
two distinct directions, leaving the third blank. Built-in coordinate systems
remain protected. Successful edits are undoable, and reopening reads the current
axis model. LocalAlignedConstrained edits reject zero or nonfinite vectors,
parallel alignment/constraint vectors, an alignment reference equal to the origin,
and a constraint coordinate system that references itself. Runtime geometry can
still become singular as objects move.

Workflow checks now cover MOEEq epoch edits and a Sun-aligned
LocalAlignedConstrained frame, including numerical transforms after save/reopen.
Other axis modes, dependency combinations and time-varying singularities still
need broader qualification.

### Report-file settings

ReportFile's Delimiter control names Space, Tab, Comma, Semicolon and Pipe; a
custom single ASCII delimiter can also be entered. Apply rejects empty,
multicharacter, quote and control-character delimiters other than tab instead of
silently accepting the engine's first-character truncation.

The Add list accepts whole parameters and array elements with positive numeric
indices, for example `Results(1,2), Sat.X`. Commas inside the array indices stay
part of the reference, including when reopening the selection list. Expressions
as array indices and the full wx parameter-selection workflow remain pending.


Report formatting checks cover fixed-width headers, ColumnWidth, left/right
justification and zero-filled significant digits. With AppendToExistingFile true,
each run adds its output (including headers when enabled) to the existing file;
with it false, a run replaces the file. Output report windows show their full
file path in the title and display read-only, unwrapped text. Large reports use
bounded pages with navigation to the complete file.


### Choosing report parameters

Report command settings include **Select…** beside Report file and Parameters.
ReportFile's Add property uses the same parameter selector. Choose an existing
reportable parameter or enter a reference, then add it to the ordered list. For
arrays, choose a row and column and use **Add element**, or use **Add parameter**
for the whole array. Remove, Up/Down and dragging control the selected order.
Cancel preserves the previous selection; applying the command or resource validates
references. New property references can also be constructed with the browser below
or entered directly.


The report selector now also offers **Browse object properties**. Select an object,
a reportable property and, when needed, its coordinate system, central body or
force model. **Use reference** fills the parameter entry; **Add parameter** adds it
to the selected order. Body-fixed properties filter incompatible coordinate
systems. Owned attitude properties and attached hardware are also available. Hardware
choices include only direct attachments of the selected object; typed references
and already-configured parameters remain available.


Hardware browsing checks cover fuel mass and a chemical-thruster coefficient,
including exclusion of unattached resources and a spacecraft with no attachments.
Owned attitude parameters use `Spacecraft.Property`; attached hardware uses
`Spacecraft.Hardware.Property`. The selector disables **Use reference** when a
required hardware dependency has no valid choice.


Force-model property browsing includes **SMADot** and **TLONGDot**. Their selected
force-model references are checked through save/reopen and numerical reports.
The shared engine also fixes TLONGDot's zero-perturbation case: an unperturbed
orbit retains its nonzero instantaneous angular rate instead of reporting zero.


### Mission command resource selection

Mission command forms now offer **Select…** beside burn, Maneuver spacecraft,
solver, event-locator and function references. Pickers show compatible configured
resources, and Cancel preserves the current field. Labels, comments and surrounding
command text remain intact. Vary accepts targeting and optimization solvers;
Target/Achieve and optimization commands filter to their respective solver types.
BeginFiniteBurn and EndFiniteBurn also provide a spacecraft picker; the engine
supports one spacecraft per finite-burn command. Maneuver includes a Backprop
checkbox for applying its burn backwards in time.


Toggle commands offer a **Select…** checklist for outputs and an **On/Off** dropdown.
Choose one or more subscribers; Cancel leaves the command unchanged. The checklist
supports dragging to retain your preferred order. Changes still go through command
Apply before updating the mission.


Global, Clear and Save command forms now have **Select…** object checklists.
Choose configured resources or user variables, arrays and strings. Global omits
resources that are automatically global from new choices; Save and Clear retain
those choices. Computed parameter values are not objects in this selector.
Cancel preserves the command, and Apply still validates it before execution.

### Script search and replacement

Edit → Find/Replace opens a reusable, nonmodal script search dialog. Find next
and previous use the standard shortcuts and wrap at document boundaries. Search
and replacement histories retain the last 12 entries for the session. Match case
and Whole words are optional; searches are literal text. Replace affects the
selected matching occurrence, while Replace All is one Undo operation and never
searches inside its newly inserted replacement text. Empty replacement deletes
matches. Read-only scripts cannot be changed. Search actions bring the Script
window forward, and failed searches report a visible message.

### Resource file selection

Browse uses an existing-file picker for known function, spacecraft model/SPAD,
ground-map, data-interface, thrust-history, custom-FOV and file-ephemeris inputs.
Report, ephemeris, event and estimator output paths use a Save-style picker and
may name files that do not yet exist. Selecting a destination does not write or
overwrite it; normal Apply/run handling still controls output creation. Cancel
preserves the current property, and accepted selections remain pending until
Apply. Unclassified plugin filename fields retain the general path picker.

### If/While condition builder

The Condition field includes **Edit conditions…** for comparisons of numeric
values, parameters and positive numeric array indices. Rows provide left/right
operands, all six GMAT comparison operators and AND/OR joins. Add and remove rows
as needed; Choose operand opens the shared parameter browser for the selected
operand cell. Incomplete rows disable OK. Cancel leaves the header untouched.
Accepting updates only the condition span; the command editor's Apply still
validates the mission and supplies Undo. Unsupported grouped or expression syntax
remains in the editable text field and is never flattened into rows.

### Single-parameter mission selection

Condition operands, Achieve goals/values, Minimize objectives and nonlinear
constraint operands now use a single-choice parameter browser. Select a configured
parameter, browse an object's property and reference frame, or choose an array
element. OK uses that one entry directly; there is no report list to assemble.
Empty entries and bare arrays disable OK. Report and ReportFile parameter lists
retain their ordered multi-selection workflow. Command Apply still validates the
chosen reference in its specific context.

Vary's variable picker filters for writable numeric parameters and array elements.
The assignment destination picker also offers user strings and whole arrays.
User variables remain selectable even though their system-parameter metadata does
not mark them as settable. Known read-only properties are omitted from property
browsing and cannot be accepted by typing them into these writable pickers.
Literal numbers cannot be selected as writable destinations. The original command
text fields still support direct editing and engine validation on Apply.

### Spacecraft SPICE kernel lists

The spacecraft SPICE section now includes orbit, attitude, spacecraft-clock and
frame kernel lists. Browse opens an ordered list: Add files accepts existing files,
Remove selected deletes entries, and dragging changes order. Duplicate paths are
not added twice. Cancel discards dialog edits; OK leaves them pending in the
resource editor until Apply. Applying replaces the complete list, so removal and
clearing work as expected. Paths with spaces or commas are retained; paths with
apostrophes, semicolons or embedded line breaks are not accepted by this editor.

SPICE orbit/attitude qualification now includes a shortened Mars Express example
configured through the kernel lists, Qt view conversion, save/reopen and report
comparison. Restoring an unavailable clock kernel permits rerunning without
restarting GMAT. The bundled example references a frame kernel absent from this
checkout, so the qualification fixture omits that file; the unmodified example
still requires its missing input.

### Spacecraft colors

OrbitColor and TargetColor now appear in the spacecraft Visualization section.
Choose color opens a visual picker; its button previews the current color. You
can also enter a GMAT color name or an RGB triple such as `[23 145 210]`. Selection
is pending until Apply, and Cancel leaves the value unchanged. Colors persist in
the mission script. Kernel-list properties are grouped together in the SPICE tab,
including attitude and spacecraft-clock kernel fields.

### Solver command options

Target/Optimize forms use dropdowns for SolveMode and ExitMode and a checkbox for
ShowProgressWindow. **Add default options** inserts only omitted settings using
the engine defaults, preserving existing settings and pending solver edits. The
action changes the command source for review; Apply validates it normally.
FindEvents' explicit Append option also uses a checkbox. Unrecognized values
remain represented rather than silently becoming a different setting.

### Thruster coefficients

Chemical and electric thrusters have a **Coefficients…** editor with separate
thrust and impulse/mass-flow tabs. Each row shows the coefficient name, editable
value and engine-provided unit. Columns start sized to their contents and can be
resized. The dialog includes pending property edits; Cancel leaves them unchanged.
OK checks every value is a finite number and returns the complete set to the
resource panel. Apply validates and saves the thruster changes.

FindEvents also offers **Add default options** when Append is omitted. Its Append
checkbox controls whether the event report keeps existing output or replaces it.
Adding the option preserves the locator selection, command label and comments.

### Camera controls in tiled viewers

**Camera…** opens a separate panel for the imported camera selector, projection,
field of view and **Keep projection**. These controls remain accessible when the
plot is narrow or tiled. Changes preview immediately; Close dismisses the panel
without reverting the preview. Reopening retains the current settings. Script
view, Fit and Sunlight remain available on the viewer toolbar.

### Live orbit display controls

**Display…** opens controls for axes, grid, object labels, legend, XY/ecliptic
planes, wireframe bodies and the origin–Sun line. Changes update the current view
without rerunning the mission or clearing replay history. Close retains the live
settings. These controls do not save script properties; use the plot resource
editor for persistent settings. Reopening the panel reflects the current view.

### Thruster tanks and mixtures

**Tanks and mixtures…** edits the ordered tank list and its ratios together. Add
selects a configured tank and starts its ratio at 1; Remove drops the selected row.
Up/Down move the tank and ratio as a pair. Ratios must be finite and positive but
do not need to sum to one. Columns are adjustable. Cancel leaves pending values
unchanged; OK returns both fields to the resource panel, and Apply updates the
mission. The separate property controls remain available.

Perspective **Fit** now uses the narrower of the horizontal and vertical viewing
angles. Tall, narrow tiled windows therefore keep the fitted scene inside both
edges. Stored camera distances remain unchanged unless Fit is selected.

### Stored OpenFrames trajectory views

Conversion now preserves whole-trajectory views with a stored Current or Default
camera location in the plot frame. They no longer track the spacecraft position.
The named trajectory object must be in the plot's Add list (or use
CoordinateSystem). Primary and additional named cameras are supported.
Automatic named whole-trajectory framing is also supported as described below.
Segment-relative views still require further implementation; conversion explains
those cases and leaves the script unchanged.

### Large report navigation

Report windows provide First, Previous, Next, Last and a page-number control.
Each page loads roughly 1 MiB; rows may continue across pages, but UTF-8 characters
and Windows line endings remain intact. **Find…** searches the displayed page.
**Reload** reads the current file again, including replacements or appended data;
if the file shrank, the page is adjusted automatically. Read failures retain the
previous display and show an error. Viewing a report does not modify its file.

Report windows also provide **Search file**, which starts at the beginning and
searches the complete file for the entered text, matching case exactly. **Next
match** continues after the current result; **Stop** cancels a scan. Searches run
in bounded chunks, navigate to the matching page, and highlight the visible part
of the result. A message identifies matches continuing on the next page. The
existing **Find…** control remains available for richer searches within one page.

### Propagate stop-condition selectors

The Propagation form has **Choose…** controls for the stop parameter and stop
value. They use the parameter browser with its object and coordinate-system
choices. Seconds/Days remain quick selections; another parameter is shown as
Other parameter. Labels, comments and existing backward-propagation modifiers
are retained when fields change. Apply validates the complete command.

The stop-parameter picker includes **Periapsis** and **Apoapsis**, with a central
body selector. Choosing either disables the stop-value field and removes the
numeric goal from the command. Existing labels and comments remain intact. Choose
a scalar stop parameter again to return to a parameter/value condition.

Propagation controls also include **Propagate backwards** and **Stop tolerance**.
The tolerance field shows GMAT's current default as its placeholder; leaving it
blank omits the option. Enter a positive tolerance to override it. Existing option
formatting, labels and comments are preserved when editing represented commands.

### Multiple propagation stops

**Stopping conditions…** opens an editable parameter/goal table in the command
editor. Add or remove conditions, move rows with their goals, or use the parameter
pickers. Periapsis/apoapsis rows have no editable goal. Columns are adjustable.
Propagation stops when any condition is satisfied. OK updates the source as one
undoable edit; Cancel leaves it unchanged, and Apply validates the mission.
Existing StopTolerance and OrbitColor options remain in the command.

### Propagator assignments

**Propagators and spacecraft…** opens an adjustable table of propagators and their
spacecraft or formations. Add/remove rows, select a propagator, and choose one or
more objects for each row. The object picker also supports dragging to reorder.
Each object must appear only once, and empty assignments prevent acceptance.
Select Independent or Synchronized mode and optionally propagate backwards.
OK changes the command source as one undoable edit; Cancel leaves it unchanged.
Apply validates the complete mission. Existing stop conditions, options, labels
and comments remain intact. **Propagate STM** and **Compute A-matrix** apply to
all spacecraft in the command, matching the wx controls. Existing quoted or
unquoted STM/AMatrix flags load into these checkboxes. Covariance propagation
currently requires the source editor.

### For-loop selectors

The For-loop command form includes a variable-only selector for Index and
parameter browsers for Start, Step and End. You can still type numeric bounds
directly. Cancel leaves the field unchanged. Header edits preserve the loop body;
Apply validates the command through GMAT.

### Applying array values and formulas together

Array numeric values and expression cells can now be changed in one resource
panel and submitted together with Apply. Validation succeeds for the complete
candidate or leaves both unchanged. Undo restores both as one edit. A resize that
would remove an existing expression cell is rejected; clear that formula first
or include its removal in the same Apply.

The expression grid uses the pending numeric grid dimensions, so formulas can be
entered in newly added cells before Apply. Existing formulas outside a pending
shrink remain visible in highlighted cells. Clear those formulas to accept the
expression edit, or Cancel and enlarge the numeric grid. Cancel preserves formulas.

### Power-system properties

Solar and nuclear power-system resources group general settings and bus
coefficients into tabs. Solar systems also have Solar coefficients and Shadow
tabs. ShadowBodies now has an ordered celestial-body selector; selections stay
pending until Apply. Invalid names are rejected without changing the script.
Changing EpochFormat converts the pending InitialEpoch while preserving the same
instant. Invalid dates leave both pending fields unchanged, with an error message.
Apply validates and stores the format and date together.

Solar ShadowModel offers None and DualCone in a dropdown, matching GMAT's supported
choices. Selecting None disables shadow attenuation when the mission runs.

Spacecraft DateFormat uses the same conversion behavior: choosing a new format
converts the pending Epoch instead of reinterpreting its text. Invalid dates
restore the previous format and stay available for correction. Apply stores the
converted epoch and format together.

### Spacecraft state representation

DisplayStateType offers the engine's representations compatible with the current
coordinate frame. Changing it converts the six pending element values and updates
their labels and units. Invalid values leave the selection and fields unchanged.
Apply coordinate-system, epoch or anomaly changes before converting the state
representation. Other pending properties remain available for the same Apply.

### Editing GMAT function files

Open a GmatFunction resource and choose **Edit function file…** to edit the file
at its FunctionPath inside Qt. The editor provides syntax highlighting, line
numbers, find/replace, Save and Cancel. Save updates the function file; Cancel
discards the dialog's pending edits. Run rebuilds the mission before executing it.

Saving preserves UTF-8 BOMs and uniform CRLF line endings. If another program
changes the file while it is open, Save leaves that file untouched and asks you to
reopen it. Unreadable or invalid UTF-8 files cannot be saved through this editor.
Save As writes a copy and updates FunctionPath as a pending resource change.
Apply that change to make the mission use the copy. Existing-file replacement
requires confirmation; failed saves leave the current path unchanged.

**New function file…** creates a file from a basic input/output template using the
resource's name. Choose an unused destination, edit the template, and Save. The
file is created only on Save; Cancel creates nothing. Apply the pending
FunctionPath to connect it to the mission. Existing files use Edit function file.

### Function-call argument selectors

The function-call form offers **Select arguments…** beside Inputs and Outputs.
Add, remove or reorder arguments; their order determines the function's input and
output positions. Whole supported objects, variables, strings, arrays and array
elements are available alongside the property browser. Output choices filter
writable parameters. The text fields remain editable, and Apply validates the
function signature. Quoted commas and array-index commas remain within one
argument when loading the selector. Cancel leaves the command unchanged.

### Omitted Vary and Achieve options

Use **Add default options** in a Vary or Achieve command form to expose settings
omitted from the script. Vary adds missing perturbation, bounds, maximum step and
scale-factor fields; Achieve adds tolerance. Values come from GMAT's command
defaults. Existing options, pending edits, labels and comments are retained.
Edit the added fields, then Apply to validate and update the mission.

Vary option fields follow the selected solver's capabilities. Unsupported
settings are disabled with an explanation, and their values are retained when
switching solvers. Select a configured solver to enable supported settings.

### DifferentialCorrector settings

The solver resource editor offers algorithm and derivative-method dropdowns in
Convergence. Output contains report style, report destination and progress
settings. Browse chooses a report output path; Apply stores the settings and
running the mission writes the report.

### Fit translated orbit scenes

Fit centers the visible objects and retained trajectories, so scenes far from the
coordinate origin use the window effectively. Hidden curves do not affect Fit.
Framing stays stable during replay. Script view restores the scripted camera.

### Automatic whole-trajectory views

When an OpenFrames view selects ViewTrajectory for a named plotted object and has
no stored camera location, conversion now creates an automatic trajectory camera.
It frames that object's retained path and updates while the mission runs. Named
views retain their own target paths. Replay keeps framing stable; Fit frames the
whole scene, and Script view restores the selected trajectory view. Automatic LookAt targets retain both ShortestAngle modes, including roll and
bounding-center orientation. Automatic CoordinateSystem views also retain the
OpenFrames default radius and viewport-aware distance, with or without LookAt.
Segment cameras remain unsupported.

Editing the primary OrbitView camera's reference, position, scale, direction or
up settings in the resource editor turns off imported automatic trajectory
framing for that camera. Your explicit pose then takes effect. Additional named
views retain their own settings, and Undo restores automatic framing.

Automatic CoordinateSystem views without LookAt use OpenFrames' twelve-Earth-
radius framing. With LookAt they use its root-frame one-unit radius fallback.
Stored Current/Default locations still take precedence. Fit remains available
for framing the actual visible scene.

### Automatic body cameras

Converted views of a planet or spacecraft without a stored camera location now
frame that object's displayed size. Native model framing includes model scale,
rotation and offset, while retaining Qt's model display convention. Relative and
LookAt orientation continue to follow the selected view. Manual primary-camera
pose edits override automatic body framing; Undo restores it.


The spacecraft property panel offers **Ballistics and mass…** for the compact
Spherical and SPAD file controls from the wx interface. Enter mass, coefficients,
areas and SPAD scales; use Browse for the input files and the interpolation
choices for each file. OK keeps changes pending in the property panel. Apply
updates the mission, and Cancel leaves the pending values unchanged.


Spacecraft tank and thruster attachment pickers include **Select all** and
**Clear selection**. Individual checkboxes and row dragging control membership
and order. OK keeps those choices pending until Apply. Clearing every attachment
is supported. The spacecraft **PowerSystem** dropdown offers available nuclear
and solar systems and **No power system** for detachment. Remove dependent mission
references before detaching hardware that those commands or reports use.


**Visual model…** opens the spacecraft's model preview beside rotation,
translation, scale and color controls. Browse loads a new model and resets its
pose; Recenter zeros the offsets and Autoscale restores scale one. Show Earth
adds a wireframe size reference. Drag/wheel and Fit adjust the preview view.
Invalid inputs keep the last valid preview while you correct them. Numeric
values follow GMAT's supported ranges. OK keeps changes pending in the
spacecraft panel; Apply updates the mission. The splitter and scrolling controls
keep the editor usable on smaller displays.

**Attitude…** opens a focused spacecraft attitude editor. Model selection updates
the available fields and retains each model's pending settings during the dialog.
Orientation and rate selectors convert pending values before displaying the new
representation. Invalid values remain available for correction. Reference frame
controls respect the model's restrictions; reference bodies and AEM input files
have dedicated selectors. OK keeps the selected model's settings pending until
spacecraft Apply, together with other spacecraft changes.


XY plot resources now provide **XY plot setup…** with Show plot, Show grid,
Solver iterations, a single X parameter and an ordered Y list. Select opens a
numeric parameter browser with object properties, reference frames and indexed
array elements. Remove, Up/Down and drag reordering edit the Y list. Strings and
bare arrays are excluded; turn off Show plot to keep a plot with no Y selections.
OK retains edits in the resource panel, and Apply validates and saves them to
the mission. Cancel leaves the pending resource values unchanged.
