# SPICE diagnostic initialization regression

This standalone test extracts the exact
`SpiceInterface::InitializeInterface()` definition from a supplied Git revision,
then compiles it against a supplied existing CSPICE_N0067 static archive. It does
not modify the repository or dependency tree, install dependencies, or use the
network. It creates all generated source, binaries, fixtures and evidence under a
fresh output directory. It never extracts or substitutes an unload/recovery
method.

The generated harness visibly defines minimal FileManager, MessageInterface,
Integer and UtilityException shims. The actual method bytes remain unchanged;
the script verifies that its emitted definition matches the extracted definition
and records both hashes. Original leading GMAT copyright/license notices are
retained in the generated harness. This is a focused source-body regression,
not a full linked GMAT, Python, mission-lifecycle or scientific test.

Requirements: Python 3, Git, a C++11 compiler, and an already available compatible
Linux CSPICE_N0067 tree containing `include/SpiceUsr.h` and `lib/cspice.a`.
No personal or repository path is hardcoded. The compiler defaults to `c++`; use
`--cxx PATH` to select the intended compiler.

Run sequentially against the full immutable baseline and candidate SHA values
selected for comparison:

```bash
python3 check_spice_diagnostics.py \
  --repo /path/to/vanilla-checkout \
  --revision BASELINE_FULL_SHA \
  --output /path/to/new-baseline-evidence \
  --cspice-dir /path/to/existing-cspice

python3 check_spice_diagnostics.py \
  --repo /path/to/vanilla-checkout \
  --revision CANDIDATE_FULL_SHA \
  --output /path/to/new-candidate-evidence \
  --cspice-dir /path/to/existing-cspice
```

Both output directories must not already exist. The test runs one small compiler
process and then three short isolated subprocesses for each revision:

1. An existing diagnostic log must retain its exact bytes. A fresh `.1.txt`
   sibling must be selected and contain the original invented diagnostic.
2. A nonexistent output directory must select console fallback and show the
   original diagnostic without `FILEOPENFAILED` noise.
3. An existing destination whose complete diagnostic filename exceeds SPICE's
   filename bound must select the same console fallback behavior.

Every case must return after the synthetic SPICE error, retain its original
`SPICE(REVIEWERROR)` identity, and permit a subsequent reset, built-in EARTH name
lookup, and vector operation. These checks define the narrow “usable after error”
claim; they do not claim that a complete GMAT mission has been exercised.

Exit statuses:

- **0**: all diagnostic contracts passed (expected fixed candidate).
- **1**: at least one contract failed (expected unmodified NASA baseline).
- **2**: infrastructure/extraction/compilation failed; not an expected baseline
  regression demonstration.

`results.json` records the resolved source SHA, method/source/harness SHA-256
identities, compiler version, exact argv/cwd commands, CSPICE archive hash and
header inventory, individual contract checks, process outcomes and limitations.
Separate compile/stdout/stderr files and the emitted exact method support review.
Use the same dependency tree and compiler for both revisions. Recorded execution
results and limits for this branch are in `TEST_RESULTS.md`.
