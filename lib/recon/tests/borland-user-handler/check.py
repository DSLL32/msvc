import pathlib,subprocess,time,json,concurrent.futures,tempfile,argparse
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon'
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path);args=parser.parse_args()
base=pathlib.Path.home()/'.ai-tmp/msvc2/borland-user-handler-check';base.mkdir(parents=True,exist_ok=True)
out=args.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));out.mkdir(parents=True,exist_ok=True)
def group(item):
 cc,profile=item;folder=out/f'{cc}-{profile}';folder.mkdir(exist_ok=True);exe=folder/'probe';flags=['-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native','-Werror=implicit-function-declaration'];defs=[]
 if profile=='borland':defs=['-DRECON_BORLAND_400_USER_HANDLER']
 elif profile=='510':defs=['-DRECON_CRT_PROFILE_MSVCRT_510']
 elif profile=='watcom':defs=['-DRECON_CRT_PROFILE_WATCOM_100A']
 command=[cc,*flags,*defs,str(lib/'tests/borland-user-handler/probe.c'),str(lib/'tests/borland-user-handler/consumer.c'),*[str(lib/'src'/f'{n}.c') for n in ['compat','crt','decimal','entry','process','win32']],'-pthread','-ldl','-o',str(exe)]
 start=time.monotonic();p=subprocess.run(command,capture_output=True);(folder/'build.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr;build=time.monotonic()-start;start=time.monotonic()
 for mode,status in ([('',0),('initial',125),('gp',125)] if profile=='borland' else [('',125),('gp',125)]):
  p=subprocess.run([str(exe)]+([mode] if mode else []),capture_output=True);(folder/f'{mode or "normal"}.log').write_bytes(p.stdout+p.stderr);assert p.returncode==status,(cc,profile,mode,p.returncode,p.stderr)
 cxx='g++' if cc=='gcc' else 'clang++';p=subprocess.run([cxx,'-m32','-std=c++98','-nostdinc++','-DRECON_NATIVE_TEST',*defs,f'-I{lib}/include',f'-I{lib}/native','-fsyntax-only',str(lib/'tests/borland-user-handler/typing.cpp')],capture_output=True);(folder/'typing.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr
 return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:groups=list(pool.map(group,[('gcc','borland'),('clang','borland'),('gcc','default'),('gcc','510'),('clang','watcom')]))
result=dict(groups=groups,total_s=time.monotonic()-start);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
