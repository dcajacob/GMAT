# Native RGB texture row layout (LGUI-041)

The texture assessment reproduced corruption in actual native plot and spacecraft-model uploads for 17-pixel-wide RGB images. wxImage rows contain exactly width*3 bytes, while OpenGL's unpack alignment may require padding. At alignment 4 or 8, the baseline uploaded shifted/corrupted rows for PNG, BMP and JPEG. Alignment 1 and 32-pixel-wide controls passed.

The patch declares tightly packed, unskipped rows for the upload and restores the caller's pixel-store state on each exit. It changes neither minification/magnification filters nor mipmap generation. The old CPU mipmap rewrite is not needed for this demonstrated defect and is not imported; its handling of odd dimensions would need separate quality/design work.

## Validation

`python3 src/UnitTests/TestLinuxGui/test_texture_upload.py build/linux-gui`

The test runs a real native GUI mission, uses its actual OpenGL context, generates PNG/BMP/JPEG fixtures at widths 17 and 32 (height 19), and exercises both ViewCanvas::LoadImage and ModelObject::Load. It reads the textures back from OpenGL and compares every RGB byte to wxImage's decoded pixels, including the intended vertical mirror for plot textures. JPEG comparison uses the decoded image, not the pre-compression input. Model fixtures use the existing Aura geometry with same-length diffuse filenames.

All 18 size/format/alignment combinations pass through both loaders. The final test also starts with non-default row length and row/pixel skips, confirms the upload ignores those unrelated caller settings, and verifies all four unpack settings are restored. Baseline evidence isolates alignment corruption with the other row settings at their defaults. No screenshot or visual resemblance substitutes for the pixel comparison.

The exact standalone patch applies cleanly and passes against prerequisite baseline 584bf1ee0b9fe226caf4b2a3b5c6438aea61d906. That checkout and executable are restored afterward. Existing GUI suite results are recorded in gmat-texture-suite.log.

- Integration production: 5189892891921afd59318bfe315c29b7ef2f03a5.
- Standalone branch: pr/native-texture-rows, 27666c2d222cb1591d0d11cbec812d9ed677be78.
- Standalone base: NASA 9363e129be366520c6edb0b4079204ed60666007.
- Scope: ModelObject.cpp and ViewCanvas.cpp only; tests/audit remain on integration.

Linux wxGTK/X11 and Mesa software OpenGL were validated. Windows/macOS, hardware-specific behavior and native Wayland are untested. Existing non-Linux mipmap/filter code remains unchanged; the tests do not claim coverage for those branches, every image format, or arbitrary external OpenGL state beyond the tested row-layout settings.

No PR, issue or external comment was submitted. Old and the Downloads reference source remain unchanged. The whole-goal completion audit is still pending.
