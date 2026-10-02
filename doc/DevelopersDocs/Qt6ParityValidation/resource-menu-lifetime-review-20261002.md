# Resource context-menu lifetime review — 2026-10-02

Read-only review of the recorded desktop failure and current source at checkpoint
`60ad5df4` (`codex/qt6-gui`, `/home/dan/GIT/GMAT-Qt`). No current menu-lifetime
defect was established. No GUI was launched and no native or offscreen test was
run for this review. Live desktop testing remains stopped.

## Current application behavior

`src/qtgui/MainWindow.cpp:580–624` builds fresh context actions and dispatches
Create, Clone or Delete only after `QMenu::exec()` returns. The stack menu and its
`QWidgetWindow` remain allocated until the handler finishes, including while a
modal dialog runs. All selected action pointers are used within that lifetime;
the engine object pointer is consulted only before `exec()`. Resource name and
source snapshot are copied values. The generic popup action is fresh rather than
the already-connected Edit-menu Create action.

`src/qtgui/main.cpp:50` owns MainWindow on the stack; the production MainWindow
does not use `WA_DeleteOnClose`. No concrete parent-destruction, double-deletion
or dangling-action path was found in the reviewed operations. The mission
context handler (`MainWindow.cpp:360–402`) similarly dispatches after menu exec.
These observations are source findings, not native acceptance evidence.

## Verified Qt behavior

`qmake6 -query QT_VERSION` returned **6.10.2**. Installed `libQt6Core.so.6` and
`libQt6WaylandClient.so.6` symlinks resolve to their `6.10.2` libraries; the Qt
checkout's CMake cache selects `/usr/lib/x86_64-linux-gnu/cmake/Qt6*`. Upstream
v6.10.2 source was inspected; downstream binary equivalence was not established.

- [qmenu.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/widgets/widgets/qmenu.cpp):
  `QMenuPrivate::activateAction` (1344), `hideMenu` (506), `exec` (2482), and
  `QMenu::hideEvent` (2532) show selection closing the menu and the hide event
  ending its event loop before the application dispatches the returned action.
- [qwidget.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/widgets/kernel/qwidget.cpp):
  `QWidgetPrivate::hide_helper` (7585) calls `closePopup` before native hiding and
  delivery of the menu's hide event.
- [qapplication.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/widgets/kernel/qapplication.cpp):
  `QApplicationPrivate::closePopup` (3115) removes the popup and releases or
  restores keyboard/mouse grabs when the last popup closes.
- [qwaylandwindow.cpp](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/plugins/platforms/wayland/qwaylandwindow.cpp):
  `QWaylandWindow::setVisible` (591) calls `resetSurfaceRole` when hidden;
  `resetSurfaceRole` (324) removes popup/transient-parent bookkeeping and deletes
  the shell surface. A still-allocated hidden QObject does not establish a
  surviving Wayland popup role or grab.

## Crash evidence and limits

`resource-category-menu-compositor-crash.txt` records GNOME Shell PID 15518
SIGSEGV at **2026-10-01 11:34:45 MDT**, its session-service dump status, and a
main-thread trace reaching `wl_resource_post_event` in libwayland-server through
libmutter-18. The native category log mtime is 11:34:45.805. This establishes a
compositor crash during the check; it does not identify the exact popup operation,
a GMAT memory defect, or the compositor's root cause. The native log's harness
PASS does not qualify that interrupted session.

`PluginCreationTests.cpp:35–66` directly emits `customContextMenuRequested` and
uses timer-driven synthetic Enter events. **Inference:** this can lack a fresh
Wayland input serial, consistent with the recorded failed-grabbing-popup warnings
about parent input. Neither those warnings nor the inference establish why
GNOME crashed. The Wayland context-test guard remains in place; it was not
retested or bypassed.

## Narrow disposition

No crash-specific code fix is supported by this review. Optional lifecycle
cleanup could copy the selected operation/type inside an inner QMenu scope and
destroy the menu before modal dispatch, with an existing offscreen fixture
checking menu destruction and absence of an active popup. This would make object
lifetimes explicit; it must not be presented as a verified compositor fix.
No delay, manual grab or extra event-loop processing is justified by this evidence.
