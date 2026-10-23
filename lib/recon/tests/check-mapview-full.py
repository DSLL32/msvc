from pathlib import Path
import subprocess,sys,tempfile
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='mapview-full-',dir=Path.home()/'.ai-tmp') as tmp:
 p=subprocess.run([binary],cwd=tmp,capture_output=True)
 assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
 data=(Path(tmp)/'mapped.dat').read_bytes();assert len(data)==0x80000 and data[0]==0x12 and data[-1]==0x34 and not any(data[1:-1])
 for mode in ('short','large','base','high','low','foreign'):
  p=subprocess.run([binary,mode],cwd=tmp,capture_output=True);assert p.returncode==125,(mode,p.returncode,p.stderr)
 p=subprocess.run([binary,'access'],cwd=tmp,capture_output=True);assert p.returncode==7,(p.returncode,p.stderr)
print('full-length MapViewOfFileEx equivalence/ownership/lifetime/FP/errors and seven unsupported controls pass')
