# Keep Linux plugin libraries mapped through process shutdown

Run an OpenFrames mission and close the application normally. The reproduced crash executes osgDB registry cleanup after its library has been unmapped.

Use guarded RTLD_NODELETE to avoid executing an osgDB registry destructor after its library has been unmapped during OpenFrames shutdown.  Tradeoff: Linux plugin static state remains until process exit; in-process unload/reload no longer resets it. Windows and macOS flags are unchanged.

## Validation

An OpenFrames mission completes and the real application window closes normally. The integration Hohmann example also converges and exits.

Maintainer design review is needed: RTLD_NODELETE keeps Linux plugin static state resident until process exit and changes in-process unload/reload behavior. Automatic-exit cleanup is in a separate branch. External OFI TimeDilator tooltip warnings are still visible.

```sh
python3 src/UnitTests/TestLinuxGui/test_plugins.py build/linux-gui
```

Prepared branch: `pr/linux-plugin-lifetime`. No PR has been opened.
