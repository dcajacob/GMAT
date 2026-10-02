# Private Wayland input, viewer lifecycle and native GTK files — 2026-10-02

This bounded route demonstrates actual Qt Wayland keyboard/pointer input, native
software-rendered viewers, main-window minimize/restore, pending Cancel/Discard
and byte-exact Save As in an owned private GNOME Shell/Mutter session. It replaces
the earlier preparation-only disposition for these particular cases. It does not
qualify the user's host GNOME session, hardware drivers, desktop portals or the
full Linux replacement goal.

## Recorded runtime and source

The qualifying session is `isolated-wayland-9qp3tryb`, originally under
`/tmp/gmat-isolated-wayland/root-lifecycle-evidence/`. Launch-time identities,
stat metadata, namespace arguments, actions, pointer readbacks, all 24 screenshots,
logs and final authored source are preserved under the ignored durable directory
`build/example-qualification/20261002/wayland/root-lifecycle-evidence/isolated-wayland-9qp3tryb/`.
`wayland/preservation-index.json` records copied-file SHA256/size and selected
screenshot provenance. These are the recorded launched bytes, not a claim about
any executable rebuilt after the session.

| Launch file | SHA256 |
| --- | --- |
| `application/bin/GmatQt-R2026a` | `c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3` |
| `application/bin/libGmatBase.so.R2026a` | `68a79c56f22c22407ba93f462e73fad4c4050d0cc9f54b0203b79f0d471085c3` |
| `application/bin/libGmatUtil.so.R2026a` | `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc` |
| Launched `IsolatedWaylandSession.py` | `ee8368a88d1b79be22bb127e6e04afe7f2a562443323374e896e6a6c0c2d944d` |
| Selected `gmat_startup_qt.txt` | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |
| Isolated startup, OUTPUT_PATH only changed | `85a6b6f87261253c0b9422a996acf2a22e458beba156f2a70b12710433b6bfd0` |
| Copied and final saved authored fixture, 5371 bytes | `2d30468533a50b6495871e381803045d687fc0429b504b4d6739f2b8cd37e8a4` |

The application/core predate the subsequent named-arc and tree-keyboard changes;
the helper predates the optional portal theme support. This session cannot
validate those changes. It uses the previously independently GUI-authored
[shared three-viewer fixture](shared-three-authored-20261002.script), copied into
`/evidence/fixture.script`. It is not a new Help tutorial construction. The
5371-byte original fixture, copied fixture and final `wayland-authored.script`
were compared after session closure and are byte-identical.

## Isolation and actual input

`session.json` records bwrap `--unshare-all --die-with-parent --new-session
--clearenv`, fresh `/dev`, private `/run` and `/tmp`, a private network namespace,
owned runtime/HOME/config/cache/data/settings/output and an owned D-Bus session.
No inherited host display, runtime, system-bus socket, physical input or GPU
was exposed. The application tree and the exact observed OpenSceneGraph installed
library directory outside it were bound read-only. That additional directory is
`depends/OpenFramesInterface/dep/OpenSceneGraph-git/installed/lib` in the local
R2026a reference checkout; credentials or the surrounding home tree were not
bound. Mesa llvmpipe/swrast software GL was requested.

The owned GNOME Shell/Mutter 50.1 compositor used `--wayland --headless --no-x11
--virtual-monitor=1600x1200 --wayland-display=gmat-private`. Its system-API bus
connection targeted only the same owned session bus, whose host services were
absent. The owned PipeWire process and paired ScreenCast monitor supplied a
1600x1200, origin-(0,0) mapping. A persistent same-peer Gio RemoteDesktop session
delivered real virtual keyboard/pointer events through Mutter and Qt's Wayland
backend; no QApplication test events or frontend hooks were used. Pointer
readbacks, bounded corrective absolute motions and held button states are in
`pointer-delivery.jsonl` and `actions.jsonl`. Screenshots came from the owned
compositor. This required no host installation or downloads for this route.

