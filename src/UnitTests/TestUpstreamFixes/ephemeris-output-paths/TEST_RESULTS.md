# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 3 intended contract failures among 9 checks; compilation/execution succeeded |
| Fix plus fixture | `3343328c67161c70612974f56f50df9f21c860c6` | Exit 0: all 9 checks passed |

Empty/nonempty output directories are rejected and preserved; an empty resolver result is rejected; valid writes and retry controls pass. Resolver/writer seams are explicit stubs.

Production source: `src/base/subscriber/EphemerisFile.cpp`. Tested Git blob: `32650b3ffa696e72273116503badece1669a1d35`. Runner blob: `760c6980c12bcbb128246afd30a83ac1ba5808dc`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/ephemeris-output-paths/run.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/ephemeris-output-paths-baseline
python3 src/UnitTests/TestUpstreamFixes/ephemeris-output-paths/run.py --repo . --revision 3343328c67161c70612974f56f50df9f21c860c6 --output /tmp/ephemeris-output-paths-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `883de82f5a917adde0a5d6c7bffb4c6c84249bacacdc44e09c9ba310be63e639`, fixed `10b97a044327b20e941ce49db9bf029c1287664b4978dfbe9c72ea05bb591a54`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
