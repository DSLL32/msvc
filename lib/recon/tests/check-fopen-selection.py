import subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
for live in (False,True):
 with tempfile.TemporaryDirectory(prefix='selection-',dir='/root/.ai-tmp/msvc2/recon-maintenance/fopen-selection')as work:
  p=subprocess.run([binary]+(['live']if live else []),cwd=work,capture_output=True)
  assert p.returncode==0,(live,p.returncode,p.stdout,p.stderr)
  assert p.stdout==b'fopen selected-slot/retained-fields/neighbors/lowio; direct-openfile no-write; success/errors/full108FP PASS\n' and not p.stderr
  assert sorted(str(q.relative_to(work))for q in Path(work).rglob('*'))==['parent','parent/new']
  assert (Path(work)/'parent/new').read_bytes()==b''
print('selected-slot full32B/retained20,24,28/neighbor+lowio/direct-openfile-failure/success/fullFP; empty/live PASS')
