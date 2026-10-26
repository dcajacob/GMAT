"""Run real Cocoa executables with deadlines; retain failure/graphics limitations."""
from pathlib import Path
import hashlib,json,os,platform,subprocess,sys,time
root=Path.cwd();out=root/'artifacts';paths=json.loads((root/'build/macos-smoke/smoke-paths.json').read_text())
env=os.environ.copy();env['QT_QPA_PLATFORM']='cocoa';env['SMOKE_ARTIFACT_DIR']=str(out)
for key in ('QT_OPENGL','QT_OPENGL_DLL','QT_WIDGETS_RHI','QT_WIDGETS_RHI_BACKEND'):env.pop(key,None)
result={'commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'platform':platform.platform(),'architecture':platform.machine(),'qpa_requested':'cocoa','build':'PASS','standalone_gui':'NOT_RUN','mission_and_3d':'NOT_RUN','binaries':{}}
for n in ('app','smoke'):
 p=Path(paths[n]);result['binaries'][n]={'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
def run(label,command,timeout):
 start=time.monotonic()
 with (out/(label+'.log')).open('wb') as log:
  try:code=subprocess.run(command,cwd=Path(paths['app']).parent,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=timeout).returncode
  except subprocess.TimeoutExpired:code='TIMEOUT'
 result[label]={'exit':code,'seconds':time.monotonic()-start}
 (out/'run.json').write_text(json.dumps(result,indent=2)+'\n');return code
# The unchanged product entry point runs the mission in a visible Cocoa window,
# captures its own composed image, and exits; the instrumented harness separately
# requires a valid context and verifies actual orbit image content and controls.
a=run('standalone_gui',[paths['app'],'--startup',paths['startup'],'--settings-dir',str(out/'app-settings'),'--run','--screenshot',str(out/'standalone.png'),str(root/'ci/macos/smoke.script')],100)
b=run('mission_and_3d',[paths['smoke'],paths['startup'],str(root/'ci/macos/smoke.script'),str(out)],120)
print(json.dumps(result,indent=2));sys.exit(0 if a==0 and b==0 else 1)
