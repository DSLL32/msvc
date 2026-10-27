from pathlib import Path
import os,subprocess,sys,tempfile,time,json
start=time.perf_counter()
base=Path.home()/'.ai-tmp/msvc2/recon-maintenance/win32-find-files'
base.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(dir=base) as directory:
    work=Path(directory)
    for name in ('updated.lib','match1.lib','match2.lib'):
        (work/name).write_bytes(b'archive')
        os.utime(work/name,(1700000000,1700000120))
    (work/'match2.lib').chmod(0o444)
    (work/'match-dir.lib').mkdir();(work/'sub').mkdir()
    (work/'sub'/'updated.lib').write_bytes(b'archive')
    (work/'aliasdir').symlink_to('sub',target_is_directory=True)
    (work/'alias.lib').symlink_to('updated.lib');os.mkfifo(work/'pipe.lib')
    setup=time.perf_counter()-start;run_start=time.perf_counter()
    for index,binary in enumerate(sys.argv[1:]):
        binary=str(Path(binary).resolve())
        result=subprocess.run([binary],cwd=work,capture_output=True)
        assert result.returncode==(125 if index else 0) and not result.stdout,(binary,result)
        if index:
            assert b'explicit MSVCRT510 profile' in result.stderr
            for mode in ('next','close'):
                result=subprocess.run([binary,mode],cwd=work,capture_output=True)
                assert result.returncode==125 and b'explicit MSVCRT510 profile' in result.stderr
        else:
            assert not result.stderr,result.stderr
            for mode in ('overlap','output','parent','drive','ads','link','dirlink','special'):
                result=subprocess.run([binary,mode],cwd=work,capture_output=True)
                assert result.returncode==125 and b'recon: unsupported native operation:' in result.stderr,(mode,result)
    print(json.dumps({'status':'pass','setup_seconds':setup,'execution_checks_seconds':time.perf_counter()-run_start,'total_seconds':time.perf_counter()-start}))
