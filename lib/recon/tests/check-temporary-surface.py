"""Unvisited SDK WINAPI signatures and unconditional denial only."""
import pathlib,re,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
result=subprocess.run([binary],capture_output=True)
assert not result.returncode and not result.stdout,(result.returncode,result.stderr)
for name in ('GetTempPathA','GetTempFileNameA','SetEnvironmentVariableA'):
    assert result.stderr.count(('unreviewed '+name).encode())==3,result.stderr
symbols=subprocess.check_output(['nm','-u',binary],text=True)
assert not re.search(r'\b(?:GetTempPathA|GetTempFileNameA|SetEnvironmentVariableA)(?:@|$)',symbols,re.M),symbols
print('three WINAPI types/9 no-access no-write denials/no host imports PASS %.6fs'%(time.monotonic()-start))
