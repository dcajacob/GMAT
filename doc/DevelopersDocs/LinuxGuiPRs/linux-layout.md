# Keep Linux navigation and message panes within the window

Enlarge the navigation/message panes, then shrink the frame to 640x480. The original workspace became negative in size. The default sidebar also clipped its third tab, and the mission tree requested a negative initial width.

Account for all sidebar tab labels and clamp sash defaults as well as drag limits when the frame shrinks. Remove the premature tree width subtraction that produced a negative GTK size.

## Validation

Focused checks pass for large sash drags, oversized and invalid console preferences, an 800x600 display and 2x scaling.

Runtime checks used the integration build, including the ephemeris startup fix. Geometry persistence is deliberately excluded from this branch.

```sh
python3 src/UnitTests/TestLinuxGui/test_layout.py build/linux-gui
```

Prepared branch: `pr/linux-layout`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
