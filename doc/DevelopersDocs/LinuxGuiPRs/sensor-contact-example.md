# Use a supported station mask in the basic sensors example

The BasicSensors example fails because ContactLocator disallows ConicalFOV on a ground-station antenna. Replace only that FOV with a supported CustomFOV at 10-degree elevation and 1-degree azimuth spacing. Document the polygon approximation; retain the spacecraft cone, orbit, propagation and contact-physics settings.

Validation: The 30-day example completes and renders at two GUI sizes. Comparing with an otherwise identical station using the circular 10-degree elevation threshold yields 30 contacts and a maximum endpoint difference of 0.128 seconds. The polygon elevation error is bounded by 0.0003731 degrees. Each patch was tested independently on the upstream-based Linux validation checkout with the pre-existing startup/GUI prerequisites documented in LinuxGuiExampleMaintenance.md. Supplemental test infrastructure remains on integration.

Prerequisites: Public Station, EventLocator and GmatEstimation plugins (Antenna), SPICE, and external OpenFramesInterface. Windows/macOS runtime validation remains outstanding.

Sample-only branch: `pr/sensor-contact-example`. No PR, issue or external comment has been submitted.
