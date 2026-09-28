#!/usr/bin/env python3
"""GroundTrack integration, station-marker, theme and live-paint regressions."""
from support import context, passed, BASE

if __name__ == '__main__':
    ctx = context('gui-followup-more')
    ctx.runtime()
    for name in ('GroundTrackStation', 'GroundTrackEditor'):
        binary = ctx.build_gui(name+'Regression.cpp')
        ctx.config.write_text(BASE)
        ctx.gui(binary, name)
        passed(name)
    binary = ctx.build_gui('TreeThemeRegression.cpp')
    for theme in ('Adwaita', 'Adwaita:dark'):
        ctx.config.write_text(BASE)
        ctx.gui(binary, theme.replace(':','-'),
                environment={**ctx.environment, 'GTK_THEME':theme})
        passed('tree theme: '+theme)
    binary = ctx.build_gui('GroundTrackLiveRegression.cpp')
    text = (ctx.source/'fixtures/groundtrack/GroundTrackDataCollectFrequency.script').read_text()
    text = text.replace('MaxStep = 2700','MaxStep = 1').replace('InitialStepSize = 60','InitialStepSize = 1')
    text = text.replace('ElapsedSecs = 6000','ElapsedSecs = 120000')
    script = ctx.work/'live.script'
    script.write_text(text)
    ctx.config.write_text(BASE)
    ctx.gui(binary, 'live', environment={**ctx.environment,'GMAT_LIVE_SCRIPT':str(script)})
    passed('GroundTrack paints during long mission')
