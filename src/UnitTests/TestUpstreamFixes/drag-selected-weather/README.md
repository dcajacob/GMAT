# DragForce selected-weather contract regression

This small C++17 test compiles source extracted from an immutable Git revision.
It needs Python 3.9 or later (standard library only), Git and a C++ compiler; it does not build or
load GMAT, install prerequisites, change a checkout or use real weather data.

The same contracts are tested against NASA baseline
`c9b78bd1c8499845cb78fde315e1f761b6b06f1c` and candidate 06. The expected result
is **baseline exit 1** (contract failures) and **candidate exit 0** (all contracts
pass). An extraction, compilation or harness error exits **2** and must not be
presented as the reproduced baseline failure. The runner accepts a branch/ref
but immediately resolves and records its full commit SHA.

```bash
python3 run.py --repo /path/to/GMAT \
  --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c \
  --output /path/to/evidence/06-baseline

python3 run.py --repo /path/to/GMAT \
  --revision <candidate-06-commit-sha> \
  --output /path/to/evidence/06-candidate
```

Use `--cxx /path/to/compiler` to select a single compiler executable. Use
`--prepare-only` to inspect exact extracted fragments and the generated C++
without invoking a compiler. Execute separate revisions serially; the runner
itself invokes one compiler process with no parallel build.
The output directory must be new or empty; nonempty evidence directories are
rejected so receipts and compiler/runtime artifacts cannot be overwritten.

## Production code exercised

The runner reads these Git blobs rather than the working-tree files:

* `src/base/forcemodel/DragForce.cpp`: the exact numeric-parameter/weather-file
  region inside `DragForce::Initialize`, from the unique
  `F107ID = atmos->GetParameterID("F107");` statement to immediately before the
  unique `if (cbFixed != NULL)` statement. This includes both historic/predicted
  selected-weather predicates and the existing path-resolution/exception calls.
* The **entire** `DragForce::SetInternalAtmosphereModel` function, including its
  null and replacement/attachment behavior and its numeric-parameter setters.
  Thus baseline's eager CSSI/Schatten filename setters and the candidate's
  deferral are both the actual selected revision's code.
* `src/base/solarsys/AtmosphereModel.cpp`: the **entire actual**
  `AtmosphereModel::SetStringParameter(Integer, const std::string&)` function.
  It performs the real file-existence and native format-tag checks; the test
  does not replace its validation with an invented approximation.

Function signatures must be unique and closing braces are matched while
excluding comments and quoted literals. Initialization-region anchors must be
unique within that complete function, ordered, contain exactly the two native
filename setters and one FileManager declaration, and exclude density/full
atmosphere initialization. Full-function end/branch gates are checked for both
baseline and candidate. Changed or ambiguous boundaries exit 2. No native
source character inside the extracted regions is rewritten. Exact regions and
source-line spans/hashes are saved alongside `harness.cpp`; `#line` directives
identify the original path/revision for compiler diagnostics.

## Contracts

The harness tests these caller-visible configuration requirements:

* Constant flux succeeds when both optional files are missing or present,
  without even probing or assigning either unselected file.
* A valid CSSI file works for historic-only, predicted-only and both CSSI
  selections with an absent unselected Schatten file.
* A valid predicted Schatten file works with an absent unselected CSSI file.
* Relative selected paths resolve through the supplied flux directory.
* Selected CSSI and Schatten inputs are each rejected when missing, including
  either missing member of the mixed CSSI+Schatten selection.
* Existing but malformed selected input is rejected through the extracted
  native atmosphere filename setter.
* Null attachment returns false. Attaching the default constant atmosphere
  before source/path configuration performs no optional-file access; a later
  explicit CSSI selection and path succeeds.
* **A legacy-defined success uses exactly the same valid inputs on both
  revisions:** attach with valid CSSI and Schatten files, select historic CSSI
  plus predicted Schatten, then initialize. Both must retain the same effective
  filenames and F107/F107A/Kp values (150/140/3). This case is recorded separately
  as `same_inputs_legacy_defined_success`.

The test expectations are explicit fixture contracts. The harness does not
reimplement the candidate's weather-source predicate as its own selection
algorithm. All fixture numbers are invented and both fixtures are created in a
temporary directory, used for real file opens, then removed. No optional NASA,
CSSI, Schatten or Marini dataset is copied or required.

## Stubs and limits

`harness.cpp.in` names every environment stub. FileManager records real
filesystem existence checks and supplies only `ATMOSPHERE_PATH`. Text utilities
use ordinary line reads, whitespace detection and uppercase conversion.
Atmosphere parameter IDs, numeric storage and coordinate pointer attachment are
minimal explicit stand-ins; its file-validation method is extracted native
code. Debug logging/memory macros are not enabled. The whole `Initialize`
function's solar-system, body/coordinate setup, atmosphere ownership selection
and eventual `Initialize`/`SetInputSource` calls are outside this focused test.

**This is a configuration/validation regression, not a density or propagation
comparison, a full GMAT integration test, or evidence of differential physical
truth.** It does not parse numerical weather records or verify that an optional
provider series predicts reality. Existing installed integration tests can
cover a later real atmospheric step separately.

On POSIX, each child receives a 60-second CPU limit, 1 GiB address-space limit
and 64 MiB generated-file limit; subprocess wall timeout is 90 seconds. No build
parallelism is requested. The output receipt records the compiler executable,
version, full argument vector, limits, full source hashes, extracted-region
hashes, generated-source/executable hashes, synthetic fixture hashes and all
per-case outcomes. `compiler.*.txt`, `native.stdout.json`, `native.stderr.txt`
and `report.json` remain under the supplied output directory. Unexpected
protocol/exit disagreement is a harness error, not a test success.

No compile or runtime result is asserted by this README; use the exact emitted
receipts from the serial baseline/candidate runs as the evidence.

Recorded execution outcomes and exact tested identities: [TEST_RESULTS.md](TEST_RESULTS.md).
