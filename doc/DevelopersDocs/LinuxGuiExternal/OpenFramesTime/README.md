# OpenFrames time-control repair (LGUI-034)

The current Linux build now uses a patched, isolated OpenFramesInterface source clone. The Downloads R2026a and Old sources remain unchanged. No PR, issue or external comment has been submitted.

## Patch and independent scope

`time-controls.patch` is a one-file production patch against OpenFramesInterface commit `82ea72584fc1c858bd3eb3899e5c3d3b31e32f3c`, from https://gitlab.com/EmergentSpaceTechnologies/OpenFramesInterface.git. Its local commit is `751a352b39ab981fa355fa6f726e1c8bbd08aa10` on `linux-gui-time-controls`. This belongs to the external plugin, not NASA GMAT source. It is archived on the GMAT integration branch for backup and review; it is not a GMAT production branch.

- Initialize cursor state as ordinary data; set the cursor and tooltip only after successful native widget creation.
- Allocate drawing buffers only at positive sizes, tolerate a temporarily absent buffer during painting, and avoid negative drawing geometry when the control is too small for its borders.
- Preserve time limits, current time, normal tooltip behavior and playback operations.

## Reproductions and results

The unpatched plugin deterministically calls GTK tooltip APIs before Create and aborts with fatal criticals enabled. A second baseline run permits that existing warning and records invalid-bitmap/invalid-DC assertions at zero size. Both baseline logs are retained.

The patched control passes construction and zero/tiny/restored-size tests at 1x, 2x and 3x with GTK criticals fatal and wx assertions counted as failures. The current time remains 150 after resizing. Full OpenFrames GUI workflow checks pass at all three scales: repeated missions, resize, plot close/recreation, Stop, editor operations, resource panels and normal close. These are Linux X11/wxGTK, Xvfb and Mesa software-rendering results. Windows, macOS, native Wayland and hardware-specific paths remain unvalidated.

The original pixman rectangle warning has not been independently reproduced and is not claimed fixed. OpenFrames context-resize warnings, GTK notebook warnings and the missing GFOIL1.JPG warning remain visible in the full logs. Full-workflow screenshots are useful context, not a pixel-accurate rendering-quality assertion.

Run the focused regression after building GMAT with the patched optional wx OpenFrames plugin:

```sh
python3 src/UnitTests/TestLinuxGui/test_openframes_time.py build/linux-gui
```

The direct widget reproduction needs only the external plugin patch; the broader workflow uses the established GMAT integration fixes. Tests are kept on the integration branch.

## Reproduce the local build

Clone the external source into a separate writable directory, check out the base above, and apply `time-controls.patch` with `git am`. For normal builds use the external project's documented dependency setup.

This machine instead reused already-built OpenFrames, OpenSceneGraph and osgEarth libraries. `local-prebuilt-dependencies.patch` is a **local build adapter only**, excluded from the production patch: it disables FetchContent population and requires existing OPENFRAMES_DIR, OSG_DIR and OSGEARTH_DIR installations. Use it only in a fresh isolated source clone with no populated dependency source directories; it is not a general dependency-management fix.

The current GMAT CMake cache points GMAT_ADDITIONAL_PLUGINS to the isolated clone's OpenFramesInterface_AdditionalPlugin.txt and GMAT_ADDITIONAL_PLUGINS_OpenFramesInterface to the clone itself. Dependency paths retain the reference installation's existing libraries. Configuration and build logs are retained. The plugin and OVtoOFI were rebuilt successfully; no dependency rebuild was performed in the reference installation.

## Remaining goal work

Animation replay and toolbar behavior; unrelated-resource deletion; plugin parent hierarchy and font fallback; texture filtering; missing Aura texture provenance; GroundTrack state-index validation; final complete-scope regression and evidence audit. The goal remains active.
