# Close plots and report failures during Linux automatic exit

Run an invalid script with --exit on Linux. The original path returns success. An OpenFrames mission can also leave live renderers when automatic process exit begins.

Destroy plot windows and pending renderer objects before the existing Linux exit path. Return status 1 when script building or execution fails.

## Validation

Native GUI and console runs produce identical state reports. An invalid script exits with status 1. The integration build also completes the OpenFrames Hohmann example and automatic exit.

Runtime checks used the integration build. Reliable OpenFrames process shutdown also needs the separate plugin-lifetime fix; this branch does not change loader flags.

```sh
python3 src/UnitTests/TestLinuxGui/test_exit.py build/linux-gui
```

Prepared branch: `pr/linux-script-exit`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
