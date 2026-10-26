import pathlib,subprocess,time,json,concurrent.futures,os,argparse,tempfile
root=pathlib.Path(__file__).resolve().parents[4]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path);arguments=parser.parse_args()
base=pathlib.Path.home()/'.ai-tmp/msvc2/native-bootstrap-check';base.mkdir(parents=True,exist_ok=True)
area=arguments.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));area.mkdir(parents=True,exist_ok=True)
lib=root/'lib/recon';sources=[lib/'src'/f'{n}.c' for n in ['crt','win32','process','compat','entry','decimal']]
def check(item):
 cc,profile=item;folder=area/f'{cc}-{profile}';folder.mkdir(exist_ok=True);exe=folder/'probe'
 flags=['-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native','-pthread','-ldl','-Werror=implicit-function-declaration']
 if profile=='510':flags+=['-DRECON_CRT_PROFILE_MSVCRT_510','-DRECON_DEFAULT_FMODE=0']
 if profile=='watcom':flags+=['-DRECON_CRT_PROFILE_WATCOM_100A']
 command=[cc,*flags,str(lib/'tests/native-bootstrap/probe.c'),*map(str,sources),'-o',str(exe)]
 start=time.monotonic();p=subprocess.run(command,capture_output=True);(folder/'build.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 build=time.monotonic()-start;start=time.monotonic()
 for label,env,args in [('minimal',{'MSC_CMD_FLAGS':'only'},[]),('empty',{},[]),('host',dict(os.environ),[]),('quote',{'MSC_CMD_FLAGS':'only'},['quote','','two words','tail\\','a"b','a\\"b','a b\\'])]:
  p=subprocess.run([str(exe),*args],cwd=folder,env=env,capture_output=True,timeout=10);(folder/f'{label}.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0 and not p.stderr,(label,p.returncode,p.stdout,p.stderr)
 return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:groups=list(pool.map(check,[('gcc','default'),('gcc','510'),('clang','watcom')]))
result=dict(total_s=time.monotonic()-start,groups=groups);(area/'basic-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
