#!/usr/bin/env python3
"""Integration runner. Supplemental focused suites remain on the integration branch."""
import argparse
from pathlib import Path
import subprocess
import sys
from support import Context, passed
from audit_environment import record_environment

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    args = parser.parse_args()
    record_environment(args.build_dir)
    source = Path(__file__).resolve().parent
    for suite in ('header', 'layout', 'geometry', 'groundtrack', 'viewport', 'editor_io', 'workflow', 'exit', 'plugins'):
        subprocess.run([sys.executable, str(source/('test_'+suite+'.py')), str(args.build_dir.resolve())], check=True)
    ctx = Context(args.build_dir, 'integration')
    ctx.runtime()
    if ctx.of_plugins:
        binary = ctx.build_gui('GroundTrackRegression.cpp')
        output = ctx.gui(binary, 'groundtrack-openframes', mode='groundtrack-default',
                         startup=ctx.of_startup, environment=ctx.of_environment)
        if 'Drawing the ground track without a map.' in output:
            raise RuntimeError('OpenFrames default ground track has no map')
        passed('integration: OpenFrames default ground-track rendering')
        output = ctx.gui(ctx.application/'bin/GMAT', 'hohmann', startup=ctx.of_startup,
                         environment=ctx.of_environment, arguments=['--run',
                         str(ctx.root/'application/samples/Ex_HohmannTransfer.script'), '--exit'])
        if 'Mission run completed.' not in output or 'The Targeter converged!' not in output:
            raise RuntimeError('Hohmann mission did not complete and converge')
        passed('integration: OpenFrames Hohmann convergence and automatic exit')
    else:
        print('SKIP: OpenFrames integration checks require optional plugins', flush=True)
    print('Logs: '+str(ctx.build/'linux-gui-tests'), flush=True)
