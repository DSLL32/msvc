from pathlib import Path
import os,subprocess,tempfile,time,json
root=Path(__file__).resolve().parents[2];tests=Path(__file__).parent
base=Path.home()/'.ai-tmp/msvc2/volume-info-tests';base.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=base));flags=['-m32','-g','-O0','-DRECON_NATIVE_TEST','-DRECON_BORLAND_400_FIND','-I'+str(root/'include'),'-I'+str(root/'native')]
sources=[str(root/'src'/x) for x in ('compat.c','crt.c','win32.c','process.c','entry.c','decimal.c')]
start=time.perf_counter();rows=[]
for cc in ('gcc','clang'):
    binary=work/(cc+'-probe');command=[cc,*flags,'-std=gnu89',str(tests/'probe.c'),*sources,'-pthread','-ldl','-o',str(binary)]
    with (work/(cc+'-build.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,work/(cc+'-build.log')
    for mode in ('','native','tmpfs','unknown','next-null','next-invalid','export-unknown','bad-output','overlap','unc'):
        tick=time.perf_counter();result=subprocess.run([str(binary)]+([mode] if mode else []),cwd=work,env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')},capture_output=True)
        expected=0 if mode in ('','native','tmpfs') else 125
        row={'compiler':cc,'mode':mode or 'positive','status':result.returncode,'elapsed_s':time.perf_counter()-tick};rows.append(row)
        assert result.returncode==expected,(row,result.stdout,result.stderr,work)
for cc in ('g++','clang++'):
    result=subprocess.run([cc,*flags,'-std=c++98','-nostdinc++','-fsyntax-only',str(tests/'typing.cpp')],capture_output=True)
    assert result.returncode==0,result.stderr
receipt={'status':'pass','runtime':rows,'cpp_typing_groups':2,'elapsed_s':time.perf_counter()-start,'scratch':str(work)}
(work/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'status':'pass','runtime_groups':len(rows),'cpp_typing_groups':2,'elapsed_s':receipt['elapsed_s'],'scratch':str(work)}))
