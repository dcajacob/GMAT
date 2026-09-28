# Restore Linux window geometry from personalization settings

Resize or reposition the Linux window, close GMAT, and reopen it. The original launch resets to the default size and position. Saved settings must also be safe after a display disappears.

Persist native wxWidgets geometry in the existing settings file. Validate saved dimensions before GTK receives them, recover off-screen windows and respect the personalization write opt-out.

## Validation

Focused checks pass for save/restore, invalid/off-screen/oversized settings, an 800x600 display, 2x scaling and personalization write opt-out.

Runtime checks used the integration build, including the separate ephemeris and layout fixes. Physical monitor changes, native Wayland and interactive maximization need further checking. Layout behavior is deliberately excluded from this branch.

```sh
python3 src/UnitTests/TestLinuxGui/test_geometry.py build/linux-gui
```

Prepared branch: `pr/linux-geometry`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
