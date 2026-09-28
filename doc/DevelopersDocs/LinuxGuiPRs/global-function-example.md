# Repair no-output global function example

GlobalSample fails because its no-output helper uses an empty output list and lacks a function command boundary. Use the supported no-output declaration and explicit mission/function boundaries, preserving the global burn and spacecraft example.

Validation: The original four-call mission completes. A controlled perigee probe confirms all four shared-spacecraft velocity increments and increasing apogee, with maximum burn error 7.22e-16 km/s. Each patch was tested independently on the upstream-based Linux validation checkout with the pre-existing startup/GUI prerequisites documented in LinuxGuiExampleMaintenance.md. Supplemental test infrastructure remains on integration.

Prerequisites: GmatFunction; retain GlobalSample.script and RaiseApogee.gmf together. Windows/macOS runtime validation remains outstanding.

Sample-only branch: `pr/global-function-example`. No PR, issue or external comment has been submitted.
