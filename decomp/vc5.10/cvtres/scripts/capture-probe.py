#!/usr/bin/env python3
"""Explicit original-only groundwork probes. Never imported by native fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from pe_inventory import inventory

ROOT = Path(__file__).resolve().parents[4]
BINARY = ROOT / 'toolchains/5.10/bin/CVTRES.EXE'
SCRATCH = Path.home() / '.ai-tmp/msvc2/vc5.10/cvtres'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'

def sha(data):
    return hashlib.sha256(data).hexdigest()

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('probe', choices=('help', 'question', 'empty', 'convert'))
p.add_argument('--res', type=Path, help='Explicit original RC-produced .res input')
p.add_argument('--output', type=Path, required=True, help='New probe directory; must not exist')
a = p.parse_args()
plan = json.loads((Path(__file__).resolve().parents[1]/'cases/plan.json').read_text())
if plan.get('shared_wine_health_hold'):
    raise SystemExit('Original probes held by root shared Wine health hold; no Wine launched.')
assert (a.probe == 'convert') == (a.res is not None), 'only convert requires --res'
assert (PREFIX/'dosdevices/r:').resolve() == ROOT
assert (PREFIX/'dosdevices/s:').resolve() == Path.home()/'.ai-tmp/msvc2'
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
work = SCRATCH / ('capture-' + a.probe + '-' + str(os.getpid()))
work.mkdir(parents=True, exist_ok=False)
inputs = {}
if a.res:
    src = a.res.resolve()
    data = src.read_bytes()
    (work/'input.res').write_bytes(data)
    inputs['input.res'] = dict(source=str(src), size=len(data), sha256=sha(data))
args = {'help':['/help'], 'question':['/?'], 'empty':[],
        'convert':['/nologo','/machine:ix86','/out:output.obj','input.res']}[a.probe]
env = os.environ.copy()
env.update(WINEPREFIX=str(PREFIX), WINEDEBUG='-all')
start = time.monotonic()
timed_out = False
try:
    r = subprocess.run(['wine', str(BINARY), *args], cwd=work, env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=45)
    status, stdout, stderr = r.returncode, r.stdout, r.stderr
except subprocess.TimeoutExpired as error:
    timed_out = True
    status, stdout, stderr = None, error.stdout or b'', error.stderr or b''
seconds = time.monotonic() - start
artifacts = {}
for name in ('input.res', 'output.obj'):
    source = work/name
    if source.exists():
        data = source.read_bytes()
        (out/name).write_bytes(data)
        artifacts[name] = dict(size=len(data), sha256=sha(data))
for name, data in [('stdout.bin',stdout),('stderr.bin',stderr)]:
    (out/name).write_bytes(data)
    artifacts[name] = dict(size=len(data),sha256=sha(data))
receipt = dict(schema=1, purpose='original-only groundwork; not function acceptance',
               target=inventory()['target'], target_sha256=sha(BINARY.read_bytes()),
               argv=args, cwd=str(work), wine_prefix=str(PREFIX), status=status, timed_out=timed_out,
               elapsed_seconds=seconds, input_provenance=inputs, artifacts=artifacts)
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2))

if timed_out:
    raise SystemExit('Original probe timed out; partial observations retained, no success claimed.')
