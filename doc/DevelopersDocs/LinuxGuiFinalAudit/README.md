# Non-Qt GUI follow-up: final evidence audit

The requested non-Qt candidates have been fixed where defects were demonstrated or given tested no-change dispositions below. The final tested production state is integration `397444d217d8f19a8697db26ec8a9036548fa61b`; subsequent audit/build-guide edits are documentation only. All 11 follow-up suites and the existing core GUI suite pass. The 210-script primary and 24-script converted sweeps have no regressions against the saved public-feature baseline.

Fifteen focused GMAT production branches and one isolated external OFI patch were prepared for this follow-up; the 20 earlier branches remain intact. Tests and audit records are on integration. No PR, issue or external comment was submitted.

## Requirement-to-evidence map

This map follows the original Old review, including its additional candidates, instead of defining scope from the patches that happened to be completed. Individual reports contain baseline failures, independent prerequisite builds and platform limits.

| Original requirement/candidate | Disposition and authoritative evidence |
|---|---|
| GroundTrack owned runtime state and independent copies | LGUI-026; ownership regression checks initialization, copy/assignment, independent coordinate resources, self-assignment, repeated initialization and source survival after copy destruction. [First batch](../LinuxGuiFollowup.md). |
| Hidden, station-only and multiple-origin GroundTracks | LGUI-026; historical fixture missions assert hidden windows, station-only initialization, invalid bodies and third-spacecraft origin mismatch. Same first-batch suite. |
| Sampling and setting validation | LGUI-029; actual publication-boundary observations check decimation, first-point retention and reset; invalid collection/update frequencies rejected; zero redraw-count setting accepted. This does not claim to add a new NumPointsToRedraw rendering implementation. |
| GroundTrack editor and Output integration | LGUI-032; actual editor opens the new GroundTrack type, Apply/save updates it, visible/hidden output entries are correct, output activation selects the existing plot. [Second batch](../LinuxGuiFollowup2.md). |
| Mixed XY/GroundTrack lookup and rename | LGUI-030; actual XY old-name collision, typed lookup, rejected misrouted action/data and correct renamed-window reuse. |
| Native OrbitView wheel zoom | LGUI-028; rendered back-buffer Earth area and camera invariants, centered/free/astronaut/Shift modes, 1x/2x/3x. |
| Native text readability and DPI | LGUI-027; actual OpenGL glyph advances in logical units, 1x/2x/3x and real canvas resizing. Font rasterization uses per-context caches. Before/after images retained in first batch. |
| Live GroundTrack updates | Tested no-change disposition; long-running mission produces repeated paint events while Publisher reports RUNNING. Latest audit: 182 paints in 11,100 ms. Direct paint-DC calls outside paint events were not imported. |
| Ground-station marker safety | LGUI-031; baseline missing-object crash; real GUI rejects missing/wrong-type objects and then accepts a valid station. |
| Dark-theme resource/mission/output trees | LGUI-033; real system colors and normal/non-savable/normal transitions, light and dark themes, contrast >=4.5 for all three trees. |
| OpenFrames time-control tooltip and tiny-size safety | LGUI-034; exact external patch archived on integration, matching isolated OFI commit. Strict GTK/wx constructor, zero/tiny/restored-size tests and full workflows at 1x/2x/3x. [External patch](../LinuxGuiExternal/OpenFramesTime/README.md). Original pixman warning is not claimed fixed. |
| Native animation replay and refresh | LGUI-035; exact chronological frame sequences, wrapped/unwrapped buffers, short/full/wide trails, replay twice, interruption/replay and untouched recording indices/data. [Native animation](../LinuxGuiAnimation/README.md). |
| GroundTrack main animation toolbar | LGUI-039; actual Play/Stop/speed dispatch, timer responsiveness, unequal spacecraft histories, cross-tab control, completion/interruption, close/recreate and empty-history behavior at 1x/2x/3x plus OFI coexistence. [GroundTrack playback](../LinuxGuiGroundTrackAnimation/README.md). |
| OpenFrames main toolbar and stopped-renderer proposal | Tested no-change disposition; main toolbar is disabled on its plugin tab; existing local playback/time/speed controls work after a mission and renderer remains active. No incomplete PluginWidget ABI extension imported. [Assessment](../LinuxGuiOpenFramesAssessment/README.md). |
| Delete unrelated resource with other editor open | LGUI-036; actual popup/confirmation flow, cancellation, dirty unrelated editor preserved, selected-object open editor and mission/resource dependencies still block deletion. [Deletion](../LinuxGuiResourceDelete/README.md). |
| Robust plugin-parent lookup | Tested no-change disposition in supported hierarchy; actual plot/configuration ancestors match MDI child; dirty state and Cancel/OK route correctly. Floating/VR alternatives not claimed covered. Same OpenFrames assessment. |
| Linux font fallback | Tested no-change disposition; actual HUD uses Liberation without Microsoft Courier at 1x/2x/3x and OSG built-in fallback when optional TTF/OTF loads are blocked. Same OpenFrames assessment. |
| Texture filtering/mipmap experiment | Old CPU mipmap rewrite not imported. The assessment instead reproduced and fixed actual RGB row corruption as LGUI-041, preserving filters and all pixels, including non-power-of-two edges, across PNG/BMP/JPEG and caller alignment/row-layout states. [Texture rows](../LinuxGuiTextureRows/README.md). |
| Missing Aura GFOIL1.JPG | LGUI-040 removes only an unresolved optional reflection declaration. NASA model/atlas provenance verified; exact native summary and complete OSG scene remain identical. Old's renamed atlas was not substituted as a reflection image. [Aura](../LinuxGuiAuraModel/README.md). |
| Additional discovered state-index defects | LGUI-037; missing/reordered/truncated/null publications preserve spacecraft identity, reject incomplete data and recover; native map filters absent positions. [State mapping](../LinuxGuiGroundTrackState/README.md). |
| Additional discovered direct-close crash | LGUI-038; unchanged independent baseline crashed, one-line event-dispatch fix passes real close/recreate with native and OFI output. [Direct close](../LinuxGuiGroundTrackClose/README.md). |
| Already-covered DE header, viewport, geometry, model-preview context, map path/short tracks | Existing core GUI suite rerun after final production change; no replacement with broad Old rendering code. Short-track safety retained; old single-point cosmetic circle is not a demonstrated correctness defect. |
| Qt and dormant floating-window redesign | Excluded as requested. No Qt implementation or floating-window architecture was imported. |
| Old and R2026a reference unchanged | Old tracked files have no modification timestamps after goal start; existing dirty diffs fingerprinted. Downloads release has no Git metadata: 3,425 source/data files inspected, none modified after goal start. These timestamp observations support the read-only work history; they are not presented as an initial-to-final cryptographic snapshot. External fixes/build adapter live in an isolated clone. |
| Reviewable patches and test separation | 35 prepared GMAT branches audited against upstream base, tracker hashes, integration ancestry and fork tips; none contains TestLinuxGui infrastructure. Fifteen of these are current follow-up production patches (026–041, excluding external 034); earlier branches are retained. Dependencies and overlap resolutions remain documented in each report. |
| External patch backup | Archived TimeDilator patch has the same stable patch ID as isolated OFI commit 751a352b39ab981fa355fa6f726e1c8bbd08aa10. Local prebuilt-library adapter is separate, explicitly non-production. |
| No PR/issue/external comment submission | Work uses local commits and explicit fork branch pushes only. Branch names reserve review scopes; they are not submissions. |

