#!/usr/bin/env python3
from support import context, passed
if __name__ == '__main__':
    ctx = context('groundtrack-close')
    ctx.runtime()
    binary = ctx.build_gui('GroundTrackCloseRegression.cpp')
    ctx.gui(binary, 'native')
    passed('direct GroundTrack close and recreation')
    if ctx.of_plugins:
        ctx.gui(binary, 'openframes', startup=ctx.of_startup, environment=ctx.of_environment)
        passed('direct GroundTrack close and recreation alongside OpenFrames')
