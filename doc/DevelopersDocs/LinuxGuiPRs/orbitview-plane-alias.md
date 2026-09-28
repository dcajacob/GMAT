# Preserve CelestialPlane when converting legacy OrbitView objects

The unchanged Hohmann and Mars B-plane tutorials ran natively but failed during OVtoOFI conversion on `CelestialPlane = Off`. Map this deprecated field to OpenFrames' `EclipticPlane` in both the getter and setter, matching native OrbitView semantics.

Reproduction: convert an OrbitView with `CelestialPlane` set to `On`, then `Off`; read both names, save, reload and verify the canonical plane state. Run `doc/help/src/files/scripts/Tut_HohmannTransfer.script` and `Tut_Mars_B_Plane_Targeting.script` unchanged.

Validation: On/Off and save/reload pass in native and converted sessions. The alias patch also passes independently against the configured external OpenFramesInterface dependency, with the existing Linux prerequisites listed in `LinuxGuiPublicFixes.md`. Both tutorials complete at 1x, 2x and 3x; their inputs remain unchanged. Screenshot review and remaining external rendering limitations are recorded separately in that report. The patch is six production lines. Windows/macOS have not been tested.

Supplemental reproduction: `OrbitAliasRegression.cpp` and `test_orbit_alias.py` on the integration branch. No renderer or external OpenFrames code is changed.

Production-only branch: `pr/orbitview-plane-alias`. No PR, issue or external comment has been submitted.
