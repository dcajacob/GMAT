#!/usr/bin/env python3
"""Integration runner. Each test_*.py is independently runnable for its PR."""
import argparse
from pathlib import Path
import subprocess
import sys
from support import Context, passed

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build_dir', type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    for suite in ('header', 'layout', 'geometry', 'groundtrack', 'exit', 'plugins'):
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