## Observed cases and unsuccessful attempts

Times below are elapsed seconds in `actions.jsonl`; screenshot labels alone
were not treated as successful results.

| Case | Evidence and disposition |
| --- | --- |
| Native Open | Ctrl+O at 46.76 s opened `Open GMAT script`; compositor inventory at 68.81 s showed the chooser mapped/focused with the same application PID 534. Ctrl+L, `/evidence/fixture.script` and Enter selected the owned fixture; subsequent pointer activation completed the dialog. Screenshots 002–005 preserve the intermediate states. |
| Save As Cancel | Ctrl+Shift+S opened Save As. A real held left-button press/release on Cancel at 129.80–130.50 s returned to the mission editor in screenshot 006. The earlier click at 98.62 s did not cancel that dialog and is not counted. |
| Mission/native viewers | Real F5 at 144.82 s executed the fixture once. The visible Message Window reports `Mission run completed` and `Total Run Time: 1.404 seconds`; screenshot 008 records all three viewers. Native OrbitView shows Earth, path, velocity segments, stars/constellations and legend; Ground Track and XY retain their rendered data. No independent numerical parity calculation was added by this session. |
| Shared middle replay | Master Start followed by a held slider drag reached 48.6% in screenshot 010. Master, Orbit and Ground controls agree at 48.6%; XY remains rendered in the same three-viewer workspace. This records one shared seek, not all playback modes or a numerical timing audit of XY. |
| Main minimize/restore | Alt+F9 at 235.61 s left the main window mapped/focused and is ineffective evidence. Super+h at 249.06 s produced `mapped=false, focus=false` at 249.36 s; screenshot 012 shows the private desktop without GMAT. Alt+Tab restored the same main window at 261.11 s with `mapped=true, focus=true`; screenshot 013 retains the three-viewer scene and 48.6%. Two additional Super+h/Alt+Tab cycles at 330.61–331.51 s record the same state transitions. Three actual cycles total; no broad stress claim. |
| Orbit close/reopen | OrbitView was closed at 261.29 s and Output selected. The first double-click at 329.79 s and Enter at 364.45 s did not visibly reopen it: screenshots 015 and 017 still lack OrbitView. The second double-click at 396.96 s did reopen it in screenshot 018 with Earth, trajectory/velocity, legend and 48.6% retained. This qualifies the confirmed reopen only. |
| Pending Close → Cancel | The XY resource form was opened, X changed to `DefaultSC.A1ModJulian` without Apply, and Close invoked. Screenshot 020 and state at 472.65 s show the actual mapped/focused `Unapplied changes` dialog. Cancel at 491.63 s retains the pending form and X value in screenshot 021; the underlying completed XY plot still labels the applied `DefaultSC.ElapsedSecs`. |
| Pending Close → Discard | A second Close and `Close without Saving` at 492.12–492.89 s removes the form in screenshot 022. The retained native Orbit scene remains visible. Final byte-exact source proves the unapplied X change was not committed. No Apply or changed-mission execution is claimed. |
| Final Save As | Ctrl+Shift+S opened the native Save chooser; real keys entered `wayland-authored.script` and Enter at 531.01–531.27 s. Screenshot 024 at 531.67 s still shows the chooser and alone does not prove Save. At 540.595 s the file action recorded the 5371-byte fixture SHA256, and the immediately following compositor state showed only the mapped/focused main window titled `wayland-authored.script — GMAT Qt 6`. Post-session byte comparison confirms the saved file equals the copied fixture. A later saved-file reopen/run was not performed. |

The Open/Save dialogs here are the in-process native GTK chooser, not a qualified
xdg-desktop-portal FileChooser request/Response route. Same application PID,
GTK chooser appearance and the selected native Qt GTK route support that limited
disposition. Private portal service activation or Qt's generic “host portal”
registration warning in `application.log` does not demonstrate portal file
selection. Portal testing uses a later opt-in helper/session and is independent
of this record.

