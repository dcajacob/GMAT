#!/usr/bin/env python3
from support import context, passed


def prepare_workflow(ctx):
    script = (ctx.source/'GuiSmoke.script').read_text().replace('BeginMissionSequence;', """Create XYPlot AuditXY;
AuditXY.XVariable = BuildTestSat.ElapsedSecs;
AuditXY.YVariables = {BuildTestSat.Earth.RMAG};
Create GroundTrackPlot AuditGround;
AuditGround.Add = {BuildTestSat};
BeginMissionSequence;""")
    (ctx.work/'workflow script.script').write_text(script)
    (ctx.work/'long.script').write_text(script.replace('ElapsedSecs = 60', 'ElapsedSecs = 1000000000'))


if __name__ == '__main__':
    ctx = context('workflow')
    ctx.runtime()
    prepare_workflow(ctx)
    binary = ctx.build_gui('WorkflowAudit.cpp')
    for name, scale, theme in [('native', 1, 'Adwaita'), ('native-dark-2x', 2, 'Adwaita:dark')]:
        ctx.gui(binary, name, screen=f'{1280*scale}x{900*scale}x24',
                environment={**ctx.environment, 'GDK_SCALE': str(scale), 'GTK_THEME': theme})
        for png in ('workflow-small', 'workflow-editor'):
            (ctx.work/(png+'.png')).replace(ctx.work/(png+'-'+name+'.png'))
        passed('workflow: '+name)
    if ctx.of_plugins:
        ctx.gui(binary, 'openframes', startup=ctx.of_startup, environment=ctx.of_environment)
        passed('workflow: openframes')
    else:
        print('SKIP: OpenFrames workflow requires optional plugins', flush=True)
