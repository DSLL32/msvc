import pathlib,subprocess,time,json,concurrent.futures,tempfile,argparse,os
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon';parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path);args=parser.parse_args();base=pathlib.Path.home()/'.ai-tmp/msvc2/borland-live-time-check';base.mkdir(parents=True,exist_ok=True);out=args.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));out.mkdir(parents=True,exist_ok=True)
def group(item):
 cc,profile=item;folder=out/f'{cc}-{profile}';folder.mkdir();exe=folder/'probe';defs=[]
 if profile=='borland':defs=['-DRECON_BORLAND_400_DOS_TIME']
 elif profile=='510':defs=['-DRECON_CRT_PROFILE_MSVCRT_510']
 elif profile=='watcom':defs=['-DRECON_CRT_PROFILE_WATCOM_100A']
 flags=['-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native','-Werror=implicit-function-declaration'];start=time.monotonic()
 p=subprocess.run([cc,*flags,*defs,str(lib/'tests/borland-live-time/probe.c'),str(lib/'tests/borland-live-time/consumer.c'),*[str(lib/'src'/f'{n}.c') for n in ['compat','crt','decimal','entry','process','win32']],'-pthread','-ldl','-o',str(exe)],capture_output=True);(folder/'build.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr;build=time.monotonic()-start;start=time.monotonic()
 if profile=='borland':
  for tz in [None,'UTC0','EST5','JST-9']:
   env=dict(os.environ);env.pop('RECON_FIXED_TIME',None)
   if tz is not None:env['TZ']=tz
   p=subprocess.run([str(exe)],env=env,capture_output=True);(folder/f'{tz or "host"}.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,(tz,p.returncode,p.stderr)
 else:
  p=subprocess.run([str(exe)],capture_output=True);assert p.returncode==125 and b'Borland400' in p.stderr
 for mode in ['null-date','null-time','wrap-date','wrap-time']:
  p=subprocess.run([str(exe),mode],capture_output=True);(folder/f'{mode}.log').write_bytes(p.stdout+p.stderr);assert p.returncode==125 and b'Borland400' in p.stderr,(mode,p.returncode,p.stderr)
 cxx='g++' if cc=='gcc' else 'clang++';p=subprocess.run([cxx,'-m32','-std=c++98','-nostdinc++','-DRECON_NATIVE_TEST',*defs,f'-I{lib}/include',f'-I{lib}/native','-fsyntax-only',str(lib/'tests/borland-signal-time/typing.cpp')],capture_output=True);(folder/'typing.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr
 return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:groups=list(pool.map(group,[('gcc','borland'),('clang','borland'),('gcc','default'),('gcc','510'),('clang','watcom')]))
result=dict(groups=groups,total_s=time.monotonic()-start);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
