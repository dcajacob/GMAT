#!/usr/bin/env python3
from support import context, passed

if __name__ == '__main__':
    ctx = context('editor-io')
    ctx.runtime()
    binary = ctx.build_gui('EditorIoRegression.cpp')
    for mode in ('save', 'reload'):
        ctx.gui(binary, mode, mode=mode)
        passed('editor: '+mode+' failure preserves work and retry succeeds')
