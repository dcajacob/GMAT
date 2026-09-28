#!/usr/bin/env python3
from support import context, passed

if __name__ == '__main__':
    ctx = context('plugins')
    ctx.runtime()
    if not ctx.of_plugins:
        print('SKIP: plugin-lifetime test requires OpenFrames and OVtoOFI', flush=True)
    else:
        binary = ctx.build_gui('CloseRegression.cpp')
        output = ctx.gui(binary, 'openframes-window-close', startup=ctx.of_startup,
                         environment=ctx.of_environment,
                         arguments=['--run', str(ctx.source/'GuiSmoke.script')])
        if 'Mission run completed.' not in output:
            raise RuntimeError('OpenFrames mission did not complete before normal close')
        passed('plugins: normal window close after an OpenFrames mission')
        if 'Gtk-CRITICAL' in output:
            print('NOTE: external OFI tooltip warning remains; see log', flush=True)
