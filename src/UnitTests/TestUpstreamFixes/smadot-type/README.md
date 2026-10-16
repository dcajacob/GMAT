# smadot-public-type-identity

This standalone regression runner reads exact native definitions through `git show`
at the requested immutable commit and compiles a small C++ probe. It uses Python's
standard library, Git, and an already installed C++17 compiler (`CXX`, default
`c++`). It does not build GMAT or require Python packages, downloaded data or a
network connection.

Run from any directory, using a **new** output directory for every revision:

```bash
python3 smadot-type/run.py --repo /path/to/GMAT --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c --output /path/to/evidence/smadot-type-baseline
python3 smadot-type/run.py --repo /path/to/GMAT --revision <full-fixed-commit-sha> --output /path/to/evidence/smadot-type-fixed
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

- The exact native SMADot constructor including its initializer list is compiled and executed; this is not a text-search assertion.
- OrbitReal/GmatBase are explicit dependency shims: the reported type is the string received by the base constructor, while name/origin/reference behavior is captured. Full factory, parameter hierarchy and SWIG are not linked.
- Expected public identifier SMADot matches ParameterFactory creation/registration in the NASA source. Parameter header includes are commented out in swig/gmat.swg; no dedicated Python SMADot class binding is claimed.
- No orbital rate evaluation or scientific formula, item identifier, ownership, ABI/layout or complete engine operation is validated. USE_PREDEFINED_COLORS remains undefined.

Recorded execution outcomes and exact tested identities: [TEST_RESULTS.md](TEST_RESULTS.md).
