# Ephemeris output-path regression probe

This standard-library Python driver extracts and compiles **actual revision-selected
GMAT C++ source**, then exercises it against disposable directories and files.
It has no Python-package, GMAT installation or full-engine build dependency.

```sh
python3 run.py --repo /path/to/GMAT --revision COMMIT_SHA --output results
```

`--output` must be a new directory. Set `CXX` if `c++` is unavailable. The runner
uses one compiler process, a 60-second compile timeout and a 20-second native-run
timeout. It does not alter the supplied repository. All fixture filesystem
operations happen inside a fresh `tempfile.TemporaryDirectory` that is removed
after the run. The test is intended for POSIX hosts; the Linux behavior of an
`ifstream` recognizing an empty directory is observed through GMAT's actual
`DoesFileExist`, not hardcoded by a test substitute.

Exit codes:

- **0:** all regression assertions pass.
- **1:** at least one behavior assertion fails (expected for the unpatched NASA baseline).
- **2:** extraction, compilation or fixture execution could not complete.

The baseline comparison should select NASA commit
`c9b78bd1c8499845cb78fde315e1f761b6b06f1c`; the candidate comparison must select the
exact reviewed commit containing the focused Ephemeris output-path patch. The
caller records that candidate commit ID. Do not treat a baseline failure as an
infrastructure failure: inspect `source-receipt.json` and `native.stdout` to verify
the expected old behavior. No baseline/candidate runs were performed merely by
authoring this test.

## What executes

The driver uses `git show COMMIT:path`; working-tree changes cannot enter its test
source. It extracts these exact contiguous regions without substitutions,
filtering or removal of the old code:

1. In `EphemerisFile::Initialize`, from the existing “Delete old file” comment
   through the complete `DoesFileExist` / `remove` block. This includes a candidate
   directory-before-delete guard when present, and always retains NASA's original
   `remove(fullPathFileName.c_str())` statement.
2. In the `Filename` branch of `EphemerisFile::SetStringParameter`, from
   `prevFileName = fileName` through its final `return true`. This includes the
   original `GmatBase::GetFullPathFileName` call, candidate empty-result rejection
   when present, parent-directory validation and `SetFileName` update.
3. The complete `std::string` overloads of `GmatFileUtil::ParsePathName`,
   `DoesDirectoryExist` and `DoesFileExist` from the **same revision**. Their actual
   `std::ifstream`, `opendir`, `closedir` and path-parsing logic executes.

Each source-file hash, slice hash, source line range, generated-harness hash,
compiler identity, command, timeout and native result is recorded in
`source-receipt.json`. Extracted `.inc` files and the generated `native-probe.cpp`
make the exact tested logic inspectable. Debug macros remain undefined, as in an
ordinary release build.

## Assertions and dependency seams

The native harness checks that initialization rejects and preserves both empty
and nonempty output directories. On the baseline Linux path, the empty-directory
test records the actual deletion caused by `DoesFileExist` followed by `remove`.
It also verifies that ordinary old output files are still removed, nonexistent
valid output files remain usable, and subsequent standard C++ writes succeed.

The filename resolver is an explicitly injected seam: one test supplies an empty
resolution result and verifies rejection before the writer-name update. Another
supplies a missing-parent path. Both reuse the same object for a later valid
resolved filename and successful file write. Existing-parent and current-directory
filenames are also accepted. Tests do not assert transactional rollback of all
fields after an exception, because the focused patch does not provide that.

The wrapper supplies only the fields/methods needed by the extracted regions.
`SetFileName` increments a counter, the resolver returns its injected value,
`GetName` returns a fixed fixture name, and `SubscriberException` carries messages.
These seams do not copy the production branch decisions being tested.

This is a **bounded native behavior test**, not full `EphemerisFile` integration:
earlier initialization validation, the already-initialized short circuit,
filename-character validation, initialized-writer shutdown, actual FileManager
resolution and actual ephemeris serialization remain outside its scope. The
ordinary writes show that preserved valid path handling still permits filesystem
output; they do not validate a generated scientific ephemeris file.

Recorded execution outcomes and exact tested identities: [TEST_RESULTS.md](TEST_RESULTS.md).
