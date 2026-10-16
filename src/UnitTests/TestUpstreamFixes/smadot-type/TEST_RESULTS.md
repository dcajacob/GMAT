# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 3 intended contract failures among 13 checks; compilation/execution succeeded |
| Fix plus fixture | `d32fd6e67445d69baa5f2158335183d1954d1745` | Exit 0: all 13 checks passed |

The exact native constructor reports SMADot consistently. This does not link ParameterFactory or the complete native engine.

Production source: `src/base/parameter/EquinoctialParameters.cpp`. Tested Git blob: `e1023fec241368808223a94b53a72578bae1a8d3`. Runner blob: `68b31e0d9cdbc025ced728edc2c5a432decf3868`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/smadot-type/run.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/smadot-type-baseline
python3 src/UnitTests/TestUpstreamFixes/smadot-type/run.py --repo . --revision d32fd6e67445d69baa5f2158335183d1954d1745 --output /tmp/smadot-type-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `6e622d91a53f8514cf9baf869cd8ad7852c15c09082bcea312b02aff8d8ccba9`, fixed `aa55265545dfe39dde8ebd29c5a22174b98050c165e13c2f05036c3f4eeed6ff`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
