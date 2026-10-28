from pathlib import Path
import subprocess,tempfile,time,json
root=Path(__file__).resolve().parents[2];tests=Path(__file__).parent
base=Path.home()/'.ai-tmp/msvc2/borland-fnsplit-tests';base.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=base));flags=['-m32','-g','-O0','-DRECON_NATIVE_TEST','-I'+str(root/'include'),'-I'+str(root/'native')]
sources=[str(root/'src'/x) for x in ('compat.c','crt.c','win32.c','process.c','entry.c','decimal.c')]
start=time.perf_counter();rows=[]
for cc in ('gcc','clang'):
    binary=work/(cc+'-probe')
    command=[cc,*flags,'-std=gnu89','-DRECON_BORLAND_400_FNSPLIT',str(tests/'probe.c'),str(tests/'consumer.c'),*sources,'-pthread','-ldl','-o',str(binary)]
    with (work/(cc+'-build.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,work/(cc+'-build.log')
    for mode in ('','overlap','outputs','null','long','nonascii','wrap'):
        tick=time.perf_counter();result=subprocess.run([str(binary)]+([mode] if mode else []),capture_output=True)
        expected=125 if mode in ('outputs','null','long','nonascii','wrap') else 0
        row={'compiler':cc,'mode':mode or 'positive','status':result.returncode,'elapsed_s':time.perf_counter()-tick};rows.append(row)
        assert result.returncode==expected,(row,result.stdout,result.stderr,work)
for cc in ('g++','clang++'):
    result=subprocess.run([cc,*flags,'-std=c++98','-nostdinc++','-DRECON_BORLAND_400_FNSPLIT','-fsyntax-only',str(tests/'typing.cpp')],capture_output=True)
    assert result.returncode==0,result.stderr
    for extra in ([],['-DRECON_CRT_PROFILE_MSVCRT_510'],['-DRECON_CRT_PROFILE_WATCOM_100A'],['-DRECON_SHIM_IMPL','-DRECON_BORLAND_400_FNSPLIT']):
        result=subprocess.run([cc,*flags,*extra,'-std=c++98','-nostdinc++','-fsyntax-only',str(tests/'isolation.cpp')],capture_output=True)
        assert result.returncode==0,result.stderr
receipt={'status':'pass','runtime':rows,'vector_optional_combinations_per_compiler':352,'typing_isolation_groups':10,'elapsed_s':time.perf_counter()-start,'scratch':str(work)}
(work/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'status':'pass','runtime_groups':len(rows),'vector_optional_combinations_per_compiler':352,'typing_isolation_groups':10,'elapsed_s':receipt['elapsed_s'],'scratch':str(work)}))
