#!/usr/bin/env python3
"""Focused PythonInterface diagnostics and return-conversion checks."""
import argparse
from pathlib import Path
import shlex
import subprocess
from support import Context, run, passed

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build_dir', type=Path)
parser.add_argument('--mode', default='all')
args = parser.parse_args()
ctx = Context(args.build_dir, 'python-interface')
versions = ctx.cache.get('GMAT_PYTHON3_VERSIONS', '').split(';')
versions = [v for v in versions if (ctx.application/'plugins'/('libPythonInterface_py'+v.replace('.', '')+'.so')).exists()]
if len(versions) != 1:
    raise RuntimeError('This focused runner requires exactly one built PythonInterface version')
name = 'PythonInterface_py'+versions[0].replace('.', '')
commands = subprocess.check_output(['ninja', '-t', 'commands', name], cwd=ctx.build, text=True).splitlines()
compile_line = next(c for c in commands if ' -c ' in c and c.endswith('/interface/PythonInterface.cpp'))
flags = shlex.split(compile_line)
flags = flags[:flags.index('-MD')]
obj, binary = ctx.work/'PythonRegression.o', ctx.work/'PythonRegression'
run(flags+['-c', str(ctx.source/'PythonRegression.cpp'), '-o', str(obj)], cwd=ctx.build)
lib = ctx.cache['_Python3_LIBRARY_RELEASE']
run(['c++', '-o', str(binary), str(obj), '-L'+str(ctx.application/'plugins'), '-l'+name,
     '-L'+str(ctx.application/'bin'), '-lGmatBase', '-lGmatUtil', lib,
     '-Wl,-rpath,'+str(ctx.application/'plugins')+':'+str(ctx.application/'bin')], cwd=ctx.build)
modes = ['diagnostics', 'vector', 'mixed', 'matrix', 'scalar', 'integer', 'string',
         'empty', 'empty-row', 'ragged', 'nonnumeric', 'overflow'] if args.mode == 'all' else [args.mode]
for mode in modes:
    run([str(binary), mode], cwd=ctx.work, log=ctx.work/(mode+'.log'))
    passed('PythonInterface '+mode)
