#!/usr/bin/env python3
"""Assess optional OpenFrames controls, stable hierarchy and existing font fallbacks."""
from pathlib import Path
import sys,shlex,subprocess
from support import context,passed
ctx=context('of-behavior');ctx.runtime()
line=next(x for x in subprocess.check_output(['ninja','-t','commands'],cwd=ctx.build,text=True).splitlines() if ' -c ' in x and x.endswith('/OFScene.cpp'))
flags=[x for x in shlex.split(line) if x.startswith('-I')];flags+=['-I'+str(ctx.source)]
plugin=ctx.application/'plugins/libOpenFramesInterface.so';of=Path(ctx.cache['OPENFRAMES_DIR']);osg=Path(ctx.cache['OSG_DIR'])
binary=ctx.build_gui('OpenFramesBehaviorRegression.cpp',extra_flags=flags,extra_link=[str(plugin),'-Wl,-rpath,'+str(plugin.parent),'-L'+str(of/'lib'),'-Wl,-rpath,'+str(of/'lib'),'-lOpenFrames','-L'+str(osg/'lib'),'-Wl,-rpath,'+str(osg/'lib'),'-losg','-losgDB','-losgText','-losgViewer','-lOpenThreads'])
for mode,scale in [('no-microsoft',1),('no-microsoft',2),('no-microsoft',3),('none',1)]:
 ctx.gui(binary,f'behavior-{mode}-{scale}x',startup=ctx.of_startup,screen=f'{1280*scale}x{900*scale}x24',environment={**ctx.of_environment,'GMAT_FONT_TEST':mode,'OSG_FILE_PATH':'','GDK_SCALE':str(scale)})
 passed(f'OpenFrames playback, stable hierarchy and HUD fallback: {mode} {scale}x')
