import subprocess,sys,tempfile
from pathlib import Path
with tempfile.TemporaryDirectory() as tmp:
    (Path(tmp)/'replacement.bin').write_bytes(b'data')
    for binary in sys.argv[1:]:
        for mode in ([],['close']):
            p=subprocess.run([str(Path(binary).resolve())]+mode,cwd=tmp,stdin=subprocess.DEVNULL,capture_output=True)
            assert p.returncode==0 and not p.stdout and not p.stderr,(binary,mode,p.returncode,p.stdout,p.stderr)
print('VC5 stdin static user-buffer aliases/lifetime survive dup2/close/reopen; default unchanged')
