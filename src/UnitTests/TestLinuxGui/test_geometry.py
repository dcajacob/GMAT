#!/usr/bin/env python3
from support import BASE, context, passed

if __name__ == '__main__':
    ctx = context('geometry')
    binary = ctx.build_gui('GeometryRegression.cpp')
    ctx.runtime()
    for name, settings, screen, extra in [
        ('save', '', '1280x900x24', {}),
        ('restore', None, '1280x900x24', {}),
        ('oversized', '[MainFrame/Linux]\nx=20000\ny=20000\nw=2147483647\nh=2147483647\nIconized=1\n', '1280x900x24', {}),
        ('invalid', '[MainFrame/Linux]\nx=-20000\ny=-20000\nw=-400\nh=0\n', '1280x900x24', {}),
        ('small', '', '800x600x24', {}),
        ('hidpi', '', '2560x1800x24', {'GDK_SCALE': '2'}),
    ]:
        if settings is not None:
            ctx.config.write_text(BASE+settings)
        ctx.gui(binary, name, mode=name, screen=screen, environment={**ctx.environment, **extra})
        passed('geometry: '+name)
    ctx.config.write_text(BASE)
    before = ctx.config.read_bytes()
    no_write = ctx.work/'startup-no-write.txt'
    no_write.write_text(ctx.startup.read_text().replace('WRITE_PERSONALIZATION_FILE = ON', 'WRITE_PERSONALIZATION_FILE = OFF'))
    ctx.gui(binary, 'no-write', mode='save', startup=no_write)
    if before != ctx.config.read_bytes():
        raise RuntimeError('Personalization changed with WRITE_PERSONALIZATION_FILE = OFF')
    passed('geometry: personalization write opt-out')
