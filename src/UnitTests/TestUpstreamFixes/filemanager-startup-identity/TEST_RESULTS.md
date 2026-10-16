# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 6 intended contract failures among 8 checks; compilation/execution succeeded |
| Fix plus fixture | `db476bf6ddcbe840ff2fbc62d56655ab8bc02e9e` | Exit 0: all 8 checks passed |

An absolute initialized startup path survives actual cwd changes. Relative/empty-state controls also pass; startup parsing is not linked.

Production source: `src/gmatutil/util/FileManager.cpp`. Tested Git blob: `c4c2509c5fca5f960a82c3ee06b2ad13718a01a1`. Runner blob: `29a542842601d1fcd1e8c327a770e960eee611fb`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/filemanager-startup-identity/run.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/filemanager-startup-identity-baseline
python3 src/UnitTests/TestUpstreamFixes/filemanager-startup-identity/run.py --repo . --revision db476bf6ddcbe840ff2fbc62d56655ab8bc02e9e --output /tmp/filemanager-startup-identity-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `c20be4aa2d2c5dbf5362dc939207880084b65bfadf5ab80d56d4d4b3962d208d`, fixed `a60563ddcbdcb75768e79e6ec5777e4a565ce123d6dcb99650fa19266998516d`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
