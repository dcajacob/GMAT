# Fix fixed-width DE ephemeris header copies

Startup on fortified Linux libc can abort when DE header labels or constant names occupy their complete binary fields without null terminators.

Copy binary labels, names and padding without assuming null terminators. Guard header self-assignment. The fortified regression fixture previously aborted; copy, assignment and self-assignment now pass.

## Validation

The standalone fortified copy/assignment/self-assignment fixture passes. It aborted against the original implementation.

The binary-copy behavior is shared across platforms. Windows/macOS builds are still pending.

```sh
python3 src/UnitTests/TestLinuxGui/test_header.py build/linux-gui
```

Prepared branch: `pr/de-header`. No PR has been opened.

Supplemental test commands above run from `linux-gui-integration`; the proposed upstream net diff contains production code only. Rebuild and validate the isolated branch before submission.
