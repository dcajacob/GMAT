# GMAT contribution preparation — 2026-09-28

No PRs, issues or comments have been submitted. These are preparation recommendations, not a claim that maintainers have approved the changes.

## What the sources establish

- The current NASA GitHub community profile exposes no CONTRIBUTING file, PR template or issue template. The current repository license is Apache 2.0. Do not import contribution or CLA requirements from other NASA projects.
- The official [For Contributors](https://gmat.atlassian.net/wiki/spaces/GW/pages/380273279/For%2BContributors) page welcomes contributions and links development/testing guidance. It was last updated in 2020; some infrastructure references are historical.
- [Governance](https://gmat.atlassian.net/wiki/spaces/GW/pages/380273277/Governance) describes JIRA-based tracking, but is dated 2013. GitHub PRs are currently receiving team replies. Confirm the preferred issue/submission route when submission is authorized; do not assume GitHub PRs are forbidden or JIRA access is mandatory.
- The [C++ style guide](https://gmat.atlassian.net/wiki/spaces/GW/pages/380273289/GMAT%2BC%2BStyle%2BGuide) emphasizes readable, maintainable code. Match surrounding formatting and conventions; avoid unrelated reformatting.
- Official [Testing](https://gmat.atlassian.net/wiki/spaces/GW/pages/380273299/Testing) and [GUI Testing](https://gmat.atlassian.net/wiki/spaces/GW/pages/380273297/GUI%2BTesting) guidance covers several test levels and an established TestComplete GUI workflow. Our Linux/Xvfb harness is supplemental evidence, not a replacement for their infrastructure.
- A [GMAT team reply on PR #12](https://github.com/nasa/GMAT/pull/12#issuecomment-5795622579) explains that scheduling developer review can take time. A separate [community comment](https://github.com/nasa/GMAT/pull/12#issuecomment-5798577521) recommends small, easily reviewed patches and avoiding unnecessary helper extraction or new testing machinery. The latter is useful advice, not a binding maintainer policy.

## How we will prepare these changes

1. Give each PR one concrete problem, a short reproduction, before/after behavior and the minimum production diff. Keep machine-specific configuration and broad refactoring out.
2. Keep the supplemental test harness and this tracking documentation on `linux-gui-integration`. Proposed `pr/*` branches now have production-only net diffs against the NASA baseline. Historical commits retain the original tests; cleanup commits avoid rewriting pushed history. Squash before submission only if maintainers prefer it.
3. Include environment, exact test commands, expected results, screenshots where useful, and limitations. Distinguish combined integration results from isolated branch validation.
4. Refresh against upstream and build/test each branch independently before submission. The existing upstream fortified DE-header startup failure is a prerequisite on this host; land that small correction first.
5. Lead with the ephemeris, CMake and viewport corrections. Map, layout, geometry and exit changes follow separately. Hold the plugin-lifetime change for design feedback because it changes unload/reload behavior.
6. Offer focused test fixtures for the maintainers' preferred test location; do not require adoption of our complete harness. Keep real issue links separate from local LGUI tracking IDs.

## Native viewport correction

At 3× display scaling the native canvas measured 722×536 logical pixels, but its framebuffer required 2166×1608 physical pixels. GMAT supplied the smaller dimensions to `glViewport`, confining the scene to the lower-left ninth of the plot. Resizing reproduced the same mismatch twice.

Three viewport calls now multiply client dimensions by `GetContentScaleFactor()` and round to pixel dimensions: shared `ViewCanvas::SetDrawingMode`, `OrbitViewCanvas::OnSize`, and the legacy OpenGL `GroundTrackCanvas::OnSize`. Camera projection and mouse coordinates remain logical. This follows the [wx GLCanvas documentation](https://docs.wxpython.org/wx.glcanvas.GLCanvas.html), which specifies physical pixel viewport dimensions.

The production patch is three files, nine added lines and three removed lines. The rebuilt GUI passes all 25 integration checks. New native OrbitView tests check actual GL viewport dimensions before and after two resizes at each of 1×, 2× and 3× scaling; all nine size assertions pass. The pre-fix 3× run failed all three assertions. Screenshots show the scene centered across the plot after correction.

Validation used wxGTK 3.2.9, X11 virtual displays and software OpenGL. Native Wayland and the user's physical Intel GPU have not been validated by this harness. The legacy GL GroundTrack resize call received the same correction but was not separately exercised as a live plot; modern GroundTrackArea remains covered by existing pixel checks. Windows and macOS builds were not run.

Run the rebuilt application from `build/linux-gui` with `./run-gmat.sh --native`. Existing default startup still enables OpenFrames.
