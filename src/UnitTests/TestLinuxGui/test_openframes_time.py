#!/usr/bin/env python3
"""Focused OpenFrames time-control regression; requires the optional wx plugin.

Requires the external time-control patch documented in
doc/DevelopersDocs/LinuxGuiExternal/OpenFramesTime/README.md.
Kept separate because the OpenFrames wx plugin is optional.
"""
from pathlib import Path
from support import context, passed
from test_workflow import prepare_workflow

if __name__ == '__main__':
    ctx = context('openframes-time')
    ctx.runtime()
    source = Path(ctx.cache['GMAT_ADDITIONAL_PLUGINS_OpenFramesInterface'])/'src'
    plugin = ctx.application/'plugins/libOpenFramesInterface.so'
    if not plugin.exists():
        raise SystemExit('OpenFramesInterface wx plugin is required for this regression')
    binary = ctx.build_gui('OpenFramesTimeRegression.cpp',
        extra_flags=['-I'+str(source/'gui/subscriber/wx'),'-I'+str(source/'base/include')],
        extra_link=[str(plugin),'-Wl,-rpath,'+str(plugin.parent)])
    prepare_workflow(ctx)
    workflow = ctx.build_gui('WorkflowAudit.cpp')
    for scale in (1, 2, 3):
        env = {**ctx.of_environment, 'G_DEBUG': 'fatal-criticals', 'GDK_SCALE': str(scale)}
        screen = f'{1280*scale}x{900*scale}x24'
        ctx.gui(binary, f'time-{scale}x', screen=screen, environment=env)
        ctx.gui(workflow, f'workflow-{scale}x', screen=screen,
                startup=ctx.of_startup, environment=env)
        for name in ('workflow-small', 'workflow-editor'):
            (ctx.work/(name+'.png')).replace(ctx.work/(name+f'-{scale}x.png'))
        passed(f'OpenFrames time control and full workflow {scale}x')
