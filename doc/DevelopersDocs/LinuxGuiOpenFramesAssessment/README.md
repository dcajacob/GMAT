# Remaining OpenFrames candidates: current-build assessment

The optional regression `test_openframes_behavior.py` exercises the real OFI plugin and GMAT GUI. It passes without Microsoft fonts at 1x, 2x and 3x, and with all optional TTF/OTF fonts blocked at 1x. Font blocking is an isolated OSG read callback inside the test; no host fonts or production files are changed.

## Main toolbar and local playback

After a real mission, the renderer is still running. The plugin's existing local playback resumes, advances time, pauses, speeds up and slows down. The actual OpenFrames child is USER_DEFINED_OBJECT (10001), and GMAT's main animation toolbar is disabled when it is active. A programmatically delivered unsupported main-toolbar command leaves plugin playback and main animation state unchanged. Closing after playback succeeds.

Disposition: retain the existing local controls. Old's main-toolbar extension is an enhancement, not a reproduced failure of the supported current interface. It adds virtual PluginWidget methods and contains an incomplete speed setter; it is not suitable for a compatibility-preserving bug fix. The proposed stopped-renderer ResumePlayback change is not needed in the tested supported lifecycle: the renderer remains active after execution. This does not certify every internal stopped-renderer/VR scenario.

## Plugin-parent lookup

The plot's actual two-parent ancestor is its GmatMdiChildFrame. Configuration panels created through the real plugin factory have the same documented structure (panel inside scrolled window inside MDI child). Dirty state reaches that child; Cancel and OK each destroy the correct child, including reopening between cases. Tests call the existing protected event handlers through test-only member pointers.

Disposition: no hierarchy patch. The supported current factory matches the existing code. The old ancestor-walking change chiefly supported the deferred floating-window experiment. A future hierarchy change should revisit it; VR-shaped-window behavior is not exercised here.

## Font fallback

The actual OpenFrames HUD selects /usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf when Microsoft fonts are unavailable and has nonempty glyph layout at all tested scales. With every optional TTF/OTF file blocked, the HUD uses OSG's built-in fallback and still has nonempty glyph layout. The test traverses the actual HUD subtree, not unrelated scene labels. Initial development accidentally inspected scene labels; those invalid test observations are not treated as production findings.

Disposition: the existing Liberation fallback meets the tested no-Microsoft-font scenario. No DejaVu-specific selection policy is introduced. The linked OpenFrames version does not supply Old's proposed resolveFontPath helper. Nonempty glyph layout verifies fallback viability, not subjective font quality or pixel-perfect appearance on every display.

## Environment and boundary

GMAT integration includes the earlier external TimeDilator fix; OFI source commit 751a352b39ab981fa355fa6f726e1c8bbd08aa10 is used with the documented local build adapter. Tests use Linux wxGTK/X11, Xvfb and Mesa software rendering. Other platforms, native Wayland and VR remain validation gaps.

These are evidence-backed no-change dispositions, not claims that all plugin behavior is defect-free. No production plugin interface or parent/font code was changed for this assessment. No PR, issue or external comment was submitted. Texture findings are recorded separately under texture-rows; the final whole-goal audit remains pending.
