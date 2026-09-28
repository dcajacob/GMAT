#!/usr/bin/env python3
"""Compare both unmodified Ex_IOD GUI outputs with direct Python execution."""
import importlib.util
import json
import re
from support import context, passed

ctx = context('python-iod')
ctx.runtime()
versions = ctx.cache['GMAT_PYTHON3_VERSIONS'].split(';')
assert len(versions) == 1
plugin = ctx.application/'plugins'/('libPythonInterface_py'+versions[0].replace('.', ''))
ctx.startup.write_text(ctx.startup.read_text()+'\nPLUGIN = '+str(plugin)+'\n')
script = ctx.root/'application/samples/Ex_IOD.script'
env = {**ctx.of_environment, 'GMAT_IOD_SCRIPT': str(script)}
env.pop('LD_PRELOAD', None)
output = ctx.gui(ctx.build_gui('PythonIODRegression.cpp'), 'iod', environment=env)
messages = output.split('IOD_MESSAGES_BEGIN\n', 1)[1].split('IOD_MESSAGES_END', 1)[0]
(ctx.work/'messages.txt').write_text(messages)
spec = importlib.util.spec_from_file_location('IODFunctions', ctx.root/'application/userfunctions/python/IODFunctions.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
# These input assignments are read from the actual example, not a duplicate fixture.
inputs, expected = {}, []
for line in script.read_text().splitlines():
    match = re.match(r'\s*(R[123])\(([123])\)\s*=\s*([-\d.]+);', line)
    if match:
        inputs.setdefault(match[1], [0.0]*3)[int(match[2])-1] = float(match[3])
    match = re.match(r'\s*(T[123])\s*=\s*([\d.]+)(?:/([\d.]+))?;', line)
    if match:
        inputs[match[1]] = float(match[2])/float(match[3] or 1)
    if line.startswith('[V2,Log]'):
        expected.append(module.ThreePositionIOD(*(inputs[key] for key in ('R1','R2','R3','T1','T2','T3')))[0])
vectors = re.findall(r'^([-+\d.eE]+)[ \t]+([-+\d.eE]+)[ \t]+([-+\d.eE]+)[ \t]*$', messages, re.M)
assert len(vectors) == len(expected) == 2, messages
actual = [[float(x) for x in vector] for vector in vectors]
error = max(abs(a-b) for av, ev in zip(actual, expected) for a,b in zip(av,ev))
assert error < 1e-10, (actual, expected, error)
(ctx.work/'comparison.json').write_text(json.dumps({'actual': actual, 'direct_python': expected, 'max_absolute_error_km_s': error}, indent=2)+'\n')
passed('unmodified IOD completes without preloading and both velocity vectors match direct Python')
