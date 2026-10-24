from pathlib import Path
import os,json,time,subprocess,concurrent.futures,hashlib
root=Path('/root/msvc2');out=Path('/root/.ai-tmp/vc1.0-cvpack/multitu-cache-build');out.mkdir()
env=os.environ.copy();env.update(WINEPREFIX='/root/.ai-tmp/vc1.0-capture/wine',WINEARCH='win32',WINEDEBUG='-all',WINESERVER='/usr/lib/wine/wineserver64',INCLUDE=r'R:\toolchains\1.0-nt\include',LIB=r'R:\toolchains\1.0-nt\lib',WINEPATH=r'R:\toolchains\1.0-nt\bin')
for key in ('CL','_CL_','LINK','_LINK_','_MSC_IDE_FLAGS','MSC_CMD_FLAGS','LINK32_DB'):env.pop(key,None)
names=['cache%02d.cpp'%i for i in range(24)]
assert len({len(n)for n in names})==1
# 24 equal-length counted source names => sum of encoded names divisible by4,
# independently of the common path prefix. Index scalar/offset arrays are WORD+WORD+DWORD arrays.
assert 24%4==0
for i,name in enumerate(names):
 text='struct Cache%02d {\n'%i+''.join('    int field_%03d;\n'%j for j in range(512))+'};\n'
 text+='extern "C" __declspec(dllexport) int __stdcall cache%02d(Cache%02d *value)\n{ return value->field_000 + value->field_511; }\n'%(i,i)
 (out/name).write_text(text)
def run(label,args):
 start=time.monotonic();p=subprocess.run(['wine']+args,cwd=out,env=env,input=b'',capture_output=True,timeout=90)
 (out/(label+'.stdout')).write_bytes(p.stdout);(out/(label+'.stderr')).write_bytes(p.stderr)
 receipt=dict(argv=p.args,status=p.returncode,elapsed_seconds=time.monotonic()-start,cwd=str(out),target_environment={},parallel_jobs=4)
 (out/(label+'.json')).write_text(json.dumps(receipt,indent=2)+'\n');print(label,p.returncode,round(receipt['elapsed_seconds'],3),flush=True)
 if p.returncode:raise RuntimeError(label+' original failure; no variation authorized')
 return receipt
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=4)as pool:
 futures=[pool.submit(run,Path(n).stem,[r'R:\toolchains\1.0-nt\bin\CL.EXE','/nologo','/c','/Od','/Z7','/G3','/Fo'+Path(n).stem+'.obj',n])for n in names]
 for f in concurrent.futures.as_completed(futures):f.result()
run('link',[r'R:\toolchains\1.0-nt\bin\LINK.EXE','/nologo','/dll','/debug:full','/debugtype:cv','/nopack','/out:cache.dll']+[Path(n).stem+'.obj'for n in names]+['libc.lib'])
raw=(out/'cache.dll').read_bytes();receipt=dict(producer_seconds=time.monotonic()-start,input_bytes=len(raw),input_sha256=hashlib.sha256(raw).hexdigest(),source_name_alignment=dict(files=24,equal_bytes=len(names[0]),encoded_name_sum_mod4=0),status='producer-only; directory/cache gate and standalone CVPACK still pending')
(out/'producer.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2),flush=True)
