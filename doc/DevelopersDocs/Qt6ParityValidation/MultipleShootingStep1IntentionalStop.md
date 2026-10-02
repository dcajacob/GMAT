# Multiple-shooting Step 1: intentional tutorial stop

Recorded 2026-10-02. **Disposition: documented intentional tutorial checkpoint.**
Both raw qualification runs remain `failed`, exit 1: 0.821 s for the sample
variant and 0.720 s for the installed Help variant. This disposition does not
rewrite either to a completed mission or successful optimization. No source byte, engine algorithm, runner result or mission value
was changed, and no test/mission was rerun for this investigation.

## Actual cause and authoritative intent

`application/samples/NeedVF13ad/Tut_MultipleShootingTutorial_Step1.script:419`
contains an explicit executable `Stop`, immediately before `EndOptimize`.
Its `Minimize NLPOpt(Cost)` command remains commented at line 413. The generated
solver text initializes 22 controls and 12 patch equality constraints; the log
then reports `GMAT execution stopped by user.` and `Mission run interrupted`
after 0.177 s of engine execution. stderr reports `Mission stopped`. That generic
engine message also covers this literal script Stop; there is no evidence of
an unexpected propagation or optimizer exception in this recorded execution.

`doc/help/src/Tut_OptimalLunarFlyby.xml:1093-1107` explicitly places Stop here
so the reader can QA the initial guess before optimization. The **Step 1: Verify
Your Configuration** section at lines 1149 onward expects display of the initial
guess. The following Step 2 at lines 1214-1224 instructs the reader to comment
out that Stop so the optimizer attempts a solution. Removing Stop or enabling
the objective in Step 1 would change the tutorial stage and duplicate later
examples rather than repair an error.

The installed Help and source Help copies preserve the same intentional Stop:

| Copy | Stop line | SHA-256 |
| --- | ---: | --- |
| application/samples/NeedVF13ad/Tut_MultipleShootingTutorial_Step1.script | 419 | 52583eea78d3a2c305c19e865270448fe2254d00eb56f6237456a343bd93b6f8 |
| application/docs/help/files/scripts/Tut_MultipleShootingTutorial_Step1.script | 374 | e4d75e3162e9a77a36a45a3a1fb038015ac6c4aea87eafa9292deb4d44c8cf95 |
| doc/help/src/files/scripts/Tut_MultipleShootingTutorial_Step1.script | 374 | e4d75e3162e9a77a36a45a3a1fb038015ac6c4aea87eafa9292deb4d44c8cf95 |

The sample and Help variants use different viewer source and legacy epoch
syntax, so they have different source hashes. Both now have independent actual
build/run evidence; their results are listed separately below. The source Help
copy is byte-identical to the executed installed Help copy. It was not separately
executed as a third script path.

## Qualification evidence and limits


| Executed variant | Build | Run | Evidence disposition |
| --- | --- | --- | --- |
| Sample | attempt-02: passed, exit 0, 0.980 s | attempt-01: raw failed, exit 1, 0.821 s | Requested tutorial Stop |
| Installed Help | attempt-01: passed, exit 0, 0.470 s | attempt-01: raw failed, exit 1, 0.720 s | Requested tutorial Stop |

Each run reports 22 VF13ad controls, 12 equality constraints and zero inequality
constraints, then reaches its literal Stop. The Help log records the same generic
interruption messages after **0.174 s** of engine execution (sample: 0.177 s).
These are two separately observed initial-guess checkpoints, not inferred parity
from the common tutorial chapter or from one variant's result.


Evidence folder: `/tmp/gmat-shipped-examples/scripts/application__samples__NeedVF13ad__Tut_MultipleShootingTutorial_Step1.script/run/attempt-01`. Retained files include `result.json`, stdout/stderr,
`output/GmatLog.txt`, `output/VF13adVF13ad1.data` and `result.png`. Log SHA-256:
`fb81847442e0c97e8f4b33a103dc2cf7e7379dc5b434fd33f7e2f59f71e68400`.
The sample is not a proprietary-dependency exception in this recorded attempt:
VF13ad is actually loaded and the sequence reaches its requested Stop. Earlier
attempts without VF13ad remain separate dependency failures in the raw ledger.

This demonstrates the intended initial-guess checkpoint executed to its Stop.
It does not establish optimizer convergence, feasibility of the initial guess,
mission completion, native viewer equivalence, or any Step 2-5 result. Preserve
the raw interrupted status alongside this explicit disposition in the ledger.


### Sample recorded artifacts

Evidence root: `/tmp/gmat-shipped-examples/scripts/application__samples__NeedVF13ad__Tut_MultipleShootingTutorial_Step1.script`. Build and run `result.json` preserve the
source/launcher/plugin/core provenance and raw status. Actual running Qt process
binary SHA-256: `45f4245e214b9f15c746fdd7ef2c381df9231c659e601a4536b4adfe259fb4c9`.

| Artifact under run/attempt-01 | Bytes | SHA-256 |
| --- | ---: | --- |
| stdout.txt | 0 | e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 |
| stderr.txt | 3636 | 8c82823131f23528fafbcdbdcf49be23e3cf71630a8859d34a39b45e2ddf573a |
| output/GmatLog.txt | 3220 | fb81847442e0c97e8f4b33a103dc2cf7e7379dc5b434fd33f7e2f59f71e68400 |
| output/VF13adVF13ad1.data | 724 | 580a4ef015698cc9c29c81f41f115ef57cdb60c2e30ebfcd46b01c0a92588612 |
| output/debugData.txt | 118 | 06e1ef8fa0c3454b71eedb9162f3b56a80c4873869de605a03663e616e76a04b |
| result.png | 197850 | 6ffe4a1122f44044f3f75298826d75adac041c4515f06394bc2cbc453cff1148 |


### Installed Help recorded artifacts

Evidence root: `/tmp/gmat-shipped-examples-vf13-tutorials/scripts/application__docs__help__files__scripts__Tut_MultipleShootingTutorial_Step1.script`. Build and run `result.json` preserve the
source/launcher/plugin/core provenance and raw status. Actual running Qt process
binary SHA-256: `bb12ba23a54f8ef10b6d203eb2143a68d4af3b633685f5670764c16042105708`.

| Artifact under run/attempt-01 | Bytes | SHA-256 |
| --- | ---: | --- |
| stdout.txt | 0 | e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 |
| stderr.txt | 3906 | 9befb1c8b10682eb6eff3e538726c94554f5d99a6104043dbac99f63324e4d5a |
| output/GmatLog.txt | 3966 | 746977eef80ba739d7b7103a7076ed670eec96240f48b69c1a1dc576808a5798 |
| output/VF13adVF13ad1.data | 724 | 580a4ef015698cc9c29c81f41f115ef57cdb60c2e30ebfcd46b01c0a92588612 |
| output/debugData.txt | 118 | 06e1ef8fa0c3454b71eedb9162f3b56a80c4873869de605a03663e616e76a04b |
| result.png | 197134 | 8f97c527a929d73806d7a174bfa601200d333d2fc227bd459794ad88ff65659d |


Both VF13ad text reports contain only the initialization header and control
list; neither claims an optimized solution. The retained debug-data text and
screenshots are outputs from the initial-guess sequence. A screenshot file's
existence does not independently qualify native rendering or numerical plot
accuracy. No Step 2-5 result is inferred or altered by this evidence update.
