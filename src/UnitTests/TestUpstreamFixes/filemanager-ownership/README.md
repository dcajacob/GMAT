# filemanager-owned-entry-lifecycle

This standalone regression runner reads exact native definitions through `git show`
at the requested immutable commit and compiles a small C++ probe. It uses Python's
standard library, Git, and an already installed C++17 compiler (`CXX`, default
`c++`). It does not build GMAT or require Python packages, downloaded data or a
network connection.

Run from any directory, using a **new** output directory for every revision:

```bash
python3 filemanager-ownership/run.py --repo /path/to/GMAT --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /path/to/evidence/filemanager-ownership-baseline
python3 filemanager-ownership/run.py --repo /path/to/GMAT --revision <full-fixed-commit-sha> --output /path/to/evidence/filemanager-ownership-fixed
```

Adjust the path to `run.py` to its location. Baseline c9b78bd1c8499845cb78fde315e1f761b6b06f1c is expected to exit **1**
with behavioral contract failures; a candidate fixing this defect should exit **0**.
Exit **2** means extraction, compiler or infrastructure failure and must not be
reported as a reproduced defect. Each invocation prints its result location.

`RESULTS.json` records source Git blob/SHA-256 identities, exact extracted
method/constructor hashes and line locations, compiler/version, commands, generated
source/binary hashes, checks, exit status and limitations. The output directory
also contains exact full source snapshots, extracted definitions, generated probe,
compiler output and probe logs. The working checkout/index is not changed.

## Contract and limits

- Exact AddFileType/RefreshFiles definitions are compiled; the complete FileManager translation unit and actual class layout are not linked.
- FileInfo is an explicit instrumented shim preserving its two string fields while tracking object construction/destruction; counters test release ownership, not allocator-specific memory consumption.
- Path-list callbacks are no-ops; DoesDirectoryExist returns false for deterministic default output path selection. Debug macros and __FM_ADD_DEFAULT_INPUT__ remain undefined.
- Tests cover three replacement branches, initial release and repeated refresh; no file I/O, startup parsing, exception-allocation failure, threading or complete engine lifecycle is validated.

Recorded execution outcomes and exact tested identities: [TEST_RESULTS.md](TEST_RESULTS.md).
