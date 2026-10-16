# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 3 intended contract failures among 3 process fixtures; compilation/execution succeeded |
| Fix plus fixture | `26b80421894314d76c77019625a60e7d205317c1` | Exit 0: all 3 process fixtures passed |

Existing log bytes are retained with a fresh diagnostic sibling; absent/long destinations use console fallback without FILEOPENFAILED noise. Baseline already returns after the error and passes reset/name/vector usability controls: no process abort or broken-engine result was demonstrated.

Production source: `src/base/spice/SpiceInterface.cpp`. Tested Git blob: `2cf1e8a983cb04aa14f2f8ba9ec0441c1e65a3a8`. Runner blob: `cc31e13931a4d18ce63e8bc4a966dbe557dfa868`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/spice-diagnostics/check_spice_diagnostics.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/spice-diagnostics-baseline --cspice-dir /path/to/existing-cspice
python3 src/UnitTests/TestUpstreamFixes/spice-diagnostics/check_spice_diagnostics.py --repo . --revision 26b80421894314d76c77019625a60e7d205317c1 --output /tmp/spice-diagnostics-fixed --cspice-dir /path/to/existing-cspice
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `65ec5631ac93e5aa4499a5f9e1e473451e4cc2279e55efc3b6ae90228038e74a`, fixed `8b52a09252c89b24990678ea8bde3af09f85144dc3cc432a29c193458927413f`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.

Both runs used the same existing CSPICE_N0067 archive, SHA-256 `08c97b49051e112eb4e36a0fc4d2bf9b4e84ab45813621db3943194372cd91cd`. Supply the dependency explicitly; no CSPICE source, headers, library or kernels are bundled in this test directory.
