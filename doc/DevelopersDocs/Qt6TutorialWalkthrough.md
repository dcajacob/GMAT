# Qt 6 Help tutorial walkthrough qualification

## Scope and current state — 2026-10-02

The user requested this as the next qualification phase after the current Linux
replacement work: follow the Help tutorials as a person using GMAT would, build
the scenarios through the Qt application, and establish that the GUI can complete
them. Shipped tutorial mission scripts are reserved for comparison afterward.
This extends the existing goal; it does not replace its unfinished acceptance
requirements. Windows, macOS and MATLAB remain deferred.

**Preparation only: every walkthrough below is pending.** The earlier shipped
script build/run corpus and tutorial repairs are not GUI walkthrough evidence.
The current runtime is in `/home/dan/GIT/GMAT-Qt/application/bin`. Live testing on
the user's desktop remains stopped following the GNOME Shell crash. A private,
authenticated X11/software-GL route with actual mouse/keyboard input is now
established; see [isolated input evidence](Qt6ParityValidation/isolated-x11-input-20261002.md).
It can support independent GUI construction while host GNOME/Wayland, portals
and hardware-driver acceptance remain open. Offscreen fixtures do not substitute
for interactive construction. No tutorial walkthrough has started.

The published order comes from `doc/help/src/Part_Tutorials.xml`; prerequisite
notes below come from the chapter introductions. Use the Help actually available
from the application when walking through it, recording missing/stale chapters
or links against the source. Reading an inventory is not a completed tutorial.

## Construction and comparison procedure

1. Start a new mission with the application's normal defaults. Where a chapter
   extends an earlier tutorial, use the mission independently built during that
   earlier walkthrough. Do not open, copy, insert or programmatically seed the
   shipped tutorial mission to construct the scenario. If the Help starts by
   loading an example, reproduce its configuration from the written instructions;
   insufficient instructions are a documented Help gap.
2. Follow the chapter's steps in order using visible resource editors, mission
   controls, plots and output. Enter the settings through the UI. Save a concise
   action record with chapter/step references, screenshots where useful, resource
   settings and branch/command structure. Engine API calls, generated scripts and
   model-setter test fixtures do not substitute for interactive construction.
3. Use the application's script/function editor when the chapter explicitly
   teaches authoring code, entering the chapter's instructions independently.
   Record script-editor steps separately from resource/mission editor coverage.
   A missing Qt control is a concrete capability finding: implement reasonable
   gaps in the authorized Linux scope and retry the affected step. Do not silently
   bypass missing GUI functionality with a prebuilt reference script.
4. Required observations, kernels, function support files and other inputs
   expressly called for by the Help are declared dependencies. Record their
   provenance and identity. Their use does not authorize importing reference
   mission/resource configuration. Preserve generated data needed by later
   chapters. Missing public inputs remain open findings; deferred/proprietary
   dependencies require an explicit, evidence-backed disposition.
5. Run each independently authored scenario and inspect the specified reports,
   plots, targets and tolerances. Save/reopen it and confirm that the constructed
   settings and mission sequence survive. For staged tutorials, preserve each
   milestone and distinguish intentional Stops from completed solves.
6. Before consulting the shipped mission script, preserve the authored mission,
   its hash, action record, runtime/build identity and outputs. Then compare
   resource settings, mission logic and numerical/visual results to the reference.
   Explain substantive differences; formatting, ordering and Qt metadata alone
   do not establish a failure. Previous exposure during corpus qualification is
   disclosed; the walkthrough must still be independently driven by the Help.
7. Record each outcome as pending, in progress, passed, failed, or blocked, with
   the exact failed step or dependency and bounded evidence. A pass requires
   successful interactive construction, execution, the chapter's result checks,
   save/reopen and the final comparison. Avoid repeating unchanged successful
   tests or expensive reference runs when existing compatible evidence suffices.

## Published tutorial sequence

All entries are **pending**. Numbering follows the Tutorials part of the Help.

