# Resource deletion with unrelated editors (LGUI-036)

Delete remains available in the resource context menu when an unrelated properties editor is open. Rename retains its existing restriction. The existing selected-editor check, confirmation prompt and resource/mission dependency checks remain unchanged.

## Evidence

The baseline actual popup menu hid Delete while an unrelated spacecraft properties editor was open. Calling the existing deletion handler already worked safely, so the fix only changes menu construction. The regression observes the real popup menu, then exercises that same deletion handler.

Light- and dark-theme checks pass for:

- Delete visible and Rename still hidden with an unrelated editor open.
- Cancelled deletion preserving the object.
- Confirmed deletion removing an unused variable from the model and tree.
- The unrelated spacecraft editor staying open with its unsaved value intact.
- The selected resource's open editor blocking deletion before confirmation.
- A Propagate command protecting its propagator, a spacecraft protecting its fuel tank, and a Target command protecting its solver.

The exact production patch also passes both themes independently on prerequisite baseline `584bf1ee0b9fe226caf4b2a3b5c6438aea61d906`. The validation checkout and binary were restored afterward. Tests used Linux wxGTK/X11 and Xvfb. Windows, macOS and native Wayland remain unvalidated.

```sh
python3 src/UnitTests/TestLinuxGui/test_resource_delete.py build/linux-gui
```

This narrow menu change received focused and independent validation; the full GUI suite last passed immediately before it, after the native animation fix. A final full-scope suite remains part of the active goal.

## Review scope

- Production integration commit: `ba5bc315deb8b8346c0ce8ac9d7d044e87f6d91b`.
- Independent branch: `pr/resource-delete-menu`, commit `57e22b63e052bd62f6e75da1b76adb6d3be76418`.
- Independent base: NASA `9363e129be366520c6edb0b4079204ed60666007`.
- Only ResourceTree.cpp changes in the production branch. Tests and audit records remain on integration.

No PR, issue or external comment was submitted. Old and R2026a sources remain unchanged. The overall goal remains active, including toolbar behavior, plugin hierarchy/font/texture assessments, missing Aura texture provenance, GroundTrack state-index checks and final evidence audit.
