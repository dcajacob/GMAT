#!/usr/bin/env python3
"""Focused regressions recovered from the earlier non-Qt GUI work."""
from support import context, passed, BASE
import shutil

if __name__ == '__main__':
    ctx = context('gui-followup')
    ctx.runtime()
    for name in ('GroundTrackOwnership', 'GroundTrackSampling', 'GroundTrackLookup'):
        binary = ctx.build_gui(name+'Regression.cpp')
        ctx.gui(binary, name)
        passed(name)
    binary = ctx.build_gui('NativeTextRegression.cpp')
    for scale in (1, 2, 3):
        ctx.config.write_text(BASE)
        ctx.gui(binary, 'text-'+str(scale), screen=f'{1280*scale}x{900*scale}x24',
                environment={**ctx.environment, 'GDK_SCALE': str(scale)})
        for name in ('text-initial', 'text-resized'):
            (ctx.work/(name+'.png')).replace(ctx.work/(name+f'-{scale}x.png'))
        passed(f'native text at {scale}x and after resize')
    binary = ctx.build_gui('NativeWheelRegression.cpp')
    for mode, scale in (('centered',1), ('centered',2), ('centered',3),
                        ('free',1), ('astronaut',1), ('shift',1)):
        ctx.config.write_text(BASE)
        ctx.gui(binary, f'wheel-{mode}-{scale}', screen=f'{1280*scale}x{900*scale}x24',
                environment={**ctx.environment, 'GDK_SCALE': str(scale), 'GMAT_WHEEL_MODE':mode})
        passed(f'wheel zoom: {mode}, {scale}x')
    binary = ctx.build_gui('ExampleSweep.cpp')
    for source in sorted((ctx.source/'fixtures/groundtrack').glob('*.script')):
        script = ctx.work/source.name
        shutil.copyfile(source, script)
        try:
            output = ctx.gui(binary, source.stem,
                environment={**ctx.of_environment, 'GMAT_EXAMPLE_SCRIPT':str(script), 'GMAT_EXAMPLE_LIMIT':'30'})
        except RuntimeError as error:
            # GMAT propagates expected interpretation/run errors to its exit code.
            if not ('Invalid' in source.stem or 'OriginFail' in source.stem):
                raise
            if not str(error).startswith(('Command failed (248):', 'Command failed (254):')):
                raise
            output = (ctx.work/(source.stem+'.log')).read_text()
            if 'AUDIT_CLOSE_RETURN' not in output or 'GMAT GUI exiting.' not in output:
                raise
        if 'Invalid' in source.stem:
            expected = 'AUDIT_BUILD_END success=0'
        elif 'OriginFail' in source.stem:
            expected = 'AUDIT_RUN_END result=-2'
            if 'Spacecraft origins must match' not in output:
                raise RuntimeError('Missing origin diagnostic: '+source.name)
        else:
            expected = 'AUDIT_RUN_END result=1'
        if expected not in output:
            raise RuntimeError('Unexpected script outcome: '+source.name)
        passed(source.stem)