| # | Chapter source and title | Prerequisites and staged coverage |
|---|---|---|
| 1 | `Tut_SimulatingAnOrbit.xml` — Simulating an Orbit | None. Spacecraft, propagator, propagate to periapsis, inspect results. |
| 2 | `Tut_SimpleOrbitTransfer.xml` — Simple Orbit Transfer | Tutorial 1. DifferentialCorrector and two impulsive maneuvers for a Hohmann transfer. |
| 3 | `Tut_TargetFiniteBurn.xml` — Target Finite Burn to Raise Apogee | Tutorials 1–2. Hardware, FiniteBurn, target burn duration, inspect results. |
| 4 | `Tut_Mars_B_Plane_Targeting.xml` — Mars B-Plane Targeting | Tutorials 1–2 and B-plane concepts. Trajectory correction followed by Mars orbit insertion. |
| 5 | `Tut_OptimalLunarFlyby.xml` — Optimal Lunar Flyby using Multiple Shooting | Tutorials 1–2, 4 and GMAT Fundamentals training/videos; VF13ad. Verify configuration, smooth trajectory, optimize, new initial guess, additional constraint. |
| 6 | `Tut_UsingGMATFunctions.xml` — Mars B-Plane Targeting Using GMAT Functions | Tutorials 1–2, 4 and B-plane concepts. Author a GmatFunction and Global objects; target inside the function, then outside it. |
| 7 | `Tut_EventLocation.xml` — Finding Eclipses and Station Contacts | Extend the independently built tutorial 2 mission. EclipseLocator, then GroundStation/ContactLocator. |
| 8 | `Tut_ElectricPropulsion.xml` — Electric Propulsion | Tutorial 1; tutorial 3 referenced for targeting. Electric propulsion hardware and finite-burn modeling. |
| 9 | `Tut_Simulate_DSN_Range_and_Doppler_Data.xml` — Simulate DSN Range and Doppler Data | Basic Mission Design Tutorials. Spacecraft/station hardware, measurements, Simulator; initial simulation then realistic measurement data. |
| 10 | `Tut_Orbit_Estimation_using_DSN_Range_and_Doppler_Data.xml` — Orbit Estimation using DSN Range and Doppler Data | Tutorial 9 and its independently generated observations. BatchEstimator and result analysis. |
| 11 | `Tut_FilterSmoother_GpsPosVec.xml` — Filter and Smoother Orbit Determination using GPS_PosVec Data | No introductory prerequisite stated. Simulation, Extended Kalman Filter, review, Fraser-Potter smoother, review, warm start, tuning. |
| 12 | `Tut_Simulate_and_Estimate_Inter_Spacecraft_Tracking.xml` — Simulate and Estimate Inter-Spacecraft Tracking | Basic Mission Design Tutorials. Two-spacecraft hardware/error models, range/range-rate simulation, estimate observed spacecraft state. |

Chapters 9, 10 and 12 describe legacy script-only navigation workflows. Chapter 5
also requires authoring an optimization sequence. Record what the Qt resource and
mission controls can actually construct, and identify remaining script-editor
boundaries; successful backend execution alone is not GUI capability coverage.
The local optional VF13ad component has previous execution evidence, which does
not establish tutorial 5's interactive workflow.

## Additional source tutorials

These twelve source chapters are absent from the current Tutorials table of
contents. Keep them separate from the twelve published chapters. Assess their
Help availability and applicability after the published sequence; all walkthroughs
remain pending, with MATLAB deferred. Earlier repairs/execution of some of these
chapters do not satisfy the new interactive acceptance requirement.

| Chapter source | Additional or legacy chapter |
|---|---|
| `Tut_ACEStationKeeping.xml` | ACE Station Keeping |
| `Tut_AlgebraicOptimization.xml` | Algebraic Optimization |
| `Tut_CreatingReport.xml` | Creating a Report |
| `Tut_ForceModels.xml` | Force Models |
| `Tut_FormationRendezvous.xml` | Formation Rendezvous |
| `Tut_L2Transfer.xml` | L2 Transfer with Swingby |
| `Tut_LEOStationKeeping.xml` | LEO Station Keeping |
| `Tut_LunarTransfer.xml` | Lunar Transfer |
| `Tut_MATLAB.xml` | Running GMAT Scripts from MATLAB — deferred |
| `Tut_MarsB-PlaneTargeting.xml` | Older Mars B-Plane Targeting chapter |
| `Tut_MinFuelLunarTransfer.xml` | Minimum Fuel for Lunar Transfer |
| `Tut_MinFuelOrbitTransfer.xml` | Minimum Fuel for Orbit Transfer |

For each actual walkthrough, add a per-chapter record under
`doc/DevelopersDocs/Qt6ParityValidation` and link it here. Preserve raw artifacts
in the ignored qualification artifact tree. No such execution record exists for
this phase yet.
