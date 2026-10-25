"""Native-only actual temporary512 state plus safety/profile controls."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
root=Path.home()/'.ai-tmp/msvc2/recon-maintenance/filbuf-capacity'
root.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=root))
rows=[]
for executable in sys.argv[1:]:
 for live in (False,True):
  start=time.monotonic()
  p=subprocess.run([str(Path(executable).resolve())]+(['live'] if live else []),cwd=work,capture_output=True)
  assert p.returncode==0,(executable,live,p.returncode,p.stdout,p.stderr)
  assert not p.stderr and p.stdout==b'temporary512 refill, owned4096 promotion and scope controls pass\n'
  rows.append(dict(binary=executable,live_fp=live,seconds=time.monotonic()-start))
receipt=dict(status='passed',rows=rows,seconds=sum(x['seconds'] for x in rows))
(work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(status='passed',runs=len(rows),seconds=receipt['seconds'],receipt=str(work/'receipt.json'))))
