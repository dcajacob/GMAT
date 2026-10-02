# Additional Help chapter availability — 2026-10-02

Read-only audit observed at **2026-10-02T21:26:53.518719+00:00** in `/home/dan/GIT/GMAT-Qt`. The twelve additional XML chapters are **not passed walkthroughs**. They are absent from the currently accessible Help sequence; no publication, GUI construction, application run, build or test was performed.

## Current Help availability

The selected startup sets `ROOT_PATH = ../` and `HELP_PATH = ROOT_PATH/docs/help`. The actual private helper launches the application from `application/bin` (`IsolatedX11Session.py:141`), resolving Help to `/home/dan/GIT/GMAT-Qt/application/docs/help`. `HELP_HTML_TUTORIALS_FILE` is `HELP_PATH/html/Tutorials.html`; that published contents page exists.

`doc/help/src/Part_Tutorials.xml` includes exactly the twelve published tutorials. It includes none of the twelve additional chapters below. `BuildHelp.py` builds that book and copies its generated `html`/`files` trees and `help.html` into the configured default Help directory. Each additional chapter's expected `html/<xml-id>.html` is currently missing. All twelve XML sources parse successfully, and all195 referenced images are locally present (194 unique image paths). This is a file/book observation, without a fresh GUI navigation claim.

| Additional source | Potential coverage | Disposition |
| --- | --- | --- |
| `Tut_ACEStationKeeping.xml` | core DC, For-loop and L1/frame scenario | Potential additional coverage; no walkthrough attempted. |
| `Tut_AlgebraicOptimization.xml` | constrained/unconstrained scalar optimizer and reports | Solver wording unresolved; no walkthrough attempted. |
| `Tut_CreatingReport.xml` | automatic/manual/decorated reports and strings/ScriptEvent | High priority potential coverage; no walkthrough attempted. |
| `Tut_ForceModels.xml` | six separate propagator force models, drag/third bodies/SRP and reports | High priority potential coverage; no walkthrough attempted. |
| `Tut_FormationRendezvous.xml` | four spacecraft, object-referenced views, separate position/velocity targeting | Reconstructed source instructions; no walkthrough attempted. |
| `Tut_L2Transfer.xml` | bisection, If/While/Stop, lunar swingby and L2 frames | Potential additional bounded coverage; no walkthrough attempted. |
| `Tut_LEOStationKeeping.xml` | DC targeting and repeated LEO station keeping | High priority potential coverage; no walkthrough attempted. |
| `Tut_LunarTransfer.xml` | multiple DC targets, lunar trajectory and coordinate views | Potential additional coverage; no walkthrough attempted. |
| `Tut_MATLAB.xml` | running GMAT scripts from MATLAB | Deferred — MATLAB; no walkthrough attempted. |
| `Tut_MarsB-PlaneTargeting.xml` | older Earth-parking-to-Mars transfer and multiple burns/propagators | Potential additional coverage; no walkthrough attempted. |
| `Tut_MinFuelLunarTransfer.xml` | legacy SQP lunar fuel minimization and plots | Legacy SQP identity unresolved; no walkthrough attempted. |
| `Tut_MinFuelOrbitTransfer.xml` | fmincon fuel-minimizing Earth orbit transfer and plots | Deferred — MATLAB/fmincon; no walkthrough attempted. |

## Finite follow-up scope

Creating a Report, Force Models and LEO Station Keeping offer the clearest additional ordinary GUI/report/control coverage if these legacy instructions are later made available. Algebraic Optimization first needs its contradictory solver names resolved: the resource instructions and picture specify VF13ad1, while the mission prose names SQPfmincon. Its independent Help objectives are X1=X2=4/F=8 with G=8, then X1=X2=2/F=0 after removing the constraint; no mission was constructed to test them.

ACE Station Keeping, Lunar Transfer and the older Mars B-Plane chapter provide distinct DC/loop/frame scenarios. Formation Rendezvous explicitly provides reconstructed initial states and separate position-then-velocity goals; historical screenshots and simultaneous docking are not acceptance claims. L2 Transfer has an updated10000/16000-km bracket, sign-check Stop and long coast bounds; it needs separate bounded interactive evidence if pursued.

Running GMAT Scripts from MATLAB and the explicitly fmincon-based Minimum Fuel for Orbit Transfer remain deferred under the user's MATLAB boundary. Minimum Fuel for Lunar Transfer names legacy SQP1 and old tolerance options without establishing a current solver identity. Its availability remains unresolved; this audit neither substitutes an optimizer nor automatically waives that chapter. Published Tutorial4 or earlier backend example execution does not qualify any additional interactive walkthrough.

## Exact identities

| Source/installed page | Bytes | SHA256 |
| --- | ---: | --- |
| `doc/help/src/Part_Tutorials.xml` | 2906 | `60965a761ce66a11658c55b12b6ad6781aca42b2984ae0438680da05e3ab4793` |
| `doc/help/src/help.xml` | 1688 | `362adc0877d2bdff2e0291078ed3a701b2af74b5ca055cae81ea237dbb3af90d` |
| `src/qtgui/BuildHelp.py` | 904 | `ad5bbdd59b9cd6a2e39476a52a71f8b9fe74df74652163acd3672ad08045ffc0` |
| `application/docs/help/html/Tutorials.html` | 31595 | `6590dcddd115bf6f5ee7d5a9d2102ad21e129b9ba679444ce5ddc09e1abf602b` |
| `application/bin/gmat_startup_qt.txt` | 10260 | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |

The accompanying `help-extra-chapters-availability-20261002.json` records every chapter source hash, XML ID, exact expected configured HTML path and missing-page observation, image counts and dependency disposition. Sample mission contents were not consulted. The twelve chapters remain separate from the published sequence; No public Help publishing or new tutorial construction was performed.

After this read-only snapshot, the single offline Help build completed at21:31:25.812008 UTC and installed the reviewed published Tutorial12 repair. Generated-page checks are retained under `build/example-qualification/20261002/help-rebuild`. The original audit hashes remain a timestamped pre-build observation; the published-book include list is unchanged. This does not publish or qualify the additional chapters.
