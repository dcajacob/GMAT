from pathlib import Path
import subprocess,os,json
root=Path('C:/GMAT-Test');results=[]
for v in ['upstream','patched']:
 app=root/v/'application';env=dict(os.environ,PATH=str(app/'bin')+';'+str(app/'plugins')+';C:\\Python312;'+os.environ['PATH'])
 for mode in ['diagnostics','vector','mixed','matrix','scalar','integer','string','empty','empty-row','ragged','nonnumeric','overflow']:
  p=root/'results'/f'python-{v}-{mode}';p.mkdir(parents=True,exist_ok=True)
  with (p/'stdout.log').open('wb') as log:
   try:r=subprocess.run([str(app/'bin/PythonRegression.exe'),mode],cwd=app/'bin',env=env,stdout=log,stderr=subprocess.STDOUT,timeout=45);code=r.returncode
   except subprocess.TimeoutExpired:code='timeout'
  text=(p/'stdout.log').read_text(errors='replace');result={'id':p.name,'returncode':code,'passes':text.count('PASS:'),'failures':text.count('FAIL:')};results.append(result);(p/'result.json').write_text(json.dumps(result,indent=2));print(result,flush=True)
(root/'python-results.json').write_text(json.dumps(results,indent=2))
