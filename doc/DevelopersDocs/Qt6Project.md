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

The first build from this checkout completed on 2026-10-01, compiling the engine,
Qt frontend and selected native plugins without copied object files. The actual
application is now rebuilt here; subsequent builds are incremental. The runtime
snapshot above records the initial bootstrap. The preset is machine-local; on
another machine use the general setup instructions
in `Qt6Gui.md` with the dependency locations for that system.

The shipped `Ex_ExternalForceModel` example also needs the public Python API.
The local preset now enables Python API generation (Java and MATLAB stay off)
and builds `gmat_py314`, `station_py314` and `navigation_py314` with the GUI.
SWIG 4.4.0 is extracted under `build/tools/swig/local`; the preset supplies its
executable and `SWIG_LIB`, without installing a system package. The Python
version comes from this machine's Python 3.14 development installation. On a
fresh machine, use its installed SWIG/Python locations and matching target suffix.

For an incremental API build using this preset:

```sh
cmake --build --preset qt-linux --target gmat_py314 station_py314 navigation_py314 --parallel 2
```

The version-specific `gmatpy` initializer is now generated for development output,
so installing GMAT is not required to import the built API. The shipped external
force callback resolves its API paths relative to its own file. The unchanged
mission script completed successfully through Qt after these setup repairs.

The optional VF13ad optimizer is available from [Thinking Systems](https://www.thinksysinc.com/downloads.html)
as a free binary for R2026a/Linux under its HSL incorporation distribution license.
Its archive is not part of this repository. The local copy is in the ignored
`application/plugins/thinksys/vf13ad/` directory, including its distributor README.
The R2026a Ubuntu archive SHA256 is
`60b6d4fc78883f0ec1d826d831d8557f12a5a4964286f45aaff408df1e8a1bf3`;
its README still labels the release R2025a, while its ELF soname/dependencies name
R2026a. A real algebraic mission confirmed loading and convergence against the
current engine before it was enabled. This does not prove every solver regime.

To retain separately installed engine plugins across startup generation, set
`GMAT_QT_EXTERNAL_PLUGINS` in the configure cache/preset to the same extensionless
paths used in GMAT startup `PLUGIN` lines. The local value is
`../plugins/thinksys/vf13ad/libVF13adOptimizer`. Multiple paths use a CMake list.
The Qt build writes the startup entries but does not install these external
libraries; packaging/deployment must supply them separately under their licenses.
wx OpenFrames window providers remain incompatible with the Qt application.

Open `/home/dan/GIT/GMAT-Qt` as a separate Codex project for future work. Creating
the directory does not automatically register a project in the desktop sidebar.
Do not resume live desktop tests until the GNOME Shell crash/safety issue is
resolved; the current focused checks use the offscreen Qt platform.
