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

## Current evidence and remaining work

The initial shell compiles with Qt 6.10.2 on Linux, initializes the engine,
loads the default mission and renders the familiar layout. File loading
checks reads before replacing the editor; saving uses QSaveFile and only
changes document identity and modified state after successful commit.

This is an implementation checkpoint, not a completed replacement. Still
required: responsive mission execution and stop/pause, actual orbit/ground
track/XY plots, editable resource and mission panels, output navigation,
script/GUI synchronization, plugin compatibility handling, automated
functional tests, and platform build/package validation. The initial Run
action is synchronous and the initial resource inspector is read-only.
Only Linux compilation and startup have been verified so far.
