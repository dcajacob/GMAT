#!/usr/bin/env python3
"""Read back native plot/model texture pixels for odd and aligned RGB rows."""
from pathlib import Path
import sys,shutil
from support import context,passed
ctx=context('texture-upload');ctx.runtime()
shutil.copy2(ctx.root/'application/data/vehicle/models/aura.3ds',ctx.work/'source.3ds')
b=ctx.build_gui('TextureUploadRegression.cpp',extra_flags=['-I'+str(ctx.source)])
ctx.gui(b,'fixed')

passed('native plot and model RGB rows and caller pixel-store state across image formats')
