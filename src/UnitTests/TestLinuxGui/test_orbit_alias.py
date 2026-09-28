#!/usr/bin/env python3
from support import context, passed
ctx = context('orbit-alias')
ctx.runtime()
(ctx.work/'alias.script').write_text((ctx.source/'GuiSmoke.script').read_text())
binary = ctx.build_gui('OrbitAliasRegression.cpp')
ctx.gui(binary, 'native', environment=ctx.of_environment)
passed('native CelestialPlane alias On/Off and save/reload')
ctx.gui(binary, 'converted', startup=ctx.of_startup, environment=ctx.of_environment)
passed('converted CelestialPlane alias On/Off and save/reload')