Selected unchanged compositor PNGs:

- [Native GTK Open](private-wayland-20261002/gtk-open.png)
- [Three viewers after the shared 48.6% seek](private-wayland-20261002/shared-middle.png)
- [Main window minimized](private-wayland-20261002/main-minimized.png)
- [Main window restored](private-wayland-20261002/main-restored.png)
- [Confirmed Orbit Output reopen](private-wayland-20261002/orbit-reopened.png)
- [Pending X retained after Cancel](private-wayland-20261002/pending-cancel.png)
- [Pending form discarded](private-wayland-20261002/pending-discard.png)

## Preserved route development and cleanup

All eight earlier closed preparation/input attempts are preserved unchanged
alongside the qualifying session, plus the private bwrap preflight. Preparation
failures remain evidence rather than being reclassified as GUI passes.

| Preserved session | Actual limit/result |
| --- | --- |
| `probe-evidence/isolated-wayland-iv30jicd` | Owned compositor exit -11 during private-service initialization; no application-input success. Sandbox exit 1. This was the private compositor, not the host GNOME process. |
| `probe-evidence/isolated-wayland-8px4f91q` | Private application mapped; helper Shell-Eval JSON decoding failed before qualification. Sandbox exit 1. |
| `probe-evidence/isolated-wayland-yvb0z_6r` | Private compositor/application initialization and screenshot only. Sandbox exit 0 does not establish input. |
| `input-probe-evidence/isolated-wayland-ae0u_zmg` | Escape/F5 and a default rendered mission observed; inaccurate pointer did not close Welcome. Sandbox cleanup exit 0. Core identity was not captured during the overlapping rebuild, so this is route preparation only. |
| `pointer-probe-evidence/isolated-wayland-s54467y0` | Relative pointer remained (0,0); bounded failure before click/mission. Sandbox exit 1. |
| `absolute-pointer-probe-evidence/isolated-wayland-5pti2qm8` | Owned paired PipeWire stream ready, then non-interactive stdin EOF; no input proof. Sandbox exit 0. |
| `absolute-pointer-probe-evidence/isolated-wayland-qu8wd45y` | Diagonal absolute target (1085,828) read back (1085,0); bounded failure, no click. Sandbox exit 1. |
| `pointer-delivery-evidence/isolated-wayland-48178kyh` | Bounded corrective absolute input reached the target; initial quick click did not close Welcome, later held-button input did. Input proof only, no mission. Sandbox exit 0. |
| `root-lifecycle-evidence/isolated-wayland-9qp3tryb` | Qualifying cases above. Sandbox exit 0; owned compositor/PipeWire exit 0 and application -15 during bounded cleanup. This is not a graceful application Quit case. |

The first four probes retain an explicit missing-core-identity provenance note;
current on-disk library bytes must not be inferred for those attempts. Later
session launch records include application/core/util/helper/startup identities.
The preflight is a namespace `/usr/bin/true` result, exit 0 in 0.00648 s, not GUI
acceptance. The absolute-input adjustment is consistent with installed-version
[Mutter directional border filtering](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/core/meta-border.c)
and [native barrier clamping](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/native/meta-barrier-native.c);
correct arrival was confirmed from compositor readback before the qualifying
clicks.

Each closed session records unchanged host GNOME PID 399961/start ticks
38989406 before/after; the qualifying session's owned application PID is 534.
This verifies that bounded route cleanup did not replace that host process. It
is not proof that the prior host crash is fixed. No host desktop, physical GPU,
physical input, system bus or portal installation was changed.

This narrows the plotting/output-lifecycle and script/pending-edit acceptance
gates for the recorded private software Wayland configuration. Broader viewer,
solver, driver, host-session and portal cases remain open. All independently
GUI-built Help tutorial walkthroughs remain Pending; Windows/macOS and MATLAB
remain deferred. No old successful matrix or tutorial was rerun for this record.
