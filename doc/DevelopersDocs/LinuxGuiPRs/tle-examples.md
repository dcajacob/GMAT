# Use SPICESGP4 and bundled relative inputs in public TLE examples

All eleven plugin sample/test scripts used the unregistered historical `TLE` propagator type. Select `SPICESGP4`, update the sample generator, replace the machine-specific Windows path, and resolve sample inputs from the supplied TLE directory. Document the precise catalog requirements without substituting orbital data or changing spacecraft identities, epochs or propagation settings.

Validation: the five fixture-backed tests complete offline when this sample-only patch is applied independently; the integration sweep also completes `PropLightsail2.script`. All eleven scripts get past interpretation. Five examples still need the catalogs documented in `doc/source/Scripting.rst`; these were not downloaded or replaced. The test runner preserves sibling TLE directories, but that infrastructure remains on the integration branch.

Prerequisites: enable the public TLEPropagator plugin and retain the supplied folder layout. Optional OpenFrames is required by samples that explicitly use it. The independent Linux test environment includes the existing prerequisites listed in `LinuxGuiPublicFixes.md`. Windows/macOS runtime validation remains outstanding.

Production-only branch: `pr/tle-examples`. No PR, issue or external comment has been submitted.
