#!/usr/bin/env python3
"""Opt-in: opens a self-closing GMAT window on the current desktop, using isolated settings."""
import os
from support import context, run, passed
from test_workflow import prepare_workflow

if __name__ == '__main__':
    ctx = context('desktop-workflow')
    ctx.runtime()
    prepare_workflow(ctx)
    binary = ctx.build_gui('WorkflowAudit.cpp')
    env = {**os.environ, 'G_DEBUG': 'fatal-criticals', 'GDK_BACKEND': 'x11',
           'GMAT_GUI_TEST_IMAGES': str(ctx.work), 'GMAT_GUI_TEST_NO_SCREENSHOT': '1'}
    for key in ('GDK_SCALE', 'GDK_DPI_SCALE', 'LIBGL_ALWAYS_SOFTWARE'):
        env.pop(key, None)
    run([str(binary), '--startup_file', str(ctx.startup), '--no_splash'],
        cwd=ctx.work, env=env, log=ctx.work/'desktop-workflow.log')
    passed('workflow: current desktop and hardware OpenGL')
