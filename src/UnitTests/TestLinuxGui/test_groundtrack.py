#!/usr/bin/env python3
import shutil
from support import BASE, context, passed

if __name__ == '__main__':
    ctx = context('groundtrack')
    binary = ctx.build_gui('GroundTrackRegression.cpp')
    ctx.runtime()
    custom_body = ctx.work/'body-textures'
    custom_body.mkdir(exist_ok=True)
    shutil.copyfile(ctx.root/'application/data/graphics/texture/ModifiedBlueMarble.jpg', custom_body/'custom-earth.jpg')
    body_startup = ctx.work/'startup-body-texture.txt'
    body_startup.write_text('\n'.join(
        'EARTH_TEXTURE_FILE = '+str(custom_body/'custom-earth.jpg')
        if line.partition('=')[0].strip() == 'EARTH_TEXTURE_FILE' else line
        for line in ctx.startup.read_text().splitlines())+'\n')
    # OpenFrames coverage is in the integration suite: that runtime also needs
    # the separate plugin-lifetime fix for reliable normal window closure.
    for name, startup, mode, screen, extra in [
        ('groundtrack', ctx.startup, 'groundtrack', '1280x900x24', {}),
        ('body-texture', body_startup, 'groundtrack-default', '1280x900x24', {}),
        ('hidpi', ctx.startup, 'groundtrack-default', '2560x1800x24', {'GDK_SCALE': '2'}),
    ]:
        ctx.config.write_text(BASE)
        output = ctx.gui(binary, name, mode=mode, screen=screen, startup=startup,
                         environment={**ctx.environment, **extra})
        if output.count('Drawing the ground track without a map.') != (2 if mode == 'groundtrack' else 0):
            raise RuntimeError('Unexpected background image warning count: '+name)
        passed('groundtrack: '+name)
