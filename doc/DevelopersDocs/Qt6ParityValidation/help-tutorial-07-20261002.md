# Help tutorial 7: Finding Eclipses and Station Contacts — 2026-10-02

**Passed for bounded private actual-input construction of the chapter's core
mission.** The actual Qt Help chapter was opened, the independently authored
[Tutorial 2 prerequisite](help-tutorial-02-20261002.md) was opened through Ctrl+O,
and Earth geometry, an EclipseLocator, Hyderabad GroundStation and ContactLocator
were configured through resource forms. The mission ran, both reports were opened
from the actual Output tree, and a saved scenario was reopened and its settings
checked before freezing it. No shipped script was used to construct the scenario.

The [unchanged authored source](help-tutorial-07-authored-20261002.script) is
6498 bytes, SHA256 `c7964540d1e47ff970989e9b8bfbe880ab308fb5c01554a4d69c27a8b1d709d5`.
It was frozen at **2026-10-02 20:12:02.032395 UTC**, before reference access.
The mission sequence after `BeginMissionSequence` remains byte exact against the
previous independently authored Tutorial 2 source.

## Help and construction scope

The authoritative chapter is `doc/help/src/Tut_EventLocation.xml`, SHA256
`5bdaecaa9c9e69ed4050360a86d2ba856b1396bfb34ef2f072f4ef223306d9ec`.
Its only named script is the Simple Orbit Transfer prerequisite, at lines 39–42
and 102–108. There is **no final shipped `Tut_EventLocation.script`** to compare.
The owned prerequisite is the frozen 5978-byte Tutorial 2 script, SHA256
`652c63e2559d6cda7e489f458c870ae8908fd8aa780adf8b518afd5cc4c4d20c`.

The first required prerequisite run converged and completed in **0.591 s**.
The actual SolarSystem panel showed DE405, its DE file, a DE405 planetary SPK and
the planetary PCK. Defaults were left unchanged. Earth properties and Orientation
were inspected, including its PCK list and ITRF93 frame. Earth radius was changed
to **6378.1366 km** and flattening to **0.00335281310845547** through the GUI.
Both values were saved and confirmed again after reopening the authored mission.

The EclipseLocator was created through the category's specific Add action.
Default Earth/Luna occulting bodies, all three eclipse types, the entire mission
interval, automatic mode, 10-s search step, light-time delay and stellar
aberration were retained. The target was explicitly set to DefaultSC.

GroundStation1 was created through its category action with Earth's default
7-degree minimum elevation, Spherical state and Ellipsoid horizon. Latitude
**17.0286 degrees**, longitude **78.1883 degrees** and altitude **0.541 km** were
entered, then the actual resource Rename dialog changed its name to Hyderabad.
Its values were confirmed in the form after creation and after own-file reopen.
ContactLocator1 used DefaultSC, observer Hyderabad, no extra occulting bodies,
**600-s** search step, the entire interval and automatic mode. Default light-time
delay, stellar aberration and **Transmit** direction were retained.

The first 600-s private session ended at its configured time bound after the
successful eclipse report was read. A second private session continued from the
saved owned eclipse milestone; it did not repeat the prerequisite or eclipse-only
run. The combined new station/contact scenario completed in **1.645 s**.
Actual Ctrl+O reopened the saved final mission, Build succeeded, and the Hyderabad,
ContactLocator, EclipseLocator and Earth forms confirmed the retained values.

## Results and retained attempts

| Stage | Actual result |
|---|---|
| Own Tutorial 2 prerequisite | Completed/converged, 0.591 s |
| First EclipseLocator attempt | Failed, 0.247 s: target blank |
| Corrected eclipse scenario | Completed, 1.567 s; Output report opened |
| Added Hyderabad and ContactLocator | Completed, 1.645 s; Output contact report opened |
| Final save/reopen | Build succeeded; required resource values read back |

The initial target failure was an operator correction: Down/Enter selected the
blank item instead of DefaultSC. The empty target survived Create, and F5 reported
that EclipseLocator1 needed a Spacecraft or PlanetographicRegion. The visible
combo was reopened, DefaultSC selected and Apply used. The failed screenshot and
all actions remain in the raw evidence; no numerical or focus-layout defect is
claimed from this attempt. The Qt creation target is initially blank, unlike
Help's described DefaultSC default; explicit selection completes the workflow.

