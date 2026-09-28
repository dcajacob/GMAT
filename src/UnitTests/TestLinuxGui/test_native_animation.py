#!/usr/bin/env python3
"""Replay recorded native orbit data without changing its ring-buffer state."""
from support import context, passed

if __name__ == '__main__':
    ctx = context('native-animation')
    ctx.runtime()
    binary = ctx.build_gui('NativeAnimationRegression.cpp')
    original = (ctx.source/'GuiSmoke.script').read_text().replace('ElapsedSecs = 60', 'ElapsedSecs = 1500')
    for name, capacity, trail in [('wrapped',31,0), ('wrapped-trail',31,5),
                                  ('unwrapped',256,0), ('unwrapped-trail',256,5),
                                  ('full',151,0), ('single',1,0), ('wide-trail',31,100)]:
        settings = f"""BuildTestView.MaxPlotPoints = {capacity};
BuildTestView.NumPointsToRedraw = {trail};
BuildTestProp.InitialStepSize = 10;
BuildTestProp.MaxStep = 10;
BeginMissionSequence;"""
        (ctx.work/'animation.script').write_text(original.replace('BeginMissionSequence;', settings))
        ctx.gui(binary,name)
        passed('native animation: '+name)
