from pathlib import Path
import importlib.util,json,re
root=Path('C:/GMAT-Test');script=root/'patched/application/samples/Ex_IOD.script'
output=(root/'results/focused-patched-PythonIOD/stdout.log').read_text(errors='replace')
messages=output.split('IOD_MESSAGES_BEGIN\n',1)[1].split('IOD_MESSAGES_END',1)[0]
spec=importlib.util.spec_from_file_location('IODFunctions',root/'patched/application/userfunctions/python/IODFunctions.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
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
result={'actual':actual,'direct_python':expected,'max_absolute_error_km_s':error}
(root/'iod-comparison.json').write_text(json.dumps(result,indent=2));print(result)
