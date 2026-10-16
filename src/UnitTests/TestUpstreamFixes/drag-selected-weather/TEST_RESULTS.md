# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 9 intended contract failures among 18 cases; compilation/execution succeeded |
| Fix plus fixture | `1ab538279bcfc2211f2343f302eac378ef1ff3f0` | Exit 0: all 18 cases passed |

Constant inputs do not probe unselected weather. CSSI/Schatten/mixed selected modes validate missing, malformed and synthetic valid files. The legacy-defined both-valid control passes in baseline and candidate. No density/force calculation is exercised.

Production source: `src/base/forcemodel/DragForce.cpp`. Tested Git blob: `6caac694865045a4855f38aba9f5604ca3c4146f`. Runner blob: `a3130f7816a6956b6db942feba40fca52cc62518`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/drag-selected-weather/run.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/drag-selected-weather-baseline
python3 src/UnitTests/TestUpstreamFixes/drag-selected-weather/run.py --repo . --revision 1ab538279bcfc2211f2343f302eac378ef1ff3f0 --output /tmp/drag-selected-weather-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `925c4086d0a5379fe13a67244cc7edb41a60ad535bd52e577902d2ebc021aa17`, fixed `d4a423a30a585cb2149950f785895f60fd13524e35e197ee005d0e662e27fc19`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
