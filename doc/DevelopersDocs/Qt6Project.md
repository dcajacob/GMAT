# Linux Qt development checkout

Use `/home/dan/GIT/GMAT-Qt` for ongoing Qt development on `codex/qt6-gui`.
This is a separate clone of the branch in `https://github.com/dcajacob/GMAT`;
`origin` is the user's fork and `upstream` is `https://github.com/nasa/GMAT`.
The original `/home/dan/GIT/GMAT/GMAT` directory is retained. The branch keeps
GMAT's history, engine and project layout; this is not a numerical-engine fork.

Read `Qt6Handoff.md`, `Qt6Gui.md`, and the latest appendices/acceptance gates in
`Qt6ReplacementQualification.md` before continuing. Historical evidence contains
the original checkout's absolute paths and remains unchanged.

Run the application from this checkout:

```sh
cd /home/dan/GIT/GMAT-Qt/application/bin
./GmatQt
```

The initial executable, core libraries, selected native plugin libraries and
startup file are copied from the just-rebuilt Qt runtime. ELF runtime search
paths are relocated to this checkout; user settings and generated mission output
are not copied. External OSG/CSPICE dependency installations remain shared.
`qt-runtime-snapshot.json` records the source commit and copied plugin inventory.
This reuses the current runtime without repeating a full engine build or old
qualification matrices. It does not close the remaining native desktop gates.

A machine-local, ignored `CMakeUserPresets.json` supplies the installed dependency
paths and a fresh build directory. It enables Qt and the selected native plugins,
and disables wx GUI, wx OpenFrames adapters and MATLAB. Windows/macOS remain
deferred. Configure and build from the project root:

```sh
cmake --preset qt-linux
cmake --build --preset qt-linux
```

The fresh build directory contains no copied object files. Its first build must
compile the engine and frontend; subsequent builds are incremental. The existing
application is immediately available while that build has not been run. The
preset is machine-local; on another machine use the general setup instructions
in `Qt6Gui.md` with the dependency locations for that system.

Open `/home/dan/GIT/GMAT-Qt` as a separate Codex project for future work. Creating
the directory does not automatically register a project in the desktop sidebar.
Do not resume live desktop tests until the GNOME Shell crash/safety issue is
resolved; the current focused checks use the offscreen Qt platform.