The eclipse-only and combined-run reports are **byte equal**: 694 bytes, SHA256
`1a7516a2864956205c6b050a5747a86c50e78a82a91020b20354d7652814f086`.
The actual report shows Earth penumbra, umbra and penumbra forming one total event:

| Start → stop, UTC on 1 Jan 2000 | Type | Duration (s) |
|---|---|---:|
| 12:10:05.138 → 12:10:15.513 | Penumbra | 10.375094600 |
| 12:10:15.513 → 12:45:00.416 | Umbra | 2084.9027570 |
| 12:45:00.416 → 12:45:10.668 | Penumbra | 10.252078017 |

Total duration is **2105.5299296 s**, about 35.09 minutes, matching the chapter's
three portions of one roughly 35-minute eclipse. No antumbra event is reported.

The 294-byte contact report, SHA256
`8b3043ad5ef20843e8a6ec71a02f01b6ab92f35a99914df02becb78ecc474224`,
shows two Hyderabad contacts with DefaultSC:

| UTC start → stop | Duration (s) |
|---|---:|
| 1 Jan 2000 11:59:28.000 → 12:05:58.248 | 390.24816554 |
| 1 Jan 2000 13:34:20.816 → 2 Jan 2000 18:32:14.157 | 104273.34070 |

These are about 6.50 minutes and 28.96 hours, consistent with the Help discussion.

## Post-freeze comparison and limits

Only after the freeze was the named shipped prerequisite read. It remains
7360 bytes, SHA256 `c12811942255832ce30a5e6224067760c59a86695969f71f692d281db835c485`.
Tutorial 2 already records its actual compatible reference run; that passing run
was not repeated. The frozen Tutorial 7 additions are checked against Help's
specified Earth shape, locator settings, station location and report expectations.
No separate final-reference event execution or reference-report equality is claimed.

The prerequisite differences remain explicit: the owned mission carries its own
previously applied Vary corrections and [-10,10] bounds, whereas the reference
starts at 1 with [0,3.14159] bounds; Target ExitMode is DiscardAndContinue versus
SaveAndContinue. The owned native OrbitView differs from the reference OpenFrames
view, including camera distance and default drawing/model fields. These were
already recorded in Tutorial 2 and were not rewritten to resemble the reference.

The current SolarSystem panel has no LSK field. Its DE405/SPK/PCK values and Earth
PCK/ITRF93 were inspected through the GUI, while the unchanged startup's LSK path
is file-level evidence only. Full GUI leap-second-kernel management or every
kernel/timescale equivalence is not claimed. The optional Further Exercises—extra
stations near burns, Propagate color changes, maneuver summary/coverage and
burn/eclipse safety checks—were not attempted. No arbitrary station network or
coverage guarantee was invented.

Runtime is app SHA256 `3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1`,
controlled base `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`,
startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.
Per-launch preflight identities precede each start. Software GL and actual private
X11 input/window rendering were used. The first session's time-bound cleanup and
second session's acknowledged helper quit both produced owned statuses 0/0/-15.
This is not graceful GUI Quit, host GNOME/Wayland/portal, physical GPU, crash-fix,
full replacement or every-state scientific acceptance. Zero-byte buffered app/core
logs remain preserved; timing/results are supported by screenshots and report files.

## Nine unchanged screenshots

- [Actual Help chapter](help-tutorial-07-help.png)
- [Required own prerequisite run](help-tutorial-07-prerequisite-run.png)
- [Applied Earth geometry](help-tutorial-07-earth-shape.png)
- [Retained blank-target failure](help-tutorial-07-blank-target-failure.png)
- [Actual eclipse Output report](help-tutorial-07-eclipse-report.png)
- [Hyderabad location entered in the creation form](help-tutorial-07-station-settings.png)
- [Actual resource rename](help-tutorial-07-station-renamed.png)
- [Actual contact Output report](help-tutorial-07-contact-report.png)
- [Hyderabad readback after own-file reopen](help-tutorial-07-reopen-station.png)

Full actions, all PNGs, startup copies, reports, stage source, freeze, preflight
identities and post-freeze comparison are indexed under ignored
`build/example-qualification/20261002/help-tutorials/07-event-location/`.
