"""Native-only four-path API/CRT error checks; no original execution."""
import subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
for live in (False,True):
 with tempfile.TemporaryDirectory(prefix='missing-parent-',dir='/root/.ai-tmp/msvc2/recon-maintenance/missing-parent-open')as work:
  p=subprocess.run([binary]+(['live']if live else []),cwd=work,capture_output=True)
  assert p.returncode==0,(live,p.returncode,p.stdout,p.stderr)
  assert p.stdout==b'missing-parent/leaf/nondirectory/success/API-CRT errors/full108FP/FILE state PASS\n' and not p.stderr
  assert sorted(str(q.relative_to(work))for q in Path(work).rglob('*'))==['parent','parent/new','regular']
  assert (Path(work)/'parent/new').read_bytes()==(Path(work)/'regular').read_bytes()==b''
print('four-path success/missing-leaf/missing-parent/ENOTDIR +invalidmode; API independenterrno +CRT mapped errors/fullFP/prepared FILE; empty/live PASS')
