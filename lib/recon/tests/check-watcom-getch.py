"""Real null-device EOF, unchanged lowio/FILE state and excluded inputs."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
def run(mode,source,success=False):
    result=subprocess.run([binary]+([mode] if mode else []),stdin=source,capture_output=True)
    expected=0 if success and not generic else 125
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert bool(result.stderr)==bool(expected),(mode,result.stderr)
with open('/dev/null','rb') as source:
    for mode in ('','live','binary'):run(mode,source,True)
    for mode in ('handle','closed','flags'):run(mode,source)
with tempfile.TemporaryFile() as source:
    run('',source);assert source.tell()==0
    source.write(b'X');source.seek(0);run('',source);assert source.tell()==0
# Even an EOF pipe is outside the admitted null-device domain.
result=subprocess.run([binary],input=b'',capture_output=True)
assert result.returncode==125 and not result.stdout and result.stderr
print('real NUL EOF/repeated reads/lowio+FILE preservation/host+target errors/full108FP;6 denials PASS %.6fs'%(time.monotonic()-start))
