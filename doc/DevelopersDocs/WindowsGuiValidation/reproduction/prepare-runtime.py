from pathlib import Path
import json,shutil,sys
root=Path(sys.argv[1]);app=root/'application'; bin=app/'bin'; work=Path('C:/GMAT-Test/cases')/root.name;work.mkdir(parents=True,exist_ok=True)
lines=[];disabled=[]
for line in (app/'bin/gmat_startup_file.public.txt').read_text().splitlines():
 key,sep,value=line.partition('=');key=key.strip();value=value.strip()
 if sep and key=='PLUGIN':
  lib=(bin/value).resolve()
  if not lib.with_suffix('.dll').exists(): disabled.append(value);continue
  line='PLUGIN = '+lib.as_posix()
 elif sep and key=='ROOT_PATH':line='ROOT_PATH = '+app.as_posix()+'/'
 elif sep and key=='OUTPUT_PATH':line='OUTPUT_PATH = '+work.as_posix()+'/'
 elif sep and key=='PERSONALIZATION_FILE':line='PERSONALIZATION_FILE = '+(work/'personalization.ini').as_posix()
 lines.append(line)
for name in ['libExtraPropagators','libPolyhedronGravity','libSaveCommand','libPythonInterface_py312','libExternalForceModel_py312']:
 plugin=app/'plugins'/name
 if plugin.with_suffix('.dll').exists() and not any(plugin.as_posix() in line for line in lines):lines.append('PLUGIN = '+plugin.as_posix())
(work/'startup.txt').write_text('\n'.join(lines)+'\nWRITE_PERSONALIZATION_FILE = OFF\n')
(work/'personalization.ini').write_text('[Main]\nShowWelcomeOnStart=false\n')
(work/'disabled-plugins.json').write_text(json.dumps(disabled,indent=2))
print(work)
