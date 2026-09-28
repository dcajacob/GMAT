# Fix satellite separation function example

BasicFunction fails interpretation before its SatSep call. Restore explicit command boundaries in the script and its two helpers, with local declarations before command mode. Correct SatSep to compute Y and Z differences from their matching coordinates; both previously reused X. Preserve global component outputs and the nested dot-product example.

Validation: The full example completes. Six axis, mixed-sign and coincident-position probes return exact component/norm results, including 13 km for (3,4,12). Each patch was tested independently on the upstream-based Linux validation checkout with the pre-existing startup/GUI prerequisites documented in LinuxGuiExampleMaintenance.md. Supplemental test infrastructure remains on integration.

Prerequisites: GmatFunction; retain BasicFunction.script, SatSep.gmf and dot.gmf in the same folder. Windows/macOS runtime validation remains outstanding.

Sample-only branch: `pr/satellite-separation-example`. No PR, issue or external comment has been submitted.
