from pathlib import Path
import os,subprocess,json,time,hashlib,concurrent.futures
root=Path('/root/msvc2');out=Path('/root/.ai-tmp/vc1.0-cvpack/two-pdb-build');out.mkdir()
env=os.environ.copy();env.update(WINEPREFIX='/root/.ai-tmp/vc1.0-capture/wine',WINEARCH='win32',WINEDEBUG='-all',WINESERVER='/usr/lib/wine/wineserver64',INCLUDE=r'R:\toolchains\1.0-nt\include',LIB=r'R:\toolchains\1.0-nt\lib',WINEPATH=r'R:\toolchains\1.0-nt\bin')
for k in('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS','LINK32_DB'):env.pop(k,None)
for suffix,num in [('one',1),('two',2)]:
 text='''enum ToneNUM { toneNUMfirst=1, toneNUMlast=9 };
struct NodeNUM { NodeNUM *next; int values[4]; ToneNUM tone; };
class ShapeNUM {
public:
    virtual int read();
    NodeNUM chain;
    int member;
};
int ShapeNUM::read() { return chain.values[0]+chain.values[3]+member; }
extern "C" __declspec(dllexport) int __stdcall provider_SUFFIX(ShapeNUM *value)
{
    int ShapeNUM::*field=&ShapeNUM::member;
    return value->read()+value->*field+value->chain.tone;
}
'''.replace('NUM',str(num)).replace('SUFFIX',suffix)
 (out/('pdb-'+suffix+'.cpp')).write_text(text)
assert len('pdb-one.cpp')==len('pdb-two.cpp')==11

def run(label,args):
 start=time.monotonic();p=subprocess.run(['wine']+args,cwd=out,env=env,input=b'',capture_output=True,timeout=90);(out/(label+'.stdout')).write_bytes(p.stdout);(out/(label+'.stderr')).write_bytes(p.stderr);r=dict(argv=p.args,status=p.returncode,elapsed_seconds=time.monotonic()-start,cwd=str(out),target_environment={});(out/(label+'.json')).write_text(json.dumps(r,indent=2)+'\n');print(label,p.returncode,round(r['elapsed_seconds'],3),p.stdout.decode('latin1')[:2000],p.stderr.decode('latin1')[:2000],flush=True)
 if p.returncode:raise RuntimeError('Producer rejected: '+label+'; no variants authorized')
 return r
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=2)as pool:
 fs=[pool.submit(run,'pdb-'+x,[r'R:\toolchains\1.0-nt\bin\CL.EXE','/nologo','/c','/Od','/Zi','/G3','/Fdpdb-'+x+'.pdb','/Fopdb-'+x+'.obj','pdb-'+x+'.cpp'])for x in('one','two')]
 for f in fs:f.result()
run('link',[r'R:\toolchains\1.0-nt\bin\LINK.EXE','/nologo','/dll','/debug:full','/debugtype:cv','/nopack','/out:providers.dll','pdb-one.obj','pdb-two.obj','libc.lib'])
files={p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())for p in out.iterdir()if p.suffix.lower()in('.dll','.pdb')};r=dict(elapsed_seconds=time.monotonic()-start,files=files,status='producer only; actual LF_TYPESERVER linkage gate pending');(out/'producer.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2),flush=True)
