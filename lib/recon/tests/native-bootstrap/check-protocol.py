import pathlib,subprocess,time,json,concurrent.futures,os,argparse,tempfile
root=pathlib.Path(__file__).resolve().parents[4]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path);arguments=parser.parse_args()
base=pathlib.Path.home()/'.ai-tmp/msvc2/native-command-check';base.mkdir(parents=True,exist_ok=True)
area=arguments.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));area.mkdir(parents=True,exist_ok=True)
lib=root/'lib/recon';sources=[lib/'src'/f'{n}.c' for n in ['crt','win32','process','compat','entry','decimal']]
def group(item):
 cc,profile=item;folder=area/f'protocol-{cc}-{profile}';folder.mkdir(exist_ok=True)
 flags=['-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native','-pthread','-ldl','-Werror=implicit-function-declaration']
 if profile=='510':flags+=['-DRECON_CRT_PROFILE_MSVCRT_510','-DRECON_DEFAULT_FMODE=0']
 start=time.monotonic()
 for source,name,inputs in [('protocol.c','protocol',sources),('unmarked.c','unmarked',[]),('probe.c','basic',sources)]:
  cmd=[cc,*flags,str(lib/'tests/native-bootstrap'/source),*map(str,inputs),'-o',str(folder/name)]
  p=subprocess.run(cmd,capture_output=True);(folder/f'{name}-build.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 build=time.monotonic()-start;start=time.monotonic();env={'MSC_CMD_FLAGS':'only'}
 def run(label,args,**extra):
  p=subprocess.run(args,cwd=folder,env=env,capture_output=True,timeout=15,**extra);(folder/f'{label}.log').write_bytes(p.stdout+p.stderr);return p
 p=run('protocol',[str(folder/'protocol'),str(folder/'unmarked')]);assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
 p=run('basic',[str(folder/'basic')]);assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
 # Child-only fixed-slot setup; no process-wide descriptor or environment changes.
 def install_unrelated():
  fd=os.open(folder/'unrelated-owner',os.O_RDWR|os.O_CREAT|os.O_TRUNC,0o600);os.write(fd,b'keep');os.dup2(fd,198);os.close(fd)
 p=run('unrelated',[str(folder/'protocol'),'unrelated'],preexec_fn=install_unrelated,close_fds=False);assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
 def malformed():
  fd=os.memfd_create('recon-windows-command-line',os.MFD_ALLOW_SEALING);os.write(fd,b'bad\0');os.dup2(fd,198);os.close(fd)
 p=run('invalid-transport',[str(folder/'protocol'),'unrelated'],preexec_fn=malformed,close_fds=False);assert p.returncode==125 and not p.stdout,(p.returncode,p.stdout,p.stderr)
 return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:groups=list(pool.map(group,[('gcc','default'),('gcc','510'),('clang','510')]))
result=dict(total_s=time.monotonic()-start,groups=groups);(area/'protocol-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
