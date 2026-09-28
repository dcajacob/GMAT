# Make force model tutorial runnable with adaptive steps

ForceModelsTutorial cannot create its report in an assumed relative subdirectory. Use the configured output directory and declare BeginMissionSequence. Execution then exposes an accuracy failure with fixed 30-second steps; allow smaller steps while retaining 1e-12 accuracy and the 30-second maximum for all six force models.

Validation: The original mission completes with seven expected report rows. A 15-second-maximum comparison differs by at most 3.01e-8 km in position and 1.35e-11 km/s in velocity. Forces, initial orbit and propagation intervals are unchanged. Each patch was tested independently on the upstream-based Linux validation checkout with the pre-existing startup/GUI prerequisites documented in LinuxGuiExampleMaintenance.md. Supplemental test infrastructure remains on integration.

Prerequisites: Standard bundled force models/data and a writable OUTPUT_PATH. Windows/macOS runtime validation remains outstanding.

Sample-only branch: `pr/force-model-tutorial`. No PR, issue or external comment has been submitted.
