import hashlib,subprocess,sys,tempfile
from pathlib import Path
probe=str(Path(sys.argv[1]).resolve());owner=str(Path(sys.argv[2]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([probe],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stdout and not p.stderr,(p.returncode,p.stderr)
    data=(Path(tmp)/'delete-probe.bin').read_bytes()
    assert len(data)==114840 and hashlib.sha256(data).hexdigest()=='383f0969549f066ec6ecef22ea344579ef81614b6a4efc672ec160c7a3132f39'
subprocess.run([owner],check=True);subprocess.run([owner,'failure'],check=True)
subprocess.run([owner,'null'],check=True)
for mode in ('foreign','interior','repeat','ordinaryfree','realloc','realloczero'):
    p=subprocess.run([owner,mode],capture_output=True);assert p.returncode==125,(mode,p.returncode,p.stderr)
print('12 canonical delete/error/surviving-payload records exact;new/free/reuse generations,realloc success/failure,NULL no-op,and6 ownership denials passed')
