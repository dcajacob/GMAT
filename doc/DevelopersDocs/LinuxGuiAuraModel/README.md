# Bundled Aura reflection reference (LGUI-040)

The missing GFOIL1.JPG warning came from an optional gold_foil reflection map in aura.3ds. Nine ordinary surface-map references already resolve to the bundled aura_map.jpg. The Old copy named GFOIL1.JPG is byte-for-byte the surface atlas, not evidence of the intended reflection image. It was not copied.

The repair removes the unresolved 31-byte MAT_REFLMAP (0xA220) chunk and updates its three ancestor lengths. All other payloads, geometry, UV coordinates, material values and the original JPEG are unchanged. Native GMAT ignores that optional map; OpenSceneGraph previously failed to load it and skipped it. Removing the dangling declaration produces the same loaded scenes, without the repeated missing-image diagnostic. No warning suppression, image substitution or renderer changes were made.

## Provenance

[NASA's Aura (A) page](https://science.nasa.gov/3d-resources/aura-a/) identifies NASA Ames Research Center and links its official model repository. The [historical original model directory](https://github.com/nasa/NASA-3D-Resources/tree/05374ee6e3b254cf869d3c19fb2f9bddb05a20c0/3D%20Models/Aura%20(A)) contains aura.3ds and aura_map.jpg, with no GFOIL1.JPG. The Git blob IDs match GMAT's pre-repair files exactly:

- Model: 1a5eeff4fc9f485604620497561a5b1228fb9512.
- Image: 67a07430a4eabd1b041b4ee39654ef5c259192b8.

This establishes the supplied model/atlas provenance, not the content or rights of an absent reflection image. No new image was imported. The production README records the source and modification.

## Validation

Run `python3 src/UnitTests/TestLinuxGui/test_aura_model.py build/linux-gui`. This optional regression requires the OpenSceneGraph build dependency, its 3DS/image plugins and the upstream base commit in the local repository. The OpenFrames mission additionally requires OFI/OVtoOFI.

The test reconstructs the exact binary edit from the upstream model and checks the original image hash. It loads both versions with GMAT's actual native reader and OpenSceneGraph's actual 3DS plugin. Every diffuse image resolves and OSG's images retain their original 1024x1024 dimensions. The native material summaries and complete serialized OSG scenes are byte-identical before and after. Their SHA-256 hashes are recorded; the 4.9 MB scene dumps are reproducible test outputs rather than duplicated audit files. Baseline logging contains the missing reflection diagnostic; fixed logging does not.

Native and OpenFrames GUI smoke missions both complete and close normally with the fixed asset. These focused tests pass in integration and in the independent prerequisite build. No executable rebuild is needed for this data-only patch. Prior whole-GUI regressions remain recorded in the GroundTrack playback batch; they are not relabeled as a new full-suite run here.

- Integration production commit: 4210a0757126e5ee3b91a85c5a00a38678d1718f.
- Standalone branch: pr/aura-reflection-map, f6894739d36787634ffd9883bbd9ced025d749ae.
- Standalone base: NASA 9363e129be366520c6edb0b4079204ed60666007.
- Independent test base: 584bf1ee0b9fe226caf4b2a3b5c6438aea61d906 with existing Linux startup/layout/map/viewport/plugin-lifetime prerequisites. Asset files were restored after testing.

Validation covers Linux wxGTK/X11 with Xvfb/Mesa software rendering. Windows/macOS and native Wayland remain untested. The loaded-scene comparison is stronger than a screenshot for preserving geometry/material state, but does not claim hardware-specific rendering coverage.

Old and the Downloads reference source were read only. No PR, issue or external comment was submitted. This resolves the missing-Aura-reference follow-up; other OpenFrames/font/texture assessments and the final scope audit remain active.
