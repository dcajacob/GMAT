# Help tutorial 8: Electric Propulsion — 2026-10-02

**Passed for bounded private actual-input construction.** The default mission
was extended through the hardware and command forms using Help. The two-day
finite electric burn completed, consumed fuel and raised the orbit. Actual own
save/reopen succeeded before freezing the source and opening the reference.
The comparison exposed two stale reference settings, repaired narrowly; the
updated reference's complete propagation summary equals the frozen own summary.

## Independent construction and results

Actual Welcome → Tutorials → Electric Propulsion opens the chapter, followed by
New Mission. The first session records `script: null`; the continuation loads
only the independently saved resource/command milestone. No shipped electric
propulsion mission was read before freeze. Help source SHA256 is
`5d2a6d035fb4d0d7ea26e161667edff2e0ae5ab794e16d3fc447ffa6edb26596`.

| Written step | Actual GUI construction and observation |
| --- | --- |
| Default mission | New Mission retains DefaultSC's Cartesian state/epoch and DefaultProp's Earth JGM2 degree/order 4 settings. No changed force model or spacecraft state. |
| Electric hardware | Create ElectricThruster1, ElectricTank1 and SolarPowerSystem1 through the Hardware context menu/unified creator. Set ConstantThrustAndIsp and ConstantThrust 5 N; attach ElectricTank1 with mix ratio 1 and set DecrementMass true. Tank fuel defaults to 756 kg; Isp remains the engine default 4200 s. |
| Power | Actual ShadowBodies selector removes Earth. Other solar power defaults remain unchanged. This was configured during creation instead of a separate later Apply. |
| Attachments | DefaultSC resource editor attaches ElectricTank1, ElectricThruster1 and SolarPowerSystem1; Apply and saved source retain all three links. Create FiniteBurn1 with ElectricThruster1. |
| Commands | Rename Propagate to `Propagate Two Days`, set DefaultSC.ElapsedDays = 2, insert BeginFiniteBurn before it and EndFiniteBurn afterward. The single available burn/spacecraft are selected automatically. Name them StartTheManeuver and EndTheManeuver through command Name fields. |
| Save and run | Actual Save As writes the final 5750-byte source; source readback confirms mass decrement, both named burn commands, correct order and two-day stop before F5. One own F5 completes in 0.850 s. |
| Results | OrbitView shows progressively wider turns after an actual overhead camera drag; temporary display controls hide stars/constellations for inspection. Propagate's exported summary gives TAI epoch 21547, SMA 8362.8537296235 km, RMAG 8276.6177104800 km and fuel 735.03014416771 kg. Fuel decreases by 20.96985583229 kg from 756. Help supplies visual intent, without a separate numeric tolerance. |
| Save/reopen | Actual Ctrl+O opens the own saved final source and Build succeeds. The complete named mission sequence remains; the reopened Propagate editor displays DefaultSC.ElapsedDays = 2. No unchanged own rerun after reopening. |

The source was frozen at **2026-10-02 20:08:14.975711 UTC** before new reference
access. The unchanged [authored source](help-tutorial-08-authored-20261002.script)
is 5750 bytes, SHA256
`9e80e2519d5a6bca634a9fcb39d3ee47f4ac7f86ce6afb67416d1406328cddb7`.
The full 3283-byte propagation summary SHA256 is
`5c63b71aeaa55a681c6e3b400eb8d42dc6a3d316c9ff9fc7f0af2d87ee5ec423`.

## Reference comparison and repair

After freeze, the original shipped `Tut_ElectricPropulsionModelling.script`
(10178 bytes, SHA256
`63fc2633b1fa713397171a58d758da7e288fe133ede7df3a9a7bb8988a14c317`)
was read and opened through Ctrl+O. Normal Convert views translates OpenFrames;
the translated buffer was never saved over the source. One original reference
F5 completes in 1.501 s, with SMA 7195.2289101138 km. Its archived original
source and exported result are retained.

Two settings contradicted the tutorial: ThrustModel was ThrustMassPolynomial,
and SolarPowerSystem1.ShadowBodies contained Earth. Only those assignments are
changed to ConstantThrustAndIsp and an empty ShadowBodies list. Existing power,
force, integration, spacecraft, command and extra plot settings remain intact.
No separate electric-propulsion reference copy exists in the Help script folders.
The updated sample is 10171 bytes, SHA256
`96ba9ccb91db6f85bc47edc459dfa1687aac32e9efb31dc901354a5a83139a87`.

One fresh actual Ctrl+O → Convert views → F5 verification completes in 0.900 s.
Its exported complete propagation summary is byte-identical to the frozen own
3283-byte summary, including state, epoch and tank mass. That bounded result
closes the numerical difference caused by the stale settings. The reference's
extra PowerVsTime and MassFlowRate plots were run only in reference sessions;
the Help does not instruct their construction. The own source leaves some
engine defaults implicit, and native/converted view geometry, camera metadata,
star counts and NAIF identifiers differ. Whole-property, pixel and independent
physical-science equality are not claimed.

## Runtime, retained attempts and limits

Three authenticated owned Xvfb/Openbox/software-GL sessions use private settings
and OUTPUT_PATH-only startup clones. All Tutorial 8 execution uses app SHA256
`3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1`,
selected startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`
and helper `4232bdb0a050ab22b5f0513a15f92339501edc88719339e2dac1c476a8d3358f`.
Controlled base cf147e23… and util 1e4e7fe6… remain unchanged. A subsequent
function-output GUI fix rebuilds the app to ef933221…; this chapter does not
claim a rerun on that later app.

The first mass-decrement combo attempt selected false, caught from the saved
source and corrected through the visible true option before the first run.
Selecting the first `b` template chose BeginFileThrust; the correct finite-burn
template replaced it, resetting the pending label, which was then entered again.
These are retained operator corrections rather than product-defect claims.
Some summary-save/close actions arrived while a dialog remained active; exported
files were verified before comparison. No modal defect is inferred.

The initial session processed helper Quit; construction continuation reached its
600-second bound while the original reference's unsaved-conversion dialog was
open. Its owned app exit is -15; graceful quit is not claimed there. The changed
reference session actually clicks Discard and exits the GUI with app/WM/Xvfb
0/0/0. The helper reports owned application exited (0) before reading the queued
Quit action. No host desktop input/process was used.

All intermediate files, actions, screenshots, Help/checklist, original reference,
freeze and comparison are copied unchanged to the ignored
[`08-electric-propulsion`](../../../build/example-qualification/20261002/help-tutorials/08-electric-propulsion/)
evidence tree with a size/SHA256 index. Nine unchanged PNGs accompany this record:
[Help](help-tutorial-08-help-20261002.png),
[thruster](help-tutorial-08-thruster-20261002.png),
[attachments](help-tutorial-08-hardware-20261002.png),
[mass decrement](help-tutorial-08-mass-20261002.png),
[saved sequence](help-tutorial-08-sequence-20261002.png),
[summary](help-tutorial-08-summary-20261002.png),
[spiral](help-tutorial-08-spiral-20261002.png),
[reopened stop](help-tutorial-08-reopened-20261002.png),
[changed reference](help-tutorial-08-reference-20261002.png).

No unaffected test, old matrix or full corpus was repeated. This bounded chapter
pass does not close full Linux, host GNOME/Wayland, hardware rendering or crash
acceptance. Other chapters continue; Windows/macOS/MATLAB remain deferred.
