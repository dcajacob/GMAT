from pathlib import Path
import subprocess,os,json,time,traceback
from PIL import ImageGrab
root=Path('C:/GMAT-Test'); results=[]
try:
 (root/'tests-done.txt').unlink(missing_ok=True)
 (root/'runner-error.txt').unlink(missing_ok=True)
 jobs=json.loads((root/'jobs.json').read_text())
 for job in jobs:
  case=root/'results'/job['id'];case.mkdir(parents=True,exist_ok=True)
  previous=case/'result.json'
  if job['id'].startswith('examples-') and previous.exists() and 'AUDIT_BUILD_BEGIN' in (case/'stdout.log').read_text(errors='replace'):
   results.append(json.loads(previous.read_text()));(root/'results.json').write_text(json.dumps(results,indent=2));continue
  env=dict(os.environ);env.update(job.get('env',{}))
  started=time.time();state='completed';code=None
  with (case/'stdout.log').open('w',encoding='utf-8') as log:
   proc=subprocess.Popen(job['command'],cwd=job['cwd'],env=env,stdout=log,stderr=subprocess.STDOUT)
   try:code=proc.wait(timeout=job.get('timeout',120))
   except subprocess.TimeoutExpired:
    state='timeout';ImageGrab.grab().save(case/'timeout.png')
    subprocess.run(['taskkill','/F','/T','/PID',str(proc.pid)],stdout=log,stderr=subprocess.STDOUT)
  text=(case/'stdout.log').read_text(errors='replace')
  result=dict(id=job['id'],script=job.get('script'),state=state,returncode=code,seconds=round(time.time()-started,2),passes=text.count('PASS:'),failures=text.count('FAIL:'))
  (case/'result.json').write_text(json.dumps(result,indent=2));results.append(result)
  (root/'results.json').write_text(json.dumps(results,indent=2))
 (root/'tests-done.txt').write_text('complete')
except Exception:
 (root/'runner-error.txt').write_text(traceback.format_exc())
 raise
