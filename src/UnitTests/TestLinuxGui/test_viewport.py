#!/usr/bin/env python3
from support import context, passed

if __name__ == '__main__':
    ctx = context('viewport')
    binary = ctx.build_gui('ViewportRegression.cpp')
    ctx.runtime()
    for scale in (1, 2, 3):
        ctx.gui(binary, 'viewport-'+str(scale), screen=f'{1280*scale}x{900*scale}x24',
                environment={**ctx.environment, 'GDK_SCALE': str(scale)})
        for name in ('native-initial', 'native-resized'):
            (ctx.work/(name+'.png')).replace(ctx.work/(name+'-'+str(scale)+'x.png'))
        passed(f'native viewport fills canvas at {scale}x before and after resizing')
