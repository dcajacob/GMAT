import sys,re,subprocess,os,json,shutil,xml.etree.ElementTree as E
from pathlib import Path
cache=Path('C:/GMAT-Test/compiler-env.json')
if not shutil.which('cl') and cache.exists():os.environ.update(json.loads(cache.read_text()))
root=Path(sys.argv[1]);src=Path('C:/GMAT-Test/patched/src/UnitTests/TestLinuxGui/PythonRegression.cpp')
proj=next((root/'build-win').rglob('PythonInterface_py312.vcxproj'))
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
g=next(g for g in E.parse(proj).findall('m:ItemDefinitionGroup',ns) if 'Release|x64' in g.get('Condition',''))
inc=g.find('m:ClCompile/m:AdditionalIncludeDirectories',ns).text.split(';');defs=g.find('m:ClCompile/m:PreprocessorDefinitions',ns).text.split(';')
inc += re.findall(r'/external:I\s+"([^"]+)"',g.findtext('m:ClCompile/m:AdditionalOptions','',ns))
flags=['/nologo','/MD','/EHsc','/std:c++17','/utf-8']+['/I'+p for p in inc if not p.startswith('%')]+['/D'+p for p in defs if not p.startswith('%') and p!='PYTHON_EXPORTS']
libs=[next((root/'build-win').rglob(name)) for name in ['PythonInterface_py312.lib','GmatBase.lib','GmatUtil.lib']]
obj=root/'build-win/PythonRegression.obj';exe=root/'application/bin/PythonRegression.exe'
r=subprocess.run(['cl',*flags,str(src),'/Fo'+str(obj),'/Fe'+str(exe),'/link',*map(str,libs),'C:/Python312/libs/python312.lib'],cwd=proj.parent);sys.exit(r.returncode)
