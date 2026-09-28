#!/usr/bin/env python3
from support import BASE, context, passed

if __name__ == '__main__':
    ctx = context('layout')
    binary = ctx.build_gui('LayoutRegression.cpp')
    ctx.runtime()
    for name, settings, screen, extra in [
        ('layout', '', '1280x900x24', {}),
        ('oversized-console', '[ConsoleWindow]\nSetConsoleHeightToPrevious=true\nPreviousConsoleHeight=10000\n', '1280x900x24', {}),
        ('invalid-console', '[ConsoleWindow]\nDefaultConsoleHeight=-500\n', '1280x900x24', {}),
        ('small', '', '800x600x24', {}),
        ('hidpi', '', '2560x1800x24', {'GDK_SCALE': '2'}),
    ]:
        ctx.config.write_text(BASE+settings)
        ctx.gui(binary, name, mode='layout' if name == 'layout' else 'bounds',
                screen=screen, environment={**ctx.environment, **extra})
        passed('layout: '+name)
