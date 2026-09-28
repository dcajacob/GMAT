#!/usr/bin/env python3
import subprocess
from support import context, passed, run

if __name__ == '__main__':
    ctx = context('exit')
    ctx.runtime()
    mission = ctx.source/'GuiSmoke.script'
    gui, console = ctx.application/'bin/GMAT', ctx.application/'bin/GmatConsole'
    output = run([str(console), '--startup_file', str(ctx.startup), '--run', str(mission)],
                 cwd=ctx.work, env=ctx.environment, log=ctx.work/'console.log')
    if 'Mission run completed.' not in output:
        raise RuntimeError('Console mission did not complete')
    reference = (ctx.work/'BuildGuiSmokeTest.txt').read_text()
    (ctx.work/'console-state.txt').write_text(reference)
    passed('exit: console mission')
    output = ctx.gui(gui, 'gui', arguments=['--run', str(mission), '--exit'])
    if 'Mission run completed.' not in output:
        raise RuntimeError('GUI mission did not complete')
    actual = (ctx.work/'BuildGuiSmokeTest.txt').read_text()
    (ctx.work/'gui-state.txt').write_text(actual)
    if actual != reference:
        raise RuntimeError('Console and native OrbitView reports differ')
    passed('exit: GUI mission and identical console state report')
    invalid = ctx.work/'invalid.script'
    invalid.write_text('Create NotAValidGmatObject Invalid;\nBeginMissionSequence;\n')
    failure = subprocess.run(['xvfb-run', '-a', str(gui), '--startup_file', str(ctx.startup), '--no_splash',
                              '--run', str(invalid), '--exit'], cwd=ctx.work, env=ctx.environment,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30)
    (ctx.work/'invalid-script.log').write_text(failure.stdout)
    if failure.returncode != 1 or 'Failed to build the script' not in failure.stdout:
        raise RuntimeError('Invalid script must produce exit status 1')
    passed('exit: invalid script returns status 1')
