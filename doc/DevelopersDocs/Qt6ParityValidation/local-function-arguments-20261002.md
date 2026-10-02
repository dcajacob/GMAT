# Local Spacecraft and String function arguments — 2026-10-02

One independently authored actual-input case passed the existing ordered GMAT
function argument controls. A whole Spacecraft and a String were offered and
selected in the Inputs picker, a Variable and a String were selected in Outputs,
and one saved/reopened mission returned the independently specified mass and
exact text. No production change, build, old matrix or prior successful mission
was repeated for this case. This is bounded function-operation evidence; the
full Linux replacement gates remain separate.

## Authorship and known result

The owned session started with `script:null` and Welcome **New mission**. A small
own scaffold was typed through the actual Script editor: `ProbeSC.DryMass = 123.5`,
`LabelIn = 'local spacecraft stamp'`, outputs `ReturnedMass` and `Echoed`, and a
single ReportFile. This deliberately avoided repeating ordinary resource/default
propagator creation. No disk mission/function seed, sample, model API or existing
function was used to construct the case.

Functions category **Add GmatFunction → New file…** opened the normal file
chooser and function editor. The actual editor received this six-line definition:

```text
function [massResult, textResult] = ReadObjectInputs(inputSat, inputLabel)
Create Variable massResult;
Create String textResult;
BeginMissionSequence;
massResult = inputSat.DryMass;
textResult = inputLabel;
```

The function resource, filename and definition name are `ReadObjectInputs`.
Formal inputs deliberately differ from caller names and are not re-created
inside the function. [Function Help](../../help/src/Resource_GmatFunction.xml)
documents that re-creating a formal input ignores its incoming settings.
There is no Global, mutation, propagation, iteration or numerical algorithm.

The Mission command form inserted the configured function before Report. The
actual ordered Inputs picker offered and selected **ProbeSC, LabelIn**; Outputs
selected **ReturnedMass, Echoed**. The call was applied once, producing:

```text
[ReturnedMass, Echoed] = ReadObjectInputs(ProbeSC, LabelIn);
Report Values ReturnedMass Echoed ProbeSC.DryMass LabelIn;
```

The mission was saved, opened through Ctrl+O and built before the sole F5.
Completion was **0.021 s**. One 105-byte report row contains, in order:

| ReturnedMass | Echoed | caller ProbeSC.DryMass | caller LabelIn |
| --- | --- | --- | --- |
| 123.5 | local spacecraft stamp | 123.5 | local spacecraft stamp |

The expected mass is explicitly declared and exactly representable. The expected
String retains its interior spaces; the comparison removes only report column
padding. The returned values and these two caller values match independently
known inputs. The actual Output tree/report viewer also displayed the result.
Mission and function hashes remained unchanged from before reopen to after run.

## Retained evidence and identities

The full original owned tree is archived at
`build/example-qualification/20261002/local-function-arguments-live/isolated-x11-d7ypcww8`.
Its index covers **45 files / 5,000,340 bytes**, including all 30 screenshots,
180 successful recorded input actions, logs, startup original/clone, authored
files, report, freeze and cleanup/runtime metadata. The index itself is an
additional **8,229-byte** file, excluded from its own digest list. Exactly one
recorded F5 was issued. Original capture names 003/004 and 009 precede successful
save/template opening; their names do not establish completion.

Curated unchanged artifacts are:

- [Authored mission](local-function-arguments-authored-20261002.script),
  **604 bytes**, SHA-256 `b04ccac44a45cb781683a0a0777e5d13ec33e36d5bc19177461b6ab86ee508a8`.
- [Authored function](local-function-arguments-function-20261002.gmf),
  **207 bytes**, SHA-256 `75ae1a26266ac997ba8f486c3bfb179b83f831c86ec689f18f24b5b4a2d80c75`.
- [Exact report](local-function-arguments-report-20261002.txt),
  **105 bytes**, SHA-256 `4dbabf79a9b5a58b7db03f0c315297d5a5bcf09523d5bdb0d14f249769d3aa6b`.
- [Six-line function editor](local-function-arguments-function-editor-20261002.png),
  [offered Spacecraft/String inputs](local-function-arguments-inputs-offered-20261002.png),
  [selected input order](local-function-arguments-input-order-20261002.png),
  [selected output order](local-function-arguments-output-order-20261002.png),
  [call before Apply](local-function-arguments-call-20261002.png),
  [reopened/built mission](local-function-arguments-reopened-20261002.png), and
  [actual Output readback](local-function-arguments-output-readback-20261002.png).

Freeze was **2026-10-02T22:56:19.231551+00:00**. The launch-captured application is
`GmatQt-R2026a`, **5,926,536 bytes**, SHA-256
`244dc41a2402dbf7b7235027c02b840f0319b77bafd4798bf23617a9645c11f0`.
Production startup SHA is
`5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`;
the clone changes only OUTPUT_PATH. After normal closure, while production
remained frozen, core/util hashes were recorded as
`cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` /
`1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`.
The helper SHA is
`4232bdb0a050ab22b5f0513a15f92339501edc88719339e2dac1c476a8d3358f`.
Parent-reported compiled source is `2f9810c0`; source/document context after
closure is `3fbd30895057d53f2275690ecbd452cc3cd9f517`. The post-close library
observation is retained with its timing limit, not claimed as a separate mapped
library audit.

Initial Save/Enter actions did not commit while a chooser remained open; the
filename was re-focused and saved through the same GUI. Those original actions
and screenshots are retained. There was no failed mission or source workaround.
The application was closed through its own title close at **596.961 s**, before
the owned 600-second deadline. Application/WM/Xvfb exits are **0/0/0**, and all
three owned PIDs are absent. The helper CLI exit **1** is its next-wait
already-exited-application diagnostic, not an application failure or timeout.

## Scope and reuse

Existing retained Compatibility evidence already covers whole-Array positional
inputs/output and the known cross product `[-0.25, 2.75, 2]`; GlobalScopes covers
shared Variable/Array/String/Spacecraft objects with scalar formal inputs.
Help Tutorial 6 covers an independently authored zero-input call. Those passing
cases were reused, not rerun. The new case adds distinct local whole-Spacecraft
and String formal inputs with numeric/String outputs and actual picker offering.

This qualifies this one local positional call, ordered typed selection, source
save/reopen, execution and report access. Other resource types, whole-object
outputs, mutation/copy-out, String expressions, dynamic/nested signatures and
numerical function algorithms remain outside this case. The private X11/software
GL session does not qualify the physical desktop/GPU or prior desktop-crash
reproduction. Original absolute `/tmp` paths remain unchanged in archived source;
replay from another directory requires selecting an available function file and
output path. No archived-copy rerun is claimed. Windows/macOS/MATLAB stay deferred.
