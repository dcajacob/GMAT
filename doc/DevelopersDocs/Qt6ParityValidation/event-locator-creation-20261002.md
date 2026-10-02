# Event-locator creation defaults — 2026-10-02

Independent Help tutorial 7 construction exposed an empty Spacecraft field in
a new EclipseLocator. The user had to select the spacecraft manually before
running. The wx creator calls Moderator::CreateEventLocator with its default
flag enabled; it selects the first configured spacecraft. Qt prepared its
unregistered draft directly through the factory and lost that default.

New Qt event-locator drafts now display the first existing spacecraft. The
creation transaction writes that target explicitly even when the form leaves
it untouched, so interpreter reconstruction and Save/reopen retain it. Contact
locators continue accepting a selected PlanetographicRegion. Explicit targets
win over defaults; missing, cleared or wrong-type targets are rejected before
source mutation. Opening or canceling the creator does not create a spacecraft.
Cloning and existing source assignments remain unchanged.

One new offscreen QtGui.EventLocatorCreation execution passes in **0.37 seconds**.
It compares drafts with the independent Moderator default creation path, uses
the actual modal Create action for unchanged Eclipse/Contact defaults and Cancel,
checks source and configured-object readback through Unicode Save/reopen, and
checks alternate spacecraft, a Contact region, invalid targets and the absence
of implicit spacecraft creation. It runs no mission or old qualification suite.
A separate read-only peer review found no substantive creation, source or
ownership regression. The original actual-input tutorial failure remains intact.

The frontend and application were rebuilt after owned sessions closed. The
first test compile failed because the test called a private method; the test
now triggers the existing public Create QAction. The corrected compile passed
before the single test execution. A post-pass identity recorder had incorrect
base/util filenames; only that recorder was corrected. All attempts, the final
CTest log and full runtime/source identities are retained in the ignored
`build/example-qualification/20261002/event-locator-creation` directory.

`application/bin/GmatQt` resolves to the rebuilt GmatQt-R2026a, SHA256
`e5337acddb900804aec9d327dd3e689b67995b2c30c1e35d9cc49421d5900f87`.
Base `cf147e23...`, util `1e4e7fe6...` and Qt startup `5f80be1f...` remain
unchanged. Remaining Help tutorials continue with that executable. This focused
creation result does not close the full Linux, host, hardware or crash gates;
Windows, macOS and MATLAB remain deferred.
