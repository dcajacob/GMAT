# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 9 intended contract failures among 17 checks; compilation/execution succeeded |
| Fix plus fixture | `0fd2c9739b544fe247201d4d6423ea9d9059585d` | Exit 0: all 17 checks passed |

All three owning-map replacement variants and refresh release their entries. Allocation counters test object lifetime, not process RSS.

Production source: `src/gmatutil/util/FileManager.cpp`. Tested Git blob: `495660562c6d30d284a4630ca61010b74173d23a`. Runner blob: `21e2ce173be87b3d197a5d97317a07886fc48f09`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/filemanager-ownership/run.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/filemanager-ownership-baseline
python3 src/UnitTests/TestUpstreamFixes/filemanager-ownership/run.py --repo . --revision 0fd2c9739b544fe247201d4d6423ea9d9059585d --output /tmp/filemanager-ownership-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `bd49322fd005a85f77840be73b5272161a44cb5cb89a2307913ca85785b2dbf2`, fixed `9454cd712cb6bf84cf6cd30af87706253fb9f7197a31ab4259185a473ca60d29`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
