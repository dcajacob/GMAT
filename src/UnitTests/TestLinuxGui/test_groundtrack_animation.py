#!/usr/bin/env python3
"""Exercise GroundTrack toolbar playback and recorded-data/lifecycle invariants."""
from support import context, passed
if __name__ == '__main__':
    ctx = context('groundtrack-animation')
    ctx.runtime()
    script = (ctx.source/'GuiSmoke.script').read_text().replace('ElapsedSecs = 60','ElapsedSecs = 5000').replace('BeginMissionSequence;', '''Create GroundTrackPlot ReplayTrack;
ReplayTrack.Add = {BuildTestSat};
BuildTestProp.InitialStepSize = 10;
BuildTestProp.MaxStep = 10;
BeginMissionSequence;''')
    (ctx.work/'animation.script').write_text(script)
    binary = ctx.build_gui('GroundTrackAnimationRegression.cpp')
    for scale in (1,2,3):
        ctx.gui(binary, f'native-{scale}x', screen=f'{1280*scale}x{900*scale}x24',
                environment={**ctx.environment,'GDK_SCALE':str(scale)})
        passed(f'GroundTrack playback and lifecycle at {scale}x')
    if ctx.of_plugins:
        ctx.gui(binary, 'openframes', startup=ctx.of_startup, environment=ctx.of_environment)
        passed('GroundTrack playback alongside OpenFrames')
