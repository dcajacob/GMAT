# Resource Rename qualification — 2026-10-02

Work is in `/home/dan/GIT/GMAT-Qt`, `codex/qt6-gui`. The actual GmatQt application
was incrementally rebuilt with two jobs. `QtGui.ResourceRename` passes in
**0.44 seconds**. No existing example pass, Clone/conversion check, old complete
suite or native desktop launch was repeated for this change.

## Delivered behavior and evidence

Resources/Edit offer Rename and F2 while the resource tree has focus. One
prefilled name dialog supports Cancel, duplicate-name correction and protected
built-ins/bodies. Running/stale-source/pending-panel guards prevent changing an
unaccepted mission. Successful changes rebuild/validate one source candidate,
commit one Undo edit, and refresh clean renamed panels. Undo/Redo and Unicode
save/reopen retain exact source.

The focused offscreen fixture exercises spacecraft/frame/report/tracking
participant references, dependent system parameters, grouped declarations,
nested commands and ScriptEvents, dynamic Array indices and managed initializer
formulas/owner markers, the wx PropSetup/associated ForceModel naming rule, and
Qt camera plot/object/vector/target references. Camera names, labels and unknown
metadata remain literal. Source assertions preserve comments, numeric spelling,
filenames/String contents, terminal property names, scientific exponents, Toggle
modes and solver option keys while updating true expressions. The final mission
completes and its explicit report asserts the independently known scalar/array
values 7 and 3.5. Temporary report files are checked in the test and cleaned;
this is not a new scientific trajectory/estimation qualification.

TrackingFileSet uses a temporary header-only GMAT observation input and a station
Range ErrorModel. It initializes the unused tracker; no observation is estimated
or measurement evaluated. Source participant changes are verified separately
from measurement-type literals.

Included defining/reference files and configured GMAT function helpers reject
before mutation because external references may remain unchecked until runtime.
Tracking rows outside the bounded signal-path syntax also reject explicitly.
These remain script-editor workflows, not silently supported external renames.

## Corrections and limits

The first focused attempt exposed a real helper error: querying GetParameterText
for RealVar Value, whose engine supports its ID/type without that text override.
The helper now restricts factory-selector text lookup to OBJECT fields. The
later final-run failure was missing observation input in the new Tracking fixture;
its required file/ErrorModel were supplied without changing engine algorithms.
Initial, diagnostic, corrected and final logs are retained verbatim, including
raw whitespace, under the filenames below. A failed assertion now prints the
actual form status instead of hiding its cause.

The resource popup now copies operation/type values inside its own scope and
is destroyed before dispatching a modal dialog. The offscreen QPointer/active
popup check passes. Source review found no demonstrated surviving Wayland grab
in the prior code; see `resource-menu-lifetime-review-20261002.md`. This cleanup
is not a verified GNOME compositor fix. Native desktop/window/portal gates remain
open; Windows/macOS and MATLAB remain deferred.

Actual rebuilt `application/bin/GmatQt-R2026a` SHA256:
`66b5d4a31e20c8ad55ef4d3714585f63b8d1440d4cb77853a653709b331c9123`.

Captured logs:

- `gmat-qt-rename-build-20261002.txt`
- `gmat-qt-rename-check-20261002.txt`
- `gmat-qt-rename-diagnostic-build-20261002.txt`
- `gmat-qt-rename-diagnostic-check-20261002.txt`
- `gmat-qt-rename-correction-build-20261002.txt`
- `gmat-qt-rename-correction-check-20261002.txt`
- `gmat-qt-rename-fixture-build-20261002.txt`
- `gmat-qt-rename-fixture-check-20261002.txt`
- `gmat-qt-rename-tracking-fixture-GmatLog-20261002.txt`
- `gmat-qt-rename-passed-GmatLog-20261002.txt`
- `gmat-qt-rename-passed-LastTest.log-20261002.txt`
