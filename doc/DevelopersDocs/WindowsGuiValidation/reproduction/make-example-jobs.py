from pathlib import Path
import json,os,shutil,re,sys
root=Path('C:/GMAT-Test');jobs=[]
selection=sys.argv[1] if len(sys.argv)>1 else '.'
items=json.loads((root/'primary-inventory.json').read_text())['scripts']
for v in ['upstream','patched']:
 app=root/v/'application';base=root/'cases'/v
 for item in items:
  if not re.search(selection,item['script']):continue
  case=root/'results'/f"examples-{v}-{item['id']}";case.mkdir(parents=True,exist_ok=True)
  stage=case/'repo';source=root/v/item['script'];copy=stage/item['script']
  shutil.copytree(app/'samples',stage/'application/samples',dirs_exist_ok=True)
  if not item['script'].startswith('application/samples/'):
   shutil.copytree(source.parent,copy.parent,dirs_exist_ok=True,ignore=shutil.ignore_patterns('.git','__pycache__'))
  if item['script'].startswith('plugins/TLEPropagatorPlugin/'):
   for rel in ['TLE','test/TLE']:
    p=Path('plugins/TLEPropagatorPlugin')/rel
    shutil.copytree(root/v/p,stage/p,dirs_exist_ok=True)
  cwd=stage/'application/bin';cwd.mkdir(parents=True,exist_ok=True)
  # Junctions expose only bundled runtime assets; mutable inputs are copied above.
  import subprocess
  for leaf in ['data','userfunctions','userincludes','matlab']:
   target=stage/'application'/leaf
   if (app/leaf).exists() and not target.exists():
    subprocess.run(['cmd','/c','mklink','/J',str(target),str(app/leaf)],check=True,stdout=subprocess.DEVNULL)
  text=(base/'startup.txt').read_text().replace(base.as_posix()+'/',case.as_posix()+'/')
  (case/'startup.txt').write_text(text);(case/'personalization.ini').write_text('[Main]\nShowWelcomeOnStart=false\n')
  env={'GMAT_GUI_TEST_IMAGES':case.as_posix(),'GMAT_EXAMPLE_SCRIPT':str(copy),'GMAT_EXAMPLE_LIMIT':'120','PATH':str(app/'bin')+';'+str(app/'plugins')+';C:\\Python312;'+os.environ['PATH']}
  jobs.append({'id':case.name,'script':item['script'],'command':[str(app/'bin/ExampleSweep.exe'),'--startup_file',str(case/'startup.txt'),'--no_splash'],'cwd':str(cwd),'env':env,'timeout':170})
(root/'jobs.json').write_text(json.dumps(jobs,indent=2));print('Prepared',len(jobs),'jobs')
