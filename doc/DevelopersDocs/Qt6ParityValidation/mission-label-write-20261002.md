# Mission command names and Write — 2026-10-02

Command panels now expose an optional Name, synchronized with their pending
full-source field. A name-only Replace patches the command prefix and preserves
branch bodies, comments, indentation, line endings and an absent final newline.
Invalid quotes/control characters keep Apply disabled and the error visible.
Structural commands and BeginMissionSequence are guarded. SaveMission retains
its full-source boundary because its first quoted token is a filename.

Write appears in insertion only when the runtime provides it. Its simple form
uses an ordered parameter picker. Full Write option dictionaries remain editable
through the source field with exact spelling/order preserved.

## Named assignment serialization defect

The first focused check found a genuine core serialization defect: with
WRITE_GMAT_KEYWORD=OFF, GmatCommand::InsertCommandName searched letters from
GMAT in the bare assignment operand. This omitted or misplaced its accepted
label and prevented reliable mission/source alignment. The narrow 11-line fix
prefixes the name at the first non-whitespace position only for named GMAT
assignments with the keyword off. Numerical execution and other command paths
are unchanged.

QtGui.MissionLabelsWrite passes **0.89 seconds** after rebuilding the actual core,
application and dependent links. It directly checks keyword OFF/ON/OFF, repeated
serialization without duplicate labels, original operands/names and restoration
of global flags. Actual command-panel checks cover pending invalid Name,
correction, Cancel/Discard, nested If/For/assignment/Report names, exact source
Undo/Redo and Unicode Save/reopen, ordered Write selection, validation errors
and full option source fallback. A real short mission reports **9 and 2.5**.

The first 0.17-second failure and the 0.42-second canonical dump preserve the
missing assignment label; they are not passing evidence. Raw first compile,
check/diagnostic and final build/check logs are retained in
`build/example-qualification/20261002/focused-checks`. No successful old command
or example matrix was repeated. A read-only peer review found no additional
concrete defects in this focused diff. Native command-form construction and all
independent Help tutorial walkthroughs remain unqualified by this offscreen check.
