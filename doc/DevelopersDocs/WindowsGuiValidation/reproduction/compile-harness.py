import sys,re,subprocess,os,json,shutil,xml.etree.ElementTree as E
from pathlib import Path
cache=Path('C:/GMAT-Test/compiler-env.json')
if shutil.which('cl'):cache.write_text(json.dumps(dict(os.environ)))
elif cache.exists():os.environ.update(json.loads(cache.read_text()))
root=Path(sys.argv[1]); source=Path(sys.argv[2]); name=source.stem+('_isolated' if len(sys.argv)>3 else '')
project=root/'build-win/src/gui/GmatGUI.vcxproj'
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
x=E.parse(project)
g=next(g for g in x.findall('m:ItemDefinitionGroup',ns) if "Release|x64" in g.get('Condition',''))
inc=g.find('m:ClCompile/m:AdditionalIncludeDirectories',ns).text.split(';')
inc += re.findall(r'/external:I\s+"([^"]+)"',g.findtext('m:ClCompile/m:AdditionalOptions','',ns))
defs=g.find('m:ClCompile/m:PreprocessorDefinitions',ns).text.split(';')
flags=['/nologo','/c','/MD','/O2','/EHsc','/std:c++17','/utf-8','/D__wrap_main=main']
flags += ['/I'+p for p in inc if not p.startswith('%')]
flags += ['/D'+p for p in defs if not p.startswith('%')]
flags += ['/I'+str(source.parent)]
out=root/'application/bin'/f'{name}.exe';obj=root/'build-win'/f'{name}.obj'
r=subprocess.run(['cl',*flags,str(source),'/Fo'+str(obj)],cwd=project.parent)
if r.returncode:sys.exit(r.returncode)
tlog=next((root/'build-win/src/gui/GmatGUI.dir/Release').rglob('link.command.1.tlog'))
lines=tlog.read_text(encoding='utf-16').splitlines()
cmd=' '.join(l for l in lines if l and not l.startswith('^'))
cmd=re.sub(r'/OUT:"[^"]*"','/OUT:"'+str(out).replace('\\','/')+'"',cmd,flags=re.I)
cmd=re.sub(r'/SUBSYSTEM:windows', '/SUBSYSTEM:CONSOLE', cmd, flags=re.I)
for replacement_path in sys.argv[3:]:
 replacement=Path(replacement_path); replacement_obj=root/'build-win'/('isolated-'+replacement.stem+'.obj')
 r=subprocess.run(['cl',*flags,str(replacement),'/Fo'+str(replacement_obj)],cwd=project.parent)
 if r.returncode:sys.exit(r.returncode)
 original='GMATGUI.DIR\\RELEASE\\'+replacement.stem.upper()+'.OBJ'
 assert original in cmd, original
 cmd=cmd.replace(original,'"'+str(replacement_obj)+'"')
cmd += ' "'+str(obj)+'"'
rsp=root/'build-win'/f'{name}-link.rsp';rsp.write_text(cmd,encoding='utf-16')
r=subprocess.run(['link','@'+str(rsp)],cwd=project.parent)
sys.exit(r.returncode)
