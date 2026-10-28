from pathlib import Path
import os,subprocess,tempfile,time,json
root=Path(__file__).resolve().parents[2]
base=Path.home()/'.ai-tmp/msvc2/borland-find-tests'
base.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=base));files=work/'files';files.mkdir()
(files/'normal.cfg').write_bytes(b'abcdefg')
(files/'readonly.cfg').write_bytes(b'readonly');(files/'readonly.cfg').chmod(0o444)
for name in ('normal.cfg','readonly.cfg'):os.utime(files/name,(1704164646,1704164646))
(files/'onlydir').mkdir();(files/'directory.cfg').mkdir();(files/'blocked').mkdir()
(files/'blocked/item.cfg').write_bytes(b'x');(files/'blocked').chmod(0)
start=time.perf_counter();rows=[];tests=Path(__file__).parent
flags=['-m32','-g','-O0','-DRECON_NATIVE_TEST','-I'+str(root/'include'),'-I'+str(root/'native')]
sources=[str(root/'src'/x) for x in ('compat.c','crt.c','win32.c','process.c','entry.c','decimal.c')]
for cc in ('gcc','clang'):
    binary=work/(cc+'-probe')
    command=[cc,*flags,'-std=gnu89','-DRECON_BORLAND_400_FIND',str(tests/'probe.c'),str(tests/'consumer.c'),*sources,'-pthread','-ldl','-o',str(binary)]
    with (work/(cc+'-build.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,work/(cc+'-build.log')
    for mode in ('','fds','permission','null','attr','overlap','long','nonascii'):
        tick=time.perf_counter()
        result=subprocess.run([str(binary)]+([mode] if mode else []),cwd=files,env={**os.environ,'TZ':'UTC'},capture_output=True)
        expected=125 if mode in ('null','attr','overlap','long','nonascii') else 0
        row={'compiler':cc,'mode':mode or 'positive','status':result.returncode,'elapsed_s':time.perf_counter()-tick}
        rows.append(row)
        assert result.returncode==expected,(row,result.stdout,result.stderr,work)
for cc in ('g++','clang++'):
    result=subprocess.run([cc,*flags,'-std=c++98','-nostdinc++','-DRECON_BORLAND_400_FIND','-fsyntax-only',str(tests/'typing.cpp')],capture_output=True)
    assert result.returncode==0,result.stderr
isolation=work/'isolation.cpp'
isolation.write_text('#include <recon.h>\n#ifdef findfirst\n#error unselected or shim implementation name changed\n#endif\n')
for extra in ([],['-DRECON_CRT_PROFILE_MSVCRT_510'],['-DRECON_CRT_PROFILE_WATCOM_100A'],['-DRECON_SHIM_IMPL','-DRECON_BORLAND_400_FIND']):
    result=subprocess.run(['g++',*flags,*extra,'-std=c++98','-nostdinc++','-fsyntax-only',str(isolation)],capture_output=True)
    assert result.returncode==0,result.stderr
receipt={'status':'pass','runtime':rows,'typing_isolation_groups':6,'elapsed_s':time.perf_counter()-start,'scratch':str(work)}
(work/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'status':'pass','runtime_groups':len(rows),'typing_isolation_groups':6,'elapsed_s':receipt['elapsed_s'],'scratch':str(work)}))
