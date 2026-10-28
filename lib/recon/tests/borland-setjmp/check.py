from pathlib import Path
import subprocess,tempfile,time,json
root=Path(__file__).resolve().parents[2];tests=Path(__file__).parent
base=Path.home()/'.ai-tmp/msvc2/borland-setjmp-tests';base.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=base));common=['-m32','-g','-O0','-DRECON_NATIVE_TEST','-I'+str(root/'include'),'-I'+str(root/'native')]
sources=[str(root/'src'/x) for x in ('compat.c','crt.c','win32.c','process.c','entry.c','decimal.c')]
start=time.perf_counter();rows=[]
def build(cc,name,extra,inputs):
    output=work/name
    with (work/(name+'-build.log')).open('w') as log:
        result=subprocess.run([cc,*common,*extra,*map(str,inputs),*sources,'-pthread','-ldl','-o',str(output)],stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,work/(name+'-build.log')
    return output
for cc in ('gcc','clang'):
    binary=build(cc,cc+'-probe',['-std=gnu89','-DRECON_BORLAND_400_SETJMP_INITIAL'],[tests/'probe.c',tests/'consumer.c'])
    for mode in ('','address-null','address-owner','jump-null','size','null','wrap','generic','jump','jump-zero'):
        tick=time.perf_counter();result=subprocess.run([str(binary)]+([mode] if mode else []),capture_output=True)
        row={'compiler':cc,'mode':mode or 'positive','status':result.returncode,'elapsed_s':time.perf_counter()-tick};rows.append(row)
        assert result.returncode==(125 if mode else 0),(row,result.stderr,work)
for cc in ('g++','clang++'):
    result=subprocess.run([cc,*common,'-DRECON_BORLAND_400_SETJMP_INITIAL','-std=c++98','-nostdinc++','-fsyntax-only',str(tests/'typing.cpp')],capture_output=True)
    assert result.returncode==0,result.stderr
    for extra in ([],['-DRECON_LEGACY_JMP_WORDS=16'],['-DRECON_CRT_PROFILE_WATCOM_100A'],['-DRECON_CRT_PROFILE_MSVCRT_510','-DRECON_LEGACY_JMP_WORDS=16','-DRECON_NATIVE_SETJMP3_COUNT0'],['-DRECON_SHIM_IMPL','-DRECON_BORLAND_400_SETJMP_INITIAL']):
        result=subprocess.run([cc,*common,*extra,'-std=c++98','-nostdinc++','-fsyntax-only',str(tests/'profiles.cpp')],capture_output=True)
        assert result.returncode==0,result.stderr
    for extra in (['-DRECON_LEGACY_JMP_WORDS=16'],['-DRECON_CRT_PROFILE_WATCOM_100A']):
        result=subprocess.run([cc,*common,'-DRECON_BORLAND_400_SETJMP_INITIAL',*extra,'-std=c++98','-nostdinc++','-fsyntax-only',str(tests/'profiles.cpp')],capture_output=True)
        assert result.returncode!=0 and b'Borland initial save requires' in result.stderr
for words in (8,16):
    binary=build('gcc','legacy-'+str(words),['-DRECON_LEGACY_JMP_WORDS='+str(words)],[root/'tests/check-jump-storage.c'])
    result=subprocess.run([str(binary)],capture_output=True);assert result.returncode==0,result.stderr
binary=build('gcc','watcom',['-DRECON_CRT_PROFILE_WATCOM_100A'],[root/'tests/watcom-setjmp-probe.c'])
result=subprocess.run(['python3',str(root/'tests/check-watcom-setjmp.py'),str(binary)],capture_output=True)
(work/'watcom-retained.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,result.stderr
result=subprocess.run(['gdb','-q','-batch','-x',str(tests/'frame.gdb'),str(work/'gcc-probe')],capture_output=True)
(work/'frame.log').write_bytes(result.stdout+result.stderr)
assert result.returncode==0 and b'LIVE CALLER PROOF: initial_save' in result.stdout,(result.stdout,result.stderr)
receipt={'status':'pass','runtime':rows,'typing_profile_groups':12,'invalid_profile_guards':4,'retained':['legacy8','legacy16','watcom52'],'frame':'GDB actualhost_setjmp caller initial_save','elapsed_s':time.perf_counter()-start,'scratch':str(work)}
(work/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'status':'pass','runtime_groups':20,'typing_profile_groups':12,'invalid_profile_guards':4,'retained':receipt['retained'],'frame':receipt['frame'],'elapsed_s':receipt['elapsed_s'],'scratch':str(work)}))
