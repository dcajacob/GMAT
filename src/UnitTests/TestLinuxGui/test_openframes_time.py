#!/usr/bin/env python3
"""Focused OpenFrames time-control regression; requires the optional wx plugin.

This reproduces a known external-plugin failure until its time-control patch is
applied. It is deliberately separate from the currently passing core GUI suites.
"""
from pathlib import Path
from support import context, passed

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
    ctx.gui(binary,'time',environment={**ctx.of_environment,'G_DEBUG':'fatal-criticals'})
    passed('OpenFrames time-control construction and resizing')
