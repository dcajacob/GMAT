# Focused native TLONGDot regression

This standalone runner reads `src/base/parameter/OrbitData.cpp` from a requested
Git commit, extracts **the exact `OrbitData::GetEquinDot` function**, and compiles
it in a small explicit dependency fixture. It needs Python 3.8+, Git and a C++17
compiler (`c++` by default); no GMAT build, third-party Python module, downloaded
data or prior task directory is required.

Run sequentially, outside the source checkout if convenient:

```sh
python3 run_tlongdot.py --repo /path/to/GMAT \
  --revision c9b78bd1c8499845cb78fde315e1f761b6b06f1c \
  --output /tmp/tlongdot-nasa-baseline

python3 run_tlongdot.py --repo /path/to/GMAT \
  --revision <full-fixed-commit-sha> \
  --output /tmp/tlongdot-fixed
```

Each output directory must be new or empty. A ref is accepted but resolved once
to an immutable commit before reading source. Optional `--cxx /path/to/clang++`
selects a compiler executable. The runner writes only its output directory;
Git commands use `--no-optional-locks` and perform no checkout/index mutation.
Compilation is one small, sequential translation unit.

Expected result: the vanilla baseline **exits 1** because zero computed
perturbation leaves TLONGDot at zero; the `h/r²` initialization fix **exits 0**.
Exit 2 indicates a setup/source-extraction/compiler/runtime problem rather than
a scientific contract failure. CLI usage errors also exit 2. Recorded execution
results and limits for this branch are in `TEST_RESULTS.md`.

The 12 cases cover circular motion; eccentric periapsis and apoapsis; inclined,
retrograde and longitude-wrap central motion; radial/tangential/normal
perturbations; normal acceleration near inclination 180°; and zero **total**
acceleration, which still yields nonzero computed perturbation after central
gravity removal. The fixture records recovered perturbation norm and asserts
that the intended zero/nonzero branch precondition was actually reached.

Central rates are checked against the independent two-body expression
`sqrt(mu/p³) * (1 + e*cos(nu))²` (converted to deg/s), with the other five
unperturbed rates required to remain zero. All longitude rates are also checked
using independent Cartesian-node `RAAN + argument-of-latitude` finite
differences at ±0.1, ±0.01 and ±0.001 seconds, with angle wrapping. Absolute
tolerances are respectively `2e-9`, `1e-10` and `1e-10` deg/s. Central analytic
tolerance is `2e-13` deg/s plus relative `2e-12`.

All six components of the six nonzero-perturbation vectors are compared against
recorded NASA outputs. These are historical GCC 15.2.0 exact-source executions
of NASA `9363e129be366520c6edb0b4079204ed60666007`, function SHA-256
`8b6411d97af0c11e5ae13c293bd529ea855780e45bfe1c8fedcd4d6d55a386ce`.
Static review verified the OrbitData source is unchanged at vanilla baseline
`c9b78bd1c8499845cb78fde315e1f761b6b06f1c`. They protect an unchanged branch and
are **not newly generated independent physical truth**. Component tolerances,
including relative `2e-11`, are recorded in each JSON check.

Outputs include `results.json` (immutable revision, file Git blob, source and
function hashes, compiler identity, exact commands, all inputs, outputs, checks
and limitations), `run.log`, exact extracted function, complete generated C++
fixture, binary and input stream. A failing numerical contract remains fully
recorded. Original copyright/license prologue is preserved in generated source.

Limits: vectors, ODEModel, reference-list/exception handling and element-state
conversion are explicit stubs. The independent finite difference uses a local
Taylor expansion, not full orbit integration. This does not qualify linked
engine setup, frame transformations, force models, Python bindings, propagation,
or all equinoctial components against independent truth. It does not test
inclination exactly 180°, radial degeneracy, unbound orbits, extreme state
scales or missing-ODE handling.
