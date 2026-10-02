# Clearing function-call outputs — 2026-10-02

Actual Help tutorial 6 construction exposed a GUI defect: starting with the
output-bearing Call function template, naming it, and clearing Outputs left
an equals sign before the callee. Apply accepted the quoted command name as a
function output; the caption disappeared, and the resulting tree row lost its
safe source mapping. The original failed actions and sources remain in the
Tutorial 6 evidence. An explicit temporary unnamed-call workaround allowed
independent work to continue; it does not close the failed GUI step.

CommandForm now removes the adjacent assignment separator when a function or
Python call's Outputs field is cleared. It preserves the existing caption,
callee, input text, comments and surrounding source. A no-output call no longer
leaves a stray assignment that can consume its caption.

The application and GmatQtGmatCallSyntaxTests were rebuilt after all private
sessions closed. One targeted QtGui.GmatCallSyntax run passes in 0.94 s. Its new
actual MDI case starts with an output-bearing call, changes the caption/function,
clears Outputs, Applies, verifies exact Undo/Redo and save/reopen, then confirms
an independently known no-output function Global increment. The shared Python
form has a bounded label/callee/comment check; no Python runtime suite or other
unchanged tests were repeated. A separate read-only peer review found no concern.

The rebuilt application/bin/GmatQt-R2026a SHA256 is
`ef933221b09577c14df825d718e08d31abe9a8ec5319e32c615b4359d0f39af5`.
Base `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`,
util `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`
and Qt startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`
remain unchanged. Build/test logs and identities are retained in the ignored
[`function-output-removal`](../../../build/example-qualification/20261002/function-output-removal/)
evidence directory. Tutorial 6's actual-input retry and remaining capture work
continue separately; no chapter or full replacement pass is claimed here.
