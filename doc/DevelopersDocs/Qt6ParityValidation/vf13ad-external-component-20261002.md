# Optional VF13ad component — 2026-10-02

The selected Qt runtime lacked VF13ad. The distributor's current
[download page](https://www.thinksysinc.com/downloads.html) provides a free,
binary-only R2026a Ubuntu package under its HSL incorporation distribution
license. This is an available optional component, rather than an accepted
missing-proprietary-component exception. No account, purchase, system package,
or contact with the distributor was needed.

- Archive: `https://www.thinksysinc.com/downloads/GMATR2026a/VF13ad_Ubuntu.tar.gz`
- Archive SHA256: `60b6d4fc78883f0ec1d826d831d8557f12a5a4964286f45aaff408df1e8a1bf3`
- Size: 85964 bytes; versioned ELF size: 256136 bytes.
- ELF soname: `libVF13adOptimizer.so.R2026a`; required GMAT libraries:
  `libGmatBase.so.R2026a` and `libGmatUtil.so.R2026a`.
- Archive README still says R2025a, while its soname and dependencies identify
  R2026a. The README and library are retained locally together.

Before enabling it, the actual unchanged `NeedVF13ad/Ex_AlgebraicOptimization`
mission ran offscreen against an isolated startup pointing to the extracted
library in /tmp. It loaded, converged to X1=X2=2 with cost approximately
1.57772181044e-30, and completed (0.7666 seconds process duration).
Raw stdout/stderr/startup/report/GMAT log are in `/tmp/gmat-vf13ad-probe`.
This preliminary compatibility probe predates the runner's plugin snapshots;
it does not establish full ABI or solver-regime compatibility.

The library is now installed only in this checkout's ignored
`application/plugins/thinksys/vf13ad/` directory. CMake's optional
`GMAT_QT_EXTERNAL_PLUGINS` list retains its startup entry through rebuilds.
No external binary or license is redistributed by a commit or CMake install.
Qt excludes the wx OpenFrames window providers as before. The 20 selected
built plugin entries remain, plus this optional external optimizer.

The original algebraic mission was subsequently qualified through the generated
user startup, using the installed path and retained plugin/core/source snapshots.
Its generated-startup build passed 0.420 seconds and run passed 1.132 seconds.
The other formerly failing VF13ad examples are being checked only after this
concrete dependency repair. The main shipped-example ledger records their exact
results and distinguishes timeouts from completed missions. Native desktop
qualification remains stopped after the GNOME Shell crash.

The missing MarsGRAM2005 component is separate. The local ForceModel manual
requires libMarsGRAM plus the legacy model data under
`data/atmosphere/MarsGRAM2005/binFiles`. NASA's current
[GRAM Suite catalog](https://software.nasa.gov/software/MFS-33888-1) identifies
a general-public release delivered through a software request, using a newer
C++ framework. That catalog does not provide this checkout's missing legacy
GMAT plugin or establish compatibility with MarsGRAM2005. No substitute
atmosphere model, account request, or message was introduced. Its exact missing
plugin/data requirements remain open; it is not classified as proprietary.
