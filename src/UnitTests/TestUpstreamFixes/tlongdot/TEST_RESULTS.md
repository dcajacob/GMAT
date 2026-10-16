# Executed regression result

Executed 2026-10-16 UTC on Linux x86_64 with GCC 15.2.0 and Python 3.14.4, using one compiler process at a time. No system packages were installed and no full GMAT build ran.

| Source | Immutable revision | Outcome |
| --- | --- | --- |
| Unmodified NASA | `c9b78bd1c8499845cb78fde315e1f761b6b06f1c` | Exit 1: 24 intended contract failures among 66 checks across 12 cases; compilation/execution succeeded |
| Fix plus fixture | `3c49c37c2156622d2d616c4e6f607d9baec83c42` | Exit 0: all 66 checks across 12 cases passed |

Six central-motion cases reach exact zero perturbation and match analytic/Cartesian checks. Six perturbed control vectors retain their recorded NASA behavior.

Production source: `src/base/parameter/OrbitData.cpp`. Tested Git blob: `1eaa9895bc368efdd0029e26592f090f2913f43b`. Runner blob: `fb2b5115a6ad40bd4a4f222deb014af54fc3caa3`. Subsequent documentation commits preserve these tested source/runner identities.

These are focused compiled native source/constructor/logic-region tests with explicit dependency shims. They do not establish complete linked-engine, GUI, script, Python-binding, Windows/macOS or scientific mission qualification. See README for the specific extraction/dependency seams and mathematical-reference provenance. Baseline failures are behavioral assertions, not extraction/compiler/infrastructure failures.

Reproduce from this repository's root with fresh output directories:

```sh
python3 src/UnitTests/TestUpstreamFixes/tlongdot/run_tlongdot.py --repo . --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /tmp/tlongdot-baseline
python3 src/UnitTests/TestUpstreamFixes/tlongdot/run_tlongdot.py --repo . --revision 3c49c37c2156622d2d616c4e6f607d9baec83c42 --output /tmp/tlongdot-fixed
```

The baseline command is expected to exit 1; the fixed command is expected to exit 0. Exit 2 signals an operational failure. Generated binaries, source snapshots and raw logs are local outputs, not committed distribution artifacts. No unload/recovery, held source-selection, whole numerical-replacement or scientific-data changes belong to this branch.

Retained raw receipt SHA-256: baseline `5ec7653e281d212200bf2d68debf700eedb5ccf1561b67cf39288484add87575`, fixed `e90a894e105ca021b8286a6237ca054cde34654c1ba8251926f5ce02e0873b04`. Reproduction creates independent receipts with new paths/timestamps/binary identities; byte-identical output is not promised.