## Final validation and review readiness

| Lane | Scripts | Completed | Interpretation failures | Mission failures | Newly failing previously completed |
|---|---:|---:|---:|---:|---:|
| Primary, 1x | 210 | 136 | 67 | 7 | 0 |
| Legacy conversion, 2x | 24 | 10 | 13 | 1 | 0 |

The primary improvements are the already-prepared BasicFunction, GlobalSample, R2020a_BasicSensors and ForceModelsTutorial repairs, now included in a full sweep. They are not attributed to the newer GUI patches. All remaining statuses match the saved baseline. The table is not a claim that every historical/proprietary/dependency-requiring script works: prior failure explanations are retained in sweep-results.csv/json, alongside current results and source hashes. There are no timeouts, crashes/startup failures, viewport mismatches, inactive-window captures or screenshot errors in either lane.

Per-script logs and result records are in evidence/. Inventory files record the exact revision, harness hashes, startup configuration and selections. All 11 focused suite exit codes are in results.json and their logs in focused/. The last core GUI run after the final production change is core-suite.log. Independent patch evidence and prerequisites are linked by requirement above; no integrated test result is mislabeled as bare-upstream runtime validation.

Branch hashes, integration ancestry and fork agreement were checked for all 35 prepared GMAT branches; all 15 current follow-up patches pass whitespace checks. The external patch ID matches the archived isolated OFI change. The current binary is up to date. Earlier historical batch reports retain their then-pending notes; this audit supersedes them.

Submission remains a separate, unauthorized step. Refresh each production branch against the maintainers' chosen base and apply its documented prerequisites/overlap resolutions before proposing it. The external TimeDilator patch belongs in OFI; its local dependency adapter must not be submitted as production code. The existing Linux plugin-lifetime tradeoff still warrants maintainer design review. Final audit backup verification is recorded in backup.json in the delivered output after the integration push.

## Limits carried into delivery

Most validation is Linux wxGTK/X11 under Xvfb with Mesa software OpenGL. Windows/macOS, native Wayland, live physical monitor-DPI transitions and VR are not certified. Existing GTK notebook/OpenFrames context-resize diagnostics and the unreproduced original pixman report remain explicit follow-ups. A passing finite shutdown matrix does not prove absence of every intermittent shutdown condition. Proprietary plugins, MATLAB installation, disabled Python API and broad historical modernization remain outside scope. No warnings were hidden to obtain passing results.

## Current local launcher

The existing launcher uses the rebuilt application and accepts `--native` as its first option to disable OFI/OVtoOFI. There is no separate run-gmat-native.sh file.

```sh
/home/dan/GIT/GMAT/GMAT/build/linux-gui/run-gmat.sh --native
```

Omit `--native` for the configured OpenFrames experience. The launcher continues to use the reference installation's prebuilt dependency libraries; it does not rebuild their sources. Automated tests use isolated startup/preferences files rather than modifying the user's live settings.
