#!/usr/bin/env python3
"""Compare the bundled Aura asset with its upstream baseline using both model loaders.
Requires OSG (the OpenFrames build dependency) and the upstream base commit locally.
"""
from pathlib import Path
import shutil,subprocess,struct,hashlib
from support import context,passed
ctx=context('aura-model');ctx.runtime();repo=ctx.root
baseline=ctx.work/'original.3ds'
baseline.write_bytes(subprocess.check_output(['git','show','9363e129be366520c6edb0b4079204ed60666007:application/data/vehicle/models/aura.3ds'],cwd=repo))
# Reconstruct the narrowly scoped edit; all other binary payloads must match.
removed=0
def remove_unresolved_reflection(data):
    global removed
    result=bytearray();offset=0
    while offset<len(data):
        tag,size=struct.unpack_from('<HI',data,offset)
        assert size>=6 and offset+size<=len(data)
        payload=data[offset+6:offset+size]
        if tag in (0x4d4d,0x3d3d,0xafff,0xa200):
            payload=remove_unresolved_reflection(payload)
        elif tag==0xa220 and b'GFOIL1.JPG\0' in payload:
            assert payload==bytes.fromhex('00a311000000')+b'GFOIL1.JPG\0'+bytes.fromhex('3000080000000600')
            removed+=1;offset+=size;continue
        result+=struct.pack('<HI',tag,len(payload)+6)+payload
        offset+=size
    return bytes(result)
assert remove_unresolved_reflection(baseline.read_bytes())==(repo/'application/data/vehicle/models/aura.3ds').read_bytes()
assert removed==1
image=(repo/'application/data/vehicle/models/aura_map.jpg').read_bytes()
assert hashlib.sha1(b'blob '+str(len(image)).encode()+b'\0'+image).hexdigest()=='67a07430a4eabd1b041b4ee39654ef5c259192b8'
passed('exactly one optional reflection chunk removed; every other payload and the original NASA image preserved')
osg=Path(ctx.cache['OSG_DIR'])
binary=ctx.build_gui('AuraModelRegression.cpp',extra_flags=['-I'+str(osg/'include'),'-I'+str(ctx.source)],extra_link=['-L'+str(osg/'lib'),'-Wl,-rpath,'+str(osg/'lib'),'-losgDB','-losg','-losgText'])
for variant,source in [('baseline',baseline),('fixed',repo/'application/data/vehicle/models/aura.3ds')]:
 d=ctx.work/variant;d.mkdir(exist_ok=True);shutil.copy2(source,ctx.work/'aura.3ds');shutil.copy2(repo/'application/data/vehicle/models/aura_map.jpg',ctx.work/'aura_map.jpg')
 out=ctx.gui(binary,variant,environment=ctx.of_environment)
 assert ('Cannot create texture GFOIL1.JPG' in out)==(variant=='baseline')
 for name in ('native-summary.txt','scene.osgt'):shutil.copy2(ctx.work/name,d/name)
for name in ('native-summary.txt','scene.osgt'):
 assert (ctx.work/'baseline'/name).read_bytes()==(ctx.work/'fixed'/name).read_bytes(),name+' changed'
passed('Aura native material summary and complete OSG scene identical; missing-reflection diagnostic removed by asset repair')

mission=ctx.build_gui('CloseRegression.cpp')
for name,startup,env in [('native',ctx.startup,ctx.environment),('openframes',ctx.of_startup,ctx.of_environment)]:
    output=ctx.gui(mission,'mission-'+name,startup=startup,environment=env,arguments=['--run',str(ctx.source/'GuiSmoke.script')])
    assert 'Mission run completed.' in output
    assert 'GFOIL1.JPG' not in output
    passed(name+' mission loads the repaired bundled Aura model and closes normally')
