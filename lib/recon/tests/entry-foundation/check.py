"""Small native-only foundation checks; no fixture/original controller."""
import argparse,concurrent.futures,json,pathlib,subprocess,tempfile,time
root=pathlib.Path(__file__).resolve().parents[4]
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path)
arguments=parser.parse_args()
base=pathlib.Path.home()/'.ai-tmp/msvc2/entry-foundation-tests';base.mkdir(parents=True,exist_ok=True)
area=arguments.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base))
area.mkdir(parents=True,exist_ok=True)
sources=[str(root/'lib/recon/src'/f'{name}.c') for name in ('crt','win32','process','compat','entry','decimal')]
tests=[str(p) for p in (root/'lib/recon/tests/entry-foundation').glob('*.c')]
flags=['-m32','-std=gnu89','-O0','-g','-Wall','-Wextra','-Werror=implicit-function-declaration','-DRECON_NATIVE_TEST',f'-I{root}/lib/recon/include',f'-I{root}/lib/recon/native','-pthread','-ldl']
def check(item):
    cc,profile=item;directory=area/f'{cc}-{profile}';directory.mkdir(exist_ok=True)
    binary=directory/'foundation';start=time.monotonic()
    profile_flags={'default':[],'510':['-DRECON_CRT_PROFILE_MSVCRT_510','-DRECON_DEFAULT_FMODE=0'],'watcom':['-DRECON_CRT_PROFILE_WATCOM_100A']}[profile]
    command=[cc,*flags,*profile_flags,*tests,*sources,'-Wl,-T,'+str(root/'lib/recon/tests/entry-foundation/reset-region.ld'),'-o',str(binary)]
    p=subprocess.run(command,capture_output=True);(directory/'build.log').write_bytes(p.stdout+p.stderr)
    assert p.returncode==0,(command,p.stderr)
    build=time.monotonic()-start;start=time.monotonic();outcomes=[]
    for mode,code,expected in [('reset',0,b'relocated data/BSS/padding/refcounts/dependencies/failure reset PASS\n'),('',0,None),('dynamic-return',0,b'BAXTbD'),('static-return',0,b'BSXTbD'),('app-exit',7,b'BSXTbD'),('exit-process',9,b'BSTbD'),('policy-isolation',0,b'policy separation PASS\n'),('legacy-exit-process',3,b'')]:
        p=subprocess.run([str(binary),*([mode] if mode else [])],cwd=directory,capture_output=True)
        (directory/(mode or 'normal')).with_suffix('.log').write_bytes(p.stdout+p.stderr)
        assert p.returncode==code and not p.stderr,(mode,p.returncode,p.stdout,p.stderr)
        if expected is not None:assert p.stdout==expected,(mode,p.stdout,expected)
        else:assert p.stdout.count(b'PASS')==3,p.stdout
        outcomes.append(dict(mode=mode or 'normal',status=p.returncode))
    return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start,outcomes=outcomes)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
    results=list(pool.map(check,[('gcc','default'),('gcc','510'),('clang','watcom')]))
# Preserve strict fixture observer API's existing positive and negative controls.
legacy=area/'legacy-provider';command=['gcc',*flags,'-DRECON_CRT_PROFILE_MSVCRT_510',str(root/'lib/recon/tests/module-provider-probe.c'),*sources,'-o',str(legacy)]
before=time.monotonic();p=subprocess.run(command,capture_output=True);(area/'legacy-build.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0,p.stderr
p=subprocess.run(['python3',str(root/'lib/recon/tests/check-module-provider.py'),str(legacy)],capture_output=True);assert p.returncode==0,(p.stdout,p.stderr)
(area/'legacy.log').write_bytes(p.stdout+p.stderr)
receipt=dict(total_s=time.monotonic()-start,groups=results,legacy_s=time.monotonic()-before,legacy=p.stdout.decode())
(area/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2));print('Results:',area)
