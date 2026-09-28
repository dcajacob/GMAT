#!/usr/bin/env python3
from support import context, passed

ctx = context('python-gui')
ctx.runtime()
versions = ctx.cache['GMAT_PYTHON3_VERSIONS'].split(';')
assert len(versions) == 1
plugin = ctx.application/'plugins'/('libPythonInterface_py'+versions[0].replace('.', ''))
ctx.startup.write_text(ctx.startup.read_text()+'\nPLUGIN = '+str(plugin)+'\nPYTHON_MODULE_PATH = '+str(ctx.work)+'\n')
(ctx.work/'gmat_python_gui.py').write_text('''class Unprintable(Exception):
    def __str__(self):
        raise RuntimeError("formatting failed")
def valid(): return 7.0
def error(): raise ValueError("bad café: 100% complete")
def unprintable(): raise Unprintable()
def empty(): return []
def ragged(): return [[1.0, 2.0], [3.0]]
def nonnumeric(): return [1.0, "bad"]
def multiple(): return 7.0, [1, 2.5, 3], "café 100%"
''')
for name in ['missing-module', 'missing-function', 'valid', 'error', 'unprintable', 'empty', 'ragged', 'nonnumeric', 'multiple']:
    function = 'Python.gmat_python_gui.'+name
    if name == 'missing-module': function = 'Python.gmat_no_such_module.valid'
    if name == 'missing-function': function = 'Python.gmat_python_gui.missing'
    outputs = '[result, vec, text]' if name == 'multiple' else 'result'
    (ctx.work/(name+'.script')).write_text('Create Variable result;\nCreate Array vec[1,3];\nCreate String text;\nBeginMissionSequence;\n'+outputs+' = '+function+'();\n')
binary = ctx.build_gui('PythonGuiRegression.cpp')
ctx.gui(binary, 'recovery', environment=ctx.of_environment)
passed('Python GUI diagnostics, multiple outputs and same-session recovery')
