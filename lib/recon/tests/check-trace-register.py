#!/usr/bin/env python3
"""Preserve a private i386 register ABI while recording its real named entry."""
import subprocess,time,json,pathlib,os,hashlib,tempfile,shutil
repo=pathlib.Path(__file__).resolve().parents[3]
scratch=pathlib.Path.home()/'.ai-tmp/msvc2/trace-register-tests'
scratch.mkdir(parents=True,exist_ok=True)
p=pathlib.Path(tempfile.mkdtemp(prefix='check-',dir=scratch))
shutil.copyfile(repo/'lib/recon/include/trace-abi.h',p/'trace-abi.h')
for name in ['probe.c','hostile.S','driver.c']:
 shutil.copyfile(repo/'lib/recon/tests/trace-register'/name,p/name)
results=[];t=time.perf_counter()
def command(args,name,**kwargs):
 r=subprocess.run([str(x) for x in args],capture_output=True,**kwargs)
 (p/(name+'.stdout')).write_bytes(r.stdout);(p/(name+'.stderr')).write_bytes(r.stderr)
 assert r.returncode==0,(name,r.returncode,r.stderr.decode(errors='replace'))
 return r
for cc in ['gcc','clang']:
 common=[cc,'-m32','-O0','-g','-fno-omit-frame-pointer','-fno-inline','-DRECON_NATIVE_TEST','-I'+str(p)]
 command([cc,'-m32',p/'driver.c','-ldl','-o',p/(cc+'-driver')],cc+'-driver-build')
 for mode in ['pie','exec','shared']:
  out=p/(cc+'-'+mode)
  modeflags={'pie':['-fPIE','-pie'],'exec':['-fno-pie','-no-pie'],'shared':['-fPIC','-shared','-Wl,-Bsymbolic-functions']}[mode]
  command(common+['-finstrument-functions','-DHOSTILE']+modeflags+[p/'probe.c',p/'hostile.S','-o',out],out.name+'-build')
  run=[p/(cc+'-driver'),out] if mode=='shared' else [out]
  r=command(run,out.name+'-run');assert r.stdout==b'register ABI preserved\n'
  dis=command(['objdump','-d',out],out.name+'-disasm').stdout.decode()
  body=dis.split('<hash_register>:',1)[1].split('\n\n',1)[0]
  assert body.count('call')>=2 and '__cyg_profile_func_exit' not in body
  # One explicit hook call and no automatic entry; local PC thunk/call is allowed.
  assert body.count('<__cyg_profile_func_enter')==1,(out.name,body)
  results.append(dict(compiler=cc,mode=mode,kind='hostile-entry',passed=True))
 for mode in ['pie','exec']:
  prefix=cc+'-real-'+mode;obj=p/(prefix+'.o');tracer=p/(prefix+'-tracer.o');out=p/prefix
  pic=['-fPIE','-pie'] if mode=='pie' else ['-fno-pie','-no-pie']
  command(common+pic[:1]+['-finstrument-functions','-c',p/'probe.c','-o',obj],prefix+'-probe-build')
  command(common+pic[:1]+['-c',repo/'lib/recon/src/trace.c','-o',tracer],prefix+'-trace-build')
  command([cc,'-m32']+pic+[obj,tracer,'-ldl','-o',out],prefix+'-link')
  env={k:v for k,v in os.environ.items() if not k.startswith('RECON_TRACE_')}
  filt=p/'filter.txt';seen=p/'seen.txt';filt.write_text('hash_register\n');seen.write_text('')
  trace=p/(prefix+'.trace')
  with trace.open('wb') as f:
   env.update(RECON_TRACE_FILTER=str(filt),RECON_TRACE_SEEN=str(seen),RECON_TRACE_FD=str(f.fileno()))
   command([out],prefix+'-run',env=env,pass_fds=(f.fileno(),))
  assert trace.read_bytes()==b'hash_register\n'
  env.pop('RECON_TRACE_FD');command([out],prefix+'-disabled',env=env)
  results.append(dict(compiler=cc,mode=mode,kind='real-trace-first/repeat/disabled',passed=True))
# No native selection means both macros are empty (also the original compiler branch).
for flags in [[],['-DRECON_NATIVE_TEST','-U__linux__']]:
 src=p/'isolation.c';src.write_text('#include "trace-abi.h"\nRECON_TRACE_REGISTER_ABI void selected(void) { RECON_TRACE_REGISTER_ENTRY(selected) }\n')
 r=command(['gcc','-m32','-E','-P','-I'+str(p)]+flags+[src],'isolation-'+str(len(flags)))
 assert r.stdout.strip()==b'void selected(void) { }'
 results.append(dict(kind='unselected/original-isolation',flags=flags,passed=True))
receipt=dict(seconds=time.perf_counter()-t,source={x.name:hashlib.sha256(x.read_bytes()).hexdigest() for x in [p/'trace-abi.h',p/'probe.c',p/'hostile.S']},checks=results)
(p/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(passed=len(results),seconds=receipt['seconds'],receipt=str(p/'results.json'))))
