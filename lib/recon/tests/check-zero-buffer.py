import subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='zero-buffer-',dir=Path.home()/'.ai-tmp/msvc2') as work:
    for args in ([],['text']):
        result=subprocess.run([binary,*args],cwd=work,capture_output=True)
        assert result.returncode==0 and not result.stdout and not result.stderr,(args,result.returncode,result.stderr)
    for mode in ('foreign','user','unowned','pointer','count','descriptor'):
        result=subprocess.run([binary,mode],cwd=work,capture_output=True)
        assert result.returncode==125,(mode,result.returncode,result.stderr)
print('owned registered zero-capacity pending/next-byte/text/flush/close/errors/full x87/tails and6 denials passed')
